# Material System Architecture

## Overview

The material system manages shader variants, material parameters, rendering state, and GPU resource bindings. It is intentionally distributed across multiple modules to separate concerns and enable efficient rendering.

---

## Component Map

### `materials/material_system/` - Material Management

**Responsibility:** Material lifecycle, instances, parameters, overrides

**Key Files:**
- `material_instance.{c,h}` - Material instance creation and lifecycle
- `material_lod.{c,h}` - LOD material variants
- `material_parameters.{c,h}` - Parameter storage and updates
- `material_overrides.{c,h}` - Per-instance parameter overrides
- `material_sorting.{c,h}` - Material state sorting for rendering
- `material_dynamic.{c,h}` - Dynamic material generation

**API Pattern:**
```c
MaterialInstance* material_instance_create(Material* template);
void material_instance_set_parameter(MaterialInstance* inst, const char* name, float value);
void material_instance_update_gpu(MaterialInstance* inst);  // Upload to GPU
```

**Responsibility Scope:**
- ✅ Creating material instances from templates
- ✅ Managing parameter data on CPU
- ✅ Dirty tracking for GPU updates
- ✅ Reference counting and lifecycle
- ❌ Shader compilation (handled by `materials/shaders/`)
- ❌ GPU buffer layout (handled by `rendering/gpu_driven/`)
- ❌ Rendering order (handled by `rendering/visibility/`)

---

### `materials/pbr/` - PBR Models

**Responsibility:** PBR BRDF calculations, parameter validation, material models

**Key Files:**
- `metallic_roughness.{c,h}` - Metallic-roughness PBR BRDF
- `pbr_conversion.{c,h}` - Convert between PBR models (spec->metallic, etc.)
- `pbr_validation.{c,h}` - Validate PBR parameter ranges
- `pbr_presets.{c,h}` - Common material presets (wood, metal, plastic, etc.)
- `pbr_parameters.{c,h}` - PBR parameter definitions

**API Pattern:**
```c
typedef struct {
    float albedo[3];      // Linear RGB
    float metallic;       // 0-1
    float roughness;      // 0-1
    float ao;             // Ambient occlusion 0-1
    float normal_strength;
} PBRParameters;

void pbr_validate_parameters(PBRParameters* params);  // Clamp/validate
float pbr_compute_fresnel(float metallic, const float* albedo);
```

**Responsibility Scope:**
- ✅ BRDF model definitions
- ✅ Parameter validation and conversion
- ✅ Material presets and recipes
- ✅ Physics-based parameter ranges
- ❌ Shader compilation
- ❌ GPU buffers
- ❌ Material instances (belongs to `material_system/`)

---

### `materials/shaders/` - Shader Frontend

**Responsibility:** Shader variant generation, caching, backend-agnostic compilation

**Key Files:**
- `shader_compiler.{c,h}` - Main shader compilation interface
- `shader_cache.{c,h}` - Compiled shader caching
- `shader_variants.{c,h}` - Shader variant selection and generation
- `shader_defines.h` - Common shader compile-time defines

**API Pattern:**
```c
ShaderVariant variant = shader_variant_select(
    material->shader_id,
    instance->parameters,
    render_context->quality_level
);

CompiledShader* compiled = shader_cache_get_or_compile(variant);
```

**Responsibility Scope:**
- ✅ Determine which shader variant needed
- ✅ Handle compile-time defines and #ifdef selection
- ✅ Cache compiled shaders
- ✅ Backend-agnostic compilation interface
- ❌ Metal/GPU-specific compilation (backend's job)
- ❌ Linking with engine services
- ❌ Runtime parameter binding (GPU buffers)

**Backend-Agnostic:**
This layer should NOT know about Metal, Vulkan, or D3D12 specifics. It produces generic shader intermediate representation or delegates to backend.

---

### `backend/metal/` - Metal Shader Backend

**Location:** `backend/metal/mtl_shader_compiler.m`

**Responsibility:** Metal-specific shader compilation (.metal → .metallib)

**Key Functions:**
```c
MetalShaderLibrary* mtl_shader_compile(const ShaderSource* source, const char** defines);
MTLFunction* mtl_get_shader_function(MetalShaderLibrary* lib, const char* entry_point);
```

**Responsibility Scope:**
- ✅ Metal-specific compilation flags
- ✅ .metallib generation and linking
- ✅ Function extraction and validation
- ✅ Metal device integration
- ❌ Shader language features (belongs to shader_compiler)
- ❌ Variant selection logic

---

### `rendering/gpu_driven/` - GPU Material Buffers

**Location:** `rendering/gpu_driven/material_data_gpu.c`

**Responsibility:** GPU buffer layout, upload, and binding for GPU-driven pipeline

**Key Structures:**
```c
typedef struct {
    float albedo[3];
    float pad0;
    float metallic;
    float roughness;
    float ao;
    float normal_strength;
    // ... layout matches shader UBO
} MaterialDataGPU;

typedef struct {
    VkBuffer buffer;
    VkDescriptorSet descriptor_set;
    MaterialDataGPU* mapped_data;
} MaterialBufferGPU;
```

**Responsibility Scope:**
- ✅ Define GPU buffer struct layout
- ✅ Allocate GPU buffers
- ✅ Upload CPU parameters to GPU
- ✅ Bind buffers for rendering
- ✅ Manage persistent mapping
- ❌ CPU-side parameter storage (material_system's job)
- ❌ Shader compilation

---

### `rendering/visibility/` - Rendering Classification

**Location:** `rendering/visibility/material_classify.c`

**Responsibility:** Material sorting, batching, and state grouping for rendering

**Key Functions:**
```c
typedef struct {
    MaterialClass class;           // Opaque, transparent, decal, etc.
    RenderingState state;          // Blend mode, cull, etc.
    uint32_t material_instance_id; // Reference to GPU buffer
} MaterialRenderingInfo;

void material_classify(MaterialInstance* inst, MaterialRenderingInfo* out);
int material_compare_for_sorting(const void* a, const void* b);
```

**Responsibility Scope:**
- ✅ Categorize materials (opaque vs transparent)
- ✅ Sort for efficient state changes
- ✅ Group materials into batches
- ✅ Extract rendering state (blend, cull, etc.)
- ❌ Material parameters
- ❌ GPU buffer layout

---

## Data Flow

### Material Creation

```
1. Material Template (loaded from disk)
   ↓
2. MaterialSystem::CreateInstance()
   ├─ Allocate MaterialInstance struct
   ├─ Copy template parameters
   └─ Initialize CPU data
   ↓
3. ShaderVariant::Select()
   ├─ Query PBR model for parameter analysis
   └─ Select variant based on parameters
   ↓
4. ShaderCompiler::GetOrCompile()
   ├─ Lookup cache
   └─ Compile if needed (delegates to backend)
```

### Material Rendering Setup

```
MaterialInstance
   ↓
[Material System] Mark dirty, store CPU parameters
   ↓
[GPU Driven] Upload to GPU uniform buffer
   ├─ material_data_gpu_update() reads from material_instance
   └─ Writes to persistent-mapped GPU buffer
   ↓
[Rendering Visibility] Classify material
   ├─ Determine render queue (opaque, transparent, etc.)
   ├─ Extract state (blend mode, cull, etc.)
   └─ Sort with other materials
   ↓
[GPU Driven] Generate draw command
   └─ Include material buffer reference
```

### At Render Time

```
GPU Scene (compute shader)
   ├─ Reads scene data
   ├─ Performs culling
   └─ Generates indirect draw commands
   ↓
GPU Rendering
   ├─ Bind material instance buffer (material_data_gpu)
   ├─ Bind compiled shader (shader_compiler result)
   ├─ Execute draw command
   └─ Shader reads from bound buffers
```

---

## Why This Distribution?

### Separation of Concerns

```
CPU Parameters          → material_system/
BRDF Science           → pbr/
Shader Selection       → shaders/
GPU Layout/Binding     → gpu_driven/
Rendering Order        → visibility/
Platform Compilation   → backend/
```

Each module has a clear, independent responsibility without circular dependencies.

### Performance Benefits

- **material_system:** Fast parameter updates without recompilation
- **pbr:** Pre-computed BRDF look-up tables, cached calculations
- **shaders:** Compile once, reuse many times (caching)
- **gpu_driven:** Persistent mapping eliminates upload stalls
- **visibility:** Sorting reduces state changes and improves cache locality

### Scalability

- Materials can be instanced with different parameters (material_system)
- Shader compilation is batched and cached (shaders/)
- GPU buffers are pooled and reused (gpu_driven/)
- Rendering order is computed once per frame (visibility/)

### Flexibility

- Can swap BRDF models (pbr/) without touching others
- Can change GPU buffer layout (gpu_driven/) without changing material_system
- Can add new variants (shader_compiler) independently
- Can change rendering order logic (visibility/) without affecting parameters

---

## Common Operations

### Set Material Parameter

```c
MaterialInstance* mat = material_instance_get(id);
material_instance_set_parameter(mat, "metallic", 0.8f);
// Parameter is stored in mat->cpu_data.metallic
// Marked dirty for next GPU sync
```

### Upload to GPU

```c
// Called once per frame for dirty materials
material_data_gpu_update(mat);
// Copies mat->cpu_data to GPU buffer via persistent mapping
// Clears dirty flag
```

### Render with Material

```c
// Visibility classifier prepares render info
MaterialRenderingInfo info;
material_classify(mat, &info);

// GPU driven renderer binds material
// Shader is already compiled and cached
// GPU buffer is already synchronized
// Shader reads from bound material buffer
```

---

## Integration Points

### With Rendering Path

**GPU-Driven:** Uses gpu_driven/material_data_gpu for buffer layout and binding

**Forward+:** Uses material_system for parameters, visibility for classification

**Deferred:** Uses pbr/ for G-buffer encoding, material_system for instances

### With Post-Processing

Post-processing can access material parameters via visibility/material_classify for effects like:
- Per-material bloom thresholds
- Per-material exposure bias
- Material-aware tonemapping

### With Editor

Editor can:
- Create MaterialInstance via material_system
- Set parameters via material_instance_set_parameter
- Preview via shader_compiler (get current shader)
- See compiled result via backend

---

## Future Improvements

- [ ] Material streaming system
- [ ] Async shader compilation
- [ ] Shader permutation reduction (fewer variants)
- [ ] Material versioning for updates
- [ ] Per-material render queue custom sorting

---

## Testing

### Unit Tests

- `material_system/` - Instance lifecycle, parameter updates
- `pbr/` - BRDF calculations, validation
- `shaders/` - Variant selection logic
- `gpu_driven/` - Buffer layout correctness, synchronization
- `visibility/` - Sorting and classification

### Integration Tests

- End-to-end material creation → GPU rendering
- Parameter change → GPU sync → visual update
- Material LOD transitions
- Different render paths with same materials

---

## See Also

- `RENDERING_ARCHITECTURE.md` - Rendering pipeline overview
- `../../../backend/metal/` - Metal backend implementation
- `../types/` - Specific material type implementations
