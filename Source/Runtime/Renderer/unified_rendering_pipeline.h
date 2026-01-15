#ifndef UNIFIED_RENDERING_PIPELINE_H
#define UNIFIED_RENDERING_PIPELINE_H

/*
 * Unified Rendering Pipeline
 * Consolidates all rendering systems into a single, coherent pipeline
 * Fixes compilation issues by providing standardized interfaces
 */

#include "unified_memory.h"
#include "unified_logging.h"
#include "error_handling.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RENDERING PIPELINE STAGES
 * ============================================================================ */

typedef enum {
    RENDER_STAGE_SHADOW_MAP = 0,
    RENDER_STAGE_DEPTH_PREPASS,
    RENDER_STAGE_GBUFFER,
    RENDER_STAGE_DEFERRED_LIGHTING,
    RENDER_STAGE_FORWARD_TRANSPARENT,
    RENDER_STAGE_POST_PROCESS,
    RENDER_STAGE_UI,
    RENDER_STAGE_DEBUG,
    RENDER_STAGE_COUNT
} render_stage_t;

typedef enum {
    RENDER_BACKEND_OPENGL = 0,
    RENDER_BACKEND_VULKAN,
    RENDER_BACKEND_METAL,
    RENDER_BACKEND_DIRECTX12,
    RENDER_BACKEND_COUNT
} render_backend_t;

/* ============================================================================
 * RENDERING CONFIGURATION
 * ============================================================================ */

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
    bool fullscreen;
    bool vsync;
    uint32_t msaa_samples;
    float anisotropy_level;
    bool enable_shadows;
    uint32_t shadow_map_size;
    bool enable_deferred;
    bool enable_post_process;
    bool enable_debug_rendering;
    render_backend_t backend;
    bool gamma_correction;
    float exposure;
    float bloom_threshold;
    bool enable_taa;
} render_config_t;

/* ============================================================================
 * RENDER TARGETS AND BUFFERS
 * ============================================================================ */

typedef struct {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t attachment_type;  // Color, depth, stencil
    bool multisampled;
} render_target_t;

typedef struct {
    render_target_t* targets;
    uint32_t target_count;
    uint32_t framebuffer_id;
    bool enabled;
} render_framebuffer_t;

/* ============================================================================
 * SHADER SYSTEM
 * ============================================================================ */

typedef struct {
    uint32_t program_id;
    uint32_t vertex_shader;
    uint32_t fragment_shader;
    uint32_t geometry_shader;
    uint32_t compute_shader;
    char* vertex_source;
    char* fragment_source;
    char* geometry_source;
    char* compute_source;
    bool compiled;
    bool linked;
} shader_program_t;

typedef struct {
    shader_program_t* programs;
    uint32_t program_count;
    uint32_t capacity;
    uint32_t current_program;
} shader_system_t;

/* ============================================================================
 * UNIFIED RENDERING PIPELINE
 * ============================================================================ */

typedef struct {
    // Configuration
    render_config_t config;
    
    // Backend-specific data
    void* backend_data;
    render_backend_t backend_type;
    
    // Render targets
    render_framebuffer_t gbuffer;
    render_framebuffer_t shadow_framebuffer;
    render_framebuffer_t post_process_framebuffer;
    render_framebuffer_t ui_framebuffer;
    
    // Shader system
    shader_system_t shader_system;
    
    // Pipeline stages
    bool stage_enabled[RENDER_STAGE_COUNT];
    uint32_t stage_order[RENDER_STAGE_COUNT];
    uint32_t stage_count;
    
    // State
    bool initialized;
    bool rendering;
    uint32_t frame_number;
    float delta_time;
    
    // Statistics
    uint32_t draw_calls;
    uint32_t triangles_rendered;
    uint32_t vertices_processed;
    uint64_t frame_time_ns;
    uint64_t gpu_time_ns;
    
} unified_rendering_pipeline_t;

/* ============================================================================
 * UNIFIED RENDERING PIPELINE API
 * ============================================================================ */

// Lifecycle management
unified_rendering_pipeline_t* unified_rendering_pipeline_create(const render_config_t* config);
void unified_rendering_pipeline_destroy(unified_rendering_pipeline_t* pipeline);
bool unified_rendering_pipeline_initialize(unified_rendering_pipeline_t* pipeline);
void unified_rendering_pipeline_shutdown(unified_rendering_pipeline_t* pipeline);

// Configuration
void unified_rendering_pipeline_set_config(unified_rendering_pipeline_t* pipeline, 
                                          const render_config_t* config);
void unified_rendering_pipeline_get_config(unified_rendering_pipeline_t* pipeline, 
                                          render_config_t* config);
void unified_rendering_pipeline_resize(unified_rendering_pipeline_t* pipeline, 
                                      uint32_t width, uint32_t height);

// Pipeline execution
void unified_rendering_pipeline_begin_frame(unified_rendering_pipeline_t* pipeline);
void unified_rendering_pipeline_end_frame(unified_rendering_pipeline_t* pipeline);
void unified_rendering_pipeline_render_stage(unified_rendering_pipeline_t* pipeline, 
                                           render_stage_t stage);
void unified_rendering_pipeline_execute(unified_rendering_pipeline_t* pipeline);

// Stage management
void unified_rendering_pipeline_enable_stage(unified_rendering_pipeline_t* pipeline, 
                                            render_stage_t stage, bool enabled);
void unified_rendering_pipeline_set_stage_order(unified_rendering_pipeline_t* pipeline, 
                                               const render_stage_t* stages, uint32_t count);
bool unified_rendering_pipeline_is_stage_enabled(unified_rendering_pipeline_t* pipeline, 
                                                render_stage_t stage);

// Backend abstraction
bool unified_rendering_pipeline_set_backend(unified_rendering_pipeline_t* pipeline, 
                                           render_backend_t backend);
render_backend_t unified_rendering_pipeline_get_backend(unified_rendering_pipeline_t* pipeline);

/* ============================================================================
 * SHADER MANAGEMENT API
 * ============================================================================ */

// Shader loading and compilation
uint32_t unified_rendering_load_shader(unified_rendering_pipeline_t* pipeline, 
                                      const char* vertex_source, const char* fragment_source);
uint32_t unified_rendering_load_compute_shader(unified_rendering_pipeline_t* pipeline, 
                                              const char* compute_source);
bool unified_rendering_compile_shader(unified_rendering_pipeline_t* pipeline, uint32_t program_id);
void unified_rendering_unload_shader(unified_rendering_pipeline_t* pipeline, uint32_t program_id);

// Shader usage
void unified_rendering_use_shader(unified_rendering_pipeline_t* pipeline, uint32_t program_id);
uint32_t unified_rendering_get_current_shader(unified_rendering_pipeline_t* pipeline);
void unified_rendering_set_shader_uniform(unified_rendering_pipeline_t* pipeline, 
                                         const char* name, const void* value, uint32_t size);

/* ============================================================================
 * RENDER TARGET MANAGEMENT API
 * ============================================================================ */

// Render target creation and management
uint32_t unified_rendering_create_render_target(unified_rendering_pipeline_t* pipeline, 
                                              uint32_t width, uint32_t height, 
                                              uint32_t format, uint32_t attachment_type);
void unified_rendering_destroy_render_target(unified_rendering_pipeline_t* pipeline, uint32_t target_id);
void unified_rendering_bind_render_target(unified_rendering_pipeline_t* pipeline, uint32_t target_id);
void unified_rendering_unbind_render_target(unified_rendering_pipeline_t* pipeline);

// Framebuffer management
uint32_t unified_rendering_create_framebuffer(unified_rendering_pipeline_t* pipeline);
void unified_rendering_destroy_framebuffer(unified_rendering_pipeline_t* pipeline, uint32_t framebuffer_id);
void unified_rendering_framebuffer_attach_texture(unified_rendering_pipeline_t* pipeline, 
                                                uint32_t framebuffer_id, uint32_t target_id, 
                                                uint32_t attachment_point);
void unified_rendering_bind_framebuffer(unified_rendering_pipeline_t* pipeline, uint32_t framebuffer_id);

/* ============================================================================
 * DRAWING COMMANDS API
 * ============================================================================ */

// Basic drawing
void unified_rendering_clear(unified_rendering_pipeline_t* pipeline, float r, float g, float b, float a);
void unified_rendering_clear_depth(unified_rendering_pipeline_t* pipeline, float depth);
void unified_rendering_set_viewport(unified_rendering_pipeline_t* pipeline, 
                                  uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void unified_rendering_set_scissor(unified_rendering_pipeline_t* pipeline, 
                                  uint32_t x, uint32_t y, uint32_t width, uint32_t height);

// Mesh drawing
void unified_rendering_draw_mesh(unified_rendering_pipeline_t* pipeline, 
                                uint32_t vertex_buffer, uint32_t index_buffer, 
                                uint32_t index_count);
void unified_rendering_draw_instanced(unified_rendering_pipeline_t* pipeline, 
                                    uint32_t vertex_buffer, uint32_t index_buffer, 
                                    uint32_t index_count, uint32_t instance_count);
void unified_rendering_draw_indirect(unified_rendering_pipeline_t* pipeline, 
                                    uint32_t command_buffer);

/* ============================================================================
 * COMPATIBILITY LAYER - Legacy renderer integration
 * ============================================================================ */

// Legacy IRenderer interface compatibility
typedef struct {
    void* (*init)(void* config);
    void (*destroy)(void* renderer);
    void (*begin_frame)(void* renderer);
    void (*end_frame)(void* renderer);
    void (*render)(void* renderer, void* scene);
    void (*set_camera)(void* renderer, void* camera);
    void (*set_viewport)(void* renderer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
} legacy_renderer_interface_t;

// Legacy renderer wrapper
typedef struct {
    unified_rendering_pipeline_t* pipeline;
    legacy_renderer_interface_t* legacy_interface;
    void* legacy_data;
} legacy_renderer_wrapper_t;

legacy_renderer_wrapper_t* legacy_renderer_create_wrapper(unified_rendering_pipeline_t* pipeline);
void legacy_renderer_destroy_wrapper(legacy_renderer_wrapper_t* wrapper);
void* legacy_renderer_get_interface(legacy_renderer_wrapper_t* wrapper);

/* ============================================================================
 * DEBUG AND PROFILING
 * ============================================================================ */

#ifdef DEBUG_BUILD
#define RENDER_DEBUG_ENABLED 1
#else
#define RENDER_DEBUG_ENABLED 0
#endif

#if RENDER_DEBUG_ENABLED
void unified_rendering_debug_begin_marker(unified_rendering_pipeline_t* pipeline, const char* name);
void unified_rendering_debug_end_marker(unified_rendering_pipeline_t* pipeline);
void unified_rendering_debug_print_stats(unified_rendering_pipeline_t* pipeline);
#else
#define unified_rendering_debug_begin_marker(pipeline, name) ((void)0)
#define unified_rendering_debug_end_marker(pipeline) ((void)0)
#define unified_rendering_debug_print_stats(pipeline) ((void)0)
#endif

// Performance profiling
void unified_rendering_begin_profile(unified_rendering_pipeline_t* pipeline, const char* name);
void unified_rendering_end_profile(unified_rendering_pipeline_t* pipeline, const char* name);
void unified_rendering_get_frame_stats(unified_rendering_pipeline_t* pipeline, 
                                     uint64_t* frame_time_ns, uint64_t* gpu_time_ns, 
                                     uint32_t* draw_calls, uint32_t* triangles);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

// Format conversion
uint32_t unified_rendering_get_internal_format(unified_rendering_pipeline_t* pipeline, uint32_t external_format);
uint32_t unified_rendering_get_pixel_type(unified_rendering_pipeline_t* pipeline, uint32_t format);
bool unified_rendering_is_format_supported(unified_rendering_pipeline_t* pipeline, uint32_t format);

// Backend information
const char* unified_rendering_get_backend_name(render_backend_t backend);
const char* unified_rendering_get_gpu_name(unified_rendering_pipeline_t* pipeline);
const char* unified_rendering_get_driver_version(unified_rendering_pipeline_t* pipeline);

// Error handling
const char* unified_rendering_get_error_string(unified_rendering_pipeline_t* pipeline);
bool unified_rendering_check_error(unified_rendering_pipeline_t* pipeline);

/* ============================================================================
 * MACROS AND CONVENIENCE FUNCTIONS
 * ============================================================================ */

// Common render stage macros
#define RENDER_SHADOW_MAP(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_SHADOW_MAP)
#define RENDER_DEPTH_PREPASS(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_DEPTH_PREPASS)
#define RENDER_GBUFFER(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_GBUFFER)
#define RENDER_DEFERRED_LIGHTING(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_DEFERRED_LIGHTING)
#define RENDER_FORWARD_TRANSPARENT(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_FORWARD_TRANSPARENT)
#define RENDER_POST_PROCESS(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_POST_PROCESS)
#define RENDER_UI(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_UI)
#define RENDER_DEBUG(pipeline) unified_rendering_pipeline_render_stage(pipeline, RENDER_STAGE_DEBUG)

// Pipeline execution macros
#define RENDER_BEGIN_FRAME(pipeline) unified_rendering_pipeline_begin_frame(pipeline)
#define RENDER_END_FRAME(pipeline) unified_rendering_pipeline_end_frame(pipeline)
#define RENDER_EXECUTE(pipeline) unified_rendering_pipeline_execute(pipeline)

// Shader macros
#define RENDER_USE_SHADER(pipeline, program) unified_rendering_use_shader(pipeline, program)
#define RENDER_SET_UNIFORM(pipeline, name, value, size) unified_rendering_set_shader_uniform(pipeline, name, value, size)

// Debug macros
#if RENDER_DEBUG_ENABLED
#define RENDER_DEBUG_MARKER(pipeline, name) \
    unified_rendering_debug_begin_marker(pipeline, name); \
    unified_rendering_debug_end_marker(pipeline)

#define RENDER_PROFILE_SCOPE(pipeline, name) \
    unified_rendering_begin_profile(pipeline, name); \
    /* code */ \
    unified_rendering_end_profile(pipeline, name)
#else
#define RENDER_DEBUG_MARKER(pipeline, name) ((void)0)
#define RENDER_PROFILE_SCOPE(pipeline, name) ((void)0)
#endif

/* ============================================================================
 * CONSTANTS AND LIMITS
 * ============================================================================ */

#define UNIFIED_RENDERING_MAX_RENDER_TARGETS 16
#define UNIFIED_RENDERING_MAX_SHADER_PROGRAMS 1024
#define UNIFIED_RENDERING_MAX_FRAMEBUFFERS 64
#define UNIFIED_RENDERING_MAX_VERTEX_BUFFERS 1024
#define UNIFIED_RENDERING_MAX_INDEX_BUFFERS 512
#define UNIFIED_RENDERING_MAX_UNIFORM_BUFFERS 256
#define UNIFIED_RENDERING_MAX_TEXTURE_UNITS 32

#define UNIFIED_RENDERING_MIN_TEXTURE_SIZE 1
#define UNIFIED_RENDERING_MAX_TEXTURE_SIZE 16384
#define UNIFIED_RENDERING_MIN_RENDER_TARGET_SIZE 1
#define UNIFIED_RENDERING_MAX_RENDER_TARGET_SIZE 16384

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */

typedef enum {
    RENDER_ERROR_NONE = 0,
    RENDER_ERROR_INVALID_PARAMETER,
    RENDER_ERROR_OUT_OF_MEMORY,
    RENDER_ERROR_BACKEND_NOT_SUPPORTED,
    RENDER_ERROR_SHADER_COMPILATION_FAILED,
    RENDER_ERROR_SHADER_LINK_FAILED,
    RENDER_ERROR_TEXTURE_LOAD_FAILED,
    RENDER_ERROR_FRAMEBUFFER_INCOMPLETE,
    RENDER_ERROR_INVALID_OPERATION,
    RENDER_ERROR_CONTEXT_LOST,
    RENDER_ERROR_COUNT
} render_error_t;

const char* render_error_string(render_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_RENDERING_PIPELINE_H */
