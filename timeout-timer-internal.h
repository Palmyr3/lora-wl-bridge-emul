#ifndef _TIMEOUT_TIMER_INTERNAL_H
#define _TIMEOUT_TIMER_INTERNAL_H

#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

struct tt_timer {
	struct timeval	start_time;
	uint32_t	interval_ms;
};

#endif
