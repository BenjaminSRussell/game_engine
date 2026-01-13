// include/render/shader_compiler.h
//
// Purpose: Shader compilation and variant management system.
// Handles compilation of material-specific shader variants with dynamic feature selection.
// Caches compiled variants for performance.
//
#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include "engine/include/common.h"
#include "include/rendering/material.h"

#define MAX_SHADER_VARIANTS 512
#define MAX_SHADER_CACHE_SIZE (128 * 1024 * 1024)  // 128 MB shader cache

// Shader compilation status
typedef enum {
    SHADER_STATUS_UNCOMPILED,
    SHADER_STATUS_COMPILING,
    SHADER_STATUS_COMPILED,
    SHADER_STATUS_FAILED,
    SHADER_STATUS_CACHED,
} ShaderStatus;

// Compiled shader module
typedef struct {
    u32 variant_id;
    ShaderVariantKey variant_key;
    ShaderStatus status;

    // SPIR-V binary (compiled shader bytecode)
    u32* spirv_data;
    u32 spirv_size;

    // Vulkan handles (populated after shader_get_module)
    void* vk_module;        // VkShaderModule

    // Metadata
    u32 compile_time_ms;    // Compilation duration
    u32 binary_size;        // SPIR-V binary size in bytes
    char source_hash[32];   // Hash of source code
    char compile_flags[128]; // Compiler flags used
} CompiledShaderVariant;

// Shader compilation request
typedef struct {
    ShaderVariantKey variant_key;
    const char* source_vertex;
    const char* source_fragment;
    const char* defines;    // Preprocessor defines
    u32 priority;           // 0=low, 1=normal, 2=high
    bool is_async;          // Compile asynchronously
} ShaderCompileRequest;

// Shader compiler system
typedef struct {
    CompiledShaderVariant variants[MAX_SHADER_VARIANTS];
    u32 variant_count;

    // LRU cache for compiled shaders
    struct {
        u8* cache_data;
        u32 cache_size;
        u32 cache_used;
        u32 entries;
    } cache;

    // Compilation queue for async compilation
    struct {
        ShaderCompileRequest queue[64];
        u32 queue_size;
        bool is_processing;
    } compile_queue;

    bool initialized;
} ShaderCompiler;

// ==============================================================================
// Shader Compiler Lifecycle
// ==============================================================================

// Initialize shader compiler
void shader_compiler_init(ShaderCompiler* compiler);

// Shutdown shader compiler
void shader_compiler_shutdown(ShaderCompiler* compiler);

// ==============================================================================
// Shader Compilation
// ==============================================================================

// Compile shader variant for a material
bool shader_compile_material_variant(ShaderCompiler* compiler,
                                     Material* material,
                                     const char* vertex_source,
                                     const char* fragment_source);

// Compile from variant key
u32 shader_compile_from_key(ShaderCompiler* compiler,
                            ShaderVariantKey key,
                            const char* vertex_source,
                            const char* fragment_source);

// Compile asynchronously (returns variant ID immediately)
u32 shader_compile_async(ShaderCompiler* compiler,
                         ShaderVariantKey key,
                         const char* vertex_source,
                         const char* fragment_source);

// Process compilation queue
void shader_compiler_process_queue(ShaderCompiler* compiler);

// ==============================================================================
// Shader Lookup & Caching
// ==============================================================================

// Get compiled shader variant by ID
CompiledShaderVariant* shader_get_variant(ShaderCompiler* compiler, u32 variant_id);

// Find variant by key
u32 shader_find_variant(ShaderCompiler* compiler, ShaderVariantKey key);

// Get variant or compile if not cached
u32 shader_get_or_compile(ShaderCompiler* compiler,
                          ShaderVariantKey key,
                          const char* vertex_source,
                          const char* fragment_source);

// ==============================================================================
// Cache Management
// ==============================================================================

// Clear shader cache
void shader_cache_clear(ShaderCompiler* compiler);

// Cache shader to disk
bool shader_cache_save(ShaderCompiler* compiler, const char* cache_file);

// Load shader from disk cache
bool shader_cache_load(ShaderCompiler* compiler, const char* cache_file);

// Get cache statistics
struct {
    u32 total_size;
    u32 used_size;
    u32 entry_count;
    u32 hit_count;
    u32 miss_count;
} shader_cache_get_stats(ShaderCompiler* compiler);

// ==============================================================================
// Material-Specific Compilation
// ==============================================================================

// Generate preprocessor defines from material
char* shader_generate_material_defines(Material* material, char* buffer, u32 buffer_size);

// Validate shader compatibility with material
bool shader_validate_material_compatibility(Material* material,
                                           ShaderVariantKey expected_variant);

// ==============================================================================
// Shader Source Providers
// ==============================================================================

// Load base shader sources from files
bool shader_load_source(const char* filepath, char* buffer, u32 buffer_size);

// Inject material-specific code into shaders
bool shader_inject_material_code(const char* base_source,
                                 Material* material,
                                 char* out_source,
                                 u32 out_size);

// ==============================================================================
// Debug & Diagnostics
// ==============================================================================

// Get shader compilation info
struct {
    const char* status_string;
    u32 compile_time_ms;
    u32 binary_size;
    char error_message[256];
} shader_get_compile_info(ShaderCompiler* compiler, u32 variant_id);

// Log shader statistics
void shader_log_statistics(ShaderCompiler* compiler);

// Validate SPIR-V binary
bool shader_validate_spirv(u32* spirv_data, u32 spirv_size);

#endif // SHADER_COMPILER_H
