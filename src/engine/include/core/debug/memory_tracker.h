#ifndef CORE_DEBUG_MEMORY_TRACKER_H
#define CORE_DEBUG_MEMORY_TRACKER_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool tracker_init(size_t capacity, bool enable_canaries, bool enable_stack_capture);
void* tracked_malloc(size_t size, const char* system_tag);
void tracked_free(void* ptr);
void tracker_dump_leaks(void);
bool tracker_validate_heap(void);
void tracker_get_stats(u64* total_allocated, u64* peak_allocated, 
                      u64* total_allocations, u64* total_frees, size_t* active_count);
void tracker_enable(bool enabled);
bool tracker_is_enabled(void);
void tracker_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // CORE_DEBUG_MEMORY_TRACKER_H
