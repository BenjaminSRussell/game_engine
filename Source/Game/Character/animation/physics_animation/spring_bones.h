/*
 * spring_bones.h
 * Spring constraint bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_SPRING_BONES_H
#define ANIMATION_SPRING_BONES_H

#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct animation_spring_bones_handle {
  uint32_t id;
} animation_spring_bones_handle_t;

typedef struct animation_spring_bone_config {
  char bone_name[64];
  float radius;
  float stiffness;
  float drag;
  float gravity_power;
  float gravity_dir[3];
  float collider_radius;
} animation_spring_bone_config_t;

typedef struct animation_spring_bones_desc {
  uint32_t flags;
  void *skeleton; // Skeleton*
  const animation_spring_bone_config_t *bones;
  uint32_t bone_count;
  void *user_data;
} animation_spring_bones_desc_t;

typedef struct animation_spring_bones_info {
  uint32_t id;
  uint32_t flags;
  uint32_t bone_count;
  bool initialized;
  uint32_t active_bone_count;
} animation_spring_bones_info_t;

typedef struct animation_spring_bones_gpu_data {
  uint32_t bone_index;
  float position[3];
  float rotation[4]; // Quaternion
} animation_spring_bones_gpu_data_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/* Initialization */
int animation_spring_bones_init(void);
void animation_spring_bones_shutdown(void);

/* Lifecycle */
int animation_spring_bones_create(animation_spring_bones_handle_t *out_handle,
                                  const animation_spring_bones_desc_t *desc);
void animation_spring_bones_destroy(animation_spring_bones_handle_t handle);

/* Operations */
int animation_spring_bones_update(animation_spring_bones_handle_t handle,
                                  float delta_time);
int animation_spring_bones_update_batch(
    const animation_spring_bones_handle_t *handles, uint32_t count,
    float delta_time);
int animation_spring_bones_update_async(
    animation_spring_bones_handle_t handle,
    float delta_time); // Async update request

/* Configuration */
void animation_spring_bones_set_lod(animation_spring_bones_handle_t handle,
                                    uint32_t lod_level);
void animation_spring_bones_set_culling(animation_spring_bones_handle_t handle,
                                        bool enabled);
void animation_spring_bones_reset(
    animation_spring_bones_handle_t handle); // Reset simulation state

/* Data Access */
bool animation_spring_bones_is_valid(animation_spring_bones_handle_t handle);
int animation_spring_bones_get_info(animation_spring_bones_handle_t handle,
                                    animation_spring_bones_info_t *out_info);
void animation_spring_bones_mark_dirty(animation_spring_bones_handle_t handle);
int animation_spring_bones_process_pending(void);

/* GPU Integration */
const animation_spring_bones_gpu_data_t *
animation_spring_bones_get_gpu_data(animation_spring_bones_handle_t handle,
                                    uint32_t *out_count);

/* Statistics */
uint32_t animation_spring_bones_get_count(void);
size_t animation_spring_bones_get_memory_usage(void);
void animation_spring_bones_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_SPRING_BONES_H */
