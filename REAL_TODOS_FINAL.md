# Real Infrastructure TODOs - Minecraft v2

**Total Real TODOs**: 340+ actionable items (non-duplicated)
**Organized by**: Priority and Dependencies

## PHASE 1: STABILIZATION (CRITICAL - Must fix first)

### 1.1 Physics System Consolidation (9 items)

**Dependencies**: None
**Blocks**: All physics-dependent systems
**Estimated**: 3-5 days

- [ ] Choose canonical physics implementation (recommend physics_solver_system.c)
- [ ] Consolidate physics_core.c, physics_system.c into physics_solver_system.c
- [ ] Merge xpbd_solver.c and sequential_impulse.c into unified solver
- [ ] Keep block_physics.c as-is for voxel physics
- [ ] Remove all disabled physics implementations
- [ ] Test block physics compilation and functionality
- [ ] Test rigid body dynamics and collision detection
- [ ] Test character physics and movement
- [ ] Test constraint solving and stability

### 1.2 Particle System Consolidation (8 items)

**Dependencies**: None
**Blocks**: All visual effects and rendering
**Estimated**: 2-3 days

- [ ] Choose canonical GPU particle implementation
- [ ] Consolidate particle_system_gpu.c variants into single implementation
- [ ] Consolidate gpu_particles.c copies into one location
- [ ] Choose particle renderer (remove duplicates)
- [ ] Remove 20+ duplicate particle files
- [ ] Remove vfx_particles_impl.c duplicates
- [ ] Test GPU particle rendering
- [ ] Test particle physics integration and CPU fallback

### 1.3 Rendering Pipeline Stabilization (7 items)

**Dependencies**: GPU particle system (1.2)
**Blocks**: All visual systems
**Estimated**: 5-7 days

- [ ] Document rendering architecture (forward/deferred/hybrid choices)
- [ ] Fix Vulkan backend: either complete it or fully remove it
- [ ] Consolidate 50+ duplicate particle renderer implementations
- [ ] Consolidate duplicate voxel renderer implementations
- [ ] Remove 50+ redundant sprite renderer implementations
- [ ] Fix Metal backend synchronization
- [ ] Test complete rendering pipeline end-to-end

### 1.4 Audio System Stabilization (5 items)

**Dependencies**: None
**Blocks**: Audio features
**Estimated**: 2-3 days

- [ ] Fix audio header mismatches preventing compilation
- [ ] Either fully enable audio DSP or properly stub it
- [ ] Test spatial audio implementation
- [ ] Test audio effects and mixing
- [ ] Document audio support status

### 1.5 Build System Cleanup (8 items)

**Dependencies**: All of 1.1-1.4
**Blocks**: Clean builds
**Estimated**: 1-2 days

- [ ] Remove all .disabled file references from CMake
- [ ] Add assertions for missing implementations
- [ ] Create build output analysis tool
- [ ] Document all excluded subsystems
- [ ] Create script to prevent accidental exclusion
- [ ] Verify clean build with no warnings
- [ ] Create build verification test
- [ ] Document build system architecture

## PHASE 2: ENABLE EXCLUDED SYSTEMS (HIGH PRIORITY)

### 2.1 NPC System Re-enablement (18 items)

**Dependencies**: Phase 1 (stabilization)
**Blocks**: Gameplay features, multiplayer
**Estimated**: 5-7 days

- [ ] Analyze 23 NPC .bak4 backup files and understand refactoring failures
- [ ] Decide: migrate to current ECS or formally remove feature
- [ ] If keeping: migrate NPC components to current ECS API
- [ ] Re-enable NPC spawning system
- [ ] Re-enable NPC AI with behavior trees
- [ ] Re-enable NPC dialogue system
- [ ] Re-enable NPC combat mechanics
- [ ] Re-enable NPC scheduling
- [ ] Re-enable NPC job system
- [ ] Re-enable NPC housing system
- [ ] Re-enable NPC relationship tracking
- [ ] Re-enable NPC trading mechanics
- [ ] Re-enable NPC pathfinding
- [ ] Test NPC spawning at startup
- [ ] Test NPC behavior and AI
- [ ] Test NPC dialogue and interactions
- [ ] Test NPC combat
- [ ] Test NPC integration with rest of game

### 2.2 Networking System Re-enablement (12 items)

**Dependencies**: Phase 1 (stabilization)
**Blocks**: Multiplayer features
**Estimated**: 4-6 days

- [ ] Decide MVP multiplayer requirements
- [ ] Re-enable network socket layer
- [ ] Re-enable packet system
- [ ] Re-enable RPC system
- [ ] Implement entity replication
- [ ] Implement client-side prediction
- [ ] Implement server reconciliation
- [ ] Implement lag compensation
- [ ] Implement bandwidth optimization
- [ ] Test basic multiplayer movement
- [ ] Test entity synchronization
- [ ] Test lag handling and stability

### 2.3 Editor System Re-enablement (10 items)

**Dependencies**: Phase 1 (stabilization)
**Blocks**: Content creation tools
**Estimated**: 3-5 days

- [ ] Identify editor compilation issues
- [ ] Re-enable asset browser
- [ ] Re-enable property inspector
- [ ] Re-enable debug visualization tools
- [ ] Fix editor-engine communication bridge
- [ ] Test asset import functionality
- [ ] Test property editing
- [ ] Test scene manipulation
- [ ] Test asset hot reload
- [ ] Test editor stability

### 2.4 Mobile Platform Support (8 items)

**Dependencies**: Phase 1 (stabilization)
**Blocks**: iOS/Android releases
**Estimated**: 3-5 days

- [ ] Fix iOS touch input handling
- [ ] Fix Android touch gestures
- [ ] Test iOS compilation and deployment
- [ ] Test Android compilation and deployment
- [ ] Fix iOS memory constraints
- [ ] Fix Android memory management
- [ ] Test mobile rendering performance
- [ ] Test mobile networking

## PHASE 3: COMPLETE GAMEPLAY FEATURES (HIGH PRIORITY)

### 3.1 Player Food System Completion (8 items)

**Dependencies**: None (independent)
**Blocks**: Survival gameplay
**Estimated**: 2-3 days

- [ ] Implement food cooking mechanics
- [ ] Implement food spoilage system
- [ ] Implement food recipe system
- [ ] Implement cooking audio effects
- [ ] Implement particle effects for cooking
- [ ] Implement food statistics tracking
- [ ] Implement food quality levels
- [ ] Test complete food system end-to-end

### 3.2 Player Magic System Completion (6 items)

**Dependencies**: None (independent)
**Blocks**: Combat features
**Estimated**: 2-3 days

- [ ] Implement spell cooldown system
- [ ] Implement spell effect system
- [ ] Implement spell statistics tracking
- [ ] Implement spell upgrade system
- [ ] Implement spell combination system
- [ ] Test complete magic system

### 3.3 Player Combat System Completion (4 items)

**Dependencies**: 3.2 (magic)
**Blocks**: Combat gameplay
**Estimated**: 2-3 days

- [ ] Implement player_attack_entity function (currently stub)
- [ ] Implement melee attack mechanics
- [ ] Implement attack animation integration
- [ ] Test complete combat system

### 3.4 Vehicle System (Decide & Implement) (10 items)

**Dependencies**: None (independent)
**Blocks**: Transportation features
**Estimated**: 4-6 days (if keeping)

- [ ] Decide: keep vehicles or remove from codebase
- [ ] If keeping: implement vehicle physics
- [ ] Implement vehicle damage system
- [ ] Implement vehicle fuel system
- [ ] Implement vehicle inventory interaction
- [ ] Implement vehicle customization
- [ ] Implement vehicle audio effects
- [ ] Implement vehicle visual effects
- [ ] Implement vehicle statistics tracking
- [ ] Test vehicle system end-to-end

### 3.5 Spirit Player Model System (8 items)

**Dependencies**: 3.2 (magic), animation fixes
**Blocks**: Character customization
**Estimated**: 2-3 days

- [ ] Implement model loading system
- [ ] Implement texture system for spirit models
- [ ] Implement model optimization
- [ ] Implement model validation
- [ ] Implement statistics tracking
- [ ] Implement debug visualization
- [ ] Create unit test framework
- [ ] Test spirit model system

### 3.6 Animation System Completion (6 items)

**Dependencies**: None (independent)
**Blocks**: Character animation
**Estimated**: 3-4 days

- [ ] Complete FABRIK IK solver (currently incomplete in 2 locations)
- [ ] Consolidate duplicate IK implementations
- [ ] Test IK solver accuracy and stability
- [ ] Complete procedural animation system (currently disabled)
- [ ] Complete foot placement system
- [ ] Test complete animation system

### 3.7 AI System Consolidation (8 items)

**Dependencies**: Phase 2.1 (NPC re-enable)
**Blocks**: NPC behavior, mob system
**Estimated**: 4-5 days

- [ ] Consolidate 3 behavior tree implementations into one
- [ ] Consolidate 3 GOAP planner implementations into one
- [ ] Consolidate pathfinding implementations
- [ ] Complete crowd simulation system
- [ ] Create unified AI API
- [ ] Test behavior tree system
- [ ] Test GOAP planning system
- [ ] Test pathfinding accuracy

### 3.8 Mob System Re-enablement (4 items)

**Dependencies**: 3.7 (AI consolidation)
**Blocks**: Survival gameplay
**Estimated**: 2-3 days

- [ ] Re-enable mob spawning system
- [ ] Re-enable mob AI behavior
- [ ] Re-enable mob combat mechanics
- [ ] Test complete mob system

## PHASE 4: FEATURE COMPLETION (MEDIUM PRIORITY)

### 4.1 Quest System Completion (6 items)

**Dependencies**: Phase 2.1 (NPC)
**Blocks**: Gameplay progression
**Estimated**: 2-3 days

- [ ] Complete quest objective tracking
- [ ] Complete quest branching logic
- [ ] Complete quest UI and markers
- [ ] Complete quest rewards system
- [ ] Implement quest failure mechanics
- [ ] Test complete quest system

### 4.2 Achievement System Completion (4 items)

**Dependencies**: None
**Blocks**: Player progression
**Estimated**: 1-2 days

- [ ] Complete achievement tracking
- [ ] Complete achievement UI display
- [ ] Complete achievement rewards
- [ ] Test complete achievement system

### 4.3 Game Modes Implementation (4 items)

**Dependencies**: None
**Blocks**: Gameplay variety
**Estimated**: 1-2 days

- [ ] Implement creative mode
- [ ] Implement adventure mode
- [ ] Implement hardcore mode
- [ ] Test all game modes

### 4.4 Advanced Crafting Features (5 items)

**Dependencies**: None (builds on existing)
**Blocks**: Advanced gameplay
**Estimated**: 2-3 days

- [ ] Complete advanced crafting recipes
- [ ] Implement recipe discovery
- [ ] Implement recipe difficulty levels
- [ ] Implement crafting statistics
- [ ] Test advanced crafting

### 4.5 Enchanting System Completion (4 items)

**Dependencies**: None (builds on existing)
**Blocks**: Item progression
**Estimated**: 1-2 days

- [ ] Complete all enchantment types
- [ ] Implement enchantment interactions
- [ ] Implement enchantment UI
- [ ] Test complete enchanting system

### 4.6 Brewing System Completion (4 items)

**Dependencies**: None (builds on existing)
**Blocks**: Potion system
**Estimated**: 1-2 days

- [ ] Complete potion effects
- [ ] Implement potion recipes
- [ ] Implement potion UI
- [ ] Test complete brewing system

### 4.7 Weather Integration (3 items)

**Dependencies**: None (weather system works)
**Blocks**: Environmental gameplay
**Estimated**: 1 day

- [ ] Integrate weather with gameplay (effects on mobs, crops, etc.)
- [ ] Implement weather audio ambience
- [ ] Test weather impact on gameplay

## PHASE 5: TESTING & DOCUMENTATION (MEDIUM PRIORITY)

### 5.1 Unit Test Suite Creation (10 items)

**Dependencies**: Phase 1-3 complete
**Blocks**: Quality assurance
**Estimated**: 3-4 days

- [ ] Create physics system unit tests
- [ ] Create particle system unit tests
- [ ] Create rendering system unit tests
- [ ] Create NPC/AI system unit tests
- [ ] Create player system unit tests
- [ ] Create crafting system unit tests
- [ ] Create combat system unit tests
- [ ] Create save/load system unit tests
- [ ] Create inventory system unit tests
- [ ] Create networking unit tests

### 5.2 Integration Test Suite (8 items)

**Dependencies**: Phase 1-3 complete
**Blocks**: Quality assurance
**Estimated**: 3-4 days

- [ ] Create multiplayer integration tests
- [ ] Create gameplay flow integration tests
- [ ] Create NPC interaction integration tests
- [ ] Create quest system integration tests
- [ ] Create crafting integration tests
- [ ] Create combat integration tests
- [ ] Create save/load integration tests
- [ ] Create end-to-end gameplay tests

### 5.3 CI/CD Pipeline Setup (7 items)

**Dependencies**: 5.1, 5.2
**Blocks**: Automated validation
**Estimated**: 2-3 days

- [ ] Set up Github Actions for compilation
- [ ] Set up automated test execution
- [ ] Set up code coverage tracking
- [ ] Set up performance benchmarking
- [ ] Set up static analysis
- [ ] Set up memory safety checking
- [ ] Create build status dashboard

### 5.4 Documentation Writing (12 items)

**Dependencies**: All systems stabilized
**Blocks**: Developer onboarding
**Estimated**: 4-5 days

- [ ] Write physics API documentation
- [ ] Write particle system documentation
- [ ] Write rendering pipeline documentation
- [ ] Write NPC/AI system documentation
- [ ] Write animation system documentation
- [ ] Write networking/multiplayer documentation
- [ ] Write editor usage guide
- [ ] Write mod developer API guide
- [ ] Write gameplay systems overview
- [ ] Write architecture overview
- [ ] Write development setup guide
- [ ] Write troubleshooting guide

### 5.5 Performance Profiling (8 items)

**Dependencies**: Phase 1 complete
**Blocks**: Optimization
**Estimated**: 2-3 days

- [ ] Profile startup performance
- [ ] Profile gameplay frame time
- [ ] Profile memory usage
- [ ] Profile physics performance
- [ ] Profile rendering performance
- [ ] Profile NPC AI performance
- [ ] Identify bottlenecks
- [ ] Create optimization plan

## PHASE 6: CODE QUALITY & CLEANUP (MEDIUM PRIORITY)

### 6.1 Dead Code Removal (12 items)

**Dependencies**: Phases 1-4 complete (ensure no references)
**Blocks**: Codebase cleanliness
**Estimated**: 2-3 days

- [ ] Delete 46+ .disabled files (archive if needed)
- [ ] Delete 230+ prototype implementations
- [ ] Delete 23 .bak4 backup files
- [ ] Delete _alt variant implementations
- [ ] Clean /old/ directories
- [ ] Remove unused backend code
- [ ] Remove unused optimization attempts
- [ ] Remove stub implementations
- [ ] Remove incomplete experimental code
- [ ] Verify no references remain
- [ ] Create deprecation policy document
- [ ] Set up code cleanup guidelines

### 6.2 Error Handling Standardization (6 items)

**Dependencies**: None (can be parallel)
**Blocks**: Code quality
**Estimated**: 2-3 days

- [ ] Define standard error codes
- [ ] Implement consistent error logging
- [ ] Standardize error propagation
- [ ] Test error handling across systems
- [ ] Document error handling patterns
- [ ] Create error handling guidelines

### 6.3 Memory Management Optimization (5 items)

**Dependencies**: Phase 1 (stabilization)
**Blocks**: Performance
**Estimated**: 2-3 days

- [ ] Enable memory pool allocator
- [ ] Enable resource manager
- [ ] Consolidate 394+ malloc/free calls
- [ ] Enable streaming system
- [ ] Test memory tracking

### 6.4 Performance Optimization (5 items)

**Dependencies**: 5.5 (profiling)
**Blocks**: Performance
**Estimated**: 3-4 days

- [ ] Enable SIMD optimizations
- [ ] Enable texture streaming
- [ ] Complete virtual texturing
- [ ] Complete LOD generation
- [ ] Test optimization results

### 6.5 Code Quality Analysis (4 items)

**Dependencies**: All systems complete
**Blocks**: Quality gates
**Estimated**: 1-2 days

- [ ] Run static analysis tool
- [ ] Run memory safety checker
- [ ] Check undefined behavior
- [ ] Verify all warnings resolved

## PHASE 7: FINAL VALIDATION & RELEASE (LOW PRIORITY)

### 7.1 Platform Validation (5 items)

**Dependencies**: All phases complete
**Blocks**: Release
**Estimated**: 2-3 days

- [ ] Validate macOS build and deployment
- [ ] Validate iOS build and deployment
- [ ] Validate Android build and deployment
- [ ] Validate web build (if applicable)
- [ ] Test on actual target devices

### 7.2 Feature Validation (4 items)

**Dependencies**: All phases complete
**Blocks**: Release
**Estimated**: 1-2 days

- [ ] Play through entire campaign
- [ ] Test all major features
- [ ] Verify achievement system
- [ ] Verify save/load system

### 7.3 Documentation Review (3 items)

**Dependencies**: 5.4 complete
**Blocks**: Release
**Estimated**: 1 day

- [ ] Review all documentation
- [ ] Update outdated docs
- [ ] Create changelog

### 7.4 Release Preparation (4 items)

**Dependencies**: All validation complete
**Blocks**: Release
**Estimated**: 1 day

- [ ] Tag release version
- [ ] Create release notes
- [ ] Prepare build artifacts
- [ ] Deploy to app stores

## TOTAL ESTIMATES

| Phase | Items | Estimated Days |
|-------|-------|-----------------|
| 1 - Stabilization | 37 | 5-7 |
| 2 - Enable Systems | 30 | 6-8 |
| 3 - Complete Features | 58 | 8-12 |
| 4 - Feature Completion | 30 | 4-6 |
| 5 - Testing & Docs | 37 | 6-8 |
| 6 - Quality & Cleanup | 32 | 6-8 |
| 7 - Final Validation | 16 | 3-5 |
| **TOTAL** | **340+** | **38-54 days** |

## DEPENDENCIES GRAPH

```
Phase 1 (Stabilization)
   Phase 2 (Enable Systems)
      Phase 3 (Complete Features)
         Phase 4 (Feature Completion)
         Phase 5 (Testing & Docs)
         Phase 6 (Quality & Cleanup)
             Phase 7 (Validation)
```

## SUCCESS METRICS

- [ ] Zero compilation warnings
- [ ] All systems pass unit tests
- [ ] 80%+ code coverage
- [ ] CI/CD pipeline green
- [ ] Performance targets met
- [ ] No duplicate implementations
- [ ] All .disabled files removed
- [ ] All TODO comments resolved
- [ ] Documentation complete
- [ ] Release ready

## NEXT STEPS

1. **Week 1**: Complete Phase 1 (Stabilization)
2. **Week 2**: Complete Phase 2 (Enable Systems)
3. **Week 3**: Complete Phase 3 (Complete Features)
4. **Week 4**: Complete Phases 4-6 (Features, Testing, Quality)
5. **Week 5**: Complete Phase 7 (Validation & Release)

**Recommend starting immediately with Phase 1 item 1.1 (Physics consolidation)**
