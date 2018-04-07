#ifndef _TIMEOUT_TIMER_H
#define _TIMEOUT_TIMER_H

#include "timeout-timer-internal.h"

void tt_timer_init(struct tt_timer *timer);
void tt_timer_start(struct tt_timer *timer, uint32_t ms);
bool tt_timer_is_timeouted(struct tt_timer *timer);
void tt_timer_reset(struct tt_timer *timer);

#endif
