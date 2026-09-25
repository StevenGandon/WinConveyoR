#ifndef WCR_EVENT_INTERNAL_H_
#define WCR_EVENT_INTERNAL_H_

#include "libwconr.h"

void wcr_emit(const wcr_state *state, wcr_event_type type, const char *fmt, ...);
void wcr_emit_progress(const wcr_state *state, size_t done, size_t total, const char *fmt, ...);

#endif /* !WCR_EVENT_INTERNAL_H_ */
