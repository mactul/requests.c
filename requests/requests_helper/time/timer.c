#include <time.h>
#include <unistd.h>
#include "requests_helper/time/timer.h"


rh_nanoseconds rh_timer_now(void)
{
    struct timespec timer;

    clock_gettime(CLOCK_MONOTONIC, &timer);

    return (uint64_t)timer.tv_sec * 1000 * 1000 * 1000 + (uint64_t)timer.tv_nsec;
}