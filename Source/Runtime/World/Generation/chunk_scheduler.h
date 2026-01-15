#ifndef CHUNK_SCHEDULER_H
#define CHUNK_SCHEDULER_H

#include <common.h>

// Update chunk generation and meshing schedule
// Handles submitting jobs to the thread pool based on player position and
// budget
void chunk_scheduler_update(f32 delta_time);

#endif
