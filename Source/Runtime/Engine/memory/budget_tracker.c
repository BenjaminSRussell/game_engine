/*
 * budget_tracker.c
 * Memory budget monitoring
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#include "core/memory/budget_tracker.h"
#include "core/threading.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define CORE_BUDGET_TRACKER_MAX_COUNT 4096
#define CORE_BUDGET_TRACKER_DEFAULT_CAPACITY 256
#define CORE_BUDGET_TRACKER_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct core_budget_tracker_internal {
  uint32_t id;
  uint32_t flags;
  CoreBudgetBackend backend;

  char name[64];
  size_t limit;
  size_t current_usage;
  size_t peak_usage;
  uint32_t allocation_count;

  void *user_data; /* Backend context or generic user data */

  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} core_budget_tracker_internal_t;

typedef struct core_budget_tracker_context {
  core_budget_tracker_internal_t *items;
  uint32_t count;
  uint32_t capacity;
  Mutex *lock;
  bool initialized;
} core_budget_tracker_context_t;

static core_budget_tracker_context_t g_budget_tracker_ctx = {0};

/* ============================================================================
 * BACKEND STUBS
 * ============================================================================
 */

static size_t backend_vulkan_get_usage(void *user_data) {
  // In a real implementation, this would query the Vulkan Allocator
  return 0;
}

static size_t backend_metal_get_usage(void *user_data) {
  // Metal specific query
  return 0;
}

static size_t backend_d3d12_get_usage(void *user_data) {
  // D3D12 specific query
  return 0;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================
 */

static bool
core_budget_tracker_validate(const core_budget_tracker_internal_t *item) {
  if (!item)
    return false;

  // Backend specific validation
  switch (item->backend) {
  case CORE_BUDGET_BACKEND_VULKAN:
    // Check if Vulkan is available?
    break;
  case CORE_BUDGET_BACKEND_METAL:
    // Check if Metal is available?
    break;
  case CORE_BUDGET_BACKEND_D3D12:
    // Check if D3D12 is available?
    break;
  default:
    break;
  }

  return true;
}

static void
core_budget_tracker_cleanup_internal(core_budget_tracker_internal_t *item) {
  if (!item)
    return;

  item->initialized = false;
  item->current_usage = 0;
  item->peak_usage = 0;
  item->allocation_count = 0;
  item->user_data = NULL;
}

static void core_budget_tracker_debug_print_internal(void) {
  printf("=== Budget Tracker Status ===\n");
  printf("Total Trackers: %u / %u\n", g_budget_tracker_ctx.count,
         g_budget_tracker_ctx.capacity);

  for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
    const core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[i];
    if (item->initialized) {
      const char *backend_str = "GENERIC";
      if (item->backend == CORE_BUDGET_BACKEND_VULKAN)
        backend_str = "VULKAN";
      else if (item->backend == CORE_BUDGET_BACKEND_METAL)
        backend_str = "METAL";
      else if (item->backend == CORE_BUDGET_BACKEND_D3D12)
        backend_str = "D3D12";

      printf("  [%u] %s (%s): %zu / %zu bytes (Peak: %zu, Allocations: %u)\n",
             item->id, item->name, backend_str, item->current_usage,
             item->limit == 0 ? (size_t)-1 : item->limit, item->peak_usage,
             item->allocation_count);
    }
  }
  printf("=============================\n");
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int core_budget_tracker_init(void) {
  if (g_budget_tracker_ctx.initialized) {
    return CORE_BUDGET_SUCCESS;
  }

  g_budget_tracker_ctx.lock = mutex_create();
  if (!g_budget_tracker_ctx.lock) {
    return CORE_BUDGET_ERROR_INTERNAL;
  }

  mutex_lock(g_budget_tracker_ctx.lock);

  g_budget_tracker_ctx.capacity = CORE_BUDGET_TRACKER_DEFAULT_CAPACITY;
  g_budget_tracker_ctx.items = calloc(g_budget_tracker_ctx.capacity,
                                      sizeof(core_budget_tracker_internal_t));
  if (!g_budget_tracker_ctx.items) {
    mutex_unlock(g_budget_tracker_ctx.lock);
    mutex_destroy(g_budget_tracker_ctx.lock);
    g_budget_tracker_ctx.lock = NULL;
    return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
  }

  g_budget_tracker_ctx.count = 0;
  g_budget_tracker_ctx.initialized = true;

  mutex_unlock(g_budget_tracker_ctx.lock);
  return CORE_BUDGET_SUCCESS;
}

void core_budget_tracker_shutdown(void) {
  if (!g_budget_tracker_ctx.initialized) {
    return;
  }

  mutex_lock(g_budget_tracker_ctx.lock);

  /* Leak detection / Validation on shutdown */
  for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
    if (g_budget_tracker_ctx.items[i].initialized &&
        g_budget_tracker_ctx.items[i].current_usage > 0) {
      fprintf(stderr,
              "[BudgetTracker] Warning: Tracker '%s' (ID %u) has %zu bytes "
              "still allocated at shutdown.\n",
              g_budget_tracker_ctx.items[i].name, i,
              g_budget_tracker_ctx.items[i].current_usage);
    }
    core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[i]);
  }

  free(g_budget_tracker_ctx.items);
  g_budget_tracker_ctx.items = NULL;
  g_budget_tracker_ctx.count = 0;
  g_budget_tracker_ctx.capacity = 0;
  g_budget_tracker_ctx.initialized = false;

  mutex_unlock(g_budget_tracker_ctx.lock);
  mutex_destroy(g_budget_tracker_ctx.lock);
  g_budget_tracker_ctx.lock = NULL;
}

int core_budget_tracker_create(core_budget_tracker_handle_t *out_handle,
                               const core_budget_tracker_desc_t *desc) {
  if (!out_handle || !desc) {
    return CORE_BUDGET_ERROR_INVALID_ARGS;
  }

  if (!g_budget_tracker_ctx.initialized) {
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;
  }

  mutex_lock(g_budget_tracker_ctx.lock);

  // Reuse free slot if available
  int32_t index = -1;
  for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
    if (!g_budget_tracker_ctx.items[i].initialized) {
      index = i;
      break;
    }
  }

  // No free slot, expand if possible
  if (index == -1) {
    if (g_budget_tracker_ctx.count >= g_budget_tracker_ctx.capacity) {
      // Expand capacity
      uint32_t new_capacity = g_budget_tracker_ctx.capacity * 2;
      if (new_capacity > CORE_BUDGET_TRACKER_MAX_COUNT) {
        new_capacity = CORE_BUDGET_TRACKER_MAX_COUNT;
      }

      if (g_budget_tracker_ctx.count >= new_capacity) {
        mutex_unlock(g_budget_tracker_ctx.lock);
        return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
      }

      core_budget_tracker_internal_t *new_items =
          realloc(g_budget_tracker_ctx.items,
                  new_capacity * sizeof(core_budget_tracker_internal_t));
      if (!new_items) {
        mutex_unlock(g_budget_tracker_ctx.lock);
        return CORE_BUDGET_ERROR_OUT_OF_MEMORY;
      }

      // Initialize new memory
      memset(new_items + g_budget_tracker_ctx.capacity, 0,
             (new_capacity - g_budget_tracker_ctx.capacity) *
                 sizeof(core_budget_tracker_internal_t));

      g_budget_tracker_ctx.items = new_items;
      g_budget_tracker_ctx.capacity = new_capacity;
    }
    index = g_budget_tracker_ctx.count++;
  }

  core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[index];

  item->id = index;
  item->flags = desc->flags;
  item->backend = desc->backend;
  item->limit = desc->limit;
  item->user_data = desc->user_data;
  item->current_usage = 0;
  item->peak_usage = 0;
  item->allocation_count = 0;
  item->initialized = true;
  item->dirty = true;
  item->frame_updated = 0;

  if (desc->name) {
    strncpy(item->name, desc->name, sizeof(item->name) - 1);
    item->name[sizeof(item->name) - 1] = '\0';
  } else {
    snprintf(item->name, sizeof(item->name), "Tracker_%u", index);
  }

  if (!core_budget_tracker_validate(item)) {
    item->initialized = false;
    mutex_unlock(g_budget_tracker_ctx.lock);
    return CORE_BUDGET_ERROR_INVALID_ARGS; // OR Validation check error
  }

  out_handle->id = index;

  mutex_unlock(g_budget_tracker_ctx.lock);
  return CORE_BUDGET_SUCCESS;
}

void core_budget_tracker_destroy(core_budget_tracker_handle_t handle) {
  if (!g_budget_tracker_ctx.initialized)
    return;

  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count) {
    mutex_unlock(g_budget_tracker_ctx.lock);
    return;
  }

  core_budget_tracker_cleanup_internal(&g_budget_tracker_ctx.items[handle.id]);

  mutex_unlock(g_budget_tracker_ctx.lock);
}

int core_budget_tracker_allocate(core_budget_tracker_handle_t handle,
                                 size_t size) {
  if (!g_budget_tracker_ctx.initialized)
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;

  int result = CORE_BUDGET_SUCCESS;
  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count ||
      !g_budget_tracker_ctx.items[handle.id].initialized) {
    result = CORE_BUDGET_ERROR_NOT_FOUND;
    goto done;
  }

  core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[handle.id];

  if (item->limit > 0 && (item->current_usage + size > item->limit)) {
    result = CORE_BUDGET_ERROR_LIMIT_EXCEEDED;
    goto done;
  }

  item->current_usage += size;
  if (item->current_usage > item->peak_usage) {
    item->peak_usage = item->current_usage;
  }
  item->allocation_count++;
  item->dirty = true;

done:
  mutex_unlock(g_budget_tracker_ctx.lock);
  return result;
}

int core_budget_tracker_deallocate(core_budget_tracker_handle_t handle,
                                   size_t size) {
  if (!g_budget_tracker_ctx.initialized)
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;

  int result = CORE_BUDGET_SUCCESS;
  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count ||
      !g_budget_tracker_ctx.items[handle.id].initialized) {
    result = CORE_BUDGET_ERROR_NOT_FOUND;
    goto done;
  }

  core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[handle.id];

  if (item->current_usage < size) {
    /* Underflow? Just set to 0. */
    item->current_usage = 0;
  } else {
    item->current_usage -= size;
  }

  if (item->allocation_count > 0) {
    item->allocation_count--;
  }
  item->dirty = true;

done:
  mutex_unlock(g_budget_tracker_ctx.lock);
  return result;
}

int core_budget_tracker_update(core_budget_tracker_handle_t handle,
                               const void *data, size_t size) {
  if (!g_budget_tracker_ctx.initialized)
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;

  int result = CORE_BUDGET_SUCCESS;
  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count ||
      !g_budget_tracker_ctx.items[handle.id].initialized) {
    result = CORE_BUDGET_ERROR_NOT_FOUND;
    goto done;
  }

  core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[handle.id];

  if (data != NULL) {
    item->user_data = (void *)data;
  }

  // Backend specific logic check
  switch (item->backend) {
  case CORE_BUDGET_BACKEND_VULKAN:
    if (size == 0 && item->user_data) {
      size = backend_vulkan_get_usage(item->user_data);
    }
    break;
  case CORE_BUDGET_BACKEND_METAL:
    if (size == 0 && item->user_data) {
      size = backend_metal_get_usage(item->user_data);
    }
    break;
  case CORE_BUDGET_BACKEND_D3D12:
    if (size == 0 && item->user_data) {
      size = backend_d3d12_get_usage(item->user_data);
    }
    break;
  default:
    break;
  }

  // Direct update: overrides current usage
  item->current_usage = size;
  if (item->current_usage > item->peak_usage) {
    item->peak_usage = item->current_usage;
  }

  item->dirty = true;

  if (item->limit > 0 && item->current_usage > item->limit) {
    result = CORE_BUDGET_ERROR_LIMIT_EXCEEDED;
  }

done:
  mutex_unlock(g_budget_tracker_ctx.lock);
  return result;
}

bool core_budget_tracker_is_valid(core_budget_tracker_handle_t handle) {
  if (!g_budget_tracker_ctx.initialized)
    return false;

  bool valid = false;
  mutex_lock(g_budget_tracker_ctx.lock);
  if (handle.id < g_budget_tracker_ctx.count) {
    valid = g_budget_tracker_ctx.items[handle.id].initialized;
  }
  mutex_unlock(g_budget_tracker_ctx.lock);
  return valid;
}

int core_budget_tracker_get_info(core_budget_tracker_handle_t handle,
                                 core_budget_tracker_info_t *out_info) {
  if (!out_info)
    return CORE_BUDGET_ERROR_INVALID_ARGS;
  if (!g_budget_tracker_ctx.initialized)
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;

  int result = CORE_BUDGET_SUCCESS;
  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count ||
      !g_budget_tracker_ctx.items[handle.id].initialized) {
    result = CORE_BUDGET_ERROR_NOT_FOUND;
    goto done;
  }

  const core_budget_tracker_internal_t *item =
      &g_budget_tracker_ctx.items[handle.id];
  out_info->id = item->id;
  out_info->flags = item->flags;
  out_info->backend = item->backend;
  out_info->current_usage = item->current_usage;
  out_info->peak_usage = item->peak_usage;
  out_info->limit = item->limit;
  out_info->initialized = item->initialized;
  strncpy(out_info->name, item->name, sizeof(out_info->name));

done:
  mutex_unlock(g_budget_tracker_ctx.lock);
  return result;
}

int core_budget_tracker_get_status(core_budget_tracker_handle_t handle,
                                   core_budget_tracker_status_t *out_status) {
  if (!out_status)
    return CORE_BUDGET_ERROR_INVALID_ARGS;
  if (!g_budget_tracker_ctx.initialized)
    return CORE_BUDGET_ERROR_NOT_INITIALIZED;

  int result = CORE_BUDGET_SUCCESS;
  mutex_lock(g_budget_tracker_ctx.lock);

  if (handle.id >= g_budget_tracker_ctx.count ||
      !g_budget_tracker_ctx.items[handle.id].initialized) {
    result = CORE_BUDGET_ERROR_NOT_FOUND;
    goto done;
  }

  const core_budget_tracker_internal_t *item =
      &g_budget_tracker_ctx.items[handle.id];
  out_status->current_usage = item->current_usage;
  out_status->peak_usage = item->peak_usage;
  out_status->limit = item->limit;
  out_status->allocation_count = item->allocation_count;
  strncpy(out_status->name, item->name, sizeof(out_status->name));

done:
  mutex_unlock(g_budget_tracker_ctx.lock);
  return result;
}

void core_budget_tracker_mark_dirty(core_budget_tracker_handle_t handle) {
  if (!g_budget_tracker_ctx.initialized)
    return;

  mutex_lock(g_budget_tracker_ctx.lock);
  if (handle.id < g_budget_tracker_ctx.count) {
    g_budget_tracker_ctx.items[handle.id].dirty = true;
  }
  mutex_unlock(g_budget_tracker_ctx.lock);
}

int core_budget_tracker_process_pending(void) {
  if (!g_budget_tracker_ctx.initialized)
    return 0;

  mutex_lock(g_budget_tracker_ctx.lock);
  int processed = 0;

  for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
    core_budget_tracker_internal_t *item = &g_budget_tracker_ctx.items[i];
    if (item->initialized && item->dirty) {
      // Check limits here if desired
      item->dirty = false;
      processed++;
    }
  }

  mutex_unlock(g_budget_tracker_ctx.lock);
  return processed;
}

uint32_t core_budget_tracker_get_count(void) {
  return g_budget_tracker_ctx.count;
}

size_t core_budget_tracker_get_memory_usage(void) {
  if (!g_budget_tracker_ctx.initialized)
    return 0;

  mutex_lock(g_budget_tracker_ctx.lock);

  size_t total = sizeof(g_budget_tracker_ctx);
  total +=
      g_budget_tracker_ctx.capacity * sizeof(core_budget_tracker_internal_t);

  for (uint32_t i = 0; i < g_budget_tracker_ctx.count; i++) {
    if (g_budget_tracker_ctx.items[i].initialized) {
      total += g_budget_tracker_ctx.items[i].current_usage;
    }
  }

  mutex_unlock(g_budget_tracker_ctx.lock);
  return total;
}

void core_budget_tracker_debug_print(void) {
  if (!g_budget_tracker_ctx.initialized)
    return;

  mutex_lock(g_budget_tracker_ctx.lock);
  core_budget_tracker_debug_print_internal();
  mutex_unlock(g_budget_tracker_ctx.lock);
}
