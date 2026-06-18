#include "libwconr.h"
#include "wcr_event_internal.h"

int uninstall_package(const wcr_state *state, const char *package_name)
{
    if (!state || !package_name) {
        wcr_emit(NULL, WCR_EVENT_ERROR, "[ERROR] uninstall_package: state or package_name is NULL");
        return -1;
    }

    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: resolving '%s'", package_name);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: stub - would now remove '%s' from system", package_name);
    wcr_emit(state, WCR_EVENT_INFO, "[INFO] uninstall: ok");
    return 0;
}
