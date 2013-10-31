#include "set_timer.h"

int set_timer(long sec, long usec, long sec_interval, long usec_interval)
{
	struct itimerval interval;

	interval.it_interval.tv_sec  = sec_interval;
	interval.it_interval.tv_usec = usec_interval;
	interval.it_value.tv_sec  = sec;
	interval.it_value.tv_usec = usec;

	if (setitimer(ITIMER_REAL, &interval, NULL) < 0) {
		//perror("setimer");
		return -1;
	}

    return 0;
}
