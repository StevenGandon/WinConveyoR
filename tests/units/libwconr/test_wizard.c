#include <criterion/criterion.h>
#include <string.h>
#include "libwconr.h"
#include "wizard_private.h"

Test(wizard_get_version, version_returning_with_valid_ctx)
{
    struct _wizard_ctx_s ctx = {.version = 0xff};

    cr_assert_eq(wizard_get_version(&ctx), 0xff, "get version should return version from wizard ctx");
}
