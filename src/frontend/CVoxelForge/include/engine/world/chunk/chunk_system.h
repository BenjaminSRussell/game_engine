#ifndef CHUNK_SYSTEM_H
#define CHUNK_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

void chunk_sys_init(void);
void chunk_sys_shutdown(void);
void chunk_sys_update(float delta_time);

// Distance management
void chunk_sys_set_load_distance(float distance);
float chunk_sys_get_load_distance(void);

void chunk_sys_set_unload_distance(float distance);
float chunk_sys_get_unload_distance(void);

// Manual control
void chunk_sys_force_load(int32_t chunk_x, int32_t chunk_z);
void chunk_sys_force_unload(int32_t chunk_x, int32_t chunk_z);

// Statistics
uint32_t chunk_sys_get_loaded_count(void);
uint64_t chunk_sys_get_memory_usage(void);

// Async loading
void chunk_sys_set_async_loading_enabled(bool enabled);
bool chunk_sys_is_async_loading_enabled(void);

#endif // CHUNK_SYSTEM_H
