// include/render/gpu_postprocess.h
//
// Purpose: GPU post-processing pipeline for tone mapping, bloom, and color
// grading. Manages framebuffers, render passes, and post-processing effects.
//
#ifndef GPU_POSTPROCESS_H
#define GPU_POSTPROCESS_H

#include "include/rendering/hdr_tonemap.h"
#include <common.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <include/rendering/vulkan.h>

// Forward declarations
typedef struct GPUPostProcessManager GPUPostProcessManager;

// Post-processing effect flags
typedef enum {
  POSTPROCESS_TONE_MAPPING = 1 << 0,
  POSTPROCESS_BLOOM = 1 << 1,
  POSTPROCESS_COLOR_GRADING = 1 << 2,
  POSTPROCESS_VIGNETTE = 1 << 3,
  POSTPROCESS_LENS_FLARE = 1 << 4,
  POSTPROCESS_CHROMATIC_ABBR = 1 << 5,
} PostProcessEffectFlags;

// Quality levels for post-processing effects
typedef enum {
  POSTPROCESS_QUALITY_LOW = 0,
  POSTPROCESS_QUALITY_MEDIUM = 1,
  POSTPROCESS_QUALITY_HIGH = 2,
  POSTPROCESS_QUALITY_ULTRA = 3,
  POSTPROCESS_QUALITY_CUSTOM = 4
} PostProcessQuality;

// Quality configuration for different effects
typedef struct {
  PostProcessQuality quality_level;

  // Bloom quality settings
  struct {
    u32 blur_iterations_low;      // Low quality blur passes
    u32 blur_iterations_medium;   // Medium quality blur passes
    u32 blur_iterations_high;     // High quality blur passes
    u32 blur_iterations_ultra;    // Ultra quality blur passes
    f32 downsample_factor_low;    // Downsample factor for low quality
    f32 downsample_factor_medium; // Downsample factor for medium quality
    f32 downsample_factor_high;   // Downsample factor for high quality
    f32 downsample_factor_ultra;  // Downsample factor for ultra quality
  } bloom_quality;

  // Tone mapping quality settings
  struct {
    bool use_aces_tonemap_low;     // Use ACES tonemapping at low quality
    bool use_aces_tonemap_medium;  // Use ACES tonemapping at medium quality
    bool use_aces_tonemap_high;    // Use ACES tonemapping at high quality
    bool use_aces_tonemap_ultra;   // Use ACES tonemapping at ultra quality
    bool use_hdr_precision_low;    // Use HDR precision at low quality
    bool use_hdr_precision_medium; // Use HDR precision at medium quality
    bool use_hdr_precision_high;   // Use HDR precision at high quality
    bool use_hdr_precision_ultra;  // Use HDR precision at ultra quality
  } tonemap_quality;

  // General quality settings
  struct {
    bool enable_chromatic_aberration_low; // Enable chromatic aberration at low
                                          // quality
    bool enable_chromatic_aberration_medium; // Enable chromatic aberration at
                                             // medium quality
    bool enable_chromatic_aberration_high;   // Enable chromatic aberration at
                                             // high quality
    bool enable_chromatic_aberration_ultra;  // Enable chromatic aberration at
                                             // ultra quality
    bool enable_lens_flare_low;              // Enable lens flare at low quality
    bool enable_lens_flare_medium; // Enable lens flare at medium quality
    bool enable_lens_flare_high;   // Enable lens flare at high quality
    bool enable_lens_flare_ultra;  // Enable lens flare at ultra quality
    bool enable_vignette_low;      // Enable vignette at low quality
    bool enable_vignette_medium;   // Enable vignette at medium quality
    bool enable_vignette_high;     // Enable vignette at high quality
    bool enable_vignette_ultra;    // Enable vignette at ultra quality
  } effect_quality;

  // Performance settings
  struct {
    bool use_async_compute_low;     // Use async compute at low quality
    bool use_async_compute_medium;  // Use async compute at medium quality
    bool use_async_compute_high;    // Use async compute at high quality
    bool use_async_compute_ultra;   // Use async compute at ultra quality
    bool use_half_precision_low;    // Use half precision at low quality
    bool use_half_precision_medium; // Use half precision at medium quality
    bool use_half_precision_high;   // Use half precision at high quality
    bool use_half_precision_ultra;  // Use half precision at ultra quality
  } performance_quality;
} PostProcessQualityConfig;

// Bloom settings
typedef struct {
  f32 threshold;     // Luminance threshold for bloom
  f32 intensity;     // Bloom intensity multiplier
  u32 iterations;    // Number of blur passes
  f32 blur_strength; // Gaussian blur strength
  bool enabled;
} BloomSettings;

// Vignette settings
typedef struct {
  Vec4 color;    // Vignette color
  f32 intensity; // Vignette intensity (0.0-1.0)
  f32 softness;  // Edge softness
  bool enabled;
} VignetteSettings;

// Lens flare settings
typedef struct {
  Vec4 color;       // Flare color
  f32 intensity;    // Flare intensity
  Vec2 ghost_count; // Number of ghosts
  bool enabled;
} LensFlareSettings;

// Post-process framebuffer
typedef struct {
  u32 fb_id;
  VkFramebuffer framebuffer;
  VkImage color_image;
  VkImageView color_view;
  VkImage depth_image;
  VkImageView depth_view;
  VkDeviceMemory color_memory;
  VkDeviceMemory depth_memory;

  u32 width, height;
  VkFormat color_format;
  VkFormat depth_format;

  // Intermediate targets for post-processing
  VkImage bloom_image;
  VkImageView bloom_view;
  VkDeviceMemory bloom_memory;

  bool initialized;
} PostProcessFramebuffer;

// Effect chaining system (POST-008)
typedef enum {
  POSTPROCESS_CHAIN_TONEMAP = 0,
  POSTPROCESS_CHAIN_BLOOM_EXTRACT,
  POSTPROCESS_CHAIN_BLOOM_BLUR,
  POSTPROCESS_CHAIN_VIGNETTE,
  POSTPROCESS_CHAIN_LENS_FLARE,
  POSTPROCESS_CHAIN_CHROMATIC_ABBR,
  POSTPROCESS_CHAIN_COLOR_GRADE,
  POSTPROCESS_CHAIN_FINAL_COMPOSITE,
  POSTPROCESS_CHAIN_MAX_STAGES
} PostProcessChainStage;

typedef struct {
  PostProcessChainStage stage;
  PostProcessEffectFlags required_effects;
  bool enabled;
  u32 input_texture;  // Texture index to use as input
  u32 output_texture; // Texture index to write output
  f32 blend_factor;   // Blend factor for this stage
} PostProcessChainNode;

typedef struct {
  PostProcessChainNode nodes[POSTPROCESS_CHAIN_MAX_STAGES];
  u32 node_count;
  u32 current_chain;  // Current active chain index
  bool auto_optimize; // Automatically disable stages based on performance
} PostProcessEffectChain;

// Statistics tracking and performance metrics (POST-002)
typedef struct {
  f32 total_frame_time;     // Total time for post-processing
  f32 tone_map_time;        // Time for tone mapping
  f32 bloom_time;           // Time for bloom processing
  f32 vignette_time;        // Time for vignette
  f32 lens_flare_time;      // Time for lens flare
  f32 chromatic_aberr_time; // Time for chromatic aberration
  f32 color_grade_time;     // Time for color grading
  f32 composite_time;       // Time for final composite
  u32 frame_count;          // Number of frames processed
  f32 average_frame_time;   // Average frame time
  f32 min_frame_time;       // Minimum frame time
  f32 max_frame_time;       // Maximum frame time
  u64 total_memory_usage;   // Total GPU memory used
  u32 texture_switches;     // Number of texture switches
  u32 pipeline_switches;    // Number of pipeline switches
  bool enable_profiling;    // Enable/disable profiling
} PostProcessStats;

// Push constants
typedef struct {
  Vec4 tonemap_params; // Exposure, contrast, saturation, gamma
  Vec4 grading_params; // Color grading adjustments
  f32 bloom_intensity;
  f32 vignette_intensity;
  f32 time;
  u32 effect_flags;
} PostProcessPushConstant;

// GPU post-processing manager
struct GPUPostProcessManager {
  // Vulkan objects
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkCommandPool transfer_pool;
  VkQueue graphics_queue;

  // Framebuffers
  PostProcessFramebuffer framebuffers[8];
  u32 framebuffer_count;

  // Render passes
  VkRenderPass scene_render_pass;  // Main scene rendering
  VkRenderPass bloom_extract_pass; // Bloom extraction
  VkRenderPass tonemap_pass;       // Tone mapping post-process
  VkRenderPass composite_pass;     // Final composition

  // Pipelines
  VkPipeline bloom_extract_pipeline;
  VkPipeline bloom_blur_pipeline;
  VkPipeline tonemap_pipeline;
  VkPipeline composite_pipeline;
  VkPipeline vignette_pipeline;
  VkPipeline lens_flare_pipeline;

  // Pipeline layouts
  VkPipelineLayout tonemap_layout;
  VkPipelineLayout composite_layout;

  // Descriptor sets
  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout scene_layout;
  VkDescriptorSetLayout tonemap_layout_set;
  VkDescriptorSet tonemap_descriptor;
  VkDescriptorSet composite_descriptor;

  // Samplers
  VkSampler linear_sampler;
  VkSampler nearest_sampler;

  // Settings
  BloomSettings bloom_settings;
  VignetteSettings vignette_settings;
  LensFlareSettings lens_flare_settings;

  // Tone mapping settings
  TonemapConfig tonemap_settings;
  TonemapAlgorithm tonemap_algorithm;

  // Quality configuration (POST-001)
  PostProcessQualityConfig quality_config;

  // Effect chaining system (POST-008)
  PostProcessEffectChain effect_chains[16];
  u32 chain_count;
  u32 active_chain_index;
  char chain_names[16][64];

  // Statistics tracking (POST-002)
  PostProcessStats stats;
  f64 profiling_start_time;

  // Effect flags
  PostProcessEffectFlags active_effects;

  // Push constants
  PostProcessPushConstant push_constants; // Use the struct type defined above

  // Logging
  bool verbose;
  bool initialized;
};

// ==============================================================================
// Lifecycle Management
// ==============================================================================

/**
 * Initialize GPU post-processing system.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param device Vulkan logical device
 * @param physical_device Vulkan physical device
 * @param graphics_queue Graphics queue for command execution
 * @param transfer_pool Command pool for transfers
 * @param width Framebuffer width
 * @param height Framebuffer height
 * @return true on success, false on failure
 */
bool gpu_postprocess_init(GPUPostProcessManager *manager, VkDevice device,
                          VkPhysicalDevice physical_device,
                          VkQueue graphics_queue, VkCommandPool transfer_pool,
                          u32 width, u32 height);

/**
 * Shutdown GPU post-processing system.
 *
 * @param manager Pointer to GPUPostProcessManager
 */
void gpu_postprocess_shutdown(GPUPostProcessManager *manager);

// ==============================================================================
// Framebuffer Management
// ==============================================================================

/**
 * Create post-process framebuffer with color and depth attachments.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param fb_id Unique framebuffer identifier
 * @param width Framebuffer width
 * @param height Framebuffer height
 * @param color_format VkFormat for color attachment
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id, u32 width, u32 height,
                                        VkFormat color_format);

/**
 * Delete post-process framebuffer and free resources.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param fb_id Framebuffer identifier
 */
void gpu_postprocess_delete_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id);

/**
 * Get framebuffer by ID.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param fb_id Framebuffer identifier
 * @return Pointer to PostProcessFramebuffer or NULL
 */
PostProcessFramebuffer *
gpu_postprocess_get_framebuffer(GPUPostProcessManager *manager, u32 fb_id);

/**
 * Resize framebuffer and all attachments.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param fb_id Framebuffer identifier
 * @param new_width New width
 * @param new_height New height
 * @return true on success, false on failure
 */
bool gpu_postprocess_resize_framebuffer(GPUPostProcessManager *manager,
                                        u32 fb_id, u32 new_width,
                                        u32 new_height);

// ==============================================================================
// Render Pass Management
// ==============================================================================

/**
 * Create main scene render pass.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_scene_render_pass(GPUPostProcessManager *manager);

/**
 * Create tone mapping render pass.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_tonemap_pass(GPUPostProcessManager *manager);

/**
 * Get scene render pass.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return VkRenderPass for scene rendering
 */
VkRenderPass
gpu_postprocess_get_scene_render_pass(GPUPostProcessManager *manager);

/**
 * Get tone mapping render pass.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return VkRenderPass for tone mapping
 */
VkRenderPass gpu_postprocess_get_tonemap_pass(GPUPostProcessManager *manager);

// ==============================================================================
// Pipeline Management
// ==============================================================================

/**
 * Create tone mapping post-processing pipeline.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_tonemap_pipeline(GPUPostProcessManager *manager);

/**
 * Create bloom extraction pipeline.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_bloom_pipeline(GPUPostProcessManager *manager);

/**
 * Create composite pipeline for final rendering.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_create_composite_pipeline(GPUPostProcessManager *manager);

// ==============================================================================
// Tone Mapping Configuration
// ==============================================================================

/**
 * Set tone mapping algorithm.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param algorithm ToneMappingAlgorithm enum value
 */
void gpu_postprocess_set_tone_mapping(GPUPostProcessManager *manager,
                                      TonemapAlgorithm algorithm);

/**
 * Set tone mapping settings (exposure, contrast, saturation, gamma).
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param settings Pointer to ToneMappingSettings
 */
void gpu_postprocess_set_tonemap_settings(GPUPostProcessManager *manager,
                                          TonemapConfig *settings);

/**
 * Set exposure value.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param exposure Exposure in EV stops (-2.0 to 2.0)
 */
void gpu_postprocess_set_exposure(GPUPostProcessManager *manager, f32 exposure);

/**
 * Set contrast.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param contrast Contrast value (0.5 to 2.0)
 */
void gpu_postprocess_set_contrast(GPUPostProcessManager *manager, f32 contrast);

/**
 * Set saturation.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param saturation Saturation value (0.0 to 2.0)
 */
void gpu_postprocess_set_saturation(GPUPostProcessManager *manager,
                                    f32 saturation);

// ==============================================================================
// Quality Configuration (POST-001)
// ==============================================================================

/**
 * Initialize quality configuration with default settings.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_init_quality_config(GPUPostProcessManager *manager);

/**
 * Set post-processing quality level.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param quality PostProcessQuality enum value
 * @return true on success, false on failure
 */
bool gpu_postprocess_set_quality_level(GPUPostProcessManager *manager,
                                       PostProcessQuality quality);

/**
 * Get current quality level.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return Current PostProcessQuality
 */
PostProcessQuality
gpu_postprocess_get_quality_level(GPUPostProcessManager *manager);

/**
 * Apply quality settings to all post-processing effects.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true on success, false on failure
 */
bool gpu_postprocess_apply_quality_settings(GPUPostProcessManager *manager);

/**
 * Configure custom quality settings.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param config Pointer to PostProcessQualityConfig
 * @return true on success, false on failure
 */
bool gpu_postprocess_set_custom_quality_config(
    GPUPostProcessManager *manager, const PostProcessQualityConfig *config);

/**
 * Get bloom quality settings for current quality level.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param iterations Output for blur iterations
 * @param downsample_factor Output for downsample factor
 * @return true on success, false on failure
 */
bool gpu_postprocess_get_bloom_quality_settings(GPUPostProcessManager *manager,
                                                u32 *iterations,
                                                f32 *downsample_factor);

/**
 * Check if effect should be enabled at current quality level.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param effect PostProcessEffectFlags to check
 * @return true if effect should be enabled, false otherwise
 */
bool gpu_postprocess_should_enable_effect(GPUPostProcessManager *manager,
                                          PostProcessEffectFlags effect);

/**
 * Get performance optimizations for current quality level.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param use_async_compute Output for async compute preference
 * @param use_half_precision Output for half precision preference
 * @return true on success, false on failure
 */
bool gpu_postprocess_get_performance_settings(GPUPostProcessManager *manager,
                                              bool *use_async_compute,
                                              bool *use_half_precision);

/**
 * Auto-adjust quality based on performance metrics.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param frame_time_ms Current frame time in milliseconds
 * @param target_frame_time_ms Target frame time in milliseconds
 * @return true if quality was adjusted, false otherwise
 */
bool gpu_postprocess_auto_adjust_quality(GPUPostProcessManager *manager,
                                         f32 frame_time_ms,
                                         f32 target_frame_time_ms);

bool gpu_postprocess_create_effect_chain(GPUPostProcessManager *manager,
                                         const char *chain_name);
bool gpu_postprocess_add_chain_node(GPUPostProcessManager *manager,
                                    const char *chain_name,
                                    const PostProcessChainNode *node);
bool gpu_postprocess_remove_chain_node(GPUPostProcessManager *manager,
                                       const char *chain_name,
                                       PostProcessChainStage stage);
bool gpu_postprocess_set_active_chain(GPUPostProcessManager *manager,
                                      const char *chain_name);
bool gpu_postprocess_execute_chain(GPUPostProcessManager *manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image,
                                   PostProcessFramebuffer *output);
bool gpu_postprocess_save_chain(GPUPostProcessManager *manager,
                                const char *chain_name, const char *file_path);
bool gpu_postprocess_load_chain(GPUPostProcessManager *manager,
                                const char *chain_name, const char *file_path);

void gpu_postprocess_enable_profiling(GPUPostProcessManager *manager,
                                      bool enabled);
void gpu_postprocess_reset_stats(GPUPostProcessManager *manager);
void gpu_postprocess_get_stats(GPUPostProcessManager *manager,
                               PostProcessStats *stats);
void gpu_postprocess_print_stats(GPUPostProcessManager *manager);
void gpu_postprocess_begin_frame_profiling(GPUPostProcessManager *manager);
void gpu_postprocess_end_frame_profiling(GPUPostProcessManager *manager);

// ==============================================================================
// Effect Configuration
// ==============================================================================

/**
 * Enable/disable bloom effect.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param enabled Enable flag
 */
void gpu_postprocess_set_bloom_enabled(GPUPostProcessManager *manager,
                                       bool enabled);

/**
 * Configure bloom settings.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param threshold Bloom threshold (luminance)
 * @param intensity Bloom intensity multiplier
 * @param iterations Number of blur iterations
 */
void gpu_postprocess_configure_bloom(GPUPostProcessManager *manager,
                                     f32 threshold, f32 intensity,
                                     u32 iterations);

/**
 * Enable/disable vignette effect.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param enabled Enable flag
 */
void gpu_postprocess_set_vignette_enabled(GPUPostProcessManager *manager,
                                          bool enabled);

/**
 * Configure vignette settings.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param color Vignette color
 * @param intensity Vignette intensity (0.0-1.0)
 */
void gpu_postprocess_configure_vignette(GPUPostProcessManager *manager,
                                        Vec4 color, f32 intensity);

/**
 * Enable/disable lens flare effect.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param enabled Enable flag
 */
void gpu_postprocess_set_lens_flare_enabled(GPUPostProcessManager *manager,
                                            bool enabled);

/**
 * Configure lens flare settings.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param color Flare color
 * @param intensity Flare intensity
 */
void gpu_postprocess_configure_lens_flare(GPUPostProcessManager *manager,
                                          Vec4 color, f32 intensity);

// ==============================================================================
// Post-Processing Execution
// ==============================================================================

/**
 * Execute bloom extraction on HDR scene.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param cmd_buffer Vulkan command buffer
 * @param source_image HDR image to extract bloom from
 * @return true on success, false on failure
 */
bool gpu_postprocess_extract_bloom(GPUPostProcessManager *manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image);

/**
 * Execute tone mapping post-processing.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param cmd_buffer Vulkan command buffer
 * @param source_image HDR source image
 * @param target_framebuffer Output framebuffer
 * @return true on success, false on failure
 */
bool gpu_postprocess_apply_tonemap(GPUPostProcessManager *manager,
                                   VkCommandBuffer cmd_buffer,
                                   VkImageView source_image,
                                   PostProcessFramebuffer *target_framebuffer);

/**
 * Execute full post-processing chain.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param cmd_buffer Vulkan command buffer
 * @param hdr_image HDR scene image
 * @param output_framebuffer Final output framebuffer
 * @return true on success, false on failure
 */
bool gpu_postprocess_execute(GPUPostProcessManager *manager,
                             VkCommandBuffer cmd_buffer, VkImageView hdr_image,
                             PostProcessFramebuffer *output_framebuffer);

// ==============================================================================
// Query Functions
// ==============================================================================

/**
 * Get number of allocated framebuffers.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return Framebuffer count
 */
u32 gpu_postprocess_get_framebuffer_count(GPUPostProcessManager *manager);

/**
 * Get active effect flags.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return PostProcessEffectFlags bitmask
 */
PostProcessEffectFlags
gpu_postprocess_get_active_effects(GPUPostProcessManager *manager);

/**
 * Check if effect is enabled.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @param effect PostProcessEffectFlags to check
 * @return true if enabled, false otherwise
 */
bool gpu_postprocess_is_effect_enabled(GPUPostProcessManager *manager,
                                       PostProcessEffectFlags effect);

// ==============================================================================
// Debug and Diagnostics
// ==============================================================================

/**
 * Log post-processing system information.
 *
 * @param manager Pointer to GPUPostProcessManager
 */
void gpu_postprocess_log_info(GPUPostProcessManager *manager);

/**
 * Log post-processing statistics.
 *
 * @param manager Pointer to GPUPostProcessManager
 */
void gpu_postprocess_log_statistics(GPUPostProcessManager *manager);

/**
 * Validate post-processing system state.
 *
 * @param manager Pointer to GPUPostProcessManager
 * @return true if valid, false otherwise
 */
bool gpu_postprocess_validate(GPUPostProcessManager *manager);

#endif // GPU_POSTPROCESS_H
