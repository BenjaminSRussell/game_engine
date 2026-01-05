#ifndef RENDERING_FRAME_TIMER_H
#define RENDERING_FRAME_TIMER_H

#include "../../../include/common.h"

typedef struct frame_timer {
    u64 start_ticks;
    u64 last_ticks;
    double delta_time;
    double total_time;
} frame_timer_t;

void frame_timer_init(frame_timer_t* timer);
void frame_timer_update(frame_timer_t* timer);
float frame_timer_get_delta(frame_timer_t* timer);

#endif // RENDERING_FRAME_TIMER_H
