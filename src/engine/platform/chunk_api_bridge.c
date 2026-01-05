#include "platform/chunk_api_bridge.h"
#include "core/logger.h"
#include "world/chunk/chunk_system.h"

void chunk_set_load_distance(float distance) {
  chunk_sys_set_load_distance(distance);
}

float chunk_get_load_distance(void) { return chunk_sys_get_load_distance(); }

void chunk_set_unload_distance(float distance) {
  chunk_sys_set_unload_distance(distance);
}

float chunk_get_unload_distance(void) {
  return chunk_sys_get_unload_distance();
}

void chunk_force_load(int32_t chunk_x, int32_t chunk_z) {
  chunk_sys_force_load(chunk_x, chunk_z);
}

void chunk_force_unload(int32_t chunk_x, int32_t chunk_z) {
  chunk_sys_force_unload(chunk_x, chunk_z);
}

uint32_t chunk_get_loaded_count(void) { return chunk_sys_get_loaded_count(); }

uint64_t chunk_get_memory_usage(void) { return chunk_sys_get_memory_usage(); }

void chunk_set_async_loading_enabled(bool enabled) {
  chunk_sys_set_async_loading_enabled(enabled);
}

bool chunk_is_async_loading_enabled(void) {
  return chunk_sys_is_async_loading_enabled();
}
