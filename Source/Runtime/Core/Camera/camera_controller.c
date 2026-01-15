#include "camera_controller.h"
#include "../../Game/game_context.h"
#include <common.h>
#include <math.h>
#include <stdlib.h>

// Camera interpolation and effects system
typedef struct {
  Vec3 previous_position;
  Vec3 target_position;
  Vec3 current_position;
  Vec3 previous_front;
  Vec3 target_front;
  Vec3 current_front;
  f32 interpolation_speed;
  bool is_interpolating;

  // Camera shake
  Vec3 shake_offset;
  f32 shake_intensity;
  f32 shake_duration;
  f32 shake_timer;

  // FOV effects
  f32 base_fov;
  f32 target_fov;
  f32 current_fov;
  f32 fov_transition_speed;

  // Smoothing
  Vec3 velocity_smooth;
  f32 smoothing_factor;
} CameraController;

static CameraController g_camera_controller = {0};

void camera_set_fov_effect(f32 target_fov) {
  g_camera_controller.target_fov = target_fov;
}

Vec3 camera_check_collision(Vec3 desired_position, f32 camera_radius) {
  Vec3 safe_position = desired_position;

  // Check blocks around camera position
  i32 check_radius = (i32)ceilf(camera_radius) + 1;

  for (i32 dx = -check_radius; dx <= check_radius; dx++) {
    for (i32 dy = -check_radius; dy <= check_radius; dy++) {
      for (i32 dz = -check_radius; dz <= check_radius; dz++) {
        Vec3 block_center =
            vec3_add(desired_position, vec3((f32)dx, (f32)dy, (f32)dz));
        i32 block_x = (i32)floorf(block_center.x);
        i32 block_y = (i32)floorf(block_center.y);
        i32 block_z = (i32)floorf(block_center.z);

        BlockID block = chunk_manager_get_block(&g_game.chunk_manager, block_x,
                                                block_y, block_z);

        // Skip air blocks
        if (block == BLOCK_AIR)
          continue;

        // Check if camera sphere intersects with this block
        Vec3 block_min = vec3((f32)block_x, (f32)block_y, (f32)block_z);
        Vec3 block_max = vec3_add(block_min, vec3(1.0f, 1.0f, 1.0f));

        // Find closest point on block to camera center
        Vec3 closest_point =
            vec3(MAX(block_min.x, MIN(desired_position.x, block_max.x)),
                 MAX(block_min.y, MIN(desired_position.y, block_max.y)),
                 MAX(block_min.z, MIN(desired_position.z, block_max.z)));

        // Check distance
        Vec3 to_camera = vec3_sub(desired_position, closest_point);
        f32 distance = vec3_length(to_camera);

        if (distance < camera_radius && distance > 0.001f) {
          // Push camera out of block
          Vec3 push_direction = vec3_normalize(to_camera);
          f32 push_distance = camera_radius - distance;
          safe_position =
              vec3_add(safe_position, vec3_mul(push_direction, push_distance));
        }
      }
    }
  }

  return safe_position;
}

void camera_controller_init(void) {
  g_camera_controller.interpolation_speed = 10.0f;
  g_camera_controller.smoothing_factor = 0.15f;
  g_camera_controller.base_fov = g_game.config.fov;
  g_camera_controller.current_fov = g_game.config.fov;
  g_camera_controller.target_fov = g_game.config.fov;
  g_camera_controller.fov_transition_speed = 5.0f;
}

void camera_controller_update(f32 delta_time) {
  if (!g_game.player_system.player)
    return;

  Vec3 player_pos = player_get_position(&g_game.player_system);
  Vec3 player_front = g_game.camera.front;

  // Update target position
  g_camera_controller.target_position = player_pos;
  g_camera_controller.target_front = player_front;

  // Initialize interpolation if needed
  if (!g_camera_controller.is_interpolating) {
    g_camera_controller.previous_position =
        g_camera_controller.current_position = player_pos;
    g_camera_controller.previous_front = g_camera_controller.current_front =
        player_front;
    g_camera_controller.is_interpolating = true;
  }

  // Smooth position interpolation
  f32 interpolation_alpha =
      MIN(1.0f, delta_time * g_camera_controller.interpolation_speed);
  g_camera_controller.current_position =
      vec3_lerp(g_camera_controller.current_position,
                g_camera_controller.target_position, interpolation_alpha);

  // Smooth front vector interpolation
  g_camera_controller.current_front =
      vec3_lerp(g_camera_controller.current_front,
                g_camera_controller.target_front, interpolation_alpha);

  // Apply camera shake
  if (g_camera_controller.shake_timer > 0.0f) {
    g_camera_controller.shake_timer -= delta_time;
    f32 shake_factor =
        g_camera_controller.shake_timer / g_camera_controller.shake_duration;
    shake_factor = MAX(0.0f, shake_factor);

    // Random shake offset
    f32 shake_x = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;
    f32 shake_y = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;
    f32 shake_z = (rand() % 200 - 100) * 0.01f *
                  g_camera_controller.shake_intensity * shake_factor;

    g_camera_controller.shake_offset = vec3(shake_x, shake_y, shake_z);
  } else {
    g_camera_controller.shake_offset = vec3(0.0f, 0.0f, 0.0f);
  }

  // FOV transitions
  f32 fov_alpha =
      MIN(1.0f, delta_time * g_camera_controller.fov_transition_speed);
  g_camera_controller.current_fov =
      g_camera_controller.current_fov +
      (g_camera_controller.target_fov - g_camera_controller.current_fov) *
          fov_alpha;

  // Apply all effects to camera
  Vec3 final_position = vec3_add(g_camera_controller.current_position,
                                 g_camera_controller.shake_offset);

  // Apply collision detection before setting final position
  final_position = camera_check_collision(final_position, 0.3f);

  camera_set_position(&g_game.camera, final_position);
  g_game.camera.front = g_camera_controller.current_front;
  g_game.camera.fov = g_camera_controller.current_fov;
}
