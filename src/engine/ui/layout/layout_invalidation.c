/*
 * layout_invalidation.c
 * Layout Invalidation and Caching System Implementation
 * Efficient layout update tracking and multi-level caching
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "layout_invalidation.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL UTILITIES
 * ============================================================================
 */

static uint32_t get_current_timestamp(void) {
  /* In a real implementation, this would use platform-specific timing */
  static uint32_t counter = 0;
  return counter++;
}

static LayoutCacheEntry *find_cache_entry(LayoutCacheLevel *cache,
                                          uint32_t element_id) {
  for (uint32_t i = 0; i < cache->entry_count; i++) {
    if (cache->entries[i].element_id == element_id) {
      return &cache->entries[i];
    }
  }
  return NULL;
}

static void evict_lru_entry(LayoutCacheLevel *cache) {
  if (cache->entry_count == 0)
    return;

  /* Find least recently used entry */
  uint32_t lru_idx = 0;
  uint32_t lru_timestamp = cache->entries[0].last_access_timestamp;

  for (uint32_t i = 1; i < cache->entry_count; i++) {
    if (cache->entries[i].last_access_timestamp < lru_timestamp) {
      lru_timestamp = cache->entries[i].last_access_timestamp;
      lru_idx = i;
    }
  }

  /* Move last entry to this position and shrink */
  cache->entries[lru_idx] = cache->entries[cache->entry_count - 1];
  cache->entry_count--;
}

/* ============================================================================
 * SYSTEM MANAGEMENT
 * ============================================================================
 */

LayoutInvalidationSystem *layout_invalidation_create(void) {
  LayoutInvalidationSystem *system = malloc(sizeof(LayoutInvalidationSystem));
  memset(system, 0, sizeof(LayoutInvalidationSystem));

  /* Initialize element tracking */
  system->element_capacity = 64;
  system->element_ids = malloc(sizeof(uint32_t) * system->element_capacity);
  system->element_generations =
      malloc(sizeof(uint32_t) * system->element_capacity);
  system->element_last_layout_time =
      malloc(sizeof(uint32_t) * system->element_capacity);
  system->element_count = 0;

  /* Initialize invalidation queue */
  system->invalidation_queue.entry_capacity = 128;
  system->invalidation_queue.entries = malloc(
      sizeof(InvalidationEntry) * system->invalidation_queue.entry_capacity);
  system->invalidation_queue.entry_count = 0;
  system->invalidation_queue.read_index = 0;

  /* Initialize cache levels */
  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    system->cache_levels[level].level = level;
    system->cache_levels[level].entry_capacity = 128;
    system->cache_levels[level].entries = malloc(
        sizeof(LayoutCacheEntry) * system->cache_levels[level].entry_capacity);
    system->cache_levels[level].entry_count = 0;
    system->cache_levels[level].hit_count = 0;
    system->cache_levels[level].miss_count = 0;
    system->cache_levels[level].hit_rate = 0.0f;
  }

  /* Default configuration */
  system->current_generation = 0;
  system->current_timestamp = 0;
  system->frame_counter = 0;
  system->enable_aggressive_caching = false;
  system->enable_generation_tracking = true;
  system->max_queue_size = 1024;
  system->max_cache_entries = 512;
  system->cache_eviction_threshold = 0.9f;

  return system;
}

void layout_invalidation_destroy(LayoutInvalidationSystem *system) {
  if (!system)
    return;

  free(system->element_ids);
  free(system->element_generations);
  free(system->element_last_layout_time);
  free(system->invalidation_queue.entries);

  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    free(system->cache_levels[level].entries);
  }

  free(system);
}

void layout_invalidation_set_cache_mode(LayoutInvalidationSystem *system,
                                        CacheLevel level) {
  if (system) {
    /* When changing cache level, clear all caches */
    for (int i = 0; i <= CACHE_LEVEL_FULL; i++) {
      system->cache_levels[i].entry_count = 0;
    }
  }
}

void layout_invalidation_enable_aggressive_caching(
    LayoutInvalidationSystem *system, bool enable) {
  if (system) {
    system->enable_aggressive_caching = enable;
  }
}

void layout_invalidation_set_max_cache_entries(LayoutInvalidationSystem *system,
                                               uint32_t max_entries) {
  if (system) {
    system->max_cache_entries = max_entries;
  }
}

/* ============================================================================
 * ELEMENT TRACKING
 * ============================================================================
 */

void layout_invalidation_register_element(LayoutInvalidationSystem *system,
                                          uint32_t element_id) {
  assert(system);

  /* Check if already registered */
  for (uint32_t i = 0; i < system->element_count; i++) {
    if (system->element_ids[i] == element_id) {
      return;
    }
  }

  /* Add new element */
  if (system->element_count >= system->element_capacity) {
    system->element_capacity *= 2;
    system->element_ids = realloc(system->element_ids,
                                  sizeof(uint32_t) * system->element_capacity);
    system->element_generations =
        realloc(system->element_generations,
                sizeof(uint32_t) * system->element_capacity);
    system->element_last_layout_time =
        realloc(system->element_last_layout_time,
                sizeof(uint32_t) * system->element_capacity);
  }

  system->element_ids[system->element_count] = element_id;
  system->element_generations[system->element_count] = 0;
  system->element_last_layout_time[system->element_count] =
      system->current_timestamp;
  system->element_count++;
}

void layout_invalidation_unregister_element(LayoutInvalidationSystem *system,
                                            uint32_t element_id) {
  assert(system);

  for (uint32_t i = 0; i < system->element_count; i++) {
    if (system->element_ids[i] == element_id) {
      /* Remove from element tracking */
      system->element_ids[i] = system->element_ids[system->element_count - 1];
      system->element_generations[i] =
          system->element_generations[system->element_count - 1];
      system->element_last_layout_time[i] =
          system->element_last_layout_time[system->element_count - 1];
      system->element_count--;

      /* Invalidate from all caches */
      for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
        LayoutCacheLevel *cache = &system->cache_levels[level];
        for (uint32_t j = 0; j < cache->entry_count; j++) {
          if (cache->entries[j].element_id == element_id) {
            cache->entries[j] = cache->entries[cache->entry_count - 1];
            cache->entry_count--;
            break;
          }
        }
      }

      break;
    }
  }
}

uint32_t layout_invalidation_get_element_generation(
    const LayoutInvalidationSystem *system, uint32_t element_id) {
  assert(system);

  for (uint32_t i = 0; i < system->element_count; i++) {
    if (system->element_ids[i] == element_id) {
      return system->element_generations[i];
    }
  }

  return 0;
}

void layout_invalidation_increment_generation(LayoutInvalidationSystem *system,
                                              uint32_t element_id) {
  assert(system);

  for (uint32_t i = 0; i < system->element_count; i++) {
    if (system->element_ids[i] == element_id) {
      system->element_generations[i]++;
      return;
    }
  }
}

/* ============================================================================
 * INVALIDATION
 * ============================================================================
 */

void layout_invalidation_invalidate(LayoutInvalidationSystem *system,
                                    uint32_t element_id,
                                    uint32_t invalidation_flags,
                                    const char *reason) {
  assert(system);

  /* Add to invalidation queue if not at capacity */
  if (system->invalidation_queue.entry_count < system->max_queue_size) {
    InvalidationEntry *entry =
        &system->invalidation_queue
             .entries[system->invalidation_queue.entry_count++];
    entry->element_id = element_id;
    entry->timestamp = system->current_timestamp;
    entry->invalidation_flags = invalidation_flags;
    entry->reason = reason;

    system->total_invalidations++;
  }

  /* Invalidate caches */
  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    LayoutCacheLevel *cache = &system->cache_levels[level];
    LayoutCacheEntry *entry = find_cache_entry(cache, element_id);
    if (entry) {
      entry->is_valid = false;
    }
  }

  /* Increment generation */
  if (system->enable_generation_tracking) {
    layout_invalidation_increment_generation(system, element_id);
  }
}

void layout_invalidation_invalidate_subtree(LayoutInvalidationSystem *system,
                                            uint32_t element_id,
                                            uint32_t invalidation_flags) {
  assert(system);

  /* In a full implementation, this would invalidate the element and all
   * children */
  layout_invalidation_invalidate(system, element_id, invalidation_flags,
                                 "subtree invalidation");
}

void layout_invalidation_invalidate_all(LayoutInvalidationSystem *system) {
  assert(system);

  /* Invalidate all tracked elements */
  for (uint32_t i = 0; i < system->element_count; i++) {
    layout_invalidation_invalidate(system, system->element_ids[i],
                                   INVALIDATE_ALL, "global invalidation");
  }

  /* Clear all caches */
  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    system->cache_levels[level].entry_count = 0;
  }

  system->current_generation++;
}

bool layout_invalidation_is_dirty(const LayoutInvalidationSystem *system,
                                  uint32_t element_id) {
  assert(system);

  /* Check if element appears in invalidation queue */
  for (uint32_t i = system->invalidation_queue.read_index;
       i < system->invalidation_queue.entry_count; i++) {
    if (system->invalidation_queue.entries[i].element_id == element_id) {
      return true;
    }
  }

  return false;
}

bool layout_invalidation_has_flag(const LayoutInvalidationSystem *system,
                                  uint32_t element_id, InvalidationFlag flag) {
  assert(system);

  /* Check if element in queue has this flag */
  for (uint32_t i = system->invalidation_queue.read_index;
       i < system->invalidation_queue.entry_count; i++) {
    if (system->invalidation_queue.entries[i].element_id == element_id) {
      return (system->invalidation_queue.entries[i].invalidation_flags &
              flag) != 0;
    }
  }

  return false;
}

/* ============================================================================
 * QUEUE PROCESSING
 * ============================================================================
 */

bool layout_invalidation_dequeue(LayoutInvalidationSystem *system,
                                 InvalidationEntry *out_entry) {
  assert(system && out_entry);

  if (system->invalidation_queue.read_index >=
      system->invalidation_queue.entry_count) {
    return false;
  }

  *out_entry = system->invalidation_queue
                   .entries[system->invalidation_queue.read_index++];
  return true;
}

bool layout_invalidation_peek_queue(const LayoutInvalidationSystem *system,
                                    InvalidationEntry *out_entry) {
  assert(system && out_entry);

  if (system->invalidation_queue.read_index >=
      system->invalidation_queue.entry_count) {
    return false;
  }

  *out_entry =
      system->invalidation_queue.entries[system->invalidation_queue.read_index];
  return true;
}

uint32_t
layout_invalidation_get_queue_size(const LayoutInvalidationSystem *system) {
  assert(system);
  return system->invalidation_queue.entry_count -
         system->invalidation_queue.read_index;
}

void layout_invalidation_queue_clear(LayoutInvalidationSystem *system) {
  assert(system);
  system->invalidation_queue.entry_count = 0;
  system->invalidation_queue.read_index = 0;
}

/* ============================================================================
 * CACHING
 * ============================================================================
 */

void layout_invalidation_cache_layout(LayoutInvalidationSystem *system,
                                      uint32_t element_id, float x, float y,
                                      float width, float height) {
  assert(system);

  /* Cache at the appropriate level */
  LayoutCacheLevel *cache = &system->cache_levels[CACHE_LEVEL_ELEMENT];

  /* Check if already cached */
  LayoutCacheEntry *entry = find_cache_entry(cache, element_id);
  if (entry) {
    entry->x = x;
    entry->y = y;
    entry->width = width;
    entry->height = height;
    entry->is_valid = true;
    entry->last_access_timestamp = system->current_timestamp;
    entry->access_count++;
    entry->cache_generation = system->current_generation;
    entry->element_generation =
        layout_invalidation_get_element_generation(system, element_id);
    return;
  }

  /* Add new entry */
  if (cache->entry_count >= cache->entry_capacity) {
    if (cache->entry_count >= system->max_cache_entries) {
      /* Evict LRU entry */
      evict_lru_entry(cache);
      system->cache_evictions++;
    } else {
      cache->entry_capacity *= 2;
      cache->entries = realloc(cache->entries, sizeof(LayoutCacheEntry) *
                                                   cache->entry_capacity);
    }
  }

  entry = &cache->entries[cache->entry_count++];
  entry->element_id = element_id;
  entry->x = x;
  entry->y = y;
  entry->width = width;
  entry->height = height;
  entry->is_valid = true;
  entry->last_access_timestamp = system->current_timestamp;
  entry->access_count = 1;
  entry->cache_generation = system->current_generation;
  entry->element_generation =
      layout_invalidation_get_element_generation(system, element_id);
}

bool layout_invalidation_get_cached_layout(
    const LayoutInvalidationSystem *system, uint32_t element_id, float *out_x,
    float *out_y, float *out_width, float *out_height) {
  assert(system && out_x && out_y && out_width && out_height);

  LayoutCacheLevel *cache = &system->cache_levels[CACHE_LEVEL_ELEMENT];
  LayoutCacheEntry *entry = find_cache_entry(cache, element_id);

  if (entry && entry->is_valid &&
      entry->element_generation ==
          layout_invalidation_get_element_generation(system, element_id)) {
    *out_x = entry->x;
    *out_y = entry->y;
    *out_width = entry->width;
    *out_height = entry->height;

    cache->hit_count++;
    return true;
  }

  cache->miss_count++;
  return false;
}

bool layout_invalidation_is_cached(const LayoutInvalidationSystem *system,
                                   uint32_t element_id) {
  assert(system);

  const LayoutCacheLevel *cache = &system->cache_levels[CACHE_LEVEL_ELEMENT];
  LayoutCacheEntry *entry = find_cache_entry(cache, element_id);
  return entry && entry->is_valid;
}

void layout_invalidation_cache_invalidate(LayoutInvalidationSystem *system,
                                          uint32_t element_id) {
  assert(system);

  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    LayoutCacheLevel *cache = &system->cache_levels[level];
    LayoutCacheEntry *entry = find_cache_entry(cache, element_id);
    if (entry) {
      entry->is_valid = false;
    }
  }
}

void layout_invalidation_cache_clear_all(LayoutInvalidationSystem *system) {
  assert(system);

  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    system->cache_levels[level].entry_count = 0;
  }
}

void layout_invalidation_cache_clear_level(LayoutInvalidationSystem *system,
                                           CacheLevel level) {
  assert(system && level <= CACHE_LEVEL_FULL);
  system->cache_levels[level].entry_count = 0;
}

/* ============================================================================
 * BATCH OPERATIONS
 * ============================================================================
 */

static bool g_batch_mode = false;
static uint32_t g_batch_invalidations = 0;

void layout_invalidation_begin_batch(LayoutInvalidationSystem *system) {
  (void)system;
  g_batch_mode = true;
  g_batch_invalidations = 0;
}

void layout_invalidation_end_batch(LayoutInvalidationSystem *system) {
  assert(system);
  g_batch_mode = false;

  if (system->enable_debug) {
    printf("[Layout Invalidation] Batch processed: %u invalidations\n",
           g_batch_invalidations);
  }
}

void layout_invalidation_invalidate_batch(LayoutInvalidationSystem *system,
                                          const uint32_t *element_ids,
                                          uint32_t count,
                                          uint32_t invalidation_flags) {
  assert(system && element_ids);

  for (uint32_t i = 0; i < count; i++) {
    layout_invalidation_invalidate(system, element_ids[i], invalidation_flags,
                                   "batch invalidation");
    g_batch_invalidations++;
  }
}

/* ============================================================================
 * PERFORMANCE MONITORING
 * ============================================================================
 */

void layout_invalidation_get_cache_stats(const LayoutInvalidationSystem *system,
                                         CacheLevel level,
                                         uint32_t *out_entries,
                                         uint32_t *out_hits,
                                         uint32_t *out_misses,
                                         float *out_hit_rate) {
  assert(system && out_entries && out_hits && out_misses && out_hit_rate);

  if (level > CACHE_LEVEL_FULL) {
    *out_entries = 0;
    *out_hits = 0;
    *out_misses = 0;
    *out_hit_rate = 0.0f;
    return;
  }

  const LayoutCacheLevel *cache = &system->cache_levels[level];
  *out_entries = cache->entry_count;
  *out_hits = cache->hit_count;
  *out_misses = cache->miss_count;

  uint32_t total = cache->hit_count + cache->miss_count;
  *out_hit_rate = total > 0 ? (float)cache->hit_count / (float)total : 0.0f;
}

void layout_invalidation_get_performance_stats(
    const LayoutInvalidationSystem *system, uint32_t *out_total_invalidations,
    uint32_t *out_total_solves, float *out_avg_solve_time_ms,
    uint32_t *out_cache_evictions) {
  assert(system && out_total_invalidations && out_total_solves &&
         out_avg_solve_time_ms && out_cache_evictions);

  *out_total_invalidations = system->total_invalidations;
  *out_total_solves = system->total_layout_solves;
  *out_avg_solve_time_ms =
      system->total_layout_solves > 0
          ? system->total_solve_time_ms / (float)system->total_layout_solves
          : 0.0f;
  *out_cache_evictions = system->cache_evictions;
}

uint32_t layout_invalidation_get_total_cache_entries(
    const LayoutInvalidationSystem *system) {
  assert(system);

  uint32_t total = 0;
  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    total += system->cache_levels[level].entry_count;
  }
  return total;
}

/* ============================================================================
 * DEBUGGING
 * ============================================================================
 */

void layout_invalidation_print_state(const LayoutInvalidationSystem *system) {
  if (!system)
    return;

  printf("=== Layout Invalidation System State ===\n");
  printf("Elements tracked: %u\n", system->element_count);
  printf("Current generation: %u\n", system->current_generation);
  printf("Queue size: %u\n", layout_invalidation_get_queue_size(system));
  printf("Total cache entries: %u\n",
         layout_invalidation_get_total_cache_entries(system));
  printf("Total invalidations: %u\n", system->total_invalidations);
  printf("Cache evictions: %u\n", system->cache_evictions);
}

void layout_invalidation_print_cache_stats(
    const LayoutInvalidationSystem *system) {
  if (!system)
    return;

  printf("=== Cache Statistics ===\n");
  for (int level = 0; level <= CACHE_LEVEL_FULL; level++) {
    uint32_t entries, hits, misses;
    float hit_rate;
    layout_invalidation_get_cache_stats(system, level, &entries, &hits, &misses,
                                        &hit_rate);

    const char *level_name = "Unknown";
    switch (level) {
    case CACHE_LEVEL_NONE:
      level_name = "None";
      break;
    case CACHE_LEVEL_ELEMENT:
      level_name = "Element";
      break;
    case CACHE_LEVEL_SUBTREE:
      level_name = "Subtree";
      break;
    case CACHE_LEVEL_FULL:
      level_name = "Full";
      break;
    }

    printf("Level %s: %u entries, %u hits, %u misses, hit rate: %.2f%%\n",
           level_name, entries, hits, misses, hit_rate * 100.0f);
  }
}

void layout_invalidation_print_history(const LayoutInvalidationSystem *system,
                                       uint32_t last_n_entries) {
  if (!system)
    return;

  printf("=== Recent Invalidations (last %u) ===\n", last_n_entries);

  uint32_t start_idx =
      system->invalidation_queue.entry_count > last_n_entries
          ? system->invalidation_queue.entry_count - last_n_entries
          : 0;

  for (uint32_t i = start_idx; i < system->invalidation_queue.entry_count;
       i++) {
    const InvalidationEntry *entry = &system->invalidation_queue.entries[i];
    printf("Entry %u: Element %u @ timestamp %u, flags: 0x%08x, reason: %s\n",
           i, entry->element_id, entry->timestamp, entry->invalidation_flags,
           entry->reason ? entry->reason : "none");
  }
}
