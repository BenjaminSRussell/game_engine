#ifndef PHYSICS_ISLAND_SOLVER_H
#define PHYSICS_ISLAND_SOLVER_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Physics Island Types
typedef enum {
  ISLAND_TYPE_STATIC = 0,
  ISLAND_TYPE_DYNAMIC,
  ISLAND_TYPE_KINEMATIC,
  ISLAND_TYPE_MIXED
} IslandType;

// Island State
typedef enum {
  ISLAND_STATE_ACTIVE = 0,
  ISLAND_STATE_SLEEPING,
  ISLAND_STATE_MERGING,
  ISLAND_STATE_SPLITTING
} IslandState;

// Island Body
typedef struct {
  EntityID entity_id;
  uint32_t island_index;
  bool is_static;
  bool is_kinematic;
  bool is_sleeping;
  Vec3 position;
  Vec3 velocity;
  Vec3 force;
  float mass;
  float inv_mass;
} IslandBody;

// Island Contact
typedef struct {
  uint32_t body_a_index;
  uint32_t body_b_index;
  Vec3 contact_point;
  Vec3 contact_normal;
  float penetration_depth;
  float restitution;
  float friction;
  bool enabled;
} IslandContact;

// Island Constraint
typedef struct {
  uint32_t body_a_index;
  uint32_t body_b_index;
  uint32_t constraint_id;
  bool enabled;
  float break_force;
  float break_torque;
} IslandConstraint;

// Physics Island
typedef struct {
  uint32_t island_id;
  IslandType type;
  IslandState state;

  // Bodies in this island
  IslandBody *bodies;
  uint32_t body_count;
  uint32_t body_capacity;

  // Contacts between bodies in this island
  IslandContact *contacts;
  uint32_t contact_count;
  uint32_t contact_capacity;

  // Constraints between bodies in this island
  IslandConstraint *constraints;
  uint32_t constraint_count;
  uint32_t constraint_capacity;

  // Island properties
  Vec3 center_of_mass;
  float total_mass;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  Vec3 total_force;
  Vec3 total_torque;

  // Bounding volume
  struct {
    Vec3 bounds_min;
    Vec3 bounds_max;
    bool dirty;
  } aabb;

  // Sleep management
  struct {
    bool can_sleep;
    float sleep_timer;
    float sleep_threshold;
    uint32_t sleep_frames;
  } sleep;

  // Simulation state
  bool active;
  bool needs_update;
  float time_step;
  int solver_iterations;

  // Performance statistics
  struct {
    uint32_t bodies_solved;
    uint32_t contacts_solved;
    uint32_t constraints_solved;
    float solve_time;
    uint32_t update_count;
  } stats;
} PhysicsIsland;

// Island Solver Context
typedef struct {
  PhysicsIsland *islands;
  uint32_t island_count;
  uint32_t island_capacity;

  // Global settings
  bool enable_island_solving;
  bool enable_sleeping;
  bool enable_merging;
  bool enable_splitting;
  float merge_distance_threshold;
  float split_distance_threshold;
  int max_solver_iterations;
  float min_island_size;
  float max_island_size;

  // Body to island mapping
  struct {
    EntityID *entity_ids;
    uint32_t *island_indices;
    uint32_t count;
    uint32_t capacity;
  } body_mapping;

  // Contact and constraint buffers
  struct {
    IslandContact *contact_buffer;
    uint32_t contact_count;
    uint32_t contact_capacity;

    IslandConstraint *constraint_buffer;
    uint32_t constraint_count;
    uint32_t constraint_capacity;
  } buffers;

  // Performance tracking
  struct {
    uint32_t total_islands;
    uint32_t active_islands;
    uint32_t sleeping_islands;
    uint32_t merged_islands;
    uint32_t split_islands;
    float island_build_time;
    float island_solve_time;
    float total_time;
    float average_island_size;
    float largest_island_size;
  } stats;
} IslandSolver;

// Island Solver Functions
bool island_solver_init(IslandSolver *solver, uint32_t max_islands,
                        uint32_t max_bodies);
void island_solver_cleanup(IslandSolver *solver);
void island_solver_update(IslandSolver *solver, PhysicsWorld *world,
                          float time_step);

// Island Management
uint32_t island_solver_create_island(IslandSolver *solver, IslandType type);
bool island_solver_remove_island(IslandSolver *solver, uint32_t island_id);
bool island_solver_merge_islands(IslandSolver *solver, uint32_t island_a_id,
                                 uint32_t island_b_id);
bool island_solver_split_island(IslandSolver *solver, uint32_t island_id);
PhysicsIsland *island_solver_get_island(IslandSolver *solver,
                                        uint32_t island_id);

// Body Management
bool island_solver_add_body_to_island(IslandSolver *solver, uint32_t island_id,
                                      EntityID entity_id, Vec3 position,
                                      Vec3 velocity, float mass, bool is_static,
                                      bool is_kinematic);
bool island_solver_remove_body_from_island(IslandSolver *solver,
                                           uint32_t island_id,
                                           EntityID entity_id);
bool island_solver_move_body_to_island(IslandSolver *solver, EntityID entity_id,
                                       uint32_t new_island_id);
uint32_t island_solver_find_body_island(IslandSolver *solver,
                                        EntityID entity_id);

// Contact and Constraint Management
bool island_solver_add_contact(IslandSolver *solver, uint32_t island_id,
                               EntityID entity_a, EntityID entity_b,
                               Vec3 contact_point, Vec3 normal,
                               float penetration, float restitution,
                               float friction);
bool island_solver_add_constraint(IslandSolver *solver, uint32_t island_id,
                                  EntityID entity_a, EntityID entity_b,
                                  uint32_t constraint_id, float break_force,
                                  float break_torque);
void island_solver_remove_expired_contacts(IslandSolver *solver,
                                           uint32_t island_id);
void island_solver_remove_broken_constraints(IslandSolver *solver,
                                             uint32_t island_id);

// Island Building and Updates
void island_solver_build_islands(IslandSolver *solver, PhysicsWorld *world);
void island_solver_update_island_properties(IslandSolver *solver,
                                            uint32_t island_id);
void island_solver_update_island_bounds(IslandSolver *solver,
                                        uint32_t island_id);
void island_solver_update_body_mapping(IslandSolver *solver);

// Island Solving
void island_solver_solve_islands(IslandSolver *solver, float time_step);
void island_solver_solve_single_island(IslandSolver *solver, uint32_t island_id,
                                       float time_step);
void island_solver_integrate_island(IslandSolver *solver, uint32_t island_id,
                                    float time_step);

// Island Merging
bool island_solver_should_merge_islands(const IslandSolver *solver,
                                        uint32_t island_a_id,
                                        uint32_t island_b_id);
void island_solver_merge_island_data(IslandSolver *solver, uint32_t target_id,
                                     uint32_t source_id);
void island_solver_check_merging_opportunities(IslandSolver *solver);

// Island Splitting
bool island_solver_should_split_island(const IslandSolver *solver,
                                       uint32_t island_id);
void island_solver_split_island_data(IslandSolver *solver, uint32_t island_id);
void island_solver_check_splitting_opportunities(IslandSolver *solver);

// Sleep Management
void island_solver_update_sleep_states(IslandSolver *solver, float time_step);
bool island_solver_should_island_sleep(const IslandSolver *solver,
                                       uint32_t island_id);
void island_solver_put_island_to_sleep(IslandSolver *solver,
                                       uint32_t island_id);
void island_solver_wake_up_island(IslandSolver *solver, uint32_t island_id);
void island_solver_wake_up_nearby_islands(IslandSolver *solver,
                                          uint32_t island_id, float radius);

// Collision Detection for Islands
void island_solver_detect_island_collisions(IslandSolver *solver);
bool island_solver_islands_can_collide(const IslandSolver *solver,
                                       uint32_t island_a_id,
                                       uint32_t island_b_id);
void island_solver_add_inter_island_contact(
    IslandSolver *solver, uint32_t island_a_id, uint32_t island_b_id,
    EntityID entity_a, EntityID entity_b, Vec3 contact_point, Vec3 normal);

// Utility Functions
Vec3 island_solver_calculate_center_of_mass(const PhysicsIsland *island);
float island_solver_calculate_total_mass(const PhysicsIsland *island);
Vec3 island_solver_calculate_total_velocity(const PhysicsIsland *island);
bool island_solver_is_island_static(const PhysicsIsland *island);
bool island_solver_is_island_sleeping(const PhysicsIsland *island);

// Debug and Visualization
void island_solver_debug_draw_islands(const IslandSolver *solver);
void island_solver_debug_draw_island_bounds(const IslandSolver *solver);
void island_solver_debug_draw_island_contacts(const IslandSolver *solver);
void island_solver_debug_draw_island_constraints(const IslandSolver *solver);

// Performance and Statistics
void island_solver_get_statistics(const IslandSolver *solver,
                                  uint32_t *total_islands,
                                  uint32_t *active_islands,
                                  uint32_t *sleeping_islands,
                                  float *average_size, float *solve_time);
void island_solver_get_island_statistics(
    const IslandSolver *solver, uint32_t island_id, uint32_t *body_count,
    uint32_t *contact_count, uint32_t *constraint_count, float *solve_time);
void island_solver_reset_statistics(IslandSolver *solver);

// Configuration
void island_solver_set_merge_threshold(IslandSolver *solver, float threshold);
void island_solver_set_split_threshold(IslandSolver *solver, float threshold);
void island_solver_set_max_iterations(IslandSolver *solver, int iterations);
void island_solver_enable_sleeping(IslandSolver *solver, bool enable);
void island_solver_enable_merging(IslandSolver *solver, bool enable);
void island_solver_enable_splitting(IslandSolver *solver, bool enable);

// Validation and Debugging
bool island_solver_validate_island(const IslandSolver *solver,
                                   const PhysicsIsland *island);
bool island_solver_validate_solver(const IslandSolver *solver);
void island_solver_debug_print_island_info(const IslandSolver *solver,
                                           uint32_t island_id);
void island_solver_debug_print_solver_stats(const IslandSolver *solver);

// Helper Functions
const char *island_type_get_name(IslandType type);
const char *island_state_get_name(IslandState state);
IslandType island_determine_type(const PhysicsIsland *island);
bool island_bodies_are_connected(const PhysicsIsland *island,
                                 uint32_t body_a_index, uint32_t body_b_index);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_ISLAND_SOLVER_H
