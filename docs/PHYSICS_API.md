# Physics API Reference

## Overview

The Minecraft V2 physics engine provides rigid body dynamics with collision detection and resolution. It implements an impulse-based solver with support for boxes and spheres.

## Quick Example

```c
#include <physics/physics.h>
#include <math/vec3.h>

// Create physics world
PhysicsConfig config = {
    .gravity = vec3(0, -9.81f, 0),
    .fixed_timestep = 1.0f/60.0f,
    .velocity_iterations = 10,
    .position_iterations = 5
};
PhysicsWorld *world = physics_world_create(config);

// Create a dynamic box
RigidBody *box = rigid_body_create(BODY_TYPE_DYNAMIC, vec3(0, 10, 0));
Collider *box_collider = collider_create_box(vec3(1, 1, 1)); // Half-extents
rigid_body_attach_collider(box, box_collider);
rigid_body_set_mass(box, 1.0f);
physics_world_add_body(world, box);

// Update loop (60 FPS)
float dt = 1.0f/60.0f;
physics_world_step(world, dt);

// Query position
Vec3 pos = rigid_body_get_position(box);
```

## Core Types

### RigidBody
Represents a physical object in the simulation.

**Types:**
- `BODY_TYPE_STATIC`: Immovable (ground, walls)
- `BODY_TYPE_DYNAMIC`: Affected by forces
- `BODY_TYPE_KINEMATIC`: User-controlled movement

### Collider
Defines the shape for collision detection.

**Shapes:**
- `COLLIDER_TYPE_BOX`: Oriented box (fast, good for most objects)
- `COLLIDER_TYPE_SPHERE`: Perfect sphere (fastest, good for projectiles)

### PhysicsWorld
Container for all physics simulation state.

## API Functions

### World Management

#### `physics_world_create`
```c
PhysicsWorld *physics_world_create(PhysicsConfig config);
```
Creates a new physics world with the specified configuration.

#### `physics_world_destroy`
```c
void physics_world_destroy(PhysicsWorld *world);
```
Destroys physics world and frees all resources.

#### `physics_world_step`
```c
void physics_world_step(PhysicsWorld *world, f32 delta_time);
```
Advances simulation by delta_time seconds. Call once per frame.

### Rigid Body Creation

#### `rigid_body_create`
```c
RigidBody *rigid_body_create(RigidBodyType type, Vec3 position);
```
Creates a rigid body at the specified position.

#### `rigid_body_attach_collider`
```c
void rigid_body_attach_collider(RigidBody *body, Collider *collider);
```
Attaches a collision shape to the body.

#### `physics_world_add_body`
```c
RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body);
```
Adds body to the world for simulation.

### Collider Creation

#### `collider_create_box`
```c
Collider *collider_create_box(Vec3 half_extents);
```
Creates a box collider. `half_extents` = half of box dimensions.

**Example:** Box 2x2x2 units → `vec3(1, 1, 1)`

#### `collider_create_sphere`
```c
Collider *collider_create_sphere(f32 radius);
```
Creates a sphere collider with given radius.

### Body Properties

#### `rigid_body_set_mass`
```c
void rigid_body_set_mass(RigidBody *body, f32 mass);
```
Sets body mass (kg). Higher mass = harder to move.

#### `rigid_body_set_friction`
```c
void rigid_body_set_friction(RigidBody *body, f32 friction);
```
Sets surface friction coefficient (0.0 = ice, 1.0 = rough).

#### `rigid_body_set_restitution`
```c
void rigid_body_set_restitution(RigidBody *body, f32 restitution);
```
Sets bounciness (0.0 = no bounce, 1.0 = perfect bounce).

### Body State Queries

#### `rigid_body_get_position`
```c
Vec3 rigid_body_get_position(const RigidBody *body);
```
Returns current world position.

#### `rigid_body_get_rotation`
```c
Quat rigid_body_get_rotation(const RigidBody *body);
```
Returns current orientation quaternion.

#### `rigid_body_get_velocity`
```c
Vec3 rigid_body_get_velocity(const RigidBody *body);
```
Returns current linear velocity (m/s).

#### `rigid_body_is_sleeping`
```c
bool rigid_body_is_sleeping(const RigidBody *body);
```
Returns true if body has settled and stopped moving.

## Configuration

### PhysicsConfig Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `gravity` | Vec3 | (0, -9.81, 0) | Gravity vector (m/s²) |
| `fixed_timestep` | f32 | 1/60 | Simulation timestep (seconds) |
| `velocity_iterations` | u32 | 10 | Solver iterations for velocity |
| `position_iterations` | u32 | 5 | Solver iterations for position correction |

### Presets
```c
PhysicsConfig config_default = physics_config_get_default();
PhysicsConfig config_mc = physics_config_get_minecraft();
```

## Common Patterns

### Creating a Ground Plane
```c
RigidBody *ground = rigid_body_create(BODY_TYPE_STATIC, vec3(0, 0, 0));
Collider *ground_col = collider_create_box(vec3(100, 0.5f, 100));
rigid_body_attach_collider(ground, ground_col);
physics_world_add_body(world, ground);
```

### Spawning a Falling Sphere
```c
RigidBody *ball = rigid_body_create(BODY_TYPE_DYNAMIC, vec3(0, 10, 0));
Collider *ball_col = collider_create_sphere(0.5f);
rigid_body_attach_collider(ball, ball_col);
rigid_body_set_mass(ball, 1.0f);
rigid_body_set_restitution(ball, 0.6f);  // Bouncy
physics_world_add_body(world, ball);
```

### Building a Stack
```c
for (int i = 0; i < 5; i++) {
    Vec3 pos = vec3(0, 1 + i * 2, 0);
    RigidBody *box = rigid_body_create(BODY_TYPE_DYNAMIC, pos);
    Collider *col = collider_create_box(vec3(0.5f, 0.5f, 0.5f));
    rigid_body_attach_collider(box, col);
    rigid_body_set_mass(box, 1.0f);
    rigid_body_set_friction(box, 0.5f);
    physics_world_add_body(world, box);
}
```

## Performance Notes

- **Broadphase**: O(N²) naive implementation (optimized BVH planned)
- **Solver**: Sequential impulse with warm starting
- **Sleeping**: Bodies stop updating when velocity < threshold
- **Typical Performance**: 100-500 bodies at 60 FPS (Debug build)

## Limitations

Current implementation has these constraints:
- Max 4096 bodies per world
- No continuous collision detection (CCD)
- No joints/constraints yet
- No convex mesh collision

## See Also

- `demo_physics_sandbox.c` - Working example
- `src/engine/physics/physics_core_impl.c` - Implementation
- `tests/test_physics_world.c` - Unit tests

## Troubleshooting

**Objects fall through ground:**
- Check ground is `BODY_TYPE_STATIC`
- Ensure colliders are attached
- Increase `velocity_iterations` in config

**Objects jitter/unstable:**
- Reduce timestep: `fixed_timestep = 1.0f/120.0f`
- Increase position iterations
- Check mass ratios (< 10:1 recommended)

** Objects don't bounce:**
- Set `restitution > 0.0f` on both bodies
- Check that bodies actually collide (use test/debug draw)
