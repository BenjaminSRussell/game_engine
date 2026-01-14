# Physics API Documentation

## Overview

The physics system provides comprehensive 2D and 3D physics simulation capabilities including rigid body dynamics, collision detection, constraints, and advanced features like cloth simulation and fluid dynamics.

## Core Physics API

### Initialization and Management

```c
// Initialize the physics system
void physics_init(void);

// Shutdown the physics system
void physics_shutdown(void);

// Update physics simulation
void physics_update(float dt);

// Set gravity
void physics_set_gravity(float x, float y, float z);

// Get gravity
void physics_get_gravity(float* x, float* y, float* z);
```

### Rigid Body Management

```c
// Create a rigid body
uint32_t physics_create_body(void);

// Destroy a rigid body
void physics_destroy_body(uint32_t body_id);

// Set body position
void physics_set_body_position(uint32_t body_id, float x, float y, float z);

// Get body position
void physics_get_body_position(uint32_t body_id, float* x, float* y, float* z);

// Set body velocity
void physics_set_body_velocity(uint32_t body_id, float x, float y, float z);

// Get body velocity
void physics_get_body_velocity(uint32_t body_id, float* x, float* y, float* z);

// Set body mass
void physics_set_body_mass(uint32_t body_id, float mass);

// Get body mass
float physics_get_body_mass(uint32_t body_id);

// Apply force to body
void physics_apply_force(uint32_t body_id, float fx, float fy, float fz);

// Apply impulse to body
void physics_apply_impulse(uint32_t body_id, float ix, float iy, float iz);

// Set body collision shape
void physics_set_body_shape(uint32_t body_id, PhysicsShape shape, const void* shape_data);

// Enable/disable body
void physics_set_body_enabled(uint32_t body_id, bool enabled);

// Set body friction
void physics_set_body_friction(uint32_t body_id, float friction);

// Set body restitution (bounciness)
void physics_set_body_restitution(uint32_t body_id, float restitution);
```

### Collision Detection

```c
// Check if two bodies are colliding
bool physics_check_collision(uint32_t body1_id, uint32_t body2_id);

// Get collision information
bool physics_get_collision_info(uint32_t body1_id, uint32_t body2_id, 
                                CollisionInfo* info);

// Set collision callback
void physics_set_collision_callback(CollisionCallback callback);

// Raycast test
bool physics_raycast(const float* start, const float* direction, 
                    float max_distance, RaycastResult* result);

// Sphere sweep test
bool physics_sphere_sweep(const float* start, const float* end, 
                         float radius, SweepResult* result);
```

### Constraints and Joints

```c
// Create a fixed joint
uint32_t physics_create_fixed_joint(uint32_t body1_id, uint32_t body2_id);

// Create a hinge joint
uint32_t physics_create_hinge_joint(uint32_t body1_id, uint32_t body2_id,
                                   const float* anchor, const float* axis);

// Create a distance joint
uint32_t physics_create_distance_joint(uint32_t body1_id, uint32_t body2_id,
                                      float min_distance, float max_distance);

// Create a spring joint
uint32_t physics_create_spring_joint(uint32_t body1_id, uint32_t body2_id,
                                     float rest_length, float stiffness, float damping);

// Destroy a joint
void physics_destroy_joint(uint32_t joint_id);

// Set joint motor
void physics_set_joint_motor(uint32_t joint_id, float target_velocity, float max_force);
```

## Advanced Physics Features

### Cloth Simulation

```c
// Create cloth object
uint32_t physics_create_cloth(const ClothConfig* config);

// Destroy cloth object
void physics_destroy_cloth(uint32_t cloth_id);

// Set cloth position
void physics_set_cloth_position(uint32_t cloth_id, const float* position);

// Apply wind force to cloth
void physics_apply_cloth_wind(uint32_t cloth_id, const float* wind_force);

// Set cloth constraints
void physics_set_cloth_constraints(uint32_t cloth_id, const ClothConstraints* constraints);

// Get cloth vertex positions
void physics_get_cloth_vertices(uint32_t cloth_id, float* vertices, uint32_t max_vertices);
```

### Fluid Dynamics

```c
// Create fluid simulation
uint32_t physics_create_fluid(const FluidConfig* config);

// Destroy fluid simulation
void physics_destroy_fluid(uint32_t fluid_id);

// Add fluid particle
void physics_add_fluid_particle(uint32_t fluid_id, const float* position, const float* velocity);

// Apply fluid forces
void physics_apply_fluid_forces(uint32_t fluid_id, const float* external_force);

// Get fluid particle data
void physics_get_fluid_particles(uint32_t fluid_id, FluidParticle* particles, uint32_t max_particles);
```

### Soft Body Physics

```c
// Create soft body
uint32_t physics_create_soft_body(const SoftBodyConfig* config);

// Destroy soft body
void physics_destroy_soft_body(uint32_t soft_body_id);

// Set soft body material properties
void physics_set_soft_body_material(uint32_t soft_body_id, float stiffness, float damping);

// Apply soft body deformation
void physics_deform_soft_body(uint32_t soft_body_id, const float* force, const float* point);
```

## Data Structures

### Physics Shapes

```c
typedef enum {
    PHYSICS_SHAPE_SPHERE,
    PHYSICS_SHAPE_BOX,
    PHYSICS_SHAPE_CAPSULE,
    PHYSICS_SHAPE_CYLINDER,
    PHYSICS_SHAPE_CONVEX_HULL,
    PHYSICS_SHAPE_TRIANGLE_MESH
} PhysicsShape;

typedef struct {
    float radius;
} SphereShape;

typedef struct {
    float width, height, depth;
} BoxShape;

typedef struct {
    float radius, height;
} CapsuleShape;
```

### Collision Information

```c
typedef struct {
    float normal[3];
    float penetration_depth;
    float contact_point[3];
    uint32_t body1_id;
    uint32_t body2_id;
} CollisionInfo;

typedef struct {
    bool hit;
    float position[3];
    float normal[3];
    float distance;
    uint32_t body_id;
} RaycastResult;

typedef struct {
    bool hit;
    float position[3];
    float normal[3];
    float fraction;
    uint32_t body_id;
} SweepResult;
```

### Cloth Configuration

```c
typedef struct {
    uint32_t width_segments;
    uint32_t height_segments;
    float width;
    float height;
    float mass;
    float stiffness;
    float damping;
    bool tearable;
    float tear_threshold;
} ClothConfig;

typedef struct {
    bool* fixed_vertices;
    uint32_t num_fixed_vertices;
} ClothConstraints;
```

### Fluid Configuration

```c
typedef struct {
    float particle_radius;
    float rest_density;
    float gas_constant;
    float viscosity;
    float surface_tension;
    uint32_t max_particles;
    float smoothing_radius;
} FluidConfig;

typedef struct {
    float position[3];
    float velocity[3];
    float density;
    float pressure;
} FluidParticle;
```

## Performance Considerations

### Optimization Tips

1. **Use Simple Shapes**: Prefer spheres and boxes over complex meshes when possible
2. **Sleep Objects**: Enable sleeping for static objects to reduce computation
3. **Broadphase Culling**: Ensure proper spatial partitioning for large scenes
4. **Batch Operations**: Group similar operations together for better cache performance
5. **Memory Management**: Reuse physics objects to avoid allocation overhead

### Performance Monitoring

```c
// Get physics performance statistics
void physics_get_performance_stats(PhysicsStats* stats);

typedef struct {
    uint32_t active_bodies;
    uint32_t sleeping_bodies;
    uint32_t collision_pairs;
    float simulation_time;
    float broadphase_time;
    float narrowphase_time;
    float constraint_solver_time;
} PhysicsStats;
```

## Integration Examples

### Basic Falling Box

```c
// Create a falling box
uint32_t box_id = physics_create_body();
physics_set_body_position(box_id, 0.0f, 10.0f, 0.0f);
physics_set_body_mass(box_id, 1.0f);

BoxShape box_shape = {1.0f, 1.0f, 1.0f};
physics_set_body_shape(box_id, PHYSICS_SHAPE_BOX, &box_shape);

// Create ground
uint32_t ground_id = physics_create_body();
physics_set_body_position(ground_id, 0.0f, -1.0f, 0.0f);
physics_set_body_mass(ground_id, 0.0f); // Static body

BoxShape ground_shape = {50.0f, 1.0f, 50.0f};
physics_set_body_shape(ground_id, PHYSICS_SHAPE_BOX, &ground_shape);
```

### Cloth Simulation

```c
// Create a cloth flag
ClothConfig cloth_config = {
    .width_segments = 20,
    .height_segments = 15,
    .width = 4.0f,
    .height = 3.0f,
    .mass = 0.1f,
    .stiffness = 0.8f,
    .damping = 0.1f,
    .tearable = false,
    .tear_threshold = 0.5f
};

uint32_t cloth_id = physics_create_cloth(&cloth_config);
physics_set_cloth_position(cloth_id, (float[]){0.0f, 5.0f, 0.0f});

// Apply wind
float wind_force[] = {2.0f, 0.0f, 1.0f};
physics_apply_cloth_wind(cloth_id, wind_force);
```

## Error Handling

The physics system uses return codes and error callbacks for error handling:

```c
typedef enum {
    PHYSICS_ERROR_NONE = 0,
    PHYSICS_ERROR_INVALID_BODY,
    PHYSICS_ERROR_INVALID_SHAPE,
    PHYSICS_ERROR_OUT_OF_MEMORY,
    PHYSICS_ERROR_INVALID_PARAMETER
} PhysicsError;

// Get last error
PhysicsError physics_get_last_error(void);

// Set error callback
void physics_set_error_callback(PhysicsErrorCallback callback);
```

## Threading Considerations

The physics system is designed to be thread-safe with the following considerations:

- **Main Thread Updates**: Physics simulation should be updated from a single thread
- **Parallel Queries**: Collision queries can be performed from multiple threads
- **Lock-Free Operations**: Most read operations are lock-free
- **Synchronization Points**: Explicit synchronization is required for certain operations

## Best Practices

1. **Initialize Early**: Initialize physics system during game startup
2. **Consistent Timestep**: Use fixed timestep for stable simulation
3. **Memory Management**: Properly destroy physics objects when done
4. **Error Checking**: Always check return values for errors
5. **Performance Monitoring**: Regularly monitor physics performance
6. **Appropriate Mass**: Use realistic mass values for stable simulation
7. **Shape Optimization**: Choose appropriate collision shapes for performance

## Debugging Features

The physics system includes comprehensive debugging capabilities:

```c
// Enable debug drawing
void physics_enable_debug_draw(bool enabled);

// Set debug draw callback
void physics_set_debug_draw_callback(DebugDrawCallback callback);

// Draw physics world
void physics_debug_draw(void);

// Print physics statistics
void physics_print_stats(void);
```

This documentation provides a comprehensive reference for the physics API, covering all major features and usage patterns.
