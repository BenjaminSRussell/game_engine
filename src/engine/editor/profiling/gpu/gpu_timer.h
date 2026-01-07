#ifndef RENDERING_GPU_TIMER_H
#define RENDERING_GPU_TIMER_H

#include "include/common.h"

typedef struct gpu_timer {
    u32 query_start;
    u32 query_end;
    bool active;
} gpu_timer_t;

void gpu_timer_init(void);
void gpu_timer_begin(void* cmd, const char* name);
void gpu_timer_end(void* cmd);
float gpu_timer_get_ms(const char* name);

#endif // RENDERING_GPU_TIMER_H
