#include "frame_timer.h"
#include <time.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

void frame_timer_init(frame_timer_t* timer) {
    memset(timer, 0, sizeof(frame_timer_t));
#if defined(__APPLE__)
    timer->start_ticks = mach_absolute_time();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    timer->start_ticks = (u64)ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
    timer->last_ticks = timer->start_ticks;
}

void frame_timer_update(frame_timer_t* timer) {
    u64 current_ticks;
#if defined(__APPLE__)
    current_ticks = mach_absolute_time();
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    u64 elapsed_nanos = (current_ticks - timer->last_ticks) * info.numer / info.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    current_ticks = (u64)ts.tv_sec * 1000000000 + ts.tv_nsec;
    u64 elapsed_nanos = current_ticks - timer->last_ticks;
#endif
    
    timer->delta_time = (double)elapsed_nanos / 1000000000.0;
    timer->total_time += timer->delta_time;
    timer->last_ticks = current_ticks;
}

float frame_timer_get_delta(frame_timer_t* timer) {
    return (float)timer->delta_time;
}
