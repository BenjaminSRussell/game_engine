#ifndef SOFT_BODY_H
#define SOFT_BODY_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Soft Body Types
typedef enum {
  SOFT_BODY_CLOTH = 0,
  SOFT_BODY_ROPE,
  SOFT_BODY_JELLY,
  SOFT_BODY_DEFORMABLE_MESH,
  SOFT_BODY_COUNT
} SoftBodyType;

// Particle Types
typedef enum {
  PARTICLE_FREE = 0,
  PARTICLE_ANCHORED,
  PARTICLE_KINEMATIC,
  PARTICLE_COUNT
} ParticleType;

// Constraint Types for Soft Bodies
typedef enum {
  SOFT_CONSTRAINT_DISTANCE = 0,
  SOFT_CONSTRAINT_BEND,
  SOFT_CONSTRAINT_SHEAR,
  SOFT_CONSTRAINT_VOLUME,
  SOFT_CONSTRAINT_COUNT
} SoftConstraintType;

// Soft Body Particle
typedef struct {
  Vec3 position;
  Vec3 old_position;
  Vec3 velocity;
  Vec3 force;
  float mass;
  float inv_mass;
  ParticleType type;
  bool collision_enabled;
  uint32_t collision_mask;
  uint32_t collision_group;
  Vec3 normal;
  float friction;
  float restitution;
} SoftParticle;

// Soft Body Constraint
typedef struct {
  uint32_t particle_a;
  uint32_t particle_b;
  SoftConstraintType type;
  float rest_length;
  float stiffness;
  float damping;
  bool enabled;
  float max_stretch;
  float max_compression;
} SoftConstraint;

// Soft Body Face (for rendering and collision)
typedef struct {
  uint32_t indices[3];
  Vec3 normal;
  float area;
  bool active;
} SoftFace;

// Soft Body Configuration
typedef struct {
  SoftBodyType type;
  uint32_t particle_count;
  uint32_t constraint_count;
  uint32_t face_count;

  // Physical properties
  float total_mass;
  float stiffness;
  float damping;
  float friction;
  float restitution;
  float thickness;

  // Simulation parameters
  int solver_iterations;
  float time_step;
  bool self_collision;
  bool collision_with_rigid_bodies;
  float wind_scale;
  float gravity_scale;

  // Material properties
  struct {
    float bending_stiffness;
    float shear_stiffness;
    float stretch_stiffness;
    float density;
    float air_resistance;
    float water_resistance;
  } material;
} SoftBodyConfig;

// Soft Body Instance
typedef struct {
  SoftBodyConfig config;
  SoftParticle *particles;
  SoftConstraint *constraints;
  SoftFace *faces;

  // Bounding volume hierarchy for collision
  struct {
    Vec3 bounds_min;
    Vec3 bounds_max;
    bool bounds_dirty;
  } aabb;

  // Simulation state
  bool active;
  bool sleeping;
  float sleep_threshold;
  float sleep_timer;

  // Rendering data
  Vec3 *vertex_positions;
  Vec3 *vertex_normals;
  uint32_t vertex_count;

  // Collision data
  struct {
    Vec3 *collision_normals;
    float *penetration_depths;
    uint32_t collision_count;
  } collision;

  // Performance statistics
  struct {
    uint32_t particles_updated;
    uint32_t constraints_solved;
    uint32_t collisions_handled;
    float simulation_time;
  } stats;
} SoftBody;

// Soft Body World
typedef struct SoftBodyWorld {
  SoftBody *bodies;
  uint32_t body_count;
  uint32_t body_capacity;

  // Global simulation parameters
  Vec3 gravity;
  Vec3 wind_velocity;
  float air_density;
  float time_scale;
  bool enable_sleeping;

  // Collision detection
  struct {
    bool enabled;
    float margin;
    uint32_t max_contacts_per_body;
  } collision;

  // Performance
  struct {
    uint32_t total_particles;
    uint32_t total_constraints;
    uint32_t active_bodies;
    float simulation_time;
    uint32_t frames_per_second;
  } stats;
} SoftBodyWorld;

// Soft Body World Functions
bool soft_body_world_init(SoftBodyWorld *world, uint32_t max_bodies);
void soft_body_world_cleanup(SoftBodyWorld *world);
void soft_body_world_update(SoftBodyWorld *world, float time_step);
void soft_body_world_set_gravity(SoftBodyWorld *world, Vec3 gravity);
void soft_body_world_set_wind(SoftBodyWorld *world, Vec3 wind_velocity);

// Soft Body Creation and Management
uint32_t soft_body_create_cloth(SoftBodyWorld *world,
                                const SoftBodyConfig *config, uint32_t width,
                                uint32_t height, float spacing);
uint32_t soft_body_create_rope(SoftBodyWorld *world,
                               const SoftBodyConfig *config,
                               uint32_t segment_count, float segment_length);
uint32_t soft_body_create_jelly(SoftBodyWorld *world,
                                const SoftBodyConfig *config,
                                uint32_t resolution);
bool soft_body_remove(SoftBodyWorld *world, uint32_t body_id);
bool soft_body_set_active(SoftBodyWorld *world, uint32_t body_id, bool active);

// Soft Body Simulation
void soft_body_update(SoftBody *body, float time_step);
void soft_body_integrate_particles(SoftBody *body, float time_step);
void soft_body_solve_constraints(SoftBody *body);
void soft_body_handle_collisions(SoftBody *body, SoftBodyWorld *world);
void soft_body_update_bounds(SoftBody *body);
void soft_body_update_normals(SoftBody *body);

// Constraint Solving
void soft_body_solve_distance_constraints(SoftBody *body, int iterations);
void soft_body_solve_bend_constraints(SoftBody *body, int iterations);
void soft_body_solve_shear_constraints(SoftBody *body, int iterations);
void soft_body_solve_volume_constraints(SoftBody *body, int iterations);

// Force Application
void soft_body_apply_force(SoftBody *body, Vec3 force);
void soft_body_apply_force_to_particle(SoftBody *body, uint32_t particle_index,
                                       Vec3 force);
void soft_body_apply_impulse(SoftBody *body, Vec3 impulse);
void soft_body_apply_gravity(SoftBody *body, Vec3 gravity);
void soft_body_apply_wind(SoftBody *body, Vec3 wind_velocity,
                          float air_density);
void soft_body_apply_drag(SoftBody *body, float drag_coefficient);

// Collision Detection and Response
bool soft_body_check_collision_with_world(const SoftBody *body,
                                          const SoftBodyWorld *world);
bool soft_body_check_self_collision(const SoftBody *body);
void soft_body_resolve_collision(SoftBody *body, uint32_t particle_index,
                                 Vec3 normal, float penetration);
void soft_body_resolve_particle_collision(SoftParticle *particle_a,
                                          SoftParticle *particle_b);

// Cloth-specific Functions
void soft_body_cloth_pin_corner(SoftBody *body, uint32_t corner_index);
void soft_body_cloth_pin_edge(SoftBody *body, uint32_t edge_index);
void soft_body_cloth_set_tearing_enabled(SoftBody *body, bool enabled);
void soft_body_cloth_tear_at_particle(SoftBody *body, uint32_t particle_index);

// Rope-specific Functions
void soft_body_rope_pin_end(SoftBody *body, uint32_t end_index);
void soft_body_rope_set_length(SoftBody *body, float length);
float soft_body_rope_get_current_length(const SoftBody *body);

// Utility Functions
Vec3 soft_body_get_center_of_mass(const SoftBody *body);
float soft_body_get_total_mass(const SoftBody *body);
Vec3 soft_body_get_velocity_at_point(const SoftBody *body, Vec3 point);
bool soft_body_is_sleeping(const SoftBody *body);
void soft_body_wake_up(SoftBody *body);

// Debug and Visualization
void soft_body_debug_draw_particles(const SoftBody *body);
void soft_body_debug_draw_constraints(const SoftBody *body);
void soft_body_debug_draw_faces(const SoftBody *body);
void soft_body_debug_draw_bounds(const SoftBody *body);

// Performance Profiling
typedef struct {
  uint32_t total_bodies;
  uint32_t active_bodies;
  uint32_t sleeping_bodies;
  uint32_t total_particles;
  uint32_t total_constraints;
  float average_simulation_time;
  float maximum_simulation_time;
  uint32_t frames_per_second;
} SoftBodyPerformanceStats;

void soft_body_get_performance_stats(const SoftBodyWorld *world,
                                     SoftBodyPerformanceStats *stats);
void soft_body_reset_performance_stats(SoftBodyWorld *world);

// Helper Functions
bool soft_body_is_valid_particle_index(const SoftBody *body, uint32_t index);
bool soft_body_is_valid_constraint_index(const SoftBody *body, uint32_t index);
float soft_body_calculate_particle_distance(const SoftParticle *a,
                                            const SoftParticle *b);
void soft_body_update_particle_normal(SoftParticle *particle,
                                      const SoftBody *body);

#ifdef __cplusplus
}
#endif

#endif // SOFT_BODY_H
