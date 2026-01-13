# Proposed Deletion Plan - FOR REVIEW ONLY

**STATUS:** NOT YET EXECUTED - AWAITING APPROVAL

---

## Summary

This document lists all duplicate files that SHOULD be deleted to consolidate the codebase.
NO DELETIONS HAVE BEEN MADE YET.

All canonical files have been verified to exist.
All duplicates have been verified to be byte-for-byte identical.

---

## FILES TO DELETE (6 vendor duplicates)

```
src/engine/include/vendor/miniaudio.h       (95,683 lines)
src/engine/include/vendor/stb_image.h       (8,002 lines)
src/engine/include/vendor/cgltf.h           (7,228 lines)
src/engine/vendor/cgltf.h                   (7,228 lines)
src/vendor/cgltf.h                          (7,228 lines)
src/engine/include/vk_mem_alloc.h           (19,845 lines)
```

**Reason:** These are duplicates of files in `src/include/vendor/` (canonical location)

---

## FILES TO DELETE (2 legacy core files)

```
src/core/monolithic_main.c                  (3,516 lines)
src/core/gamestate_main.c                   (3,289 lines)
```

**Reason:** These are byte-for-byte identical to `src/engine/core/` versions (canonical location)

---

## DIRECTORIES TO DELETE (entire asset IO subsystem)

```
src/assets/io/                              (70+ files across subdirectories)
src/assets/textures/                        (50+ files)
src/assets/importer/                        (3+ files)
src/assets/resources/                       (5+ files)
src/assets/system/                          (10+ files)
src/assets/asset_manager.c                  (1 file)
```

**After these deletions, entire `src/assets/` directory will be empty and can be removed.**

**Reason:** All identical to files in `src/engine/assets/` (canonical location)

---

## CANONICAL LOCATIONS (RETAINED)

All canonical files verified to exist and will NOT be deleted:

```
src/include/vendor/miniaudio.h              ✓
src/include/vendor/stb_image.h              ✓
src/include/vendor/cgltf.h                  ✓
src/include/vk_mem_alloc.h                  ✓
src/engine/core/monolithic_main.c           ✓
src/engine/core/gamestate_main.c            ✓
src/engine/assets/io/                       ✓
src/engine/assets/textures/                 ✓
src/engine/assets/importer/                 ✓
src/engine/assets/resources/                ✓
src/engine/assets/system/                   ✓
src/engine/assets/asset_manager.c           ✓
```

---

## IMPACT ANALYSIS

✓ **SAFE TO DELETE** - Verified:
- All canonical files exist
- No broken references in source code
- No broken includes
- No CMake references to deleted paths

⚠️ **WARNINGS** - Please note:
- This will remove ~1.3MB of duplicate code
- All changes are reversible via git
- No broken references detected, but always test build after

---

## HOW TO PROCEED

Option 1: **APPROVE** - I will execute all deletions and create git commit
Option 2: **PARTIAL** - Approve only specific categories (e.g., just vendor libs)
Option 3: **HOLD** - Don't delete anything, just document the issue
Option 4: **MANUAL** - You review and delete specific files yourself

**PLEASE ADVISE:**
- Which categories to delete?
- Any concerns about the canonical locations?
- Should we test build first?

