#include "libwconr.h"
#include "wcr_event_internal.h"
#include "pkg_registry.h"
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RMRF_CMD_MAX 2048
#define PKG_SUBDIR_MAX 256

int uninstall_package(const wcr_state *state, const char *package_name)
{
    char pkg_subdir[PKG_SUBDIR_MAX];
    char cmd[RMRF_CMD_MAX];
    char *install_dir;

    if (!state || !package_name) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] uninstall_package: state or package_name is NULL");
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: resolving '%s'", package_name);

    snprintf(pkg_subdir, sizeof(pkg_subdir), "packages/%s", package_name);
    install_dir = get_cache_path(state, pkg_subdir);
    if (!install_dir)
        return -1;

    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", install_dir);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: removing %s", install_dir);

    if (system(cmd) != 0) {
        wcr_emit(state, WCR_EVENT_ERROR, "[ERROR] uninstall: failed to remove %s", install_dir);
        free(install_dir);
        return -1;
    }
    free(install_dir);

    if (remove_installed(state, package_name) != 0) {
        wcr_emit(state, WCR_EVENT_WARNING, "[WARNING] uninstall: '%s' not in registry", package_name);
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: '%s' removed successfully", package_name);
    return 0;
}
