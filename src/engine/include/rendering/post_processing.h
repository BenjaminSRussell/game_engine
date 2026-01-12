// Post-Processing System Header
// POST-PROCESS-001: Post-processing pipeline architecture
// POST-PROCESS-002: Tone mapping and exposure
// POST-PROCESS-003: Color grading and LUTs
// POST-PROCESS-004: Bloom and glow effects
// POST-PROCESS-005: Depth of field
// POST-PROCESS-006: Motion blur
// POST-PROCESS-007: Screen space reflections
// POST-PROCESS-008: Ambient occlusion
// POST-PROCESS-009: Anti-aliasing (FXAA, TAA)
// POST-PROCESS-010: Performance optimization

#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include "math/math.h"
#include "include/rendering/vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Post-processing effects
typedef enum {
  POST_EFFECT_TONE_MAPPING = 0x0001,
  POST_EFFECT_COLOR_GRADING = 0x0002,
  POST_EFFECT_BLOOM = 0x0004,
  POST_EFFECT_DEPTH_OF_FIELD = 0x0008,
  POST_EFFECT_MOTION_BLUR = 0x0010,
  POST_EFFECT_SSR = 0x0020,
  POST_EFFECT_SSAO = 0x0040,
  POST_EFFECT_FXAA = 0x0080,
  POST_EFFECT_TAA = 0x0100,
  POST_EFFECT_VIGNETTE = 0x0200,
  POST_EFFECT_CHROMATIC_ABERRATION = 0x0400,
  POST_EFFECT_FILM_GRAIN = 0x0800,
  POST_EFFECT_ALL = 0xFFFF
} PostEffectFlags;

// Tone mapping operators
typedef enum {
  TONE_MAP_LINEAR = 0,
  TONE_MAP_REINHARD,
  TONE_MAP_ACES,
  TONE_MAP_UNCHARTED2,
  TONE_MAP_AGX,
  TONE_MAP_COUNT
} ToneMapOperator;

// Anti-aliasing methods
typedef enum {
  AA_NONE = 0,
  AA_FXAA,
  AA_TAA,
  AA_SMAA,
  AA_COUNT
} AntiAliasingMethod;

// Color grading modes
typedef enum {
  COLOR_GRADE_NONE = 0,
  COLOR_GRADE_LUT,
  COLOR_GRADE_COLOR_CORRECTION,
  COLOR_GRADE_COUNT
} ColorGradeMode;

// Post-processing configuration
typedef struct {
  // Effect enable flags
  u32 enabledEffects;

  // Tone mapping
  ToneMapOperator toneMapOperator;
  float exposure;
  float gamma;
  float contrast;
  float brightness;

  // Color grading
  ColorGradeMode colorGradeMode;
  float saturation;
  float hueShift;
  Vec3 colorFilter;
  Vec3 lift;
  Vec3 gammaCorrection;
  Vec3 gain;

  // Bloom
  bool bloomEnabled;
  float bloomThreshold;
  float bloomIntensity;
  float bloomKnee;
  u32 bloomIterations;

  // Depth of field
  bool dofEnabled;
  float dofFocusDistance;
  float dofAperture;
  float dofMaxBlur;

  // Motion blur
  bool motionBlurEnabled;
  float motionBlurStrength;
  u32 motionBlurSamples;

  // SSR
  bool ssrEnabled;
  float ssrMaxDistance;
  float ssrFadeDistance;
  u32 ssrMaxSteps;

  // SSAO
  bool ssaoEnabled;
  float ssaoRadius;
  float ssaoBias;
  float ssaoPower;
  u32 ssaoSamples;

  // Anti-aliasing
  AntiAliasingMethod aaMethod;
  float fxaaEdgeThreshold;
  float fxaaEdgeThresholdMin;

  // Screen effects
  bool vignetteEnabled;
  float vignetteStrength;
  Vec3 vignetteColor;

  bool chromaticAberrationEnabled;
  float chromaticAberrationStrength;

  bool filmGrainEnabled;
  float filmGrainStrength;
  float filmGrainSize;

  // Performance
  bool enableAsyncCompute;
  u32 renderScale;
} PostProcessingConfig;

// Post-processing statistics
typedef struct {
  u64 totalFramesProcessed;
  double averageFrameTime;
  double toneMappingTime;
  double bloomTime;
  double dofTime;
  double motionBlurTime;
  double ssrTime;
  double ssaoTime;
  double aaTime;
  u64 memoryUsage;
  float averageRenderScale;
} PostProcessingStats;

// Post-processing pipeline
typedef struct PostProcessingPipeline {
#ifdef VULKAN_BUILD
  // Input/output images
  VkImage inputImage;
  VkImageView inputImageView;
  VkDeviceMemory inputMemory;

  VkImage outputImage;
  VkImageView outputImageView;
  VkDeviceMemory outputMemory;

  // Intermediate render targets
  VkImage intermediateImages[8];
  VkImageView intermediateImageViews[8];
  VkDeviceMemory intermediateMemories[8];

  // Bloom specific
  VkImage bloomImages[4];
  VkImageView bloomImageViews[4];
  VkDeviceMemory bloomMemories[4];

  // SSAO specific
  VkImage ssaoImage;
  VkImageView ssaoImageView;
  VkDeviceMemory ssaoMemory;
  VkImage ssaoBlurImage;
  VkImageView ssaoBlurImageView;
  VkDeviceMemory ssaoBlurMemory;

  // TAA specific
  VkImage historyImages[2];
  VkImageView historyImageViews[2];
  VkDeviceMemory historyMemories[2];

  // LUT texture
  VkImage lutImage;
  VkImageView lutImageView;
  VkSampler lutSampler;
  VkDeviceMemory lutMemory;

  // Compute pipelines
  VkPipeline toneMapPipeline;
  VkPipeline bloomPipeline;
  VkPipeline dofPipeline;
  VkPipeline motionBlurPipeline;
  VkPipeline ssrPipeline;
  VkPipeline ssaoPipeline;
  VkPipeline aaPipeline;

  // Graphics pipelines
  VkPipeline finalPipeline;

  // Pipeline layouts and descriptor sets
  VkPipelineLayout pipelineLayouts[8];
  VkDescriptorSetLayout descriptorLayouts[8];
  VkDescriptorSet descriptorSets[8];

  // Samplers
  VkSampler samplers[4];
#endif
  bool initialized;
  PostProcessingConfig config;
  PostProcessingStats stats;
  u32 currentFrame;
} PostProcessingPipeline;

// Core post-processing functions
bool post_process_init(PostProcessingPipeline *pipeline,
                       VulkanRenderer *renderer,
                       const PostProcessingConfig *config);
void post_process_shutdown(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer);
bool post_process_is_initialized(const PostProcessingPipeline *pipeline);

// Main processing function
bool post_process_frame(PostProcessingPipeline *pipeline,
                        VulkanRenderer *renderer, VkCommandBuffer commandBuffer,
                        VkImage inputImage, VkImage outputImage, u32 width,
                        u32 height);

// Effect-specific functions
bool post_process_tone_map(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer,
                           VkCommandBuffer commandBuffer);
bool post_process_bloom(PostProcessingPipeline *pipeline,
                        VulkanRenderer *renderer,
                        VkCommandBuffer commandBuffer);
bool post_process_depth_of_field(PostProcessingPipeline *pipeline,
                                 VulkanRenderer *renderer,
                                 VkCommandBuffer commandBuffer);
bool post_process_motion_blur(PostProcessingPipeline *pipeline,
                              VulkanRenderer *renderer,
                              VkCommandBuffer commandBuffer);
bool post_process_ssr(PostProcessingPipeline *pipeline,
                      VulkanRenderer *renderer, VkCommandBuffer commandBuffer);
bool post_process_ssao(PostProcessingPipeline *pipeline,
                       VulkanRenderer *renderer, VkCommandBuffer commandBuffer);
bool post_process_anti_aliasing(PostProcessingPipeline *pipeline,
                                VulkanRenderer *renderer,
                                VkCommandBuffer commandBuffer);

// Configuration management
void post_process_set_config(PostProcessingPipeline *pipeline,
                             const PostProcessingConfig *config);
void post_process_get_config(const PostProcessingPipeline *pipeline,
                             PostProcessingConfig *outConfig);
void post_process_enable_effects(PostProcessingPipeline *pipeline,
                                 u32 effectFlags);
void post_process_disable_effects(PostProcessingPipeline *pipeline,
                                  u32 effectFlags);
bool post_process_is_effect_enabled(const PostProcessingPipeline *pipeline,
                                    u32 effectFlag);

// LUT management
bool post_process_load_lut(PostProcessingPipeline *pipeline,
                           VulkanRenderer *renderer, const char *lutPath);
bool post_process_generate_lut(PostProcessingPipeline *pipeline,
                               VulkanRenderer *renderer, u32 size);

// Performance optimization
void post_process_set_render_scale(PostProcessingPipeline *pipeline,
                                   u32 renderScale);
void post_process_enable_async_compute(PostProcessingPipeline *pipeline,
                                       bool enable);
void post_process_optimize_for_gpu(PostProcessingPipeline *pipeline);

// Statistics and debugging
void post_process_get_stats(const PostProcessingPipeline *pipeline,
                            PostProcessingStats *outStats);
void post_process_reset_stats(PostProcessingPipeline *pipeline);
void post_process_debug_print_stats(const PostProcessingPipeline *pipeline);
bool post_process_validate_pipeline(const PostProcessingPipeline *pipeline);

// Utility functions
u64 post_process_estimate_memory_usage(const PostProcessingConfig *config,
                                       u32 width, u32 height);
bool post_process_check_gpu_support(VulkanRenderer *renderer);
const char *post_process_get_effect_name(u32 effectFlag);
const char *post_process_get_tone_map_operator_name(ToneMapOperator op);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESSING_H
