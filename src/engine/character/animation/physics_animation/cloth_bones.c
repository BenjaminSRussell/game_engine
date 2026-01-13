/*
 * cloth_bones.c
 * Cloth bone simulation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#define _POSIX_C_SOURCE 199309L
#include "character/animation/physics_animation/cloth_bones.h"
#include "math/vec3.h"
#include "rendering/render_graph/render_pass_node.h"
#include <core/logger.h>
#include <core/performance.h>
#include <core/threading.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define ANIMATION_CLOTH_BONES_MAX_COUNT 4096
#define ANIMATION_CLOTH_BONES_ALIGNMENT 16
#define ANIMATION_CLOTH_BONES_MAX_ITERATIONS 4
#define ANIMATION_CLOTH_BONES_BATCH_SIZE 64

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct animation_cloth_bones_internal {
  uint32_t id;
  uint32_t flags;
  bool initialized;
  bool dirty;

  // Physics Configuration
  uint32_t bone_count;
  uint32_t *bone_indices;
  float stiffness;
  float damping;
  float drag;
  float mass;
  float wind_influence;
  Vec3 gravity;
  Vec3 accumulated_force;

  // Simulation State
  Vec3 *current_positions;
  Vec3 *previous_positions;
  float *rest_lengths;
  float previous_dt;

  // Render Graph Integration
  rendering_render_pass_node_handle_t render_node;

  // Thread Safety
  pthread_mutex_t mutex;

  // Performance
  uint64_t last_update_time_ns;

  // LOD/Culling
  float lod_distance_threshold;
  bool culled;
} animation_cloth_bones_internal_t;

typedef struct animation_cloth_bones_context {
  // Array of pointers to ensure stable memory addresses for mutexes
  animation_cloth_bones_internal_t *items[ANIMATION_CLOTH_BONES_MAX_COUNT];

  // Use atomics for global counters to avoid locking in update loop
  atomic_uint_fast32_t count;
  atomic_bool initialized;

  // Global Stats (Atomic where possible or approximation)
  atomic_uint_fast64_t stat_update_count;
  atomic_uint_fast64_t
      stat_total_time_ns; // Accumulate NS then convert to MS for display
  atomic_uint_fast32_t stat_active_sims;

  // Stats structure for getter
  animation_cloth_bones_stats_t cached_stats;

  pthread_mutex_t global_mutex; // Protects creation/destruction logic
} animation_cloth_bones_context_t;

static animation_cloth_bones_context_t g_cloth_bones_ctx = {0};

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static uint64_t get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void animation_cloth_bones_execute_render(void *cmd, void *user_data) {
  // This function is called by the render graph executor
  // uint32_t id = (uint32_t)(uintptr_t)user_data;
  (void)cmd;
  (void)user_data;
  // Implementation would dispatch compute shader here
}

static void
animation_cloth_bones_cleanup_internal(animation_cloth_bones_internal_t *item) {
  if (!item)
    return;

  pthread_mutex_lock(&item->mutex);

  if (item->bone_indices) {
    free(item->bone_indices);
    item->bone_indices = NULL;
  }
  if (item->current_positions) {
    free(item->current_positions);
    item->current_positions = NULL;
  }
  if (item->previous_positions) {
    free(item->previous_positions);
    item->previous_positions = NULL;
  }
  if (item->rest_lengths) {
    free(item->rest_lengths);
    item->rest_lengths = NULL;
  }

  // Cleanup render node if necessary (assuming RG handles it, but we clear
  // handle)
  item->render_node.id = 0;

  item->initialized = false;
  item->bone_count = 0;

  pthread_mutex_unlock(&item->mutex);
  pthread_mutex_destroy(&item->mutex);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int animation_cloth_bones_init(void) {
  if (atomic_load(&g_cloth_bones_ctx.initialized)) {
    LOG_WARN(LOG_CAT_ANIMATION, "Cloth bones system already initialized");
    return 0; // Already initialized
  }

  if (pthread_mutex_init(&g_cloth_bones_ctx.global_mutex, NULL) != 0) {
    LOG_FATAL(LOG_CAT_ANIMATION,
              "Failed to create mutex for cloth bones system");
    return -2;
  }

  memset(g_cloth_bones_ctx.items, 0, sizeof(g_cloth_bones_ctx.items));
  atomic_store(&g_cloth_bones_ctx.count, 0);
  atomic_store(&g_cloth_bones_ctx.stat_update_count, 0);
  atomic_store(&g_cloth_bones_ctx.stat_total_time_ns, 0);
  atomic_store(&g_cloth_bones_ctx.stat_active_sims, 0);

  atomic_store(&g_cloth_bones_ctx.initialized, true);

  LOG_INFO(LOG_CAT_ANIMATION, "Cloth bones system initialized");
  return 0;
}

void animation_cloth_bones_shutdown(void) {
  if (!atomic_load(&g_cloth_bones_ctx.initialized)) {
    return;
  }

  pthread_mutex_lock(&g_cloth_bones_ctx.global_mutex);
  for (uint32_t i = 0; i < ANIMATION_CLOTH_BONES_MAX_COUNT; i++) {
    if (g_cloth_bones_ctx.items[i]) {
      if (g_cloth_bones_ctx.items[i]->initialized) {
        animation_cloth_bones_cleanup_internal(g_cloth_bones_ctx.items[i]);
      }
      free(g_cloth_bones_ctx.items[i]);
      g_cloth_bones_ctx.items[i] = NULL;
    }
  }

  atomic_store(&g_cloth_bones_ctx.count, 0);
  atomic_store(&g_cloth_bones_ctx.initialized, false);
  pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
  pthread_mutex_destroy(&g_cloth_bones_ctx.global_mutex);

  LOG_INFO(LOG_CAT_ANIMATION, "Cloth bones system shutdown");
}

int animation_cloth_bones_create(animation_cloth_bones_handle_t *out_handle,
                                 const animation_cloth_bones_desc_t *desc) {
  if (!out_handle || !desc)
    return -1;
  if (!atomic_load(&g_cloth_bones_ctx.initialized))
    return -2;

  // Needs at least basic physics setup
  if (desc->bone_count == 0 || !desc->bone_indices)
    return -4;
  // Initial positions optional? Assuming required for rest lengths
  if (!desc->initial_positions)
    return -7;

  pthread_mutex_lock(&g_cloth_bones_ctx.global_mutex);

  // Find free slot
  int index = -1;
  for (uint32_t i = 0; i < ANIMATION_CLOTH_BONES_MAX_COUNT; ++i) {
    if (g_cloth_bones_ctx.items[i] == NULL) {
      index = (int)i;
      break;
    }
  }

  if (index == -1) {
    pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
    LOG_ERROR(LOG_CAT_ANIMATION, "Cloth bones capacity reached");
    return -3; // Capacity limit reached
  }

  // Allocate the item container
  animation_cloth_bones_internal_t *item =
      calloc(1, sizeof(animation_cloth_bones_internal_t));
  if (!item) {
    pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
    return -6;
  }

  if (pthread_mutex_init(&item->mutex, NULL) != 0) {
    free(item);
    pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
    return -5;
  }

  g_cloth_bones_ctx.items[index] = item;
  atomic_fetch_add(&g_cloth_bones_ctx.count, 1);

  item->id = index;
  item->flags = desc->flags;
  item->initialized = true;
  item->dirty = true;
  item->render_node.id = 0;

  // Copy Physics Params
  item->bone_count = desc->bone_count;
  item->stiffness = desc->stiffness;
  item->damping = desc->damping;
  item->drag = desc->drag;
  item->mass = desc->mass > 0.001f ? desc->mass : 1.0f;
  item->wind_influence = desc->wind_influence;
  item->gravity = desc->gravity;
  item->lod_distance_threshold = desc->lod_distance_threshold > 0.0f
                                     ? desc->lod_distance_threshold
                                     : 100.0f;
  item->accumulated_force = vec3_zero();
  item->previous_dt = 0.016f; // Default assumption for first frame

  // Allocate State
  item->bone_indices = malloc(sizeof(uint32_t) * item->bone_count);
  item->current_positions = malloc(sizeof(Vec3) * item->bone_count);
  item->previous_positions = malloc(sizeof(Vec3) * item->bone_count);
  item->rest_lengths = malloc(sizeof(float) * item->bone_count);

  if (!item->bone_indices || !item->current_positions ||
      !item->previous_positions || !item->rest_lengths) {
    animation_cloth_bones_cleanup_internal(item);
    free(item);
    g_cloth_bones_ctx.items[index] = NULL;
    atomic_fetch_sub(&g_cloth_bones_ctx.count, 1);
    pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
    return -6;
  }

  memcpy(item->bone_indices, desc->bone_indices,
         sizeof(uint32_t) * item->bone_count);

  // Initialize positions from bind pose
  for (uint32_t i = 0; i < item->bone_count; ++i) {
    item->current_positions[i] = desc->initial_positions[i];
    item->previous_positions[i] = desc->initial_positions[i];
  }

  // Calculate rest lengths from bind pose
  item->rest_lengths[0] = 0.0f; // Root has no length to parent
  for (uint32_t i = 1; i < item->bone_count; ++i) {
    float dist = vec3_distance(desc->initial_positions[i],
                               desc->initial_positions[i - 1]);
    item->rest_lengths[i] = dist;
  }

  out_handle->id = index;

  atomic_fetch_add(&g_cloth_bones_ctx.stat_active_sims, 1);

  pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
  return 0;
}

void animation_cloth_bones_destroy(animation_cloth_bones_handle_t handle) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return;

  pthread_mutex_lock(&g_cloth_bones_ctx.global_mutex);
  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (item) {
    animation_cloth_bones_cleanup_internal(item);
    free(item);
    g_cloth_bones_ctx.items[handle.id] = NULL;
    atomic_fetch_sub(&g_cloth_bones_ctx.count, 1);
    atomic_fetch_sub(&g_cloth_bones_ctx.stat_active_sims, 1);
  }
  pthread_mutex_unlock(&g_cloth_bones_ctx.global_mutex);
}

int animation_cloth_bones_update(animation_cloth_bones_handle_t handle,
                                 float dt) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return -1;
  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (!item)
    return -1;

  if (!item->initialized)
    return -2;

  // Performance timer wrapper
  Timer *timer = perf_timer_create("cloth_bones_update_physics");
  perf_timer_start(timer);

  pthread_mutex_lock(&item->mutex);

  uint64_t start_time = get_time_ns();

  if (dt > 0.0001f) {
    // Handle variable timestep
    float dt_correction = dt / item->previous_dt;
    // Clamp correction to avoid instability
    if (dt_correction > 2.0f)
      dt_correction = 2.0f;
    if (dt_correction < 0.5f)
      dt_correction = 0.5f;

    item->previous_dt = dt;

    // Forces
    Vec3 force_accel = vec3_div(item->accumulated_force, item->mass);

    // Add Wind (Placeholder logic)
    if (item->wind_influence > 0.0f) {
      // Vec3 wind = get_global_wind_at(item->current_positions[0]);
      // force_accel = vec3_add(force_accel, vec3_mul(wind,
      // item->wind_influence));
    }

    // Total Acceleration: Gravity + (Force / Mass)
    Vec3 accel = vec3_add(item->gravity, force_accel);

    // Reset accumulated force
    item->accumulated_force = vec3_zero();

    for (uint32_t i = 0; i < item->bone_count; ++i) {
      if (i == 0)
        continue; // Root is kinematic

      Vec3 pos = item->current_positions[i];
      Vec3 prev = item->previous_positions[i];

      Vec3 velocity = vec3_sub(pos, prev);
      // Apply damping
      velocity = vec3_mul(velocity, (1.0f - item->damping));
      // Apply time correction
      velocity = vec3_mul(velocity, dt_correction);

      Vec3 delta = vec3_add(velocity, vec3_mul(accel, dt * dt));

      item->previous_positions[i] = pos;
      item->current_positions[i] = vec3_add(pos, delta);
    }

    // Constraints
    for (int iter = 0; iter < ANIMATION_CLOTH_BONES_MAX_ITERATIONS; ++iter) {
      for (uint32_t i = 1; i < item->bone_count; ++i) {
        Vec3 p1 = item->current_positions[i - 1];
        Vec3 p2 = item->current_positions[i];
        Vec3 dir = vec3_sub(p2, p1);
        float dist = vec3_length(dir);
        float rest = item->rest_lengths[i];

        if (dist > 0.0001f) {
          float diff = (dist - rest) / dist;
          Vec3 correction = vec3_mul(dir, diff * 0.5f * item->stiffness);

          if (i - 1 == 0) {
            item->current_positions[i] = vec3_sub(item->current_positions[i],
                                                  vec3_mul(correction, 2.0f));
          } else {
            item->current_positions[i - 1] =
                vec3_add(item->current_positions[i - 1], correction);
            item->current_positions[i] =
                vec3_sub(item->current_positions[i], correction);
          }
        }
      }
    }
  }

  uint64_t end_time = get_time_ns();
  item->last_update_time_ns = end_time - start_time;

  item->dirty = true;

  pthread_mutex_unlock(&item->mutex);

  // Atomic stats update
  atomic_fetch_add(&g_cloth_bones_ctx.stat_update_count, 1);
  atomic_fetch_add(&g_cloth_bones_ctx.stat_total_time_ns,
                   item->last_update_time_ns);

  perf_timer_stop(timer);
  perf_timer_destroy(timer);

  return 0;
}

int animation_cloth_bones_set_root_transform(
    animation_cloth_bones_handle_t handle, Vec3 position) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return -1;

  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (!item)
    return -1;

  pthread_mutex_lock(&item->mutex);
  if (item->initialized && item->bone_count > 0) {
    item->current_positions[0] = position;
  }
  pthread_mutex_unlock(&item->mutex);
  return 0;
}

int animation_cloth_bones_reset(animation_cloth_bones_handle_t handle) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return -1;
  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (!item)
    return -1;

  pthread_mutex_lock(&item->mutex);
  if (item->initialized) {
    for (uint32_t i = 0; i < item->bone_count; ++i) {
      item->previous_positions[i] = item->current_positions[i];
    }
    item->accumulated_force = vec3_zero();
    item->previous_dt = 0.016f;
  }
  pthread_mutex_unlock(&item->mutex);
  return 0;
}

bool animation_cloth_bones_is_valid(animation_cloth_bones_handle_t handle) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return false;
  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  return item && item->initialized;
}

int animation_cloth_bones_get_info(animation_cloth_bones_handle_t handle,
                                   animation_cloth_bones_info_t *out_info) {
  if (!out_info)
    return -1;
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return -2;

  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (!item)
    return -2;

  pthread_mutex_lock(&item->mutex);
  out_info->id = item->id;
  out_info->flags = item->flags;
  out_info->initialized = item->initialized;
  out_info->bone_count = item->bone_count;
  out_info->last_update_time_ns = item->last_update_time_ns;
  out_info->is_simulating = true;
  pthread_mutex_unlock(&item->mutex);
  return 0;
}

void animation_cloth_bones_mark_dirty(animation_cloth_bones_handle_t handle) {
  if (handle.id < ANIMATION_CLOTH_BONES_MAX_COUNT) {
    animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
    if (item)
      item->dirty = true;
  }
}

int animation_cloth_bones_process_pending(void) {
  // Batch processing stub for graph integration
  return 0;
}

uint32_t animation_cloth_bones_create_render_node(
    animation_cloth_bones_handle_t handle) {
  if (handle.id >= ANIMATION_CLOTH_BONES_MAX_COUNT)
    return 0;

  animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[handle.id];
  if (!item || !item->initialized)
    return 0;

  // Create a render graph node
  rendering_render_pass_node_desc_t desc = {0};
  desc.name = "ClothSimulationPass";
  desc.type = RENDERING_PASS_TYPE_COMPUTE;
  desc.execute = animation_cloth_bones_execute_render;
  desc.user_data = (void *)(uintptr_t)item->id;

  rendering_render_pass_node_handle_t node_handle;
  if (rendering_render_pass_node_create(&node_handle, &desc) == 0) {
    item->render_node = node_handle;
    return node_handle.id;
  }
  return 0;
}

int animation_cloth_bones_serialize(animation_cloth_bones_handle_t handle,
                                    void *buffer, size_t size,
                                    size_t *out_written) {
  // Serialization not yet compatible with new physics data structure
  return -1;
}

int animation_cloth_bones_deserialize(animation_cloth_bones_handle_t handle,
                                      const void *buffer, size_t size) {
  return -1;
}

uint32_t animation_cloth_bones_get_count(void) {
  return (uint32_t)atomic_load(&g_cloth_bones_ctx.count);
}

size_t animation_cloth_bones_get_memory_usage(void) {
  size_t total = sizeof(g_cloth_bones_ctx);
  for (uint32_t i = 0; i < ANIMATION_CLOTH_BONES_MAX_COUNT; i++) {
    animation_cloth_bones_internal_t *item = g_cloth_bones_ctx.items[i];
    if (item) {
      total += sizeof(animation_cloth_bones_internal_t);
      // Add approx heap usage
      total += sizeof(uint32_t) * item->bone_count;
      total += sizeof(Vec3) * item->bone_count * 2;
      total += sizeof(float) * item->bone_count;
    }
  }
  return total;
}

int animation_cloth_bones_get_stats(animation_cloth_bones_stats_t *out_stats) {
  if (!out_stats)
    return -1;
  out_stats->active_simulations =
      atomic_load(&g_cloth_bones_ctx.stat_active_sims);
  out_stats->update_count = atomic_load(&g_cloth_bones_ctx.stat_update_count);
  out_stats->total_update_time_ms =
      (double)atomic_load(&g_cloth_bones_ctx.stat_total_time_ns) / 1000000.0;
  out_stats->updates_per_frame =
      0; // Not tracked with atomics per frame easily without reset logic
  out_stats->memory_usage = animation_cloth_bones_get_memory_usage();
  return 0;
}

void animation_cloth_bones_debug_print(void) {
  LOG_INFO(LOG_CAT_ANIMATION, "Cloth Bones Debug Info:");
  LOG_INFO(LOG_CAT_ANIMATION, "  Active Sims: %u",
           atomic_load(&g_cloth_bones_ctx.stat_active_sims));
}

// Stubs for future
int animation_cloth_bones_set_lod(animation_cloth_bones_handle_t handle,
                                  int lod_level) {
  return 0;
}
int animation_cloth_bones_enable_gpu(animation_cloth_bones_handle_t handle,
                                     bool enable) {
  return 0;
}
int animation_cloth_bones_set_params(animation_cloth_bones_handle_t handle,
                                     float stiffness, float damping) {
  return 0;
}
int animation_cloth_bones_apply_force(animation_cloth_bones_handle_t handle,
                                      Vec3 force) {
  return 0;
}
int animation_cloth_bones_lock(animation_cloth_bones_handle_t handle) {
  return 0;
}
int animation_cloth_bones_unlock(animation_cloth_bones_handle_t handle) {
  return 0;
}

/* End of cloth_bones.c */
