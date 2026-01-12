# Physics Demos

A comprehensive collection of physics simulation demonstrations showcasing various physics scenarios and behaviors.

## Available Demos

### 1. Stack Collapse
- **Description**: Demonstrates stacking stability and chain reactions
- **Features**: 
  - Tall stack of boxes with realistic physics
  - Heavy ball triggers collapse
  - Random impulses for chaos
- **Controls**: Space to reset

### 2. Newton's Cradle
- **Description**: Conservation of momentum and energy
- **Features**:
  - 5 suspended balls with realistic collision response
  - Spring constraints simulate string suspension
  - Energy transfer visualization
- **Controls**: 
  - A: Push left ball
  - D: Push right ball
  - Space: Reset

### 3. Domino Chain
- **Description**: Chain reaction simulation with complex patterns
- **Features**:
  - Curved domino arrangement
  - Staircase pattern
  - Multiple trigger balls
- **Controls**:
  - P: Add pusher ball
  - Space: Reset

### 4. Ragdoll Physics
- **Description**: Character simulation with articulated constraints
- **Features**:
  - 15 body parts with realistic joints
  - Ball and hinge constraints
  - Natural falling and collision behavior
- **Controls**:
  - F: Apply random impact
  - R: Drop from height
  - Space: Reset

### 5. Vehicle Physics
- **Description**: Car simulation with wheels and suspension
- **Features**:
  - Spring-based suspension system
  - Engine force and steering
  - Realistic tire friction
  - Obstacle course
- **Controls**:
  - W/S or Up/Down: Accelerate/Brake
  - A/D or Left/Right: Steer
  - Space: Reset

### 6. Fluid Simulation
- **Description**: Particle-based fluid dynamics using SPH
- **Features**:
  - Smoothed Particle Hydrodynamics (SPH)
  - Pressure and viscosity forces
  - Container with walls
  - Interactive particle addition
- **Controls**:
  - P: Add particles
  - E: Create explosion
  - Space: Reset

## Building and Running

### Prerequisites
- CMake 3.10+
- OpenGL 3.3+ or Metal/Vulkan support
- C compiler (GCC, Clang, MSVC)

### Build Commands
```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build physics demos
make physics_demos

# Run the demos
./bin/physics_demos
```

### Controls
- **1-5**: Switch between different demos
- **ESC**: Exit application
- **Space**: Reset current demo
- **Demo-specific**: See individual demo descriptions

## Physics Features Demonstrated

### Core Physics
- Rigid body dynamics
- Collision detection and response
- Constraint solving
- Force integration

### Advanced Features
- **Stack Collapse**: Stability, friction, restitution
- **Newton's Cradle**: Conservation laws, spring constraints
- **Domino Chain**: Chain reactions, precise timing
- **Ragdoll**: Articulated bodies, joint limits
- **Vehicle Physics**: Suspension, friction models, steering
- **Fluid Simulation**: SPH, pressure forces, viscosity

### Performance Optimizations
- Spatial partitioning for collision detection
- Fixed timestep integration
- Object pooling for particles
- Efficient constraint solving

## Technical Details

### Physics Engine Integration
- Uses the game's core physics engine
- Demonstrates both high-level and low-level APIs
- Shows proper integration patterns

### Rendering Integration
- Real-time visualization of physics objects
- Debug rendering for constraints and forces
- Performance metrics display

### Extensibility
- Easy to add new demos
- Modular demo registration system
- Reusable utility functions

## Educational Value

These demos serve as:
1. **Physics Validation**: Test and verify physics engine correctness
2. **Performance Benchmarks**: Measure physics simulation performance
3. **Learning Tools**: Understand physics concepts visually
4. **Integration Examples**: Show proper physics engine usage
5. **Regression Tests**: Catch physics engine bugs and regressions

## Troubleshooting

### Common Issues
- **Low Performance**: Reduce particle count in fluid demo
- **Instability**: Check timestep size and constraint parameters
- **Visual Glitches**: Verify rendering integration

### Debug Mode
Enable debug rendering to see:
- Collision shapes
- Constraint connections
- Force vectors
- Contact points

## Future Enhancements

### Planned Demos
- Cloth simulation
- Soft body physics
- Destruction and fracture
- Rope and cable physics
- Buoyancy and floating

### Improvements
- GPU acceleration for particle systems
- Advanced material properties
- Multi-threading support
- Better user interface
