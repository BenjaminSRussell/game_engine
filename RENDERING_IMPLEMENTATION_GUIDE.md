# Minecraft v2 - 3D Rendering Pipeline Implementation Guide

**Status**: Critical Issues Identified
**Date**: 2026-01-11
**Focus**: Stabilizing & Extending Rendering System

---

## CRITICAL ISSUES BLOCKING RENDERING

### 1. **VULKAN BACKEND COMPLETELY DISABLED IN CMAKE** ⚠️ BLOCKER

**Location**: `cmake/sources.cmake`, Line 397

```cmake
# Vulkan backend (not needed for macOS/Metal build and has compilation errors)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*\\.c$")
```

**Issue**: The entire Vulkan backend (43 files) is excluded from compilation
- **Impact**: Zero rendering on Linux/Windows
- **Reason**: Compilation errors + macOS focus (but Metal is also broken)
- **Files Excluded**: All Vulkan backend files

**Solution**:
1. Conditionally compile Vulkan only on non-Apple platforms
2. Fix compilation errors in Vulkan backend
3. Separate platform-specific builds

---

### 2. **METAL BACKEND .C FILES ALSO DISABLED** ⚠️ BLOCKER

**Location**: `cmake/sources.cmake`, Line 400

```cmake
# Metal backend .c files (mostly broken synchronization examples/stubs)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/.*\\.c$")
```

**Issue**: Metal backend C files are disabled despite Metal being primary on macOS
- **Impact**: No rendering on macOS
- **Status**: Only `.m` (Objective-C) files compiled
- **Problem**: C files contain critical implementation

**Solution**:
1. Re-enable Metal backend C files with fixes
2. Identify which C files are stubs vs functional
3. Fix synchronization issues

---

### 3. **VOXEL RENDERER PARTIALLY IMPLEMENTED**

**Location**: `src/engine/core/services/voxel_renderer.c`

**Issues**:
- Sprite rendering methods are stubs (lines 189-221)
- Text rendering is stub (line 214-221)
- UI quad rendering is stub (line 207-213)
- Only chunk/mesh rendering is implemented

**Missing**:
- 2.5D UI system
- Text rendering backend
- Sprite batching

---

### 4. **DUPLICATE VULKAN IMPLEMENTATIONS** 🔄 REFACTOR

**7 Duplicate Files** (with _alt variants):

1. `vulkan_render.c` vs `vulkan_render_alt.c`
2. `vulkan_framebuffer.c` vs `vulkan_framebuffer_alt.c`
3. `vulkan_image.c` vs `vulkan_image_alt.c`
4. `vulkan_pipeline.c` vs `vulkan_pipeline_alt.c`
5. `vulkan_buffers.c` vs `vulkan_buffers_alt.c`
6. `vulkan_capabilities.c` vs `vulkan_capabilities_alt.c`
7. `vulkan_surface.c` vs `vulkan_surface_alt.c`

**Action**: Delete _alt files, consolidate to primary versions

---

### 5. **POST-PROCESSING DISABLED**

**Location**: `src/engine/core/services/post_processing.c`

**Status**: Only task definitions, no implementation
- Bloom structure defined but not implemented
- Tone mapping not implemented
- HDR not implemented

---

## RENDERING SYSTEM ARCHITECTURE

### Active Renderer Types
```c
RENDERER_TYPE_VOXEL      // 3D voxel block rendering (uses Vulkan/Metal)
RENDERER_TYPE_SPRITE_3D  // 2.5D sprite rendering (NOT ENABLED)
RENDERER_TYPE_SPRITE_2D  // 2D overlay rendering (NOT ENABLED)
```

### Backend Status

| Backend | Status | Files | Issue |
|---------|--------|-------|-------|
| **Vulkan** | EXCLUDED | 43 | Disabled in CMake (line 397) |
| **Metal** | PARTIAL | 24+.m | Only .m files compiled, .c files disabled (line 400) |
| **STUB** | FALLBACK | 8 | Used when backend unavailable |

### Rendering Pipeline

**Currently Active**:
- Forward rendering (being used)
- Chunk mesh rendering
- Block highlight
- Physics debug visualization

**Disabled but Architecturally Complete**:
- Deferred rendering
- Ray tracing
- Post-processing
- Screen-space effects (SSAO, SSR, TAA)

---

## IMPLEMENTATION ROADMAP

### PHASE 1: STABILIZATION (CRITICAL) 🔴

#### 1.1 Re-enable Vulkan Backend

**File**: `cmake/sources.cmake`

**Change**:
```cmake
# OLD (Line 397)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*\\.c$")

# NEW - Conditionally exclude only on Apple
if(NOT APPLE)
    # On non-Apple platforms, include Vulkan
    # (Will need separate compilation config)
endif()

# For now, if Vulkan has errors, create minimal stubs for non-Apple
```

**Tasks**:
- [ ] Identify specific Vulkan compilation errors
- [ ] Fix or stub problematic files
- [ ] Test Vulkan build on Linux/Windows
- [ ] Add platform-specific build configs

**Estimated**: 4-6 hours

---

#### 1.2 Fix Metal Backend

**File**: `cmake/sources.cmake`

**Change**:
```cmake
# OLD (Line 400)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/.*\\.c$")

# NEW - Re-enable with fixes
# Metal .c files needed:
"src/engine/backend/metal/mtl_device.c"
"src/engine/backend/metal/mtl_command.c"
"src/engine/backend/metal/mtl_buffer.c"
"src/engine/backend/metal/mtl_texture.c"
# etc.
```

**Tasks**:
- [ ] Identify which Metal .c files are functional vs stubs
- [ ] Fix synchronization issues
- [ ] Implement missing device/command management
- [ ] Test Metal rendering on macOS

**Estimated**: 4-6 hours

---

#### 1.3 Remove Vulkan Duplicates

**Files to Delete**:
1. `src/engine/backend/vulkan/vulkan_render_alt.c`
2. `src/engine/backend/vulkan/vulkan_framebuffer_alt.c`
3. `src/engine/backend/vulkan/vulkan_image_alt.c`
4. `src/engine/backend/vulkan/vulkan_pipeline_alt.c`
5. `src/engine/backend/vulkan/vulkan_buffers_alt.c`
6. `src/engine/backend/vulkan/vulkan_capabilities_alt.c`
7. `src/engine/backend/vulkan/vulkan_surface_alt.c`

**Tasks**:
- [ ] Verify primary files have all functionality from _alt files
- [ ] Consolidate any unique code from _alt to primary
- [ ] Delete _alt files
- [ ] Update CMake if needed

**Estimated**: 2 hours

---

#### 1.4 Complete Voxel Renderer

**Location**: `src/engine/core/services/voxel_renderer.c`

**Missing Implementations**:
- Sprite rendering (lines 189-206)
- Text rendering (lines 214-221)
- UI quad rendering (lines 207-213)

**Solution**:
```c
// Instead of stubs, dispatch to appropriate backends
static void voxel_renderer_render_sprite(IRenderer *self, Vec3 position,
                                         Vec2 size, u32 texture_id,
                                         f32 rotation) {
  VoxelRendererData *data = (VoxelRendererData *)self->impl_data;
  if (data && data->sprite_renderer)
    sprite_3d_renderer_render_sprite(data->sprite_renderer, position, size,
                                     texture_id, rotation);
}

// Similarly for text and UI
```

**Tasks**:
- [ ] Implement sprite rendering dispatch
- [ ] Implement text rendering dispatch
- [ ] Implement UI quad rendering dispatch
- [ ] Test 2.5D overlays on top of voxel world

**Estimated**: 3-4 hours

---

### PHASE 2: FEATURE IMPLEMENTATION 🟡

#### 2.1 Post-Processing Pipeline

**File**: `src/engine/core/services/post_processing.c`

**Implement**:

```c
// Bloom System
typedef struct {
  VkImageView input_image;
  VkImageView bright_pass_image;
  VkImageView blur_images[BLOOM_MIPS];
  VkSampler sampler;
  VkDescriptorSet descriptor_set;
  f32 threshold;
  f32 intensity;
} BloomData;

bool post_processing_bloom_execute(PostProcessing *pp,
                                   VkCommandBuffer cmd,
                                   VkImage input, VkImage output) {
  // 1. Extract bright pixels (threshold)
  // 2. Downsample with Gaussian blur
  // 3. Upsample and accumulate
  // 4. Composite with original
}

// Tone Mapping
typedef struct {
  f32 exposure;
  f32 gamma;
  ToneMapMode mode; // Linear, Reinhard, ACES
} ToneMapData;

bool post_processing_tonemap_execute(PostProcessing *pp,
                                     VkCommandBuffer cmd,
                                     VkImage hdr_input, VkImage ldr_output) {
  // Apply tone mapping based on mode
}

// HDR Support
typedef struct {
  bool enabled;
  VkFormat hdr_format; // VK_FORMAT_R16G16B16A16_SFLOAT
  f32 max_brightness;
} HDRData;
```

**Tasks**:
- [ ] Implement bloom extraction shader
- [ ] Implement Gaussian blur compute shader
- [ ] Implement tone mapping shaders (Reinhard, ACES)
- [ ] Implement HDR framebuffer support
- [ ] Test post-processing pipeline

**Estimated**: 8-10 hours

---

#### 2.2 Temporal Anti-Aliasing (TAA)

**File**: `src/engine/rendering/effects/taa.c` (create new)

```c
typedef struct {
  VkImage color_current;
  VkImage color_history;
  VkImage velocity;
  VkSampler bilinear_sampler;
  VkDescriptorSet descriptor_set;
  f32 blend_factor; // 0.1-0.3 typical
} TAAData;

bool taa_execute(TAA *taa, VkCommandBuffer cmd,
                 VkImage input, VkImage output, VkImage velocity) {
  // 1. Sample current frame
  // 2. Reproject using velocity
  // 3. Blend with history
  // 4. Clamp to neighborhood
  // 5. Output TAA result
}
```

**Tasks**:
- [ ] Implement velocity buffer generation
- [ ] Implement TAA reprojection shader
- [ ] Implement history clamping
- [ ] Add jittered camera for subpixel sampling
- [ ] Test temporal stability

**Estimated**: 6-8 hours

---

#### 2.3 Screen-Space Ambient Occlusion (SSAO)

**File**: `src/engine/rendering/effects/ssao.c` (create new)

```c
typedef struct {
  VkImage depth;
  VkImage normal;
  VkImage ao_output;
  VkSampler point_sampler;
  VkDescriptorSet descriptor_set;
  u32 num_samples; // 8-16
  f32 radius;
  f32 bias;
} SSAOData;

bool ssao_execute(SSAO *ssao, VkCommandBuffer cmd,
                  VkImage depth, VkImage normal, VkImage output) {
  // 1. Sample depth and normal
  // 2. Generate random vectors
  // 3. Sample neighborhood for occlusion
  // 4. Blur result
  // 5. Output AO
}
```

**Tasks**:
- [ ] Implement depth/normal sampling
- [ ] Implement SSAO kernel
- [ ] Implement bilateral blur
- [ ] Add temporal reprojection
- [ ] Test AO quality and performance

**Estimated**: 6-8 hours

---

#### 2.4 Screen-Space Reflections (SSR)

**File**: `src/engine/rendering/effects/ssr.c` (create new)

```c
typedef struct {
  VkImage color;
  VkImage normal;
  VkImage depth;
  VkImage roughness;
  VkImage ssr_output;
  VkSampler trilinear_sampler;
  VkDescriptorSet descriptor_set;
  u32 max_steps;
  f32 step_size;
} SSRData;

bool ssr_execute(SSR *ssr, VkCommandBuffer cmd,
                 VkImage color, VkImage normal, VkImage depth,
                 VkImage roughness, VkImage output) {
  // 1. Generate reflection ray for each pixel
  // 2. Trace through depth buffer
  // 3. Sample color from hit location
  // 4. Apply roughness-based blur
  // 5. Output reflections
}
```

**Tasks**:
- [ ] Implement ray generation
- [ ] Implement raytracing shader
- [ ] Implement edge fade
- [ ] Implement temporal reprojection
- [ ] Test reflection quality

**Estimated**: 8-10 hours

---

### PHASE 3: OPTIMIZATION 🟢

#### 3.1 GPU-Driven Rendering

**Implement indirect dispatch for culled geometry**

```c
typedef struct {
  VkBuffer indirect_command_buffer;
  VkBuffer instance_data_buffer;
  VkBuffer culled_command_buffer;
  uint32_t max_commands;
} GPUDrivenRenderingData;

bool gpu_driven_rendering_execute(GPUDrivenRendering *gr,
                                   VkCommandBuffer cmd,
                                   const RenderScene *scene) {
  // 1. Upload instance data
  // 2. Execute culling compute shader
  // 3. Generate indirect commands
  // 4. Execute indirect draw
}
```

**Estimated**: 8-10 hours

---

#### 3.2 Material LOD System

**Implement automatic material quality reduction**

```c
typedef struct {
  Material *high;
  Material *medium;
  Material *low;
  f32 transition_distance_high_med;
  f32 transition_distance_med_low;
} MaterialLOD;

Material *material_select_lod(MaterialLOD *lod, Vec3 camera_pos,
                              Vec3 object_pos) {
  f32 dist = vec3_distance(camera_pos, object_pos);
  if (dist < lod->transition_distance_high_med) return lod->high;
  if (dist < lod->transition_distance_med_low) return lod->medium;
  return lod->low;
}
```

**Estimated**: 4-6 hours

---

## FILE LISTING: WHAT TO IMPLEMENT

### Critical (Must Fix)
- [ ] `cmake/sources.cmake` - Platform-specific backend compilation
- [ ] `src/engine/backend/vulkan/*.c` - Fix and enable Vulkan
- [ ] `src/engine/backend/metal/*.c` - Re-enable Metal C files
- [ ] `src/engine/core/services/voxel_renderer.c` - Complete sprite/text/UI dispatch

### High Priority (Next)
- [ ] `src/engine/core/services/post_processing.c` - Bloom, tone mapping, HDR
- [ ] `src/engine/rendering/effects/taa.c` - Temporal anti-aliasing
- [ ] `src/engine/rendering/effects/ssao.c` - Screen-space AO
- [ ] `src/engine/rendering/effects/ssr.c` - Screen-space reflections

### Medium Priority (Then)
- [ ] GPU-driven rendering system
- [ ] Material LOD implementation
- [ ] Deferred rendering path (alternative to forward)
- [ ] Consolidated rendering passes

### Duplicates to Delete
- [ ] `vulkan_render_alt.c`
- [ ] `vulkan_framebuffer_alt.c`
- [ ] `vulkan_image_alt.c`
- [ ] `vulkan_pipeline_alt.c`
- [ ] `vulkan_buffers_alt.c`
- [ ] `vulkan_capabilities_alt.c`
- [ ] `vulkan_surface_alt.c`

---

## EXPECTED OUTCOME

When complete, you'll have:

✅ **Working Rendering Pipeline**
- Vulkan on Linux/Windows
- Metal on macOS
- Full 3D voxel rendering

✅ **Post-Processing Effects**
- Bloom and HDR tone mapping
- Temporal anti-aliasing
- Screen-space ambient occlusion
- Screen-space reflections

✅ **Performance Optimizations**
- GPU-driven rendering
- Material LOD system
- Efficient batch management

✅ **Clean Codebase**
- No duplicate files
- Platform-specific builds
- Clear feature organization

---

## COMPILATION HINTS

### Platform-Specific Build Configs

```cmake
# In CMakeLists.txt
if(APPLE)
    # Metal backend
    list(APPEND ENGINE_SOURCES ${BACKEND_METAL_SOURCES})
else()
    # Vulkan backend (Linux/Windows)
    list(APPEND ENGINE_SOURCES ${BACKEND_VULKAN_SOURCES})
endif()
```

### Shader Compilation

Ensure shaders are properly compiled:
```cmake
# Add shader compilation targets
add_custom_command(
    OUTPUT ${SHADER_OUTPUT}
    COMMAND glslc -O -fshader-stage=compute -o ${SHADER_OUTPUT} ${SHADER_SOURCE}
    DEPENDS ${SHADER_SOURCE}
)
```

---

## TESTING CHECKLIST

- [ ] CMake builds without errors
- [ ] Voxel rendering works (chunks visible)
- [ ] Block highlighting works
- [ ] Sprite rendering works (overlay elements)
- [ ] Text rendering works (UI text)
- [ ] Post-processing pipeline initializes
- [ ] Bloom effect visible
- [ ] TAA reduces aliasing
- [ ] SSAO adds ambient shadows
- [ ] SSR shows reflections
- [ ] GPU-driven rendering improves perf

---

## KNOWN BUGS TO INVESTIGATE

1. **Computer crashes during rendering**
   - Likely: Memory leak in Vulkan buffer management
   - Check: `vk_memory.c`, `vk_buffer.c` for allocation/deallocation
   - Fix: Add reference counting to Vulkan resources

2. **Disabled rendering backends**
   - Root cause: Compilation errors in past
   - Fix: Conditional compilation by platform

3. **Missing sprite/text/UI rendering**
   - Root cause: Incomplete voxel renderer implementation
   - Fix: Add dispatchers to other renderers

4. **No post-processing**
   - Root cause: Infrastructure defined, not implemented
   - Fix: Implement bloom, tone mapping, HDR

---

## NEXT STEPS

1. **Start with Phase 1.1**: Re-enable Vulkan backend (identify errors)
2. **Move to Phase 1.2**: Fix Metal backend
3. **Quick win Phase 1.3**: Remove duplicate Vulkan files
4. **Complete Phase 1.4**: Finish voxel renderer
5. **Then Phase 2**: Add post-processing effects

Good luck! 🚀
