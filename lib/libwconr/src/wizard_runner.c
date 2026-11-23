#include "wizard_runner.h"
#include "wizard_private.h"
#include "wizard_options.h"
#include "wizard_install_info.h"
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
    #include <pwd.h>
    #include <unistd.h>
    #define WIZARD_CHDIR(p) chdir(p)
    #define WIZARD_GETCWD(b, n) getcwd(b, n)
#endif

/* out = a/b ; -1 if the result does not fit */
static int path_join(char *out, size_t size, const char *a, const char *b)
{
    int n = snprintf(out, size, "%s/%s", a, b);

    return (n < 0 || (size_t)n >= size) ? -1 : 0;
}

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

/* everything needed to run one section of a package's .WIZARD */
struct wizard_run_s {
    const wcr_state *state;
    const char *package_name;
    uint32_t type;
    char pkg_dir[WIZARD_PATH_MAX];
    char wizard_path[WIZARD_PATH_MAX];
    char files_dir[WIZARD_PATH_MAX];
    char prefix[WIZARD_PATH_MAX];
    const char *workdir;
    const wcr_install_opts *opts;    /* user answers (install only, may be NULL) */
};

static int default_prefix(char *out, size_t size)
{
#ifdef _WIN32
    const char *base = getenv("LOCALAPPDATA");
    const char *fmt = "%s/" WIZARD_DEFAULT_PREFIX_WIN;
#else
    const char *base = getenv("HOME");
    const char *fmt = "%s/" WIZARD_DEFAULT_PREFIX_POSIX;
#endif

    if (!base)
        return -1;
    snprintf(out, size, fmt, base);
    return 0;
}

static int resolve_prefix(struct wizard_run_s *run, const char *requested)
{
    if (requested && requested[0] != '\0') {
        snprintf(run->prefix, sizeof(run->prefix), "%s", requested);
        return 0;
    }
    if (run->type != WIZARD_SECTION_INSTALL
        && wizard_info_load_prefix(run->pkg_dir, run->prefix, sizeof(run->prefix)) == 0)
        return 0;
    if (default_prefix(run->prefix, sizeof(run->prefix)) != 0) {
        wcr_emit(run->state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot compute default install prefix");
        return -1;
    }
    return 0;
}

static int set_var_path(struct _wizard_vm_s *vm, const char *name, const char *prefix, const char *sub)
{
    char value[WIZARD_PATH_MAX];

    if (path_join(value, sizeof(value), prefix, sub) != 0)
        return -1;
    return wizard_vm_set_var(vm, name, value);
}

/* name of the user running the install (default target user) */
static const char *current_user(void)
{
#ifdef _WIN32
    const char *name = getenv("USERNAME");
#else
    struct passwd *pw = getpwuid(geteuid());
    const char *name = pw ? pw->pw_name : getenv("USER");
#endif

    return name ? name : "";
}

static int set_install_vars(struct _wizard_vm_s *vm, const struct wizard_run_s *run)
{
    if (wizard_vm_set_var(vm, "PREFIX", run->prefix) != 0
        || wizard_vm_set_var(vm, "USER", current_user()) != 0
        || set_var_path(vm, "MACHINE_BIN", run->prefix, WIZARD_SUBDIR_BIN) != 0
        || set_var_path(vm, "MACHINE_LIB", run->prefix, WIZARD_SUBDIR_LIB) != 0
        || set_var_path(vm, "MACHINE_INCLUDE", run->prefix, WIZARD_SUBDIR_INCLUDE) != 0
        || set_var_path(vm, "MACHINE_MAN", run->prefix, WIZARD_SUBDIR_MAN) != 0
        || wizard_vm_set_var(vm, "PKG_NAME", run->package_name) != 0
        || wizard_vm_set_var(vm, "PKG_DIR", run->workdir) != 0) {
        wcr_emit(run->state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot set install variables");
        return -1;
    }
    return 0;
}

/* creates <prefix>/{bin,lib,include,share/man} so packages can copy into them */
static int prepare_prefix(const struct wizard_run_s *run)
{
    static const char * const subdirs[] = {
        WIZARD_SUBDIR_BIN, WIZARD_SUBDIR_LIB, WIZARD_SUBDIR_INCLUDE, WIZARD_SUBDIR_MAN, NULL
    };
    char path[WIZARD_PATH_MAX];
    size_t i;

    for (i = 0; subdirs[i]; i++) {
        if (path_join(path, sizeof(path), run->prefix, subdirs[i]) != 0 || mkdir_p(path) != 0) {
            wcr_emit(run->state, WCR_EVENT_ERROR,
                     "[ERROR] wizard: cannot create %s (permission denied? try another --prefix)", path);
            return -1;
        }
    }
    return 0;
}

/* install: resolve package options from the user answers / defaults */
static int install_with_options(const struct wizard_run_s *run, struct _wizard_vm_s *vm,
                                struct _wizard_ctx_s *ctx)
{
    struct wizard_option_s *options = NULL;
    size_t count = 0;
    int rc;

    if (wizard_read_options(vm, ctx, &options, &count) != 0) {
        wcr_emit(run->state, WCR_EVENT_ERROR, "[ERROR] wizard: invalid options section");
        return -1;
    }

    rc = wizard_apply_options(run->state, vm, options, count, run->opts);
    if (rc == 0)
        rc = run_in_dir(run->state, vm, ctx, run->type, run->workdir);
    if (rc == 0 && wizard_info_save(run->pkg_dir, run->prefix, vm, options, count) != 0)
        wcr_emit(run->state, WCR_EVENT_WARNING, "[WARNING] wizard: cannot save install info for '%s'", run->package_name);

    wizard_free_options(options, count);
    return rc;
}

static int run_with_vm(const struct wizard_run_s *run, struct _wizard_vm_s *vm,
                       struct _wizard_ctx_s *ctx)
{
    if (set_install_vars(vm, run) != 0)
        return -1;
    if (run->type == WIZARD_SECTION_INSTALL)
        return install_with_options(run, vm, ctx);

    /* uninstall/purge: reuse the values chosen at install time */
    if (wizard_info_load_vars(run->pkg_dir, vm) != 0) {
        wcr_emit(run->state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot reload install info");
        return -1;
    }
    return run_in_dir(run->state, vm, ctx, run->type, run->workdir);
}

static int run_wizard_file(const struct wizard_run_s *run)
{
    const wcr_state *state = run->state;
    struct _wizard_ctx_s *ctx;
    struct _wizard_vm_s *vm;
    int rc;

    if (!state->assets_path) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: assets_path not configured (set WCR_ASSETS_PATH)");
        return -1;
    }

    ctx = wizard_open(run->wizard_path);
    if (!ctx) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: invalid wizard file %s", run->wizard_path);
        return -1;
    }

    vm = wizard_vm_init(state->assets_path);
    if (!vm) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot load instruction sets from %s", state->assets_path);
        wizard_close(ctx);
        return -1;
    }

    rc = run_with_vm(run, vm, ctx);
    if (rc != 0 && wizard_vm_error(vm)[0] != '\0')
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: %s", wizard_vm_error(vm));

    wizard_vm_close(vm);
    wizard_close(ctx);
    return rc;
}

static int extract_payload(const struct wizard_run_s *run)
{
    char payload[WIZARD_PATH_MAX];

    if (path_join(payload, sizeof(payload), run->pkg_dir, WIZARD_PAYLOAD_NAME) != 0)
        return -1;
    if (!path_exists(payload)) {
        wcr_emit(run->state, WCR_EVENT_DEBUG, "[DEBUG] wizard: no %s, running in package dir", WIZARD_PAYLOAD_NAME);
        return 0;
    }
    return extract_archive_to(run->state, payload, run->files_dir);
}

/* fills paths; returns 1 if the package has a .WIZARD, 0 if not, -1 on error */
static int init_run(struct wizard_run_s *run, const wcr_state *state,
                    const char *package_name, uint32_t type, const char *pkg_dir)
{
    memset(run, 0, sizeof(*run));
    run->state = state;
    run->package_name = package_name;
    run->type = type;

    if (!pkg_dir)
        return -1;
    snprintf(run->pkg_dir, sizeof(run->pkg_dir), "%s", pkg_dir);

    if (path_join(run->wizard_path, sizeof(run->wizard_path), run->pkg_dir, WIZARD_FILE_NAME) != 0
        || path_join(run->files_dir, sizeof(run->files_dir), run->pkg_dir, WIZARD_FILES_DIR) != 0)
        return -1;
    return path_exists(run->wizard_path) ? 1 : 0;
}

static int wizard_run(const wcr_state *state, const char *package_name,
                      uint32_t type, const wcr_install_opts *opts)
{
    struct wizard_run_s run;
    char *pkg_dir;
    int has_wizard;

    if (!state || !package_name)
        return -1;

    pkg_dir = package_dir(state, package_name);
    has_wizard = init_run(&run, state, package_name, type, pkg_dir);
    free(pkg_dir);
    if (has_wizard <= 0) {
        if (has_wizard == 0)
            wcr_emit(state, WCR_EVENT_DEBUG, "[DEBUG] wizard: no %s for '%s', skipping", WIZARD_FILE_NAME, package_name);
        return has_wizard;
    }

    run.opts = opts;
    if (resolve_prefix(&run, opts ? opts->prefix : NULL) != 0)
        return -1;
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] wizard: prefix=%s", run.prefix);

    if (type == WIZARD_SECTION_INSTALL
        && (extract_payload(&run) != 0 || prepare_prefix(&run) != 0))
        return -1;

    run.workdir = path_exists(run.files_dir) ? run.files_dir : run.pkg_dir;
    return run_wizard_file(&run);
}

static int check_procedure(const wcr_state *state, const char *procedure)
{
    struct _wizard_ctx_s *ctx = wizard_open(procedure);

    if (!ctx) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: invalid procedure file %s", procedure);
        return -1;
    }
    wizard_close(ctx);
    return 0;
}

/* F30: the user procedure replaces the package .WIZARD (kept for uninstall) */
static int apply_procedure(const wcr_state *state, const char *package_name, const char *procedure)
{
    char dest[WIZARD_PATH_MAX];
    char *pkg_dir;
    int rc = -1;

    if (check_procedure(state, procedure) != 0)
        return -1;
    pkg_dir = package_dir(state, package_name);
    if (pkg_dir && path_join(dest, sizeof(dest), pkg_dir, WIZARD_FILE_NAME) == 0)
        rc = vm_copy_file(procedure, dest);
    free(pkg_dir);
    if (rc != 0)
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot use procedure %s", procedure);
    else
        wcr_emit(state, WCR_EVENT_INFO, "[INFO] wizard: package procedure overridden by %s", procedure);
    return rc;
}

int wizard_install(const wcr_state *state, const char *package_name, const wcr_install_opts *opts)
{
    if (state && package_name && opts && opts->procedure
        && apply_procedure(state, package_name, opts->procedure) != 0)
        return -1;
    return wizard_run(state, package_name, WIZARD_SECTION_INSTALL, opts);
}

int wizard_uninstall(const wcr_state *state, const char *package_name)
{
    return wizard_run(state, package_name, WIZARD_SECTION_UNINSTALL, NULL);
}

/* ── Options preview (wcr options <pkg>) ─────────────────────────────────── */

static int copy_option(struct _wizard_vm_s *vm, const struct wizard_option_s *src, wcr_pkg_option *dst)
{
    char *expanded = wizard_vm_expand(vm, src->default_value);

    dst->name = strdup(src->name);
    dst->type = strdup(src->type);
    dst->prompt = strdup(src->prompt);
    dst->default_value = expanded ? expanded : strdup(src->default_value);
    return (dst->name && dst->type && dst->prompt && dst->default_value) ? 0 : -1;
}

static int export_options(struct _wizard_vm_s *vm, const struct wizard_option_s *options, size_t count,
                          wcr_pkg_option **out, size_t *out_count)
{
    size_t i;

    if (count == 0)
        return 0;
    *out = (wcr_pkg_option *)calloc(count, sizeof(**out));
    if (!*out)
        return -1;
    *out_count = count;
    for (i = 0; i < count; i++) {
        if (copy_option(vm, &options[i], &(*out)[i]) != 0)
            return -1;
    }
    return 0;
}

static int list_with_vm(const struct wizard_run_s *run, struct _wizard_vm_s *vm, struct _wizard_ctx_s *ctx,
                        wcr_pkg_option **out, size_t *out_count)
{
    struct wizard_option_s *options = NULL;
    size_t count = 0;
    int rc;

    if (set_install_vars(vm, run) != 0 || wizard_read_options(vm, ctx, &options, &count) != 0)
        return -1;
    rc = export_options(vm, options, count, out, out_count);
    wizard_free_options(options, count);
    return rc;
}

int wizard_list_options(const wcr_state *state, const char *pkg_dir, const char *procedure,
                        const char *package_name, const char *prefix,
                        wcr_pkg_option **out, size_t *out_count)
{
    struct wizard_run_s run;
    struct _wizard_ctx_s *ctx;
    struct _wizard_vm_s *vm;
    int rc;

    if (!state || !package_name || !out || !out_count)
        return -1;
    *out = NULL;
    *out_count = 0;

    rc = init_run(&run, state, package_name, WIZARD_SECTION_OPTIONS, pkg_dir);
    if (rc >= 0 && procedure) {
        if (check_procedure(state, procedure) != 0)
            return -1;
        snprintf(run.wizard_path, sizeof(run.wizard_path), "%s", procedure);
        rc = 1;
    }
    if (rc <= 0)
        return rc;
    if (resolve_prefix(&run, prefix) != 0 || !state->assets_path)
        return -1;
    run.workdir = run.pkg_dir;

    ctx = wizard_open(run.wizard_path);
    vm = ctx ? wizard_vm_init(state->assets_path) : NULL;
    rc = vm ? list_with_vm(&run, vm, ctx, out, out_count) : -1;
    if (rc != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] wizard: cannot read options of '%s'", package_name);
        free_package_options(*out, *out_count);
        *out = NULL;
        *out_count = 0;
    }
    wizard_vm_close(vm);
    wizard_close(ctx);
    return rc;
}
