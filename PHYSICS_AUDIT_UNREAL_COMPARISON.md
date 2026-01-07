# Physics Engine Audit: Unreal Engine (Chaos) Parity Gap Analysis

## Executive Summary
The current physics engine is a basic custom implementation utilizing a simple semi-implicit Euler integrator and broadphase BVH. It lacks the depth, optimization, and feature richness of Unreal Engine's Chaos Physics system. While the core rigid body simulation is functional, advanced features like destruction, vehicles, cloth, and fluid are either in early stages or entirely stubbed.

## 1. Core Simulation & Solver

| Feature | Unreal Engine (Chaos) | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Integrator** | Highly optimized, constraint-based, sub-stepping. | Semi-implicit Euler, basic iterative solver. | **Critical** |
| **Async Physics** | Decoupled physics thread with double buffering. | Synchronous `physics_world_step` on main thread. | **High** |
| **Broadphase** | Advanced dynamic BVH, island management, sleeping. | Naive BVH, basic sleeping stubs. | **High** |
| **Optimization** | SIMD, SoA (Structure of Arrays), massive parallelism. | AoS (Array of Structures), scalar math. | **High** |

## 2. Collision Queries

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Raycasts** | Multi-threaded, filtering, back-face culling. | Single-threaded `core_physics_world_raycast`. | **Medium** |
| **Sweeps (Shapecasts)** | Sphere, Box, Capsule sweeps against world. | **Missing** (Stub in `physics_queries.c`). | **Critical** |
| **Overlaps** | Sphere, Box, Capsule, Convex overlaps. | **Missing** (Stub in `physics_queries.c`). | **Critical** |
| **Scene Query System** | Async queries, batching. | Immediate mode only. | **Medium** |

## 3. Vehicles (Chaos Vehicles)

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Simulation** | Detailed Engine, Transmission, Differential, Suspension. | Skeleton code (`vehicle_physics_impl.c`) with 800+ TODOs. | **Blocking** |
| **Tire Model** | Pacejka Magic Formula, surface friction. | Stub function. | **Blocking** |
| **Aerodynamics** | Drag, downforce, wind interaction. | None. | **Low** |

## 4. Destruction (Chaos Destruction)

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Fracturing** | Voronoi, Planar, Radial, Clustering, Levels. | Basic random box fracture (`physics_destruction.c`). | **High** |
| **Connection Graph** | Real-time strain evaluation, structural integrity. | None. | **High** |
| **Fields** | Master Fields (Anchor, Strain, Disable). | None. | **High** |

## 5. Cloth & Soft Bodies

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Cloth** | Chaos Cloth (XPBD), wind, self-collision. | Stub (`cloth_physics_system.c`). | **High** |
| **Soft Bodies** | Volume-preserving soft bodies. | Basic Verlet Rope demo (`physics_softbody.c`). | **High** |

## 6. Fluids (Niagara/Chaos)

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Simulation** | 2D/3D Grid (Eulerian) and Particle (Lagrangian/SPH). | Stub (`fluid_physics_system.c`). | **High** |

## 7. Character Controller

| Feature | Unreal Engine | Current Engine | Gap |
| :--- | :--- | :--- | :--- |
| **Movement** | Robust Kinematic Controller (Walk, Swim, Fly). | Basic implementation exists. | **Medium** |
| **Network Prediction** | Client-side prediction, server reconciliation. | None. | **High** |

## Recommendations for Immediate Action

1.  **Implement Spatial Queries:** Sweeps and Overlaps are fundamental for gameplay logic (triggers, melee attacks, character grounding). This is the highest priority missing core feature.
2.  **Stabilize Vehicle Physics:** The vehicle implementation is a "massive TODO". Bringing this to a basic drivable state would be a significant milestone.
3.  **Solver Improvements:** Move towards a constraint-based solver (PGS or similar) to improve stacking stability and joint behavior.
