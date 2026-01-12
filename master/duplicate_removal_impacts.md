# Duplicate Removal Impacts

This report summarizes likely impacts of removing or consolidating duplicates found in the codebase.

## High-Risk Impact Areas
- Public header consolidation will change include paths and may break builds if not updated everywhere.
- Duplicate subsystem implementations imply divergent behavior; merging can change runtime semantics.
- Tools vs engine duplicates can affect build targets and runtime/editor separation.
- Placeholder ("// Disabled") duplicates indicate missing functionality; removal requires caller rewiring.

## Targeted Impact Findings
- Header collisions between `src/engine/include/` and `src/engine/`: 123 groups.
- Animation vs character animation duplicates: 49 groups.
- Network vs networking duplicates: 24 groups.
- Raytracing advanced path duplicates: 8 groups.
- Asset manager duplication: 1 groups.
- Audio vs cinematics audio duplicates: 1 groups.
- UI placeholder duplicates ("// Disabled"): 1 groups.

## Expected Work Required
- Update include paths and ensure `src/engine/include/` exports only one canonical header per API.
- Merge duplicated logic and re-run tests for behavior changes in animation, networking, rendering.
- Unify build lists (CMake/Swift/other) to reference only canonical paths.
- Replace placeholder files with real implementations or remove and redirect call sites.

## Impact-Driving Groups (examples)
- Animation vs character animation: `src/engine/animation/*` vs `src/engine/character/animation/*`.
- Networking split: `src/engine/network/*` vs `src/engine/networking/*`.
- Raytracing advanced duplicates under rendering.
- Header duplicates between `src/engine/include/*` and `src/engine/*`.
- UI placeholder duplicates: multiple `src/engine/ui/*.c` are identical "// Disabled" stubs.
