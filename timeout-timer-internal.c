#include <stddef.h>
#include <stdio.h>

#include "timeout-timer-internal.h"
#include "timeout-timer.h"

void tt_timer_init(struct tt_timer *timer) {}

void tt_timer_start(struct tt_timer *timer, uint32_t ms)
{
	gettimeofday(&timer->start_time, NULL);
	timer->interval_ms = ms;
}

bool tt_timer_is_timeouted(struct tt_timer *timer)
{
	struct timeval curr_time;
	struct timeval check_time;
	struct timeval delta = {
		.tv_sec = timer->interval_ms / 1000,
		.tv_usec = ((timer->interval_ms % 1000) * 1000ULL)
	};

	timeradd(&timer->start_time, &delta, &check_time);
	/* As send time + time_on_air <= current time we
	 * successfuly send packet, and other device probably recieved
	 * it */

	gettimeofday(&curr_time, NULL);
	if (timercmp(&check_time, &curr_time, <=)) {
		printf("timer: timeouted\n");
		return true;
	} else {
//		printf("timer: not timeouted\n");
		return false;
	}
}

void tt_timer_reset(struct tt_timer *timer) {}
