// include/render/gpu_shader_reload.h
//
// Purpose: Dynamic shader hot-reload system for rapid iteration during
// development. Monitors shader files for changes and recompiles/rebinds them
// without full application restart.
//
#ifndef GPU_SHADER_RELOAD_H
#define GPU_SHADER_RELOAD_H

#include <common.h>

#include <time.h>
#include <include/rendering/vulkan.h>

// Forward declarations
typedef struct GPUShaderReloadManager GPUShaderReloadManager;
typedef struct ShaderFileWatcher ShaderFileWatcher;

// Shader modification callback
typedef void (*ShaderReloadCallback)(u32 pipeline_id, void *user_data);

// Shader file info
typedef struct {
  u32 watcher_id;
  char file_path[512];
  char shader_name[128];
  time_t last_modified;
  u32 file_hash;
  bool is_watched;
} ShaderFileInfo;

// Shader pipeline info for hot-reload
typedef struct {
  u32 pipeline_id;
  char vertex_path[512];
  char fragment_path[512];
  char geometry_path[512];
  char compute_path[512];

  // Compiled binaries
  u8 *vertex_binary;
  u32 vertex_binary_size;
  u8 *fragment_binary;
  u32 fragment_binary_size;
  u8 *geometry_binary;
  u32 geometry_binary_size;
  u8 *compute_binary;
  u32 compute_binary_size;

  // Vulkan objects
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
  VkShaderModule vertex_module;
  VkShaderModule fragment_module;
  VkShaderModule geometry_module;
  VkShaderModule compute_module;

  // Reload tracking
  u32 reload_count;
  time_t last_reload_time;
  bool pending_reload;

  // Callbacks
  ShaderReloadCallback on_reload;
  void *user_data;

  bool initialized;
} ShaderPipelineInfo;

// GPU shader hot-reload manager
struct GPUShaderReloadManager {
  // Vulkan objects
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkCommandPool transfer_pool;

  // Pipelines tracked for hot-reload
  ShaderPipelineInfo pipelines[64];
  u32 pipeline_count;

  // File watchers
  ShaderFileInfo file_watchers[128];
  u32 watcher_count;

  // Configuration
  bool enabled;
  bool verbose;
  u32 check_interval_ms; // How often to check for file changes
  time_t last_check_time;

  // Statistics
  u32 total_reloads;
  u32 failed_reloads;
  u32 successful_reloads;

  bool initialized;
};

// ==============================================================================
// Lifecycle Management
// ==============================================================================

/**
 * Initialize shader hot-reload system.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param device Vulkan logical device
 * @param physical_device Vulkan physical device
 * @param transfer_pool Command pool for transfers
 * @param enable_on_init Enable hot-reload immediately
 * @return true on success, false on failure
 */
bool gpu_shader_reload_init(GPUShaderReloadManager *manager, VkDevice device,
                            VkPhysicalDevice physical_device,
                            VkCommandPool transfer_pool, bool enable_on_init);

/**
 * Shutdown shader hot-reload system.
 *
 * @param manager Pointer to GPUShaderReloadManager
 */
void gpu_shader_reload_shutdown(GPUShaderReloadManager *manager);

/**
 * Enable/disable shader hot-reload.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param enabled Enable flag
 */
void gpu_shader_reload_set_enabled(GPUShaderReloadManager *manager,
                                   bool enabled);

/**
 * Check if shader hot-reload is enabled.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return true if enabled, false otherwise
 */
bool gpu_shader_reload_is_enabled(GPUShaderReloadManager *manager);

// ==============================================================================
// Pipeline Registration
// ==============================================================================

/**
 * Register shader pipeline for hot-reload watching.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Unique pipeline identifier
 * @param vertex_path Path to vertex shader file
 * @param fragment_path Path to fragment shader file
 * @param geometry_path Path to geometry shader file (optional, NULL if unused)
 * @param compute_path Path to compute shader file (optional, NULL if unused)
 * @return true on success, false on failure
 */
bool gpu_shader_reload_register_pipeline(GPUShaderReloadManager *manager,
                                         u32 pipeline_id,
                                         const char *vertex_path,
                                         const char *fragment_path,
                                         const char *geometry_path,
                                         const char *compute_path);

/**
 * Unregister shader pipeline from hot-reload watching.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 */
void gpu_shader_reload_unregister_pipeline(GPUShaderReloadManager *manager,
                                           u32 pipeline_id);

/**
 * Get shader pipeline info.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 * @return Pointer to ShaderPipelineInfo or NULL
 */
ShaderPipelineInfo *
gpu_shader_reload_get_pipeline(GPUShaderReloadManager *manager,
                               u32 pipeline_id);

// ==============================================================================
// File Watching
// ==============================================================================

/**
 * Watch shader file for changes.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param file_path Path to shader file
 * @return Watcher ID on success, 0xFFFFFFFF on failure
 */
u32 gpu_shader_reload_watch_file(GPUShaderReloadManager *manager,
                                 const char *file_path);

/**
 * Stop watching shader file.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param watcher_id Watcher identifier
 */
void gpu_shader_reload_unwatch_file(GPUShaderReloadManager *manager,
                                    u32 watcher_id);

/**
 * Check all watched files for modifications.
 * Call this regularly (e.g., once per frame) to detect changes.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Number of files with detected changes
 */
u32 gpu_shader_reload_check_for_changes(GPUShaderReloadManager *manager);

// ==============================================================================
// Shader Compilation
// ==============================================================================

/**
 * Compile shader from source file.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param source_path Path to shader source file
 * @param output_binary Pointer to receive compiled binary
 * @param output_size Pointer to receive binary size
 * @return true on success, false on failure
 */
bool gpu_shader_reload_compile_shader(GPUShaderReloadManager *manager,
                                      const char *source_path,
                                      u8 **output_binary, u32 *output_size);

/**
 * Create shader module from compiled binary.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param binary Compiled shader binary
 * @param binary_size Size of binary in bytes
 * @param out_module Pointer to receive VkShaderModule
 * @return true on success, false on failure
 */
bool gpu_shader_reload_create_module(GPUShaderReloadManager *manager,
                                     u8 *binary, u32 binary_size,
                                     VkShaderModule *out_module);

// ==============================================================================
// Pipeline Reloading
// ==============================================================================

/**
 * Reload specific shader pipeline.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 * @return true on success, false on failure
 */
bool gpu_shader_reload_reload_pipeline(GPUShaderReloadManager *manager,
                                       u32 pipeline_id);

/**
 * Reload all pipelines.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Number of successfully reloaded pipelines
 */
u32 gpu_shader_reload_reload_all(GPUShaderReloadManager *manager);

/**
 * Register callback for pipeline reload event.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 * @param callback Callback function to invoke on reload
 * @param user_data User data passed to callback
 */
void gpu_shader_reload_set_callback(GPUShaderReloadManager *manager,
                                    u32 pipeline_id,
                                    ShaderReloadCallback callback,
                                    void *user_data);

// ==============================================================================
// Update Loop
// ==============================================================================

/**
 * Update shader hot-reload system.
 * Call this regularly (typically once per frame) to detect file changes
 * and automatically reload modified shaders.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Number of pipelines reloaded this frame
 */
u32 gpu_shader_reload_update(GPUShaderReloadManager *manager);

/**
 * Process pending shader reloads.
 * Executes any shaders marked for reload due to file changes.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Number of pipelines reloaded
 */
u32 gpu_shader_reload_process_pending(GPUShaderReloadManager *manager);

// ==============================================================================
// Query Functions
// ==============================================================================

/**
 * Get number of registered pipelines.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Pipeline count
 */
u32 gpu_shader_reload_get_pipeline_count(GPUShaderReloadManager *manager);

/**
 * Get number of watched files.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return Watcher count
 */
u32 gpu_shader_reload_get_watcher_count(GPUShaderReloadManager *manager);

/**
 * Get reload statistics.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param out_total Pointer to receive total reload count
 * @param out_successful Pointer to receive successful reload count
 * @param out_failed Pointer to receive failed reload count
 */
void gpu_shader_reload_get_statistics(GPUShaderReloadManager *manager,
                                      u32 *out_total, u32 *out_successful,
                                      u32 *out_failed);

/**
 * Get last reload time for pipeline.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 * @return Unix timestamp of last reload (0 if never reloaded)
 */
time_t gpu_shader_reload_get_last_reload_time(GPUShaderReloadManager *manager,
                                              u32 pipeline_id);

/**
 * Get reload count for pipeline.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @param pipeline_id Pipeline identifier
 * @return Number of times this pipeline has been reloaded
 */
u32 gpu_shader_reload_get_reload_count(GPUShaderReloadManager *manager,
                                       u32 pipeline_id);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

/**
 * Log shader hot-reload system information.
 *
 * @param manager Pointer to GPUShaderReloadManager
 */
void gpu_shader_reload_log_info(GPUShaderReloadManager *manager);

/**
 * Log all registered pipelines.
 *
 * @param manager Pointer to GPUShaderReloadManager
 */
void gpu_shader_reload_log_pipelines(GPUShaderReloadManager *manager);

/**
 * Log all watched files.
 *
 * @param manager Pointer to GPUShaderReloadManager
 */
void gpu_shader_reload_log_watchers(GPUShaderReloadManager *manager);

/**
 * Log reload statistics.
 *
 * @param manager Pointer to GPUShaderReloadManager
 */
void gpu_shader_reload_log_statistics(GPUShaderReloadManager *manager);

/**
 * Validate shader hot-reload system state.
 *
 * @param manager Pointer to GPUShaderReloadManager
 * @return true if valid, false otherwise
 */
bool gpu_shader_reload_validate(GPUShaderReloadManager *manager);

#endif // GPU_SHADER_RELOAD_H
