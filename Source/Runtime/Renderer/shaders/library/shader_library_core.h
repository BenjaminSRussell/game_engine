/**
 * =================================================================================================
 *                              SHADER LIBRARY - CORE SYSTEM
 *                                   Agent: AGENT_SHADER_1
 * =================================================================================================
 */

#ifndef SHADER_LIBRARY_CORE_H
#define SHADER_LIBRARY_CORE_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    SHADER TYPES
 * =================================================================================================
 */

typedef enum ShaderLibraryType {
  SHADER_LIB_PBR_STANDARD,
  SHADER_LIB_PBR_CLEARCOAT,
  SHADER_LIB_PBR_ANISOTROPIC,
  SHADER_LIB_PBR_SUBSURFACE,
  SHADER_LIB_PBR_IRIDESCENCE,
  SHADER_LIB_STYLIZED_CEL,
  SHADER_LIB_STYLIZED_WATERCOLOR,
  SHADER_LIB_STYLIZED_PIXELART,
  SHADER_LIB_STYLIZED_SKETCH,
  SHADER_LIB_ENV_WATER_OCEAN,
  SHADER_LIB_ENV_WATER_RIVER,
  SHADER_LIB_ENV_FOLIAGE,
  SHADER_LIB_ENV_SNOW,
  SHADER_LIB_ENV_TERRAIN,
  SHADER_LIB_VFX_PARTICLE,
  SHADER_LIB_VFX_BEAM,
  SHADER_LIB_VFX_HOLOGRAM,
  SHADER_LIB_VFX_DISSOLVE,
  SHADER_LIB_POST_BLOOM,
  SHADER_LIB_POST_DOF,
  SHADER_LIB_POST_MOTION_BLUR,
  SHADER_LIB_POST_COLOR_GRADE,
  SHADER_LIB_VOLUMETRIC_CLOUDS,
  SHADER_LIB_ATMOSPHERE_SCATTERING,
  SHADER_LIB_FUR,
  SHADER_LIB_GLASS,
  SHADER_LIB_DECAL,
  SHADER_LIB_PARALLAX_OCCLUSION,
  SHADER_LIB_TESSELLATION,
  SHADER_LIB_PROCEDURAL_NOISE,
  SHADER_LIB_COUNT
} ShaderLibraryType;

/* =================================================================================================
 *                                  SHADER VARIANT SYSTEM
 * =================================================================================================
 */

typedef struct ShaderVariantKey {
  uint64_t feature_flags; // Bitfield of enabled features
  uint32_t quality_level; // 0=Low, 1=Medium, 2=High, 3=Ultra
  uint32_t backend_type;  // Vulkan, Metal, DX12, OpenGL
} ShaderVariantKey;

typedef struct ShaderVariant {
  ShaderVariantKey key;
  void *compiled_shader; // Backend-specific compiled shader
  uint32_t compiled_size;
  uint64_t compile_time_ms;
  bool is_valid;
} ShaderVariant;

/* =================================================================================================
 *                                  SHADER PARAMETERS
 * =================================================================================================
 */

typedef enum ShaderParameterType {
  SHADER_PARAM_FLOAT,
  SHADER_PARAM_FLOAT2,
  SHADER_PARAM_FLOAT3,
  SHADER_PARAM_FLOAT4,
  SHADER_PARAM_INT,
  SHADER_PARAM_BOOL,
  SHADER_PARAM_TEXTURE_2D,
  SHADER_PARAM_TEXTURE_CUBE,
  SHADER_PARAM_TEXTURE_3D,
  SHADER_PARAM_MATRIX_4X4,
  SHADER_PARAM_COLOR_RGB,
  SHADER_PARAM_COLOR_RGBA,
  SHADER_PARAM_GRADIENT,
  SHADER_PARAM_CURVE,
  SHADER_PARAM_BUFFER,
  SHADER_PARAM_SAMPLER,
} ShaderParameterType;

typedef struct ShaderParameter {
  char name[64];
  ShaderParameterType type;
  union {
    float float_value;
    float float2_value[2];
    float float3_value[3];
    float float4_value[4];
    int32_t int_value;
    bool bool_value;
    uint32_t texture_id;
    float matrix_value[16];
  } default_value;
  float min_value;
  float max_value;
  char display_name[64];
  char tooltip[256];
  uint32_t ui_order;
  bool is_hidden;
} ShaderParameter;

/* =================================================================================================
 *                                  SHADER LIBRARY ENTRY
 * =================================================================================================
 */

typedef struct ShaderLibraryEntry {
  ShaderLibraryType type;
  char name[64];
  char description[256];
  char category[32];
  char thumbnail_path[256];

  ShaderParameter *parameters;
  uint32_t parameter_count;

  ShaderVariant *variants;
  uint32_t variant_count;

  // Shader source paths
  char vertex_shader_path[256];
  char fragment_shader_path[256];
  char geometry_shader_path[256];
  char compute_shader_path[256];
  char tessellation_control_path[256];
  char tessellation_eval_path[256];

  // Feature flags
  bool supports_instancing;
  bool supports_skinning;
  bool supports_morph_targets;
  bool supports_tessellation;
  bool supports_raytracing;
  bool is_transparent;
  bool is_double_sided;
  uint32_t render_queue;
} ShaderLibraryEntry;

/* =================================================================================================
 *                                  SHADER LIBRARY MANAGER
 * =================================================================================================
 */

typedef struct ShaderLibraryManager {
  ShaderLibraryEntry *entries;
  uint32_t entry_count;
  uint32_t entry_capacity;

  // Variant cache
  void *variant_cache;
  uint64_t cache_size_bytes;
  uint64_t cache_max_size_bytes;

  // Compilation state
  bool is_compiling;
  uint32_t pending_compilations;

  // Statistics
  uint64_t total_variants_compiled;
  uint64_t total_compilation_time_ms;
  uint64_t cache_hits;
  uint64_t cache_misses;
} ShaderLibraryManager;

/* =================================================================================================
 *                                  API FUNCTIONS
 * =================================================================================================
 */

// Core lifecycle
bool shader_library_init(ShaderLibraryManager *manager, uint32_t max_entries);
void shader_library_shutdown(ShaderLibraryManager *manager);
void shader_library_update(ShaderLibraryManager *manager, float delta_time);

// Shader registration
uint32_t shader_library_register(ShaderLibraryManager *manager,
                                 const ShaderLibraryEntry *entry);
bool shader_library_unregister(ShaderLibraryManager *manager,
                               uint32_t shader_id);

// Shader lookup
const ShaderLibraryEntry *
shader_library_get_by_id(ShaderLibraryManager *manager, uint32_t shader_id);
const ShaderLibraryEntry *
shader_library_get_by_type(ShaderLibraryManager *manager,
                           ShaderLibraryType type);
const ShaderLibraryEntry *
shader_library_get_by_name(ShaderLibraryManager *manager, const char *name);

// Variant management
ShaderVariant *shader_library_get_variant(ShaderLibraryManager *manager,
                                          uint32_t shader_id,
                                          ShaderVariantKey key);
void shader_library_precompile_variants(ShaderLibraryManager *manager,
                                        uint32_t shader_id);
void shader_library_evict_variant(ShaderLibraryManager *manager,
                                  uint32_t shader_id, ShaderVariantKey key);

// Parameter management
bool shader_library_set_parameter(ShaderLibraryManager *manager,
                                  uint32_t shader_id, const char *param_name,
                                  void *value);
bool shader_library_get_parameter(ShaderLibraryManager *manager,
                                  uint32_t shader_id, const char *param_name,
                                  void *out_value);

// Serialization
bool shader_library_save(ShaderLibraryManager *manager, const char *path);
bool shader_library_load(ShaderLibraryManager *manager, const char *path);

/* =================================================================================================
 *                              SHADER COMPILATION UTILITIES
 * =================================================================================================
 */

void shader_compile_glsl_to_spirv(const char *src);
void shader_compile_hlsl_to_dxil(const char *src);
void shader_reflect_parameters(void *shader_binary);
void shader_optimize_bytecode(void *shader_binary);

/* =================================================================================================
 *                              SHADER GRAPH INTEGRATION
 * =================================================================================================
 */

void shader_graph_generate_glsl(void *graph);
bool shader_graph_validate_connections(void *graph);
void shader_graph_optimize(void *graph);

/* =================================================================================================
 *                              MATERIAL INTEGRATION
 * =================================================================================================
 */

void material_bind_shader(uint32_t material_id, uint32_t shader_id);
void material_override_instance(uint32_t instance_id, const char *param,
                                void *value);
void material_batch_draw(void);
void material_streaming_update(void);

/* =================================================================================================
 *                              RENDERING INTEGRATION
 * =================================================================================================
 */

void shader_bind_forward(uint32_t shader_id);
void shader_bind_deferred(uint32_t shader_id);
void shader_bind_raytracing(uint32_t shader_id);
void shader_get_shadow_variant(uint32_t shader_id);
void shader_get_depth_variant(uint32_t shader_id);

/* =================================================================================================
 *                              DEBUG AND VISUALIZATION
 * =================================================================================================
 */

void shader_debug_enable(bool enabled);
void shader_visualize_complexity(void);
void shader_visualize_overdraw(void);
void shader_validate_active(void);

#endif // SHADER_LIBRARY_CORE_H
