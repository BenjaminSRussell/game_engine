# Minecraft v2 Infrastructure & Development Todo Summary

## EXECUTIVE SUMMARY

This document outlines ALL real infrastructure work needed for the Minecraft v2 codebase. Based on comprehensive analysis, the codebase has:

- **~300+ source files** (only ~50% actively compiled)
- **46+ .disabled files** (entire systems excluded)
- **100+ duplicate implementations** (particle, physics, rendering, AI)
- **8 major excluded subsystems** (NPC, networking, editor, cinematics, audio, vehicles, animation, mobs)
- **749+ TODO comments** (incomplete implementations)
- **230+ prototype files** needing cleanup
- **~15,000+ lines of unused/broken code**

## CRITICAL ISSUES (MUST FIX IMMEDIATELY)

### Phase 1: System Consolidation (Weeks 1-2)

#### 1. Physics System Consolidation
- **Issue**: 6 different physics implementations with unclear relationships
- **Files**: physics_core.c, physics_system.c, physics_solver_system.c, xpbd_solver.c, sequential_impulse.c, block_physics.c
- **Status**: physics_core.c and physics_system.c marked "Disabled"
- **Action**: Choose canonical implementation (recommend physics_solver_system.c), remove all others, ensure compilation
- **Testing Required**: Block physics, collision detection, rigid body dynamics, constraint solving
- **Estimated Impact**: Fix 15+ build exclusions

#### 2. Particle System Consolidation
- **Issue**: 30+ particle system implementations with duplicated functionality
- **Files**: 10+ GPU particle implementations, 5+ CPU particle implementations, multiple particle renderers
- **Status**: Multiple competing implementations with unclear relationships
- **Action**: Consolidate to single GPU particle system with unified API, remove 20+ duplicate files
- **Testing Required**: GPU rendering, CPU fallback, physics integration, pooling
- **Estimated Impact**: Reduce 1000+ lines of duplication

#### 3. Rendering Pipeline Stabilization
- **Issue**: 365+ rendering files with overlapping responsibilities
- **Status**: Forward, deferred, and hybrid renderers partially working
- **Action**: Commit to Metal backend for macOS, consolidate overlapping implementations, remove broken Vulkan
- **Testing Required**: All visual effects, particle rendering, voxel rendering, light baking
- **Estimated Impact**: Enable compilation, improve maintainability

#### 4. Audio System Fix
- **Issue**: Audio DSP and effects excluded due to header mismatches
- **Status**: Only basic stubs included
- **Action**: Either fully enable audio subsystem (fix headers) or properly stub all remaining functionality
- **Testing Required**: Spatial audio, effects, streaming, music transitions
- **Estimated Impact**: Remove build failures, clarify support status

### Phase 2: Excluded Systems (Weeks 3-4)

#### 5. NPC System Re-enablement
- **Issue**: 23 files excluded, 23 .bak4 backup files indicating failed refactoring
- **Status**: Dependencies: dialogue, combat, scheduling, housing, jobs, AI all excluded together
- **Action**: Migrate NPC system to current ECS API or formally document removal
- **Testing Required**: NPC spawning, AI behavior, dialogue, combat, trading
- **Estimated Impact**: Re-enable 10,000+ lines of gameplay code

#### 6. Networking System Fix
- **Issue**: 44+ files excluded, multiplayer non-functional
- **Status**: Entity replication, lag compensation, client prediction all excluded
- **Action**: Decide on MVP multiplayer requirements, re-implement focused subset
- **Testing Required**: Entity synchronization, player movement, object interaction across network
- **Estimated Impact**: Enable multiplayer development and testing

#### 7. Editor System Re-enablement
- **Issue**: All editor functionality excluded
- **Status**: Asset browser, property inspector, debug tools all broken
- **Action**: Re-enable editor subsystem and fix compilation
- **Testing Required**: Asset import, property editing, scene manipulation, hot reload
- **Estimated Impact**: Enable content creation workflow

### Phase 3: Feature Completion (Weeks 5-6)

#### 8. Player System Completion
- **Player Food System** (8 TODOs):
  - Cooking system
  - Food spoilage mechanics
  - Recipe system
  - Audio/particle effects
  - Statistics tracking
  - Tooltip system
  - Quality levels
  - Expiration tracking

- **Player Combat** (1 TODO):
  - Implement player_attack_entity function (currently stub)

- **Player Magic System** (6 TODOs):
  - Cooldown system
  - Effect system
  - Statistics tracking
  - Upgrade system
  - Combination system
  - Spell book system

- **Player Vehicle System** (10 TODOs):
  - Physics integration
  - Damage system
  - Fuel mechanics
  - Inventory interaction
  - Customization system
  - Audio effects
  - Visual effects
  - Statistics tracking
  - Save/load persistence
  - AI traffic

- **Spirit Player Model System** (8 TODOs):
  - Model loading
  - Texture system
  - Optimization
  - Validation
  - Statistics tracking
  - Debug visualization
  - Unit testing
  - Documentation

#### 9. Animation System Unification
- **Issue**: Multiple incomplete IK solvers, disabled procedural animation variants
- **Files**: FABRIK implementations in 2 locations (both incomplete), foot placement disabled, procedural animation disabled
- **Action**: Complete IK solvers, consolidate animation system, remove duplicate implementations
- **Testing Required**: Skeletal animation, IK solving, ragdoll physics, state machine transitions
- **Estimated Impact**: Enable advanced character animation

#### 10. AI System Consolidation
- **Issue**: 83+ AI files with 6+ competing implementations
- **Files**: Multiple behavior trees, GOAP planners, pathfinding implementations
- **Action**: Consolidate behavior trees, GOAP planners, pathfinding into canonical implementations
- **Testing Required**: NPC behavior, planning, pathfinding, crowd simulation
- **Estimated Impact**: Enable NPC and mob systems

### Phase 4: Code Quality & Testing (Weeks 7-8)

#### 11. Dead Code Removal
- Delete 46+ .disabled files
- Delete 230+ prototype files ("complete", "advanced", "impl", "stub" variants)
- Delete _alt variant implementations
- Delete 23 backup files (.bak4)
- Clean up /old/ directories

#### 12. Error Handling Standardization
- **Issue**: 880+ error references with inconsistent handling
- **Action**: Establish consistent error codes, logging, propagation patterns
- **Testing Required**: Error propagation across all systems

#### 13. Memory Management Consolidation
- Enable memory pool allocator (currently disabled)
- Enable resource manager
- Consolidate 394+ malloc/free calls
- Enable streaming system
- **Testing Required**: Memory tracking, leak detection, performance

#### 14. Performance Optimization
- Re-enable SIMD optimizations (currently disabled)
- Enable GPU memory management
- Complete LOD generation
- Enable virtual texturing
- **Testing Required**: Performance profiling, optimization verification

## DETAILED SYSTEM STATUS

### Core Engine Systems

| System | Status | Issues | Priority |
|--------|--------|--------|----------|
| ECS | Working | None | - |
| Physics | Broken | 6 implementations, build disabled | CRITICAL |
| Rendering | Partial | 365 files, overlapping, Vulkan broken | CRITICAL |
| Audio | Disabled | Header mismatches, DSP excluded | CRITICAL |
| Animation | Partial | IK incomplete, duplicates, disabled variants | HIGH |
| AI | Partial | 83 files, 6+ competing systems | HIGH |
| Networking | Disabled | 44 files excluded, multiplayer broken | HIGH |

### Gameplay Systems

| System | Status | Issues | Priority |
|--------|--------|--------|----------|
| Blocks | Working | None | - |
| Items | Working | None | - |
| Inventory | Working | UI needs polish | MEDIUM |
| Crafting | Partial | Tech crafting needs completion | MEDIUM |
| NPCs | Disabled | 23 files excluded, refactoring failed | CRITICAL |
| Mobs | Disabled | AI excluded | HIGH |
| Combat | Partial | Player attacks stub, NPC combat disabled | HIGH |
| Food | Partial | 8 TODOs incomplete | MEDIUM |
| Vehicles | Disabled | Excluded, 10 TODOs | MEDIUM |
| Magic | Partial | 6 TODOs incomplete | MEDIUM |
| Weather | Working | None | - |
| World Gen | Working | None | - |
| Quests | Partial | Framework exists, needs completion | MEDIUM |
| Achievements | Partial | Framework exists, needs completion | MEDIUM |
| Save/Load | Working | None | - |

### Tools & Infrastructure

| System | Status | Issues | Priority |
|--------|--------|--------|----------|
| Editor | Disabled | All excluded | HIGH |
| Console | Working | None | - |
| Profiler | Working | None | - |
| Asset Management | Partial | Hot reload works, compilation needs completion | MEDIUM |
| Modding | Working | API needs documentation | LOW |
| Cinematics | Disabled | Excluded, broken | MEDIUM |

## COMPREHENSIVE TODO LIST (by Phase)

### Phase 1: System Stabilization (CRITICAL)
- [ ] Consolidate physics system into single API
- [ ] Remove 20+ duplicate particle implementations
- [ ] Fix rendering pipeline compilation
- [ ] Fix audio system headers and DSP
- [ ] Document canonical system choices

### Phase 2: Enable Excluded Systems
- [ ] Re-enable NPC system with current ECS
- [ ] Re-enable networking subsystem
- [ ] Re-enable editor infrastructure
- [ ] Fix mobile platform support

### Phase 3: Complete Gameplay Features
- [ ] Complete player food system (8 items)
- [ ] Complete player magic system (6 items)
- [ ] Implement player combat (1 item)
- [ ] Complete vehicle system (10 items) or remove
- [ ] Complete spirit model system (8 items)
- [ ] Complete animation IK (FABRIK, analysis, tests)
- [ ] Complete AI consolidation (behavior trees, GOAP, pathfinding)

### Phase 4: Testing & Cleanup
- [ ] Delete 46+ .disabled files
- [ ] Delete 230+ prototype implementations
- [ ] Delete _alt variants and backup files
- [ ] Standardize error handling (880+ references)
- [ ] Consolidate memory management (394+ calls)
- [ ] Enable performance optimizations

## QUICK REFERENCE: FILES TO DELETE

### .disabled Files (46+)
- `src/engine/core/optimization/memory_allocators.c.disabled`
- `src/engine/core/sync/fiber_system.c.disabled`
- `src/engine/core/sync/job_scheduler.c.disabled`
- `src/engine/animation/ik_advanced/fullbody_ik.c.disabled`
- *[40+ more - see build_status.md for complete list]*

### Duplicate Implementations (100+)
- Particle systems: `particle_system_gpu.c` variants (10+ copies)
- Physics: `physics_core.c`, `physics_system.c` (disabled)
- AI: Multiple behavior trees, GOAP implementations
- Rendering: Duplicate particle/voxel/sprite renderers
- *[See duplicate_analysis.md for complete list]*

### Backup Files (23)
- All `.bak4` files in `src/game/blockgame/npc/`

### Prototype Files (230+)
- All `*_complete.c` files
- All `*_advanced.c` files (not labeled "advanced features")
- All `*_impl.c` files (implementation variants)
- All `*_stub.c` files
- All `*_alt.c` files

## METRICS

- **Duplicate Code**: 100+ files representing 5,000+ lines
- **Disabled Code**: 46+ .disabled files, 10,000+ lines
- **TODOs**: 749+ TODO comments
- **Prototype Files**: 230+
- **Unused Code**: Estimated 15,000+ lines

## SUCCESS CRITERIA

1. All systems compile without warnings
2. Physics system has single canonical API
3. Particle system has unified interface
4. NPC system re-enabled and working
5. Networking basic multiplayer working
6. Player systems complete (combat, magic, food, vehicles)
7. All gameplay systems tested end-to-end
8. No duplicate implementations
9. All .disabled and prototype files deleted
10. CI/CD pipeline prevents regressions

## NEXT IMMEDIATE ACTIONS

1. Create architecture decision document (ADDs) for each major system
2. Branch current main to backup_pre_cleanup
3. Start Phase 1 stabilization (physics, particles, rendering, audio)
4. Set up automated testing for each system
5. Create detailed compilation fix roadmap
