#ifndef WCR_EVENT_H_
#define WCR_EVENT_H_

#include <stddef.h>

typedef enum {
    WCR_EVENT_DEBUG = 0,
    WCR_EVENT_INFO = 1,
    WCR_EVENT_WARNING = 2,
    WCR_EVENT_ERROR = 3,
    WCR_EVENT_PROGRESS = 4
} wcr_event_type;

typedef struct {
    wcr_event_type type;
    const char *message;
    size_t bytes_done;
    size_t bytes_total;
} wcr_event;

typedef void (*wcr_event_callback_t)(const wcr_event *event, void *user_data);

struct wcr_state_s;

void wcr_set_event_callback(struct wcr_state_s *state, wcr_event_callback_t callback, void *user_data);

#endif /* !WCR_EVENT_H_ */
