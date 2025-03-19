#include "libwconr.h"

#include <stdlib.h>

wcr_state *
new_state(void)
{
    wcr_state *state = (wcr_state *)malloc(sizeof(wcr_state));

    if (!state)
        return (NULL);
    return (state);
}

void
close_state(wcr_state *state)
{
    if (!state)
        return;
    free(state);
}
