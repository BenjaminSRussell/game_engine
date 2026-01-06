#include "renderer/shadows/shadow_cache.h"
#include <stdbool.h>

void shadow_cache_init() {}

void shadow_cache_invalidate_region(float min[3], float max[3]) {}

bool shadow_cache_lookup(void *object_id, void **cached_shadow) {
    return false;
}

void shadow_cache_store(void *object_id, void *shadow_data) {}
