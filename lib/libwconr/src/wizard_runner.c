#include "wizard_runner.h"
#include "wizard_private.h"
#include "file_utils.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define WIZARD_CHDIR(p) _chdir(p)
    #define WIZARD_GETCWD(b, n) _getcwd(b, n)
#else
    #include <unistd.h>
    #define WIZARD_CHDIR(p) chdir(p)
    #define WIZARD_GETCWD(b, n) getcwd(b, n)
#endif

static int path_exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0;
}

static char *package_dir(const wcr_state *state, const char *package_name)
{
    char subdir[WIZARD_PATH_MAX];

    snprintf(subdir, sizeof(subdir), "packages/%s", package_name);
    return get_cache_path(state, subdir);
}

#ifdef _WIN32
    #define WIZARD_CURRENT_OS_FLAG WIZARD_SECTION_FLAG_WINDOWS
#else
    #define WIZARD_CURRENT_OS_FLAG WIZARD_SECTION_FLAG_POSIX
#endif

/*
 * The packager emits one complete section per target (e.g. install_posix,
 * install_windows) plus an "_any" fallback: pick the OS-specific one first,
 * else the fallback. Only one section of a given type is ever executed.
 */
static const struct _wizard_section_entry_raw_s *select_section(const struct _wizard_ctx_s *ctx,
                                                                uint32_t type)
{
    const struct _wizard_section_entry_raw_s *section;
    const struct _wizard_section_entry_raw_s *fallback = NULL;
    uint64_t count;
    uint64_t i;

    count = wizard_get_section_count(ctx);
    for (i = 0; i < count; i++) {
        section = wizard_get_section(ctx, i);
        if (!section || wizard_get_section_type(section) != type)
            continue;
        if (wizard_get_section_flags(section) & WIZARD_CURRENT_OS_FLAG)
            return section;
        if (wizard_get_section_flags(section) == WIZARD_SECTION_FLAG_ANY && !fallback)
            fallback = section;
    }
    return fallback;
}

static int run_section(const wcr_state *state, struct _wizard_vm_s *vm,
                       struct _wizard_ctx_s *ctx, uint32_t type)
{
    const struct _wizard_section_entry_raw_s *section;
    const char *name;
    uint32_t name_len;

    section = select_section(ctx, type);
    if (!section) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wizard: no section of type %u for this OS", type);
        return 0;
    }
    if (wizard_get_section_name(ctx, section, &name, &name_len) != 0) {
        name = "?";
        name_len = 1;
    }
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] wizard: running section '%.*s'", (int)name_len, name);
    if (wizard_vm_exec(vm, ctx, section) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: section '%.*s' failed", (int)name_len, name);
        return -1;
    }
    return 0;
}

static int run_in_dir(const wcr_state *state, struct _wizard_vm_s *vm,
                      struct _wizard_ctx_s *ctx, uint32_t type, const char *workdir)
{
    char prev_dir[WIZARD_PATH_MAX];
    int rc;

    if (!WIZARD_GETCWD(prev_dir, sizeof(prev_dir))) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot read current directory");
        return -1;
    }
    if (WIZARD_CHDIR(workdir) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot enter %s", workdir);
        return -1;
    }

    rc = run_section(state, vm, ctx, type);

    if (WIZARD_CHDIR(prev_dir) != 0)
        wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] wizard: cannot restore directory %s", prev_dir);
    return rc;
}

static int run_wizard_file(const wcr_state *state, const char *wizard_path,
                           const char *workdir, uint32_t type)
{
    struct _wizard_ctx_s *ctx;
    struct _wizard_vm_s *vm;
    int rc;

    if (!state->assets_path) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: assets_path not configured (set WCR_ASSETS_PATH)");
        return -1;
    }

    ctx = wizard_open(wizard_path);
    if (!ctx) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: invalid wizard file %s", wizard_path);
        return -1;
    }

    vm = wizard_vm_init(state->assets_path);
    if (!vm) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot load instruction sets from %s", state->assets_path);
        wizard_close(ctx);
        return -1;
    }

    rc = run_in_dir(state, vm, ctx, type, workdir);

    wizard_vm_close(vm);
    wizard_close(ctx);
    return rc;
}

/* builds <pkg_dir>/.WIZARD and the working dir (files/ if present, else pkg_dir) */
static int resolve_paths(const char *pkg_dir, char *wizard_path, char *files_dir)
{
    snprintf(wizard_path, WIZARD_PATH_MAX, "%s/%s", pkg_dir, WIZARD_FILE_NAME);
    snprintf(files_dir, WIZARD_PATH_MAX, "%s/%s", pkg_dir, WIZARD_FILES_DIR);
    return path_exists(wizard_path);
}

static int extract_payload(const wcr_state *state, const char *pkg_dir, const char *files_dir)
{
    char payload[WIZARD_PATH_MAX];

    snprintf(payload, sizeof(payload), "%s/%s", pkg_dir, WIZARD_PAYLOAD_NAME);
    if (!path_exists(payload)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wizard: no %s, running in package dir", WIZARD_PAYLOAD_NAME);
        return 0;
    }
    return extract_archive_to(state, payload, files_dir);
}

static int wizard_run(const wcr_state *state, const char *package_name, uint32_t type)
{
    char wizard_path[WIZARD_PATH_MAX];
    char files_dir[WIZARD_PATH_MAX];
    char *pkg_dir;
    int rc;

    if (!state || !package_name)
        return -1;

    pkg_dir = package_dir(state, package_name);
    if (!pkg_dir)
        return -1;

    if (!resolve_paths(pkg_dir, wizard_path, files_dir)) {
        wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wizard: no %s for '%s', skipping", WIZARD_FILE_NAME, package_name);
        free(pkg_dir);
        return 0;
    }

    rc = 0;
    if (type == WIZARD_SECTION_INSTALL)
        rc = extract_payload(state, pkg_dir, files_dir);
    if (rc == 0)
        rc = run_wizard_file(state, wizard_path, path_exists(files_dir) ? files_dir : pkg_dir, type);

    free(pkg_dir);
    return rc;
}

int wizard_install(const wcr_state *state, const char *package_name)
{
    return wizard_run(state, package_name, WIZARD_SECTION_INSTALL);
}

int wizard_uninstall(const wcr_state *state, const char *package_name)
{
    return wizard_run(state, package_name, WIZARD_SECTION_UNINSTALL);
}
