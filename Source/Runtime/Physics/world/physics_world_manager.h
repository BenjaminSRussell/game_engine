/**
 * @file physics_world_manager.h
 * @brief Physics world manager
 *
 * Central management system for the physics world that coordinates
 * all physics systems, provides unified API, and manages object lifecycles.
 */

#ifndef PHYSICS_WORLD_MANAGER_H
#define PHYSICS_WORLD_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsWorld PhysicsWorld;
typedef struct PhysicsObject PhysicsObject;
typedef struct PhysicsBody PhysicsBody;
typedef struct PhysicsConstraint PhysicsConstraint;
typedef struct PhysicsMaterial PhysicsMaterial;

// ========================================
// Physics Object Types
// ========================================

typedef enum PhysicsObjectType {
    PHYSICS_OBJECT_RIGID_BODY,        // Rigid body object
    PHYSICS_OBJECT_SOFT_BODY,         // Soft body object
    PHYSICS_OBJECT_CLOTH,             // Cloth object
    PHYSICS_OBJECT_PARTICLE,          // Particle system
    PHYSICS_OBJECT_FLUID,             // Fluid system
    PHYSICS_OBJECT_TERRAIN,           // Terrain object
    PHYSICS_OBJECT_TRIGGER,           // Trigger volume
    PHYSICS_OBJECT_SENSOR,            // Sensor volume
    PHYSICS_OBJECT_JOINT,              // Joint/constraint
    PHYSICS_OBJECT_FORCE_FIELD         // Force field
} PhysicsObjectType;

// ========================================
// Physics Body Types
// ========================================

typedef enum PhysicsBodyType {
    PHYSICS_BODY_STATIC,              // Static body (immovable)
    PHYSICS_BODY_DYNAMIC,            // Dynamic body (movable)
    PHYSICS_BODY_KINEMATIC,           // Kinematic body (controlled)
    PHYSICS_BODY_SLEEPING             // Sleeping body (optimized)
} PhysicsBodyType;

// ========================================
// Physics Material Properties
// ========================================

struct PhysicsMaterial {
    float density;                  // Material density
    float friction;                 // Friction coefficient
    float restitution;              // Restitution (bounciness)
    float damping;                  // Linear damping
    float angular_damping;          // Angular damping
    float thickness;                // Material thickness
    bool is_trigger;                // Is trigger material
    bool is_sensor;                 // Is sensor material
    uint32_t material_id;            // Material ID
    char name[64];                  // Material name
};

// ========================================
// Physics Object Structure
// ========================================

struct PhysicsObject {
    uint64_t id;                     // Unique object ID
    PhysicsObjectType type;          // Object type
    PhysicsBodyType body_type;       // Body type
    void *data;                      // Object-specific data
    PhysicsMaterial *material;      // Material properties
    bool active;                     // Object is active
    bool sleeping;                    // Object is sleeping
    bool visible;                     // Object is visible
    uint32_t layer_mask;            // Collision layer mask
    uint32_t category_mask;          // Collision category mask
    float transform[16];              // 4x4 transform matrix
    float velocity[3];                // Linear velocity
    float angular_velocity[3];         // Angular velocity
    float force[3];                   // Applied force
    float torque[3];                  // Applied torque
    float mass;                       // Object mass
    float inertia[9];                 // Inertia tensor (3x3)
    float center_of_mass[3];           // Center of mass
    float bounding_box[6];            // AABB bounding box
    float bounding_sphere[4];          // Bounding sphere (center, radius)
    void *user_data;                  // User data pointer
    uint32_t ref_count;              // Reference count
};

// ========================================
// Physics Body Structure
// ========================================

struct PhysicsBody {
    PhysicsObject *object;            // Parent object
    float position[3];                // Position
    float orientation[4];              // Quaternion orientation
    float linear_velocity[3];         // Linear velocity
    float angular_velocity[3];        // Angular velocity
    float linear_acceleration[3];     // Linear acceleration
    float angular_acceleration[3];    // Angular acceleration
    float force_accumulator[3];       // Accumulated forces
    float torque_accumulator[3];      // Accumulated torques
    float mass;                       // Mass
    float inverse_mass;               // Inverse mass
    float inertia[9];                 // Inertia tensor
    float inverse_inertia[9];         // Inverse tensor
    float sleep_threshold;            // Sleep threshold
    bool is_sleeping;                 // Is sleeping
    uint32_t island_id;               // Island ID for sleeping
};

// ========================================
// Physics Constraint Structure
// ========================================

struct PhysicsConstraint {
    uint64_t id;                     // Constraint ID
    uint64_t body_a_id;               // First body ID
    uint64_t body_b_id;               // Second body ID
    uint32_t type;                    // Constraint type
    void *data;                      // Constraint-specific data
    bool enabled;                     // Constraint is enabled
    float break_force;                // Force at which constraint breaks
    float current_force;              // Current force on constraint
    bool is_broken;                   // Constraint is broken
};

// ========================================
// Physics World Configuration
// ========================================

typedef struct PhysicsWorldConfig {
    // Simulation parameters
    float gravity[3];                 // Gravity vector
    float time_step;                 // Fixed time step
    float max_time_step;              // Maximum time step
    int velocity_iterations;          // Velocity solver iterations
    int position_iterations;          // Position solver iterations
    
    // Performance parameters
    bool enable_sleeping;            // Enable sleeping objects
    bool enable_warm_starting;        // Enable warm starting
    bool enable_adaptive_time_step;    // Enable adaptive time stepping
    float sleep_threshold;           // Sleep threshold
    float wake_up_threshold;          // Wake up threshold
    
    // Collision parameters
    bool enable_collision_detection;  // Enable collision detection
    bool enable_collision_response;    // Enable collision response
    bool enable_continuous_collision; // Enable continuous collision detection
    float collision_margin;           // Collision margin
    
    // Debug parameters
    bool enable_debug_draw;           // Enable debug drawing
    bool enable_profiling;            // Enable profiling
    bool enable_validation;           // Enable validation
    bool enable_logging;              // Enable logging
    
    // Memory parameters
    int max_objects;                 // Maximum objects
    int max_constraints;              // Maximum constraints
    int max_contacts;                // Maximum contacts
    size_t memory_budget;            // Memory budget in bytes
    
} PhysicsWorldConfig;

// ========================================
// Physics World Structure
// ========================================

struct PhysicsWorld {
    // Configuration
    PhysicsWorldConfig config;
    
    // Object management
    PhysicsObject *objects;            // Object array
    int object_count;                 // Number of objects
    int object_capacity;              // Object array capacity
    uint64_t next_object_id;          // Next object ID
    
    // Body management
    PhysicsBody *bodies;               // Body array
    int body_count;                   // Number of bodies
    int body_capacity;                 // Body array capacity
    
    // Constraint management
    PhysicsConstraint *constraints;    // Constraint array
    int constraint_count;             // Number of constraints
    int constraint_capacity;           // Constraint array capacity
    
    // Material management
    PhysicsMaterial *materials;        // Material array
    int material_count;               // Number of materials
    int material_capacity;             // Material array capacity
    
    // Simulation state
    float current_time;               // Current simulation time
    float accumulated_time;           // Accumulated time
    int substep_count;               // Number of substeps
    
    // Performance metrics
    float update_time_ms;             // Update time in milliseconds
    int active_objects;              // Number of active objects
    int sleeping_objects;            // Number of sleeping objects
    int collision_pairs;              // Number of collision pairs
    int constraint_count_active;       // Number of active constraints
    
    // Debug state
    bool debug_enabled;               // Debug drawing enabled
    void *debug_renderer;             // Debug renderer interface
    void *profiler;                   // Performance profiler
    
    // User callbacks
    void (*collision_callback)(uint64_t obj_a, uint64_t obj_b, const float *contact_point, const float *normal, float penetration);
    void (*break_callback)(uint64_t constraint_id, uint64_t obj_a, uint64_t obj_b);
    void (*sleep_callback)(uint64_t object_id, bool sleeping);
    void *callback_user_data;
    
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics world
 * @param config World configuration (NULL for default)
 * @return Physics world instance or NULL on failure
 */
PhysicsWorld* physics_world_create(const PhysicsWorldConfig *config);

/**
 * Destroy physics world
 * @param world Physics world instance
 */
void physics_world_destroy(PhysicsWorld *world);

// ========================================
// Configuration Management
// ========================================

/**
 * Get default world configuration
 * @return Default configuration
 */
PhysicsWorldConfig physics_world_get_default_config(void);

/**
 * Set world configuration
 * @param world Physics world instance
 *param config New configuration
 */
void physics_world_set_config(PhysicsWorld *world, const PhysicsWorldConfig *config);

/**
 * Get current world configuration
 * @param world Physics world instance
 * @return Current configuration
 */
PhysicsWorldConfig physics_world_get_config(PhysicsWorld *world);

// ========================================
// Object Management
// ========================================

/**
 * Create physics object
 * @param world Physics world instance
 * @param type Object type
 * @param material Material properties
 * @return Object ID or 0 on failure
 */
uint64_t physics_world_create_object(PhysicsWorld *world, PhysicsObjectType type, const PhysicsMaterial *material);

/**
 * Destroy physics object
 * @param world Physics world instance
 * @param object_id Object ID
 * @return True if successful
 */
bool physics_world_destroy_object(PhysicsWorld *world, uint64_t object_id);

/**
 * Get physics object
 * @param world Physics world instance
 * @param object_id Object ID
 * @return Object pointer or NULL if not found
 */
PhysicsObject* physics_world_get_object(PhysicsWorld *world, uint64_t object_id);

/**
 * Find objects by type
 * @param world Physics world instance
 * @param type Object type
 * @param objects Output object array
 * @param max_objects Maximum objects to return
 * @return Number of objects found
 */
int physics_world_find_objects_by_type(PhysicsWorld *world, PhysicsObjectType type, 
                                       PhysicsObject **objects, int max_objects);

/**
 * Find objects in region
 * @param world Physics world instance
 * @param min_point Region minimum point
 * @param max_point Region maximum point
 * @param objects Output object array
 * @param max_objects Maximum objects to return
 * @return Number of objects found
 */
int physics_world_find_objects_in_region(PhysicsWorld *world, const float *min_point, 
                                         const float *max_point, PhysicsObject **objects, int max_objects);

/**
 * Find objects by layer mask
 * @param world Physics world instance
 * @param layer_mask Layer mask to match
 * @param objects Output object array
 * @param max_objects Maximum objects to return
 * @return Number of objects found
 */
int physics_world_find_objects_by_layer(PhysicsWorld *world, uint32_t layer_mask, 
                                        PhysicsObject **objects, int max_objects);

/**
 * Set object transform
 * @param world Physics world instance
 * @param object_id Object ID
 * @param transform Transform matrix (4x4)
 */
void physics_world_set_object_transform(PhysicsWorld *world, uint64_t object_id, const float *transform);

/**
 * Get object transform
 * @param world Physics world instance
 * @param object_id Object ID
 * @param transform Output transform matrix (4x4)
 */
void physics_world_get_object_transform(PhysicsWorld *world, uint64_t object_id, float *transform);

/**
 * Set object velocity
 * @param world Physics world instance
 * @param object_id Object ID
 * @param velocity Linear velocity
 * @param angular_velocity Angular velocity
 */
void physics_world_set_object_velocity(PhysicsWorld *world, uint64_t object_id, 
                                       const float *velocity, const float *angular_velocity);

/**
 * Get object velocity
 * @param world Physics world instance
 * @param object_id Object ID
 * @param velocity Output linear velocity
 * @param angular_velocity Output angular velocity
 */
void physics_world_get_object_velocity(PhysicsWorld *world, uint64_t object_id, 
                                       float *velocity, float *angular_velocity);

/**
 * Apply force to object
 * @param world Physics world instance
 * @param object_id Object ID
 * @param force Force vector
 * @param point Application point (world space)
 */
void physics_world_apply_force(PhysicsWorld *world, uint64_t object_id, 
                               const float *force, const float *point);

/**
 * Apply impulse to object
 * @param world Physics world instance
 * @param object_id Object ID
 * @param impulse Impulse vector
 * @param point Application point (world space)
 */
void physics_world_apply_impulse(PhysicsWorld *world, uint64_t object_id, 
                                  const float *impulse, const float *point);

/**
 * Apply torque to object
 * @param world Physics world instance
 * @param object_id Object ID
 * @param torque Torque vector
 */
void physics_world_apply_torque(PhysicsWorld *world, uint64_t object_id, const float *torque);

/**
 * Set object sleeping state
 * @param world Physics world instance
 * @param object_id Object ID
 * @param sleeping Sleeping state
 */
void physics_world_set_object_sleeping(PhysicsWorld *world, uint64_t object_id, bool sleeping);

/**
 * Get object sleeping state
 * @param world Physics world instance
 * @param object_id Object ID
 * @return True if object is sleeping
 */
bool physics_world_is_object_sleeping(PhysicsWorld *world, uint64_t object_id);

// ========================================
// Material Management
// ========================================

/**
 * Create physics material
 * @param world Physics world instance
 * @param name Material name
 * @param density Material density
 * @param friction Friction coefficient
 * @param restitution Restitution coefficient
 * @return Material ID or 0 on failure
 */
uint32_t physics_world_create_material(PhysicsWorld *world, const char *name, 
                                        float density, float friction, float restitution);

/**
 * Destroy physics material
 * @param world Physics world instance
 * @param material_id Material ID
 * @return True if successful
 */
bool physics_world_destroy_material(PhysicsWorld *world, uint32_t material_id);

/**
 * Get physics material
 * @param world Physics world instance
 * @param material_id Material ID
 * @return Material pointer or NULL if not found
 */
PhysicsMaterial* physics_world_get_material(PhysicsWorld *world, uint32_t material_id);

/**
 * Find material by name
 * @param world Physics world instance
 * @param name Material name
 * @return Material ID or 0 if not found
 */
uint32_t physics_world_find_material(PhysicsWorld *world, const char *name);

// ========================================
// Simulation Control
// ========================================

/**
 * Update physics simulation
 * @param world Physics world instance
 * @param dt Time step
 */
void physics_world_update(PhysicsWorld *world, float dt);

/**
 * Step physics simulation with fixed time step
 * @param world Physics world instance
 */
void physics_world_step(PhysicsWorld *world);

/**
 * Reset physics simulation
 * @param world Physics world instance
 */
void physics_world_reset(PhysicsWorld *world);

/**
 * Pause physics simulation
 * @param world Physics world instance
 */
void physics_world_pause(PhysicsWorld *world);

/**
 * Resume physics simulation
 * @param world Physics world instance
 */
void physics_world_resume(PhysicsWorld *world);

/**
 * Get simulation time
 * @param world Physics world instance
 *return Current simulation time
 */
float physics_world_get_time(PhysicsWorld *world);

/**
 * Set simulation time
 * @param world Physics world instance
 * @param time Simulation time
 */
void physics_world_set_time(PhysicsWorld *world, float time);

// ========================================
// Collision Management
// ========================================

/**
 * Enable/disable collision detection
 * @param world Physics world instance
 * @param enabled Enable collision detection
 */
void physics_world_set_collision_enabled(PhysicsWorld *world, bool enabled);

/**
 * Set collision layers
 * @param world Physics world instance
 * @param object_id Object ID
 * @param layer_mask Layer mask
 */
void physics_world_set_collision_layers(PhysicsWorld *world, uint64_t object_id, uint32_t layer_mask);

/**
 * Set collision categories
 * @param world Physics world instance
 * @param object_id Object ID
 * @param category_mask Category mask
 */
void physics_world_set_collision_categories(PhysicsWorld *world, uint64_t object_id, uint32_t category_mask);

/**
 * Ray cast in physics world
 * @param world Physics world instance
 * @param origin Ray origin
 * @param direction Ray direction (normalized)
 * @param max_distance Maximum distance
 *param hit_object Output hit object ID
 *param hit_point Output hit point
 * hit_normal Output hit normal
 * @param hit_distance Output hit distance
 * @return True if ray hit something
 */
bool physics_world_ray_cast(PhysicsWorld *world, const float *origin, const float *direction,
                           float max_distance, uint64_t *hit_object, 
                           float *hit_point, float *hit_normal, float *hit_distance);

/**
 * Sphere cast in physics world
 * @param world Physics world instance
 * @param center Sphere center
 * @param radius Sphere radius
 *param objects Output hit object IDs
 * @param max_objects Maximum objects to return
 * @return Number of objects hit
 */
int physics_world_sphere_cast(PhysicsWorld *world, const float *center, float radius,
                             uint64_t *objects, int max_objects);

// ========================================
// Constraint Management
// ========================================

/**
 * Create constraint
 * @param world Physics world instance
 * @param body_a_id First body ID
 * @param body_b_id Second body ID
 * @param type Constraint type
 * @param data Constraint-specific data
 * @return Constraint ID or 0 on failure
 */
uint64_t physics_world_create_constraint(PhysicsWorld *world, uint64_t body_a_id, uint64_t body_b_id,
                                         uint32_t type, const void *data);

/**
 * Destroy constraint
 * @param world Physics world instance
 * @param constraint_id Constraint ID
 * @return True if successful
 */
bool physics_world_destroy_constraint(PhysicsWorld *world, uint64_t constraint_id);

/**
 * Get constraint
 * @param world Physics world instance
 * @param constraint_id Constraint ID
 * @return Constraint pointer or NULL if not found
 */
PhysicsConstraint* physics_world_get_constraint(PhysicsWorld *world, uint64_t constraint_id);

/**
 * Enable/disable constraint
 * @param world Physics world instance
 * @param constraint_id Constraint ID
 * @param enabled Enable constraint
 */
void physics_world_set_constraint_enabled(PhysicsWorld *world, uint64_t constraint_id, bool enabled);

/**
 * Set constraint break force
 * @param world Physics world instance
 * @param constraint_id Constraint ID
 * @param break_force Break force
 */
void physics_world_set_constraint_break_force(PhysicsWorld *world, uint64_t constraint_id, float break_force);

// ========================================
// Callback Management
// ========================================

/**
 * Set collision callback
 * @param world Physics world instance
 * @param callback Collision callback function
 * @param user_data User data pointer
 */
void physics_world_set_collision_callback(PhysicsWorld *world, 
                                           void (*callback)(uint64_t, uint64_t, const float*, const float*, float),
                                           void *user_data);

/**
 * Set break callback
 * @param world Physics world instance
 * @param callback Break callback function
 * @param user_data User data pointer
 */
void physics_world_set_break_callback(PhysicsWorld *world, 
                                       void (*callback)(uint64_t, uint64_t, uint64_t),
                                       void *user_data);

/**
 * Set sleep callback
 * @param world Physics world instance
 * @param callback Sleep callback function
 * @param user_data User data pointer
 */
void physics_world_set_sleep_callback(PhysicsWorld *world, 
                                      void (*callback)(uint64_t, bool),
                                      void *user_data);

// ========================================
// Statistics and Debugging
// ========================================

/**
 * Get world statistics
 * @param world Physics world instance
 * @param object_count Number of objects
 * @param body_count Number of bodies
 * @param constraint_count Number of constraints
 * @param collision_pairs Number of collision pairs
 * @param active_objects Number of active objects
 * @param sleeping_objects Number of sleeping objects
 * @param update_time Update time in milliseconds
 */
void physics_world_get_stats(PhysicsWorld *world, int *object_count, int *body_count,
                              int *constraint_count, int *collision_pairs,
                              int *active_objects, int *sleeping_objects,
                              float *update_time);

/**
 * Enable debug drawing
 * @param world Physics world instance
 * @param enabled Enable debug drawing
 */
void physics_world_set_debug_enabled(PhysicsWorld *world, bool enabled);

/**
 * Set debug renderer
 * @param world Physics world instance
 * @param renderer Debug renderer interface
 */
void physics_world_set_debug_renderer(PhysicsWorld *world, void *renderer);

/**
 * Enable profiling
 * @param world Physics world instance
 * @param enabled Enable profiling
 */
void physics_world_set_profiling_enabled(PhysicsWorld *world, bool enabled);

/**
 * Get performance report
 * @param world Physics world instance
 *param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Number of characters written
 */
int physics_world_get_performance_report(PhysicsWorld *world, char *buffer, int buffer_size);

/**
 * Validate world state
 * @param world Physics world instance
 * @return True if world state is valid
 */
bool physics_world_validate(PhysicsWorld *world);

// ========================================
// Serialization
// ========================================

/**
 * Serialize world state to buffer
 * @param world Physics world instance
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Number of bytes written
 */
size_t physics_world_serialize(PhysicsWorld *world, void *buffer, size_t buffer_size);

/**
 * Deserialize world state from buffer
 * @param world Physics world instance
 * @param buffer Input buffer
 * @param buffer_size Buffer size
 * @return Number of bytes read
 */
size_t physics_world_deserialize(PhysicsWorld *world, const void *buffer, size_t buffer_size);

/**
 * Save world to file
 * @param world Physics world instance
 * @param filename Output filename
 * @return True if successful
 */
bool physics_world_save_to_file(PhysicsWorld *world, const char *filename);

/**
 * Load world from file
 * @param world Physics world instance
 * @param filename Input filename
 * @return True if successful
 */
bool physics_world_load_from_file(PhysicsWorld *world, const char *filename);

// ========================================
// Utility Functions
// ========================================

/**
 * Get gravity vector
 * @param world Physics world instance
 * @param gravity Output gravity vector
 */
void physics_world_get_gravity(PhysicsWorld *world, float *gravity);

/**
 * Set gravity vector
 * @param world Physics world instance
 * @param gravity New gravity vector
 */
void physics_world_set_gravity(PhysicsWorld *world, const float *gravity);

/**
 * Get time step
 * @param world Physics world instance
 * @return Time step
 */
float physics_world_get_time_step(PhysicsWorld *world);

/**
 * Set time step
 * @param world Physics world instance
 * @param time_step New time step
 */
void physics_world_set_time_step(PhysicsWorld *world, float time_step);

/**
 * Get object count
 * @param world Physics world instance
 * @return Number of objects
 */
int physics_world_get_object_count(PhysicsWorld *world);

/**
 * Get body count
 * @param world Physics world instance
 * @return Number of bodies
 */
int physics_world_get_body_count(PhysicsWorld *world);

/**
 * Get constraint count
 * @param world Physics world instance
 * @return Number of constraints
 */
int physics_world_get_constraint_count(PhysicsWorld *world);

/**
 * Check if world is valid
 * @param world Physics world instance
 * @return True if world is valid
 */
bool physics_world_is_valid(PhysicsWorld *world);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_WORLD_MANAGER_H */
