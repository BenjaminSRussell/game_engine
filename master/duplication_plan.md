# Duplication Separation Plan

## Purpose
Prevent duplicated work by defining canonical locations, separation rules, and helper function ownership per subsystem.

## Inputs
- `master/duplicate_basename_report.txt`
- `master/duplicate_content_report.txt`

## Global Rules
- One canonical implementation per feature; all duplicates must be merged or removed.
- Canonical headers live in `src/engine/include/`, implementations in `src/engine/`.
- Helper functions belong to the owning subsystem under `src/engine/<subsystem>/helpers/`.
- No `.disabled` or placeholder stubs; replace with real implementations or delete with integration updates.

## Subsystem Separation Plans

### Physics
- Canonical source: `src/engine/physics/` and headers in `src/engine/include/physics/`.
- Consolidate `_trash`, `_alt`, or duplicate physics files into a single solver path.
- Create helper modules for common operations:
  - `physics_math_helpers.c/h` (vector ops, projections, transforms)
  - `collision_helpers.c/h` (shared collision tests)
  - `constraint_helpers.c/h` (constraint setup and solve utilities)

### Rendering + Materials + Shading
- Canonical source: `src/engine/rendering/`, `src/engine/materials/`, `src/engine/shading/`.
- Move any duplicated rendering paths in `src/engine/rendering/advanced/` and `src/engine/rendering/post_processing/` into a single pipeline.
- Centralize material API in `src/engine/materials/material_system.c` and expose a single public header.

### Animation + Character Animation
- Canonical source: `src/engine/animation/` (shared) with character-specific wrappers in `src/engine/character/animation/`.
- Merge duplicate IK, state machine, and blend tree implementations into shared animation modules.
- Keep character modules as thin adapters; no duplicated core logic.

### Networking
- Canonical source: `src/engine/network/` (preferred) OR `src/engine/networking/` (pick one and migrate).
- Unify security, bandwidth, replication, and RPC layers under one namespace.

### Audio + Cinematics Audio
- Canonical source: `src/engine/audio/`.
- Cinematics audio should call into core audio; no separate audio system implementation.

### Editor + Tools
- Canonical source: `src/engine/editor/` for editor runtime.
- Tools under `src/engine/tools/` should be build-time or standalone; no runtime duplication.

### UI
- Canonical source: `src/engine/ui/`.
- Replace placeholder UI files that only contain “Disabled” with actual implementations or delete and rewire callers.

### Asset/Resource Management
- Canonical source: `src/engine/assets/` for asset pipelines, `src/engine/core/resource/` for shared resource APIs.
- Ensure only one asset manager implementation is used.

### Platform + Backend
- Canonical source: `src/engine/platform/` and `src/engine/backend/`.
- Avoid duplicate platform abstraction layers; wrap platform-specific logic behind shared interfaces.

## Separation Checklist
- Identify duplicate basenames and pick a canonical file.
- Merge unique logic into canonical file; update includes and build scripts.
- Replace remaining duplicates with thin wrappers or remove them.
- Add unit/integration tests for each consolidated module.

## Expected Outputs
- Consolidation TODOs in `master/todo.md` and `master/todo.csv`.
- A canonical file map per subsystem.
- Updated build files to reference only canonical paths.
