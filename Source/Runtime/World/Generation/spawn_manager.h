#ifndef SPAWN_MANAGER_H
#define SPAWN_MANAGER_H

#include <common.h>

// Initialize async spawn generation
void async_spawn_init(Vec3 spawn_point);

// Update async spawn generation
void async_spawn_update(void);

// Check if async spawn is complete
bool async_spawn_is_complete(void);

// Find a suitable spawn point
Vec3 find_suitable_spawn_point(void);

// Spawn marker visualization
void spawn_marker_init(Vec3 position);
void spawn_marker_update(f32 delta_time);
void spawn_marker_render(void);

#endif
