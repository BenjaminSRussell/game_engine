# Actual Git Vendor File Structure - VERIFIED

**Status:** ACCURATELY MAPPED from git ls-files

---

## Summary of Vendor Files in Git

Only **10 vendor-related files** are actually tracked in git:

```
1. include/vendor/vk_mem_alloc.h              (top-level include/)
2. src/engine/cgltf.h                         (top-level src/engine/)
3. src/engine/include/vendor/miniaudio.h      (src/engine includes)
4. src/include/vendor/miniaudio.h             (src includes)
5. src/engine/vendor/cgltf/cgltf_impl.c       (implementation)
6. src/engine/vendor/stb_image/stb_image_impl.c (implementation)

PLUS 4 NON-VENDOR files that contain "vk_memory":
7. src/engine/backend/vulkan/vk_memory.c
8. src/engine/backend/vulkan/vk_memory.h
9. src/engine/platform/vulkan/vk_memory.c
10. src/engine/platform/vulkan/vk_memory.h
```

---

## Organized by Purpose

### Header Libraries (Vendor - 4 files)
```
✓ include/vendor/vk_mem_alloc.h              CANONICAL
✓ src/engine/cgltf.h                         CANONICAL  
✓ src/engine/include/vendor/miniaudio.h      ENGINE VERSION
✓ src/include/vendor/miniaudio.h             LEGACY VERSION
```

### Implementation Files (Vendor)
```
✓ src/engine/vendor/cgltf/cgltf_impl.c       Implementation
✓ src/engine/vendor/stb_image/stb_image_impl.c Implementation
```

### Non-Vendor Files (Vulkan memory)
```
✓ src/engine/backend/vulkan/vk_memory.c
✓ src/engine/backend/vulkan/vk_memory.h
✓ src/engine/platform/vulkan/vk_memory.c
✓ src/engine/platform/vulkan/vk_memory.h
```

---

## Key Findings

### Vendor Libraries NOT in Git (as analyzed)
```
✗ src/include/vendor/stb_image.h     - DOESN'T EXIST in git
✗ src/include/vendor/cgltf.h         - DOESN'T EXIST in git
✗ src/include/vk_mem_alloc.h         - NOT IN SRC (only in include/)
✗ src/engine/include/vendor/stb_image.h - NOT IN GIT
✗ src/engine/include/vendor/cgltf.h  - NOT IN GIT
✗ src/engine/include/vk_mem_alloc.h  - NOT IN GIT
✗ src/vendor/cgltf.h                 - NOT IN GIT
```

### Why This Matters
Our analysis assumed files existed that DON'T exist in git. These files may:
- Have existed in filesystem but never tracked
- Were deleted from git already
- Are generated or temporary files

---

## Actual Duplicate Analysis

### Potential Duplicate: miniaudio.h
```
src/include/vendor/miniaudio.h           (seems to be legacy location)
src/engine/include/vendor/miniaudio.h    (seems to be canonical location)

STATUS: NEED TO CHECK IF IDENTICAL
```

### Cgltf Situation
```
src/engine/cgltf.h                       (header at top level)
src/engine/vendor/cgltf/cgltf_impl.c     (implementation)

STATUS: These are not duplicates, they're implementation files
```

### Stb_image Situation
```
src/engine/vendor/stb_image/stb_image_impl.c  (only in git)

STATUS: No stb_image.h header in git - implementation only
```

### VK_mem_alloc Situation
```
include/vendor/vk_mem_alloc.h  (at ROOT LEVEL include/)

STATUS: Only one location - not a duplicate situation
```

---

## What We CAN Safely Delete

Based on ACTUAL git structure:

1. **miniaudio.h duplicate** ✓ (if identical)
   - src/include/vendor/miniaudio.h (check if this is legacy)
   - src/engine/include/vendor/miniaudio.h (check if this is canonical)

That's basically it for vendor files!

---

## What We CANNOT Delete (Not Duplicates)

- `src/engine/cgltf.h` - Actual cgltf header
- `src/engine/vendor/cgltf/cgltf_impl.c` - Implementation, not duplicate
- `src/engine/vendor/stb_image/stb_image_impl.c` - Implementation, not duplicate
- `include/vendor/vk_mem_alloc.h` - Only copy of this file

---

## Revised Phase 1 Plan

### NEW Phase 1: miniaudio.h Only
**Action:** Compare miniaudio.h versions and delete if identical

```bash
# Compare
diff src/include/vendor/miniaudio.h src/engine/include/vendor/miniaudio.h

# If no output (identical):
git rm src/include/vendor/miniaudio.h  # Delete the legacy version
```

**Risk:** VERY LOW
**Impact:** ~100KB saved
**Effort:** 5 minutes

---

## Conclusion

**OUR INITIAL ANALYSIS WAS INCORRECT**

- We identified vendor files that don't exist in git
- We were looking at filesystem state, not git tracking
- The actual duplicate situation is much smaller
- Only miniaudio.h appears to be a true duplicate

**MUCH SAFER:** The actual Phase 1 is simpler and lower-risk!

---

## Next Action

1. Verify miniaudio.h versions are identical
2. Delete only the legacy version (if confirmed duplicate)
3. Test build
4. Then proceed to Phases 2 and 3

