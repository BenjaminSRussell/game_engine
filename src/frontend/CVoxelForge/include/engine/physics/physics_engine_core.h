/**
 * =================================================================================================
 *                              PHYSICS ENGINE CORE
 *                              Agent: AGENT_PHYSICS_1
 * =================================================================================================
 *
 * Advanced physics simulation with rigid bodies, soft bodies, and cloth.
 *
 * =================================================================================================
 */

#ifndef PHYSICS_ENGINE_CORE_H
#define PHYSICS_ENGINE_CORE_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// Physics engine constants
#define MAX_PHYSICS_BODIES 4096
#define MAX_CONSTRAINTS 1024
#define MAX_CONTACTS 2048
#define MAX_CONTACT_PAIRS 8192
#define MAX_SOFT_BODIES 64
#define MAX_CLOTHS 32

// Forward declarations
struct PhysicsWorld;

typedef enum CollisionShapeType {
  COLLISION_SHAPE_SPHERE,
  COLLISION_SHAPE_BOX,
  COLLISION_SHAPE_CAPSULE,
  COLLISION_SHAPE_CYLINDER,
  COLLISION_SHAPE_CONE,
  COLLISION_SHAPE_CONVEX_HULL,
  COLLISION_SHAPE_MESH,
  COLLISION_SHAPE_HEIGHTFIELD,
  COLLISION_SHAPE_COMPOUND,
} CollisionShapeType;

typedef struct CollisionShape {
  uint32_t id;
  CollisionShapeType type;
  float local_transform[16];
  float bounds_min[3];
  float bounds_max[3];
  union {
    struct {
      float radius;
    } sphere;
    struct {
      float half_extents[3];
    } box;
    struct {
      float radius;
      float height;
    } capsule;
    struct {
      float radius;
      float height;
    } cylinder;
    struct {
      float radius;
      float height;
    } cone;
    struct {
      float *vertices;
      uint32_t vertex_count;
    } convex_hull;
    struct {
      float *vertices;
      uint32_t *indices;
      uint32_t tri_count;
    } mesh;
    struct {
      float *heights;
      uint32_t width;
      uint32_t depth;
      float scale[3];
    } heightfield;
  } data;
} CollisionShape;

CollisionShape *shape_sphere_create(float radius);
CollisionShape *shape_box_create(float hx, float hy, float hz);
CollisionShape *shape_capsule_create(float radius, float height);
CollisionShape *shape_cylinder_create(float radius, float height);
CollisionShape *shape_cone_create(float radius, float height);
CollisionShape *shape_convex_hull_create(float *vertices, uint32_t count);
CollisionShape *shape_mesh_create(float *vertices, uint32_t v_count,
                                  uint32_t *indices, uint32_t i_count);
CollisionShape *shape_heightfield_create(float *heights, uint32_t width,
                                         uint32_t depth, float scale_x,
                                         float scale_y, float scale_z);
CollisionShape *shape_compound_create(void);
void shape_destroy(CollisionShape *shape);
void shape_calculate_bounds(CollisionShape *shape);
void shape_calculate_inertia(CollisionShape *shape, float mass, float *inertia);
void shape_support_point(CollisionShape *shape, const float *dir,
                         float *out_point);
bool shape_raycast(CollisionShape *shape, const float *origin, const float *dir,
                   float max_dist, float *out_hit);

/* =================================================================================================
 *                                    RIGID BODY
 * =================================================================================================
 */

typedef enum RigidBodyType {
  RIGID_BODY_STATIC,
  RIGID_BODY_KINEMATIC,
  RIGID_BODY_DYNAMIC,
} RigidBodyType;

typedef struct RigidBody {
  uint32_t id;
  RigidBodyType type;

  // Transform
  float position[3];
  float rotation[4];
  float velocity[3];
  float angular_velocity[3];

  // Properties
  float mass;
  float inv_mass;
  float inertia_tensor[9];
  float inv_inertia_tensor[9];
  float linear_damping;
  float angular_damping;
  float friction;
  float restitution;

  // Collision
  CollisionShape *shape;
  uint32_t collision_layer;
  uint32_t collision_mask;

  // Forces
  float accumulated_force[3];
  float accumulated_torque[3];

  // State
  bool is_active;
  bool is_sleeping;
  float sleep_timer;
  float energy_threshold;

  // Constraints
  uint32_t *constraint_ids;
  uint32_t constraint_count;

  // User data
  void *user_data;
  uint32_t entity_id;
} RigidBody;

RigidBody *rigid_body_create(uint32_t id, RigidBodyType type);
void rigid_body_destroy(RigidBody *body);
void rigid_body_set_position(RigidBody *body, const float *pos);
void rigid_body_set_rotation(RigidBody *body, const float *rot);
void rigid_body_set_velocity(RigidBody *body, const float *vel);
void rigid_body_set_angular_velocity(RigidBody *body, const float *vel);
void rigid_body_apply_force(RigidBody *body, const float *force);
void rigid_body_apply_force_at_point(RigidBody *body, const float *force,
                                     const float *point);
void rigid_body_apply_torque(RigidBody *body, const float *torque);
void rigid_body_apply_impulse(RigidBody *body, const float *impulse);
void rigid_body_apply_impulse_at_point(RigidBody *body, const float *impulse,
                                       const float *point);
void rigid_body_integrate(RigidBody *body, float dt);
void rigid_body_update_inertia(RigidBody *body);
void rigid_body_check_sleeping(RigidBody *body, float dt);
void rigid_body_wake_up(RigidBody *body);
void rigid_body_put_to_sleep(RigidBody *body);
void rigid_body_get_velocity_at_point(RigidBody *body, const float *point,
                                      float *out_vel);
void rigid_body_serialize(RigidBody *body, void *buffer);
void rigid_body_deserialize(RigidBody *body, void *buffer);

/* =================================================================================================
 *                                    COLLISION DETECTION
 * =================================================================================================
 */

typedef struct ContactPoint {
  float position_world_a[3];
  float position_world_b[3];
  float position_local_a[3];
  float position_local_b[3];
  float normal[3];
  float penetration_depth;
  float impulse_normal;
  float impulse_tangent[2];
  float friction;
  float restitution;
} ContactPoint;

typedef struct ContactManifold {
  uint32_t body_a;
  uint32_t body_b;
  ContactPoint points[4];
  uint32_t point_count;
  float normal[3];
  float tangent[2][3];
} ContactManifold;

// RayHit struct needed for functions
typedef struct RayHit {
  RigidBody *body;
  float point[3];
  float normal[3];
  float distance;
  bool hit;
} RayHit;

bool collision_sphere_sphere(const void *a, const void *b, ContactManifold *m);
bool collision_sphere_box(const void *a, const void *b, ContactManifold *m);
bool collision_sphere_capsule(const void *a, const void *b, ContactManifold *m);
bool collision_box_box(const void *a, const void *b, ContactManifold *m);
bool collision_box_capsule(const void *a, const void *b, ContactManifold *m);
bool collision_capsule_capsule(const void *a, const void *b,
                               ContactManifold *m);
bool collision_convex_convex_gjk(const void *a, const void *b,
                                 ContactManifold *m);
bool collision_convex_convex_epa(const void *a, const void *b,
                                 ContactManifold *m);
bool collision_mesh_sphere(const void *a, const void *b, ContactManifold *m);
bool collision_mesh_box(const void *a, const void *b, ContactManifold *m);
bool collision_mesh_convex(const void *a, const void *b, ContactManifold *m);
bool collision_heightfield_sphere(const void *a, const void *b,
                                  ContactManifold *m);
bool collision_heightfield_box(const void *a, const void *b,
                               ContactManifold *m);
bool collision_detect_pair(RigidBody *a, RigidBody *b, ContactManifold *m);
ContactManifold *manifold_create(void);
void manifold_add_point(ContactManifold *m, const ContactPoint *pt);
void manifold_reduce_points(ContactManifold *m);
void manifold_refresh(ContactManifold *m);

/* =================================================================================================
 *                                    BROADPHASE
 * =================================================================================================
 */

typedef struct BroadPhaseAABB {
  float min[3];
  float max[3];
  uint32_t body_id;
} BroadPhaseAABB;

typedef struct BroadPhasePair {
  uint32_t body_a;
  uint32_t body_b;
} BroadPhasePair;

typedef struct BVHNode {
  float bounds_min[3];
  float bounds_max[3];
  uint32_t left_child;
  uint32_t right_child;
  uint32_t body_id;
  bool is_leaf;
} BVHNode;

typedef struct BroadPhase {
  BVHNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;
  uint32_t root;

  BroadPhasePair *pairs;
  uint32_t pair_count;
  uint32_t pair_capacity;

  bool needs_rebuild;
} BroadPhase;

void broadphase_init(BroadPhase *bp);
void broadphase_shutdown(BroadPhase *bp);
void broadphase_insert(BroadPhase *bp, uint32_t body_id, const float *aabb_min,
                       const float *aabb_max);
void broadphase_remove(BroadPhase *bp, uint32_t body_id);
void broadphase_update(BroadPhase *bp);
void broadphase_build_bvh(BroadPhase *bp);
void broadphase_rebuild_incremental(BroadPhase *bp);
void broadphase_find_pairs(BroadPhase *bp, BroadPhasePair *pairs,
                           uint32_t *count);
bool broadphase_query_aabb(BroadPhase *bp, const float *min, const float *max,
                           uint32_t *results, uint32_t *count);
bool broadphase_raycast(BroadPhase *bp, const float *origin, const float *dir,
                        float max_dist, RayHit *out_hit);
float bvh_node_cost(const BVHNode *node);
void bvh_sah_split(BroadPhase *bp, uint32_t node_idx);

/* =================================================================================================
 *                                    CONSTRAINTS
 * =================================================================================================
 */

typedef enum ConstraintType {
  CONSTRAINT_FIXED,
  CONSTRAINT_POINT_TO_POINT,
  CONSTRAINT_HINGE,
  CONSTRAINT_SLIDER,
  CONSTRAINT_CONE_TWIST,
  CONSTRAINT_GENERIC_6DOF,
  CONSTRAINT_SPRING,
  CONSTRAINT_MOTOR,
  CONSTRAINT_GEAR,
} ConstraintType;

typedef struct Constraint {
  uint32_t id;
  ConstraintType type;
  uint32_t body_a;
  uint32_t body_b;

  float local_anchor_a[3];
  float local_anchor_b[3];
  float local_axis_a[3];
  float local_axis_b[3];

  // Constraint-specific data
  union {
    struct {
      float relative_position[3];
      float relative_rotation[4];
    } fixed;
    struct {
      float pivot_a[3];
      float pivot_b[3];
    } point_to_point;
    struct {
      float axis[3];
      float angle_min;
      float angle_max;
      bool has_limits;
    } hinge;
    struct {
      float axis[3];
      float min_limit;
      float max_limit;
      bool has_limits;
    } slider;
  } data;

  // Limits
  float linear_limits[3][2];
  float angular_limits[3][2];
  bool limit_enabled[6];

  // Motor
  float motor_target_velocity[6];
  float motor_max_force[6];
  bool motor_enabled[6];

  // Spring
  float spring_stiffness[6];
  float spring_damping[6];
  bool spring_enabled[6];

  // Solver data
  float accumulated_impulse[6];
  float bias[6];
  float mass_matrix[6];

  bool is_broken;
  float break_force;
  float break_torque;
} Constraint;

Constraint *constraint_fixed_create(RigidBody *a, RigidBody *b);
Constraint *constraint_point_to_point_create(RigidBody *a, RigidBody *b,
                                             const float *pivot_a,
                                             const float *pivot_b);
Constraint *constraint_hinge_create(RigidBody *a, RigidBody *b,
                                    const float *axis);
Constraint *constraint_slider_create(RigidBody *a, RigidBody *b,
                                     const float *axis);
Constraint *constraint_cone_twist_create(RigidBody *a, RigidBody *b);
Constraint *constraint_6dof_create(RigidBody *a, RigidBody *b);
Constraint *constraint_spring_create(RigidBody *a, RigidBody *b);
Constraint *constraint_motor_create(RigidBody *a, RigidBody *b);
Constraint *constraint_gear_create(RigidBody *a, RigidBody *b);
void constraint_destroy(Constraint *c);
void constraint_set_limits(Constraint *c, int axis, float min, float max);
void constraint_set_motor(Constraint *c, int axis, float target_vel,
                          float max_force);
void constraint_set_spring(Constraint *c, int axis, float stiffness,
                           float damping);
void constraint_solve(Constraint *c, float dt);
bool constraint_check_break(Constraint *c);
void constraint_warm_start(Constraint *c);

/* =================================================================================================
 *                                    SOFT BODY
 * =================================================================================================
 */

typedef struct SoftBodyNode {
  float position[3];
  float velocity[3];
  float force[3];
  float mass;
  float inv_mass;
  bool is_fixed;
} SoftBodyNode;

typedef struct SoftBodyLink {
  uint32_t node_a;
  uint32_t node_b;
  float rest_length;
  float stiffness;
} SoftBodyLink;

typedef struct SoftBody {
  uint32_t id;
  SoftBodyNode *nodes;
  uint32_t node_count;
  SoftBodyLink *links;
  uint32_t link_count;

  float global_stiffness;
  float damping;
  float pressure;
  float volume_rest;

  uint32_t *triangles;
  uint32_t triangle_count;

  bool self_collision;
  float self_collision_margin;

  float gravity[3];
  float wind[3];
} SoftBody;

SoftBody *soft_body_create(uint32_t id);
void soft_body_destroy(SoftBody *sb);
SoftBody *soft_body_create_from_mesh(float *vertices, int count);
void soft_body_add_node(SoftBody *sb, const float *pos, float mass);
void soft_body_add_link(SoftBody *sb, int node_a, int node_b);
void soft_body_generate_links(SoftBody *sb);
void soft_body_fix_node(SoftBody *sb, int node, bool fixed);
void soft_body_integrate(SoftBody *sb, float dt);
void soft_body_solve_links(SoftBody *sb);
void soft_body_solve_volume(SoftBody *sb);
void soft_body_apply_gravity(SoftBody *sb, const float *g);
void soft_body_apply_wind(SoftBody *sb, const float *wind);
void soft_body_self_collision(SoftBody *sb);
void soft_body_collide_rigid(SoftBody *sb, RigidBody *rb);
void soft_body_update_mesh(SoftBody *sb);
void soft_body_calculate_normals(SoftBody *sb);

/* =================================================================================================
 *                                    CLOTH SIMULATION
 * =================================================================================================
 */

typedef struct ClothConfig {
  uint32_t resolution_x;
  uint32_t resolution_y;
  float width;
  float height;
  float mass_per_node;
  float structural_stiffness;
  float shear_stiffness;
  float bend_stiffness;
  float damping;
  uint32_t solver_iterations;
} ClothConfig;

typedef struct Cloth {
  SoftBody *soft_body;
  ClothConfig config;

  // Attachments
  struct {
    uint32_t node;
    uint32_t body;
    float local_pos[3];
  } attachments[16];
  uint32_t attachment_count;

  // Rendering
  uint32_t mesh_id;
  bool double_sided;
} Cloth;

Cloth *cloth_create(uint32_t width, uint32_t height);
void cloth_destroy(Cloth *c);
void cloth_create_rect(Cloth *c, float w, float h, int res_x, int res_y);
void cloth_attach_to_body(Cloth *c, int node, RigidBody *rb);
void cloth_detach(Cloth *c, int node);
void cloth_tear(Cloth *c, int link_idx);
void cloth_simulate(Cloth *c, float dt);
void cloth_update_render_mesh(Cloth *c);

/* =================================================================================================
 *                                    PHYSICS WORLD
 * =================================================================================================
 */

typedef struct PhysicsWorld {
  RigidBody *bodies;
  uint32_t body_count;
  uint32_t body_capacity;

  Constraint *constraints;
  uint32_t constraint_count;

  SoftBody *soft_bodies;
  uint32_t soft_body_count;

  Cloth *cloths;
  uint32_t cloth_count;

  BroadPhase broadphase;
  ContactManifold *manifolds;
  uint32_t manifold_count;

  // Contact storage for collision resolution
  ContactManifold *contacts;
  uint32_t contact_count;
  uint32_t contact_capacity;

  float gravity[3];
  float timestep;
  uint32_t velocity_iterations;
  uint32_t position_iterations;

  bool paused;
  float time_accumulator;

  // Callbacks
  void (*on_collision_enter)(uint32_t body_a, uint32_t body_b,
                             ContactPoint *contact);
  void (*on_collision_exit)(uint32_t body_a, uint32_t body_b);
  void (*on_constraint_break)(uint32_t constraint_id);
} PhysicsWorld;

PhysicsWorld *physics_world_create(void);
void physics_world_destroy(PhysicsWorld *world);
void physics_world_step(PhysicsWorld *world, float dt);
void physics_world_step_fixed(PhysicsWorld *world, float dt);
void physics_world_add_body(PhysicsWorld *world, RigidBody *body);
void physics_world_remove_body(PhysicsWorld *world, RigidBody *body);
void physics_world_add_constraint(PhysicsWorld *world, Constraint *c);
void physics_world_remove_constraint(PhysicsWorld *world, Constraint *c);
void physics_world_add_soft_body(PhysicsWorld *world, SoftBody *sb);
void physics_world_add_cloth(PhysicsWorld *world, Cloth *c);
bool physics_world_raycast(PhysicsWorld *world, const float *origin,
                           const float *dir, float max_dist, RayHit *out_hit);
void physics_world_raycast_all(PhysicsWorld *world, const float *origin,
                               const float *dir, float max_dist);
void physics_world_sweep_sphere(PhysicsWorld *world, const float *origin,
                                float radius, const float *dir);
void physics_world_overlap_sphere(PhysicsWorld *world, const float *origin,
                                  float radius);
void physics_world_overlap_box(PhysicsWorld *world, const float *center,
                               const float *extents);
void physics_world_find_contacts(PhysicsWorld *world);
void physics_world_solve_constraints(PhysicsWorld *world, float dt);
void physics_world_solve_positions(PhysicsWorld *world);
void physics_world_integrate_velocities(PhysicsWorld *world, float dt);
void physics_world_integrate_positions(PhysicsWorld *world, float dt);
void physics_world_debug_draw(PhysicsWorld *world);
void physics_world_serialize(PhysicsWorld *world, const char *path);
void physics_world_deserialize(PhysicsWorld *world, const char *path);

/* =================================================================================================
 *                                    CHARACTER CONTROLLER
 * =================================================================================================
 */

typedef struct CharacterController {
  uint32_t id;

  // Capsule shape
  float radius;
  float height;
  float step_height;
  float slope_limit;

  // Position
  float position[3];
  float velocity[3];

  // State
  bool is_grounded;
  float ground_normal[3];
  bool is_sliding;
  float time_in_air;

  // Collision
  uint32_t collision_layer;
  uint32_t collision_mask;

  // Movement
  float move_speed;
  float jump_velocity;
  float gravity_scale;
  float air_control;
  float ground_friction;
} CharacterController;

// CharacterController declarations moved to include/physics/physics.h to avoid
// conflicts CharacterController *character_controller_create(void); void
// character_controller_destroy(CharacterController *cc); void
// character_controller_move(CharacterController *cc, const float *motion,
//                                float dt);
// void character_controller_slide(CharacterController *cc, const float
// *normal); void character_controller_step_up(CharacterController *cc); void
// character_controller_step_down(CharacterController *cc); bool
// character_controller_ground_check(CharacterController *cc); bool
// character_controller_slope_check(CharacterController *cc); void
// character_controller_jump(CharacterController *cc); void
// character_controller_push_bodies(CharacterController *cc);

#endif // PHYSICS_ENGINE_CORE_H
