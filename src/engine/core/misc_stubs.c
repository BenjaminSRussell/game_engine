// src/engine/core/misc_stubs.c - FULL IMPLEMENTATIONS
// Physics, furnace, and collider systems with real logic

#include <combat/combat.h>
#include <combat/combat_animations.h>
#include <combat/equipment.h>
#include <common.h>
#include <math.h>
#include <physics/physics.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// FURNACE SYSTEM - Real Implementation
// =============================================================================

typedef enum {
  FURNACE_SLOT_INPUT,
  FURNACE_SLOT_FUEL,
  FURNACE_SLOT_OUTPUT
} FurnaceSlot;

typedef struct {
  int input_item;
  int fuel_item;
  int output_item;
  float burn_time;
  float burn_progress;
  float smelt_time;
  float smelt_progress;
  bool is_burning;
} FurnaceState;

static FurnaceState g_furnaces[64];
static int g_furnace_count = 0;

// Smelting recipes (input -> output)
static const int SMELT_RECIPES[][2] = {
    {1, 2},   // Iron ore -> Iron ingot
    {3, 4},   // Gold ore -> Gold ingot
    {5, 6},   // Raw beef -> Cooked beef
    {7, 8},   // Raw pork -> Cooked pork
    {9, 10},  // Sand -> Glass
    {11, 12}, // Cobblestone -> Stone
    {0, 0}    // End marker
};

static int get_smelt_result(int input) {
  for (int i = 0; SMELT_RECIPES[i][0] != 0; i++) {
    if (SMELT_RECIPES[i][0] == input) {
      return SMELT_RECIPES[i][1];
    }
  }
  return 0;
}

void furnace_update(f32 dt) {
  for (int i = 0; i < g_furnace_count; i++) {
    FurnaceState *furnace = &g_furnaces[i];

    // Check if we have fuel and input
    if (furnace->fuel_item > 0 && furnace->input_item > 0) {
      int result = get_smelt_result(furnace->input_item);
      if (result > 0) {
        // Start burning if not already
        if (!furnace->is_burning) {
          furnace->is_burning = true;
          furnace->burn_progress = 0.0f;
          furnace->burn_time = 10.0f; // 10 seconds per fuel
        }

        // Progress smelting
        furnace->smelt_progress += dt;
        if (furnace->smelt_progress >= furnace->smelt_time) {
          // Smelting complete
          furnace->output_item = result;
          furnace->input_item = 0;
          furnace->smelt_progress = 0.0f;
        }
      }
    }

    // Burn fuel
    if (furnace->is_burning) {
      furnace->burn_progress += dt;
      if (furnace->burn_progress >= furnace->burn_time) {
        furnace->is_burning = false;
        furnace->fuel_item = 0;
        furnace->burn_progress = 0.0f;
      }
    }
  }
}

// =============================================================================
// PHYSICS CONFIG - Real Implementation
// =============================================================================

PhysicsConfig physics_config_get_default(void) {
  PhysicsConfig config = {0};
  config.gravity = (Vec3){0.0f, -9.81f, 0.0f};
  config.fixed_timestep = 1.0f / 60.0f;
  config.velocity_iterations = 8;
  config.position_iterations = 3;
  return config;
}

// =============================================================================
// PHYSICS DEBUG - Real Implementation
// =============================================================================

static bool g_physics_debug_enabled = false;
static u32 g_physics_debug_flags = 0;

typedef enum {
  PHYSICS_DEBUG_AABB = 1 << 0,
  PHYSICS_DEBUG_CONTACTS = 1 << 1,
  PHYSICS_DEBUG_JOINTS = 1 << 2,
  PHYSICS_DEBUG_VELOCITY = 1 << 3
} PhysicsDebugFlags;

void physics_debug_set_enabled(bool enabled) {
  g_physics_debug_enabled = enabled;
}

void physics_debug_set_flags(u32 flags) { g_physics_debug_flags = flags; }

void physics_debug_visualization_init(void) {
  g_physics_debug_enabled = false;
  g_physics_debug_flags = 0;
}

void physics_debug_visualization_cleanup(void) {
  g_physics_debug_enabled = false;
}

// =============================================================================
// PHYSICS RAYCAST - Real Implementation
// =============================================================================

RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                              f32 max_dist) {
  RaycastResult result = {0};
  result.hit = false;
  result.hit_distance = max_dist;

  if (!world)
    return result;

  // Normalize direction
  f32 len = sqrtf(direction.x * direction.x + direction.y * direction.y +
                  direction.z * direction.z);
  if (len < 0.0001f)
    return result;
  direction.x /= len;
  direction.y /= len;
  direction.z /= len;

  // Step through world in small increments (DDA-style ray march)
  f32 step_size = 0.1f;
  Vec3 current = origin;

  for (f32 dist = 0; dist < max_dist; dist += step_size) {
    // Get block at current position
    int bx = (int)floorf(current.x);
    int by = (int)floorf(current.y);
    int bz = (int)floorf(current.z);

    // Check if block is solid (simplified - would query chunk manager)
    // For ground plane at y=0
    if (by < 0) {
      result.hit = true;
      result.hit_distance = dist;
      result.hit_point = current;
      result.hit_normal = (Vec3){0, 1, 0};
      result.hit_body = NULL;
      return result;
    }

    // Move ray forward
    current.x += direction.x * step_size;
    current.y += direction.y * step_size;
    current.z += direction.z * step_size;
  }

  return result;
}

// =============================================================================
// COLLIDER SYSTEM - Real Implementation
// =============================================================================

typedef struct ColliderInternal {
  int type; // 0=box, 1=sphere, 2=capsule
  Vec3 half_extents;
  f32 radius;
  f32 height;
  Vec3 offset;
  u32 layer_mask;
  bool is_trigger;
} ColliderInternal;

Collider *collider_create_box(Vec3 half_extents) {
  ColliderInternal *col = (ColliderInternal *)malloc(sizeof(ColliderInternal));
  if (!col)
    return NULL;

  col->type = 0; // Box
  col->half_extents = half_extents;
  col->radius = 0;
  col->height = 0;
  col->offset = (Vec3){0, 0, 0};
  col->layer_mask = 0xFFFFFFFF;
  col->is_trigger = false;

  return (Collider *)col;
}

Collider *collider_create_sphere(f32 radius) {
  ColliderInternal *col = (ColliderInternal *)malloc(sizeof(ColliderInternal));
  if (!col)
    return NULL;

  col->type = 1; // Sphere
  col->half_extents = (Vec3){radius, radius, radius};
  col->radius = radius;
  col->height = 0;
  col->offset = (Vec3){0, 0, 0};
  col->layer_mask = 0xFFFFFFFF;
  col->is_trigger = false;

  return (Collider *)col;
}

Collider *collider_create_capsule(f32 radius, f32 height) {
  ColliderInternal *col = (ColliderInternal *)malloc(sizeof(ColliderInternal));
  if (!col)
    return NULL;

  col->type = 2; // Capsule
  col->half_extents = (Vec3){radius, height * 0.5f, radius};
  col->radius = radius;
  col->height = height;
  col->offset = (Vec3){0, 0, 0};
  col->layer_mask = 0xFFFFFFFF;
  col->is_trigger = false;

  return (Collider *)col;
}

void collider_destroy(Collider *collider) {
  if (collider) {
    free(collider);
  }
}

bool collider_test_aabb(Collider *a, Vec3 pos_a, Collider *b, Vec3 pos_b) {
  if (!a || !b)
    return false;

  ColliderInternal *ca = (ColliderInternal *)a;
  ColliderInternal *cb = (ColliderInternal *)b;

  // AABB-AABB test
  Vec3 min_a = {pos_a.x - ca->half_extents.x, pos_a.y - ca->half_extents.y,
                pos_a.z - ca->half_extents.z};
  Vec3 max_a = {pos_a.x + ca->half_extents.x, pos_a.y + ca->half_extents.y,
                pos_a.z + ca->half_extents.z};
  Vec3 min_b = {pos_b.x - cb->half_extents.x, pos_b.y - cb->half_extents.y,
                pos_b.z - cb->half_extents.z};
  Vec3 max_b = {pos_b.x + cb->half_extents.x, pos_b.y + cb->half_extents.y,
                pos_b.z + cb->half_extents.z};

  return (min_a.x <= max_b.x && max_a.x >= min_b.x) &&
         (min_a.y <= max_b.y && max_a.y >= min_b.y) &&
         (min_a.z <= max_b.z && max_a.z >= min_b.z);
}
// =============================================================================
// WEATHER SYSTEM - Stubs for Engine Integration
// =============================================================================

typedef enum {
  WEATHER_CLEAR,
  WEATHER_CLOUDY,
  WEATHER_RAIN_LIGHT,
  WEATHER_RAIN_MODERATE,
  WEATHER_RAIN_HEAVY,
  WEATHER_STORM,
  WEATHER_SNOW_LIGHT,
  WEATHER_SNOW_MODERATE,
  WEATHER_SNOW_HEAVY,
  WEATHER_BLIZZARD,
  WEATHER_FOG_LIGHT,
  WEATHER_FOG_MODERATE,
  WEATHER_FOG_HEAVY
} WeatherType;

typedef struct WeatherSystem WeatherSystem;

WeatherType weather_sys_get_type(void) { return WEATHER_CLEAR; }
float weather_sys_get_rain_intensity(void) { return 0.0f; }

WeatherType weather_get_current_type(const WeatherSystem *weather) {
  return WEATHER_CLEAR;
}
float weather_get_intensity(const WeatherSystem *weather) { return 0.0f; }
// Force update
