#ifndef RH_TIMER_H
#define RH_TIMER_H

#include <stdint.h>

typedef uint64_t rh_generic_time;
typedef uint64_t rh_nanoseconds;
typedef uint64_t rh_microseconds;
typedef uint64_t rh_milliseconds;
typedef uint64_t rh_seconds;


rh_nanoseconds rh_timer_now(void);


static inline rh_nanoseconds rh_timer_elapsed_ns(rh_nanoseconds start)
{
    rh_nanoseconds old = rh_timer_now();
    if(old < start)
    {
        return 0;
    }
    return old - start;
}

static inline rh_microseconds rh_timer_elapsed_us(rh_nanoseconds start)
{
    return (rh_microseconds)(rh_timer_elapsed_ns(start) / (rh_nanoseconds)1000);
}

static inline rh_milliseconds rh_timer_elapsed_ms(rh_nanoseconds start)
{
    return (rh_milliseconds)(rh_timer_elapsed_us(start) / (rh_microseconds)1000);
}

static inline rh_seconds rh_timer_elapsed_s(rh_nanoseconds start)
{
    return (rh_seconds)(rh_timer_elapsed_ms(start) / (rh_milliseconds)1000);
}


static inline rh_generic_time rh_duration(rh_generic_time total, rh_generic_time elapsed_ns)
{
    if(elapsed_ns > total)
    {
        return 0;
    }
    return total - elapsed_ns;
}


#endif