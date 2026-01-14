# Physics Subsystem Architecture

> **Priority 2** | Est. 8,000+ files | 800K+ LOC | ⭐⭐⭐⭐⭐ Complexity

The physics subsystem handles all physical simulation including rigid bodies, soft bodies, fluids, and vehicles.

---

## Directory Structure

```
Physics/
├── Private/
├── Public/
│
├── Core/                       # Physics core systems
│   ├── World/                  # Physics world management
│   │   ├── Partitioning/       # Spatial partitioning
│   │   ├── Stepping/           # Time stepping
│   │   └── Islands/            # Sleeping islands
│   ├── Bodies/                 # Body management
│   │   ├── Static/
│   │   ├── Dynamic/
│   │   └── Kinematic/
│   └── Transform/              # Transform integration
│
├── Collision/                  # Collision detection
│   ├── Broadphase/             # Broad phase algorithms
│   │   ├── SAP/                # Sweep and prune
│   │   ├── BVH/                # Bounding volume hierarchy
│   │   ├── Grid/               # Uniform grid
│   │   └── Octree/             # Octree spatial
│   ├── Narrowphase/            # Narrow phase tests
│   │   ├── GJK/                # Gilbert-Johnson-Keerthi
│   │   ├── EPA/                # Expanding Polytope
│   │   ├── SAT/                # Separating Axis Theorem
│   │   └── MPR/                # Minkowski Portal Refinement
│   ├── Shapes/                 # Collision shapes
│   │   ├── Sphere/
│   │   ├── Box/
│   │   ├── Capsule/
│   │   ├── Cylinder/
│   │   ├── Cone/
│   │   ├── ConvexHull/
│   │   ├── TriangleMesh/
│   │   └── Compound/
│   ├── Contacts/               # Contact management
│   │   ├── Generation/
│   │   ├── Caching/
│   │   └── Manifold/
│   └── Raycasting/
│       ├── Single/
│       ├── Batch/
│       └── Continuous/
│
├── Dynamics/                   # Dynamics simulation
│   ├── RigidBody/
│   │   ├── RigidBody/
│   │   │   ├── Inertia/
│   │   │   ├── Velocity/
│   │   │   └── Forces/
│   │   ├── Integrators/
│   │   │   ├── Euler/
│   │   │   ├── Verlet/
│   │   │   ├── RK4/
│   │   │   └── Symplectic/
│   │   └── Friction/
│   │       ├── Coulomb/
│   │       └── Anisotropic/
│   ├── Constraints/            # Joint constraints
│   │   ├── DistanceJoint/
│   │   ├── HingeJoint/
│   │   ├── BallJoint/
│   │   ├── SliderJoint/
│   │   ├── FixedJoint/
│   │   ├── UniversalJoint/
│   │   ├── ConeTwistJoint/
│   │   ├── Generic6DOF/
│   │   └── Motors/
│   │       ├── PositionMotor/
│   │       ├── VelocityMotor/
│   │       └── ServoMotor/
│   └── Solver/                 # Constraint solver
│       ├── Sequential/         # Sequential impulse
│       ├── PGS/                # Projected Gauss-Seidel
│       ├── XPBD/               # Extended PBD
│       └── MLCP/               # Mixed LCP
│
├── SoftBody/                   # Soft body simulation
│   ├── Cloth/
│   │   ├── Constraints/
│   │   │   ├── Distance/
│   │   │   ├── Bending/
│   │   │   └── Collision/
│   │   ├── Wind/
│   │   ├── Tearing/
│   │   └── SelfCollision/
│   ├── Deformable/
│   │   ├── FEM/                # Finite Element Method
│   │   ├── MassSpring/
│   │   └── ShapeMatching/
│   └── Rope/
│       ├── Spline/
│       └── Chain/
│
├── Fluid/                      # Fluid simulation
│   ├── SPH/                    # Smoothed Particle Hydro
│   │   ├── Particles/
│   │   ├── Density/
│   │   ├── Pressure/
│   │   └── Viscosity/
│   ├── PBD/                    # Position-based fluids
│   ├── FLIP/                   # Fluid Implicit Particle
│   ├── Surface/
│   │   ├── Reconstruction/
│   │   ├── Rendering/
│   │   └── Foam/
│   └── Buoyancy/
│
├── Vehicle/                    # Vehicle physics
│   ├── Wheel/
│   │   ├── Suspension/
│   │   ├── Tire/
│   │   │   ├── Friction/
│   │   │   ├── Slip/
│   │   │   └── Pacejka/
│   │   └── Braking/
│   ├── Drivetrain/
│   │   ├── Engine/
│   │   ├── Transmission/
│   │   └── Differential/
│   ├── Steering/
│   ├── Aerodynamics/
│   │   ├── Drag/
│   │   ├── Lift/
│   │   └── Downforce/
│   └── Tank/                   # Tracked vehicles
│
├── Character/                  # Character physics
│   ├── Controller/
│   ├── Capsule/
│   ├── Stepping/
│   └── Slopes/
│
├── Destruction/                # Destruction physics
│   ├── Fracture/
│   │   ├── Voronoi/
│   │   ├── Meshless/
│   │   └── Clustering/
│   ├── Debris/
│   └── Explosion/
│
├── Ragdoll/                    # Ragdoll simulation
│   ├── Skeleton/
│   ├── Constraints/
│   ├── Powered/
│   └── Blending/
│
├── CCD/                        # Continuous collision
│   ├── Conservative/
│   ├── Speculative/
│   └── TOI/                    # Time of impact
│
├── Debug/                      # Debug visualization
│   ├── Shapes/
│   ├── Contacts/
│   ├── Constraints/
│   └── Profiling/
│
└── Tools/                      # Physics tools
    ├── ConvexDecomposition/
    ├── ColliderGeneration/
    └── MassProperties/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Physics/Collision/ | src/Runtime/Physics/collision/ | ~80 |
| Physics/Dynamics/ | src/Runtime/Physics/dynamics/ | ~100 |
| Physics/SoftBody/ | src/Runtime/Physics/cloth/ | ~30 |
| Physics/Vehicle/ | src/Runtime/Physics/vehicle/ | ~40 |
| Physics/Fluid/ | src/Runtime/Physics/fluid/ | ~20 |

---

## Key Technologies

- **XPBD (Extended Position-Based Dynamics)** - Primary constraint solver
- **BVH + SAP** - Hybrid broadphase for static/dynamic objects
- **GJK+EPA** - Accurate narrow phase for convex shapes
- **SPH** - Particle-based fluid simulation
