# CLEAN TODO LIST - Only Unimplemented Features

**Generated**: 2026-01-11
**Last Updated**: After comprehensive codebase audit
**Total Real Todos**: 180+ (deduped and verified)

## REMOVED (ALREADY IMPLEMENTED)

These were removed from the todo list because they're already working:
- ✅ ECS System
- ✅ Block System
- ✅ Item System (400+ items)
- ✅ Inventory Management
- ✅ Crafting Table (3x3 recipes)
- ✅ Furnace (smelting)
- ✅ World Generation (biomes, caves, structures)
- ✅ Chunk System
- ✅ Save/Load System
- ✅ Weather System (13+ types)
- ✅ Console & Profiler
- ✅ Material System
- ✅ Rendering Core (forward pass, post-processing)
- ✅ Animation System (skeletal, state machines)
- ✅ Logging System
- ✅ Configuration System
- ✅ Serialization System
- ✅ Modding Framework
- ✅ Asset Pipeline

## PHASE 1: SYSTEM CONSOLIDATION (REAL WORK)

### 1.1 Physics Consolidation
- [x] Consolidate 214 physics files to 8 canonical implementations
- [x] Remove duplicate physics solvers
- [x] Update CMakeLists.txt for physics
- [ ] Verify build succeeds with consolidated physics
- [ ] Test physics functionality (falling blocks, collisions)
- [ ] Test block physics edge cases

### 1.2 Particle System Consolidation
- [ ] Consolidate 30+ particle implementations to 1 GPU particle system
- [ ] Consolidate CPU particle fallback
- [ ] Remove duplicate particle renderers
- [ ] Update CMakeLists.txt for particles
- [ ] Test particle rendering

### 1.3 Rendering Pipeline Consolidation
- [ ] Consolidate 365 rendering files to coherent pipeline
- [ ] Choose Metal as primary backend (keep it)
- [ ] Fix or remove broken Vulkan backend
- [ ] Remove duplicate renderers
- [ ] Update CMakeLists.txt for rendering

### 1.4 Audio System Consolidation
- [ ] Fix audio header mismatches
- [ ] Either fully enable audio or cleanly stub it
- [ ] Test audio system

### 1.5 AI System Consolidation
- [ ] Consolidate 3 behavior tree implementations to 1
- [ ] Consolidate 3 GOAP planners to 1
- [ ] Consolidate pathfinding implementations
- [ ] Test AI system

### 1.6 Dead Code Removal
- [ ] Delete 46+ .disabled files
- [ ] Delete 230+ prototype files
- [ ] Delete 23 .bak4 backup files
- [ ] Delete _alt variant implementations
- [ ] Delete /old/ directories

## PHASE 2: RE-ENABLE EXCLUDED SYSTEMS

### 2.1 NPC System (23 files currently disabled)
- [ ] Fix NPC system ECS compatibility
- [ ] Re-enable NPC AI with behavior trees
- [ ] Re-enable NPC dialogue system
- [ ] Re-enable NPC combat
- [ ] Re-enable NPC scheduling
- [ ] Re-enable NPC job system
- [ ] Test NPC system end-to-end

### 2.2 Networking System (44 files currently disabled)
- [ ] Decide on MVP multiplayer requirements
- [ ] Re-enable network socket layer
- [ ] Re-enable entity replication
- [ ] Implement client-side prediction
- [ ] Test multiplayer networking

### 2.3 Editor System (completely disabled)
- [ ] Re-enable asset browser
- [ ] Re-enable property inspector
- [ ] Re-enable debug visualization
- [ ] Test editor functionality

### 2.4 Mob System (completely disabled)
- [ ] Re-enable mob spawning
- [ ] Re-enable mob AI
- [ ] Re-enable mob combat
- [ ] Test mob system

## PHASE 3: COMPLETE INCOMPLETE FEATURES

### 3.1 Player Food System (8 TODOs incomplete)
- [ ] Implement food cooking mechanics
- [ ] Implement food spoilage over time
- [ ] Implement cooking recipes
- [ ] Add cooking audio effects
- [ ] Add cooking particle effects
- [ ] Track food statistics
- [ ] Implement food quality levels
- [ ] Test complete food system

### 3.2 Player Magic System (6 TODOs incomplete)
- [ ] Implement spell cooldown system
- [ ] Implement spell effect system
- [ ] Implement spell statistics tracking
- [ ] Implement spell upgrade system
- [ ] Implement spell combination system
- [ ] Test magic system

### 3.3 Player Combat System
- [ ] Implement player_attack_entity function (currently stub)
- [ ] Implement melee attack mechanics
- [ ] Implement ranged attack mechanics
- [ ] Test combat system

### 3.4 Vehicle System (10 TODOs incomplete OR remove entirely)
- [ ] DECISION: Keep or remove vehicle system?
- [ ] If keeping: Implement vehicle physics
- [ ] If keeping: Implement vehicle damage
- [ ] If keeping: Implement vehicle fuel system
- [ ] If keeping: Implement vehicle customization

### 3.5 Spirit Player Model System (8 TODOs incomplete)
- [ ] Implement model loading
- [ ] Implement texture system
- [ ] Implement model optimization
- [ ] Implement model validation
- [ ] Test spirit model system

### 3.6 Animation IK Solvers (incomplete in 2 locations)
- [ ] Complete FABRIK IK solver (location 1)
- [ ] Complete FABRIK IK solver (location 2)
- [ ] Consolidate IK implementations
- [ ] Test IK solver accuracy

### 3.7 Quest System (framework exists, incomplete)
- [ ] Complete quest objective tracking
- [ ] Complete quest branching logic
- [ ] Complete quest UI and markers
- [ ] Complete quest rewards
- [ ] Implement quest failure mechanics
- [ ] Test quest system

### 3.8 Achievement System (framework exists, incomplete)
- [ ] Complete achievement tracking
- [ ] Complete achievement UI
- [ ] Complete achievement rewards
- [ ] Test achievement system

## PHASE 4: TESTING & QUALITY

### 4.1 Build System
- [ ] Ensure full compilation without warnings
- [ ] Verify no linker errors
- [ ] Test build across all platforms

### 4.2 Unit Tests
- [ ] Create physics system tests
- [ ] Create particle system tests
- [ ] Create rendering tests
- [ ] Create gameplay system tests

### 4.3 Integration Tests
- [ ] Create multiplayer integration tests
- [ ] Create end-to-end gameplay tests
- [ ] Create quest system tests

### 4.4 Performance
- [ ] Profile startup time
- [ ] Profile gameplay performance
- [ ] Profile memory usage
- [ ] Optimize bottlenecks

## PHASE 5: DOCUMENTATION & CLEANUP

### 5.1 Documentation
- [ ] Document physics API
- [ ] Document particle system API
- [ ] Document rendering pipeline
- [ ] Document AI/NPC system
- [ ] Document networking API
- [ ] Create developer guide

### 5.2 Code Quality
- [ ] Standardize error handling
- [ ] Run static analysis
- [ ] Run memory safety checker
- [ ] Resolve all compiler warnings

## TOTAL REAL TODOS BY CATEGORY

| Category | Count | Priority | Effort |
|----------|-------|----------|--------|
| **Phase 1: System Consolidation** | 20+ | CRITICAL | 5-7 days |
| **Phase 2: Re-enable Systems** | 30+ | HIGH | 6-8 days |
| **Phase 3: Complete Features** | 50+ | HIGH | 8-12 days |
| **Phase 4: Testing & Quality** | 35+ | MEDIUM | 6-8 days |
| **Phase 5: Documentation** | 25+ | MEDIUM | 4-6 days |
| **TOTAL** | **160+** | Varies | 4-6 weeks |

## WHAT'S BEING SKIPPED (Already Done)

- Logging infrastructure ✅
- Configuration system ✅
- Serialization ✅
- Error handling patterns ✅
- Memory management ✅
- Input system ✅
- Platform abstraction ✅
- Asset loading ✅
- ECS core ✅
- Block system ✅
- Item system ✅
- Inventory ✅
- Crafting basics ✅
- World generation ✅
- Chunk system ✅
- Save/load ✅
- Weather ✅
- And 50+ more systems

## WHAT NEEDS WORK (Real Todos)

- Physics: 214 files → 8 (consolidation)
- Particles: 30+ files → 1 (consolidation)
- Rendering: 365 files → coherent (consolidation)
- Audio: Fix or stub (consolidation)
- AI: Multiple implementations → 1 (consolidation)
- Dead code: Delete 300+ files (cleanup)
- NPC: Re-enable 23 disabled files
- Networking: Re-enable 44 disabled files
- Editor: Re-enable completely
- Mobs: Re-enable completely
- Player systems: Complete 8+ incomplete features
- Testing: Create test suites
- Documentation: Write APIs
- Quality: Standardize patterns

## IMMEDIATE NEXT STEPS

1. **Complete Phase 1** (System consolidation) - **Est: 5-7 days**
   - Finish physics consolidation
   - Consolidate particles
   - Consolidate rendering
   - Fix audio
   - Consolidate AI

2. **Start Phase 2** (Re-enable systems) - **Est: 6-8 days**
   - Re-enable NPCs
   - Re-enable networking
   - Re-enable editor
   - Re-enable mobs

3. **Phase 3** (Complete features) - **Est: 8-12 days**
   - Finish all incomplete player systems
   - Finish animation IK
   - Complete quests/achievements

**Total Project**: ~4-6 weeks for complete stabilization

## How to Use This Todo List

1. Work through phases sequentially
2. Within each phase, complete items in order
3. Each item represents real, unimplemented work
4. No duplicate todos - each todo appears once
5. All "already done" work has been removed
6. Track progress in Claude Code's built-in todo system

---

**This is the canonical clean todo list**. All other todo files are historical/planning documents.
