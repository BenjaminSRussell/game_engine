# Rendering Pipeline Bug Tracking

**Last Updated**: 2026-01-11
**Status**: Critical Issues Identified
**Severity**: 🔴 BLOCKER - No rendering possible

---

## CRITICAL BUGS (BLOCKING RENDERING)

### BUG-001: Vulkan Backend Completely Disabled 🔴
**Severity**: BLOCKER
**Status**: Identified, Needs Fix
**File**: `cmake/sources.cmake:397`

```
ISSUE: All Vulkan backend files excluded from compilation
IMPACT: Zero rendering on Linux/Windows platforms
CAUSE: Past compilation errors, macOS-focused development
FIX: Conditional platform compilation
ESTIMATE: 2-4 hours
```

**Details**:
- Line 397 excludes all `.*/backend/vulkan/.*\.c` files
- Affects 43 Vulkan implementation files
- No alternative backend exists for Linux/Windows
- Voxel renderer hardcoded to use Vulkan (will crash on Windows without it)

**Resolution Steps**:
1. [ ] Identify specific compilation errors in Vulkan files
2. [ ] Create platform-specific CMake config
3. [ ] Stub or fix failing Vulkan files
4. [ ] Test compilation on target platforms

---

### BUG-002: Metal Backend C Files Disabled 🔴
**Severity**: BLOCKER
**Status**: Identified, Needs Fix
**File**: `cmake/sources.cmake:400`

```
ISSUE: Metal backend .c files excluded (only .m compiled)
IMPACT: Incomplete Metal implementation on macOS
CAUSE: "Broken synchronization examples/stubs"
FIX: Re-enable and fix Metal C files
ESTIMATE: 3-5 hours
```

**Details**:
- Line 400 excludes all `.*/backend/metal/.*\.c` files
- Only `.m` (Objective-C) files compiled (lines 368-381)
- Creates incomplete Metal backend
- C files contain critical device/buffer management code

**Resolution Steps**:
1. [ ] Identify which Metal C files are functional vs stubs
2. [ ] Fix synchronization issues in:
   - `mtl_sync_primitives.c`
   - `mtl_sync_manager.c`
   - `mtl_hazard_tracking.c`
3. [ ] Re-enable device/buffer/texture management
4. [ ] Test Metal rendering on macOS

---

### BUG-003: Computer Crashes During Rendering 🔴
**Severity**: CRITICAL
**Status**: Under Investigation
**Symptoms**:
- System crashes during rendering operations
- No error messages in logs
- Happens randomly (memory corruption suspected)

**Likely Root Causes**:
1. **Memory leak in Vulkan buffer management**
   - File: `src/engine/backend/vulkan/vk_memory.c`
   - Issue: Allocation/deallocation tracking broken
   - Evidence: `vk_memory.c` may have improper freeing

2. **Double-free or use-after-free in GPU resources**
   - File: `src/engine/backend/vulkan/vk_texture.c`
   - Issue: Texture cleanup in voxel_renderer
   - Line: Check cleanup handlers

3. **Stack overflow in recursive calls**
   - Check rendering call stack depth
   - Watch for unbounded recursion in mesh rendering

4. **Uninitialized pointers in struct**
   - Check: `VoxelRendererData` initialization
   - Ensure all fields zeroed before use

**Investigation Steps**:
1. [ ] Run with `ASAN_OPTIONS=detect_leaks=1` to find leaks
2. [ ] Add memory guards: `malloc_hooks` or valgrind
3. [ ] Log all Vulkan resource allocation/deallocation
4. [ ] Check voxel_renderer resource cleanup
5. [ ] Profile memory usage during rendering

---

## HIGH-PRIORITY BUGS

### BUG-004: Voxel Renderer Incomplete 🟠
**Severity**: HIGH
**Status**: Identified, Partial Fix Available
**File**: `src/engine/core/services/voxel_renderer.c`

**Issues**:
1. Sprite rendering stub (lines 189-206)
   - Method exists but does nothing
   - Prevents UI elements from rendering
   - Severity: HIGH

2. Text rendering stub (lines 214-221)
   - Method exists but does nothing
   - HUD text won't display
   - Severity: HIGH

3. UI quad rendering stub (lines 207-213)
   - Method exists but does nothing
   - Menu system won't render
   - Severity: HIGH

**Impact**: No UI, no overlay text, no sprite rendering
**Fix**: Implement dispatch to appropriate sub-renderers

---

### BUG-005: Post-Processing Not Implemented 🟠
**Severity**: HIGH
**Status**: Infrastructure exists, no implementation
**File**: `src/engine/core/services/post_processing.c`

**Issues**:
1. Bloom structure defined, no shader code
2. Tone mapping commented out
3. HDR not implemented
4. No texture management for effects

**Impact**: No visual effects, no HDR support
**Fix**: Implement bloom, tone mapping, HDR

---

## MEDIUM-PRIORITY BUGS

### BUG-006: Duplicate Vulkan Files 🟡
**Severity**: MEDIUM
**Status**: Identified, Cleanup Needed
**Files**: 7 `_alt` files in `src/engine/backend/vulkan/`

**Issues**:
- Code duplication creates maintenance burden
- Unclear which version is authoritative
- May cause linker conflicts if both compiled

**Files to Delete**:
1. `vulkan_render_alt.c`
2. `vulkan_framebuffer_alt.c`
3. `vulkan_image_alt.c`
4. `vulkan_pipeline_alt.c`
5. `vulkan_buffers_alt.c`
6. `vulkan_capabilities_alt.c`
7. `vulkan_surface_alt.c`

**Fix**: Consolidate to primary files, delete _alt

---

### BUG-007: Disabled Rendering Paths 🟡
**Severity**: MEDIUM
**Status**: Architectural, Many Disabled
**Files**:
- `src/engine/rendering/forward/forward_plus.c` (disabled)
- `src/engine/rendering/deferred/*.c` (all disabled)
- `src/engine/rendering/raytracing/*.c` (all disabled)

**Issues**:
- Many rendering systems marked "// Disabled"
- Infrastructure defined but inactive
- Makes codebase confusing

**Fix**: Either fully implement or delete disabled code

---

## LOW-PRIORITY BUGS

### BUG-008: Metal Voxel Renderer Stub 🟢
**Severity**: LOW
**Status**: Known limitation
**File**: `src/engine/core/services/voxel_renderer_metal.c`

**Issue**: Metal voxel renderer returns NULL
```c
// From voxel_renderer_metal.c (line ~50)
IRenderer *voxel_renderer_create() {
  LOG_ERROR("Metal voxel renderer not implemented");
  return NULL; // ← Returns NULL, can't render
}
```

**Impact**: Can only use Vulkan path on macOS
**Fix**: Implement Metal voxel renderer or use Metal backend wrapper

---

### BUG-009: Sprite 3D Renderer Not Enabled 🟢
**Severity**: LOW
**Status**: Identified, Lower priority
**File**: `src/engine/rendering/renderer_factory.c:14-16`

```c
case RENDERER_TYPE_SPRITE_3D:
  // return sprite_3d_renderer_create();
  LOG_ERROR("Sprite 3D renderer not yet implemented");
  return NULL;
```

**Issue**: Sprite 3D renderer commented out
**Impact**: Can't use 2.5D rendering mode
**Fix**: Enable sprite 3D renderer or remove this type

---

## BUG INVESTIGATION GUIDE

### When Computer Crashes:

**Step 1: Enable Debug Output**
```c
// In voxel_renderer.c, add logging
static bool voxel_renderer_init(IRenderer *self, RendererInitParams *params) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  LOG_DEBUG("Initializing voxel renderer");
  if (!data || !data->vulkan_renderer) {
    LOG_ERROR("Invalid voxel renderer data");
    return false;
  }
  LOG_DEBUG("Calling vulkan_init");
  bool success = vulkan_init(data->vulkan_renderer, params->window,
                             params->width, params->height, params->config);
  LOG_DEBUG("vulkan_init returned: %d", success);
  return success;
}
```

**Step 2: Memory Leak Detection**
```bash
# On macOS/Linux
ASAN_OPTIONS=detect_leaks=1 ./minecraft_app
# Look for leak reports

# With valgrind
valgrind --leak-check=full --show-leak-kinds=all ./minecraft_app
```

**Step 3: Add Crash Handlers**
```c
// In engine initialization
void segfault_handler(int sig) {
  LOG_ERROR("SEGMENTATION FAULT at instruction pointer %p", __builtin_return_address(0));
  exit(1);
}
signal(SIGSEGV, segfault_handler);
```

**Step 4: Check Stack Trace**
```bash
# Build with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=Debug
# Run with debugger
lldb ./minecraft_app
(lldb) run
# When it crashes, type: `bt` for backtrace
```

---

## RENDERING FEATURE STATUS

| Feature | Status | Bug ID | Priority |
|---------|--------|--------|----------|
| Vulkan Backend | BROKEN | BUG-001 | 🔴 CRITICAL |
| Metal Backend | PARTIAL | BUG-002 | 🔴 CRITICAL |
| Voxel Rendering | PARTIAL | BUG-004 | 🟠 HIGH |
| Sprite Rendering | STUB | BUG-004 | 🟠 HIGH |
| Text Rendering | STUB | BUG-004 | 🟠 HIGH |
| UI Rendering | STUB | BUG-004 | 🟠 HIGH |
| Post-Processing | STUB | BUG-005 | 🟠 HIGH |
| Forward+ Pipeline | PARTIAL | - | 🟡 MEDIUM |
| Deferred Pipeline | DISABLED | BUG-007 | 🟡 MEDIUM |
| Ray Tracing | DISABLED | BUG-007 | 🟡 MEDIUM |
| Bloom | STUB | BUG-005 | 🟠 HIGH |
| Tone Mapping | STUB | BUG-005 | 🟠 HIGH |
| TAA | NOT IMPL | - | 🟡 MEDIUM |
| SSAO | NOT IMPL | - | 🟡 MEDIUM |
| SSR | NOT IMPL | - | 🟡 MEDIUM |
| Instancing | IMPL | - | ✅ WORKING |
| LOD System | PARTIAL | - | 🟡 MEDIUM |
| Culling | PARTIAL | - | 🟡 MEDIUM |

---

## RESOLUTION PRIORITY

1. **IMMEDIATE** (Today):
   - [ ] BUG-001: Re-enable Vulkan backend
   - [ ] BUG-002: Re-enable Metal C files
   - [ ] BUG-003: Debug crash (add logging)

2. **TODAY** (Next 4-6 hours):
   - [ ] BUG-004: Complete voxel renderer
   - [ ] BUG-006: Remove duplicate _alt files
   - [ ] Validate compilation

3. **THIS WEEK** (Next 1-2 days):
   - [ ] BUG-005: Implement post-processing
   - [ ] Implement TAA, SSAO, SSR
   - [ ] Stress test rendering system

4. **LATER** (Nice to have):
   - [ ] BUG-007: Implement disabled rendering paths
   - [ ] BUG-008: Metal voxel renderer
   - [ ] BUG-009: Enable sprite 3D renderer

---

## TESTING AFTER FIXES

```bash
# After each fix, run these tests:

# 1. Compilation test
cmake -B build && cmake --build build && echo "✅ BUILD PASSED"

# 2. Rendering test (manual)
./minecraft_app --render-test

# 3. Memory check
ASAN_OPTIONS=detect_leaks=1 ./minecraft_app

# 4. Performance baseline
./minecraft_app --benchmark

# 5. Stress test (rendering complex scenes)
./minecraft_app --stress-test
```

---

## References

- Main roadmap: `IMPLEMENTATION_ROADMAP_FINAL.md`
- Quick fix guide: `RENDERING_FIXES_QUICK_START.md`
- Feature guide: `RENDERING_IMPLEMENTATION_GUIDE.md`
- Vulkan spec: https://www.khronos.org/vulkan/
- Metal spec: https://developer.apple.com/metal/

---

**Next Action**: Start with BUG-001 (Vulkan backend re-enabling)
