#include "libwconr.h"
#include "wcr_event_internal.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define WCR_EVENT_MSG_MAX 512

void wcr_set_event_callback(wcr_state *state, wcr_event_callback_t callback, void *user_data)
{
    if (!state)
        return;
    state->event_callback = callback;
    state->event_user_data = user_data;
}

void wcr_emit(const wcr_state *state, wcr_event_type type, const char *fmt, ...)
{
    char buf[WCR_EVENT_MSG_MAX];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (state && state->event_callback) {
        wcr_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = type;
        ev.message = buf;
        state->event_callback(&ev, state->event_user_data);
        return;
    }

    if (type == WCR_EVENT_ERROR || type == WCR_EVENT_WARNING)
        fprintf(stderr, "%s\n", buf);
    else
        printf("%s\n", buf);
}

void wcr_emit_progress(const wcr_state *state, size_t done, size_t total, const char *fmt, ...)
{
    char buf[WCR_EVENT_MSG_MAX];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (state && state->event_callback) {
        wcr_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = WCR_EVENT_PROGRESS;
        ev.message = buf;
        ev.bytes_done = done;
        ev.bytes_total = total;
        state->event_callback(&ev, state->event_user_data);
        return;
    }

    if (total > 0)
        printf("%s (%zu / %zu)\n", buf, done, total);
    else
        printf("%s (%zu bytes)\n", buf, done);
}
