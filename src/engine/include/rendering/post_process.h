// rendering/post_process.h
// Complete post-processing pipeline
#ifndef POST_PROCESS_H
#define POST_PROCESS_H

#include <common.h>
#import <Metal/Metal.h>

// Post-process effects
typedef struct {
  // Anti-aliasing
  bool enable_taa;
  f32 taa_sharpness;
  u32 taa_sample_count;

  // Screen space reflections
  bool enable_ssr;
  f32 ssr_intensity;
  u32 ssr_max_steps;
  f32 ssr_step_size;
  f32 ssr_thickness;

  // Ambient occlusion
  bool enable_ao;
  f32 ao_radius;
  f32 ao_intensity;
  u32 ao_sample_count;
  bool use_gtao; // Ground truth AO vs SSAO

  // Bloom
  bool enable_bloom;
  f32 bloom_threshold;
  f32 bloom_intensity;
  f32 bloom_scatter;
  u32 bloom_iterations;

  // Depth of Field
  bool enable_dof;
  f32 dof_focus_distance;
  f32 dof_aperture;
  f32 dof_focal_length;
  f32 dof_bokeh_size;
  u32 dof_sample_count;

  // Motion Blur
  bool enable_motion_blur;
  f32 motion_blur_intensity;
  u32 motion_blur_samples;

  // Color Grading
  bool enable_color_grading;
  id<MTLTexture> lut_texture; // 3D LUT
  f32 lut_intensity;
  f32 exposure;
  f32 contrast;
  f32 saturation;
  Vec3 color_filter;

  // Tone mapping
  bool enable_tone_mapping;
  f32 tone_map_white_point;

  // Lens effects
  bool enable_chromatic_aberration;
  f32 chromatic_aberration_intensity;
  bool enable_lens_distortion;
  f32 lens_distortion_intensity;
  bool enable_vignette;
  f32 vignette_intensity;
  f32 vignette_smoothness;

  // Film grain
  bool enable_film_grain;
  f32 film_grain_intensity;

} PostProcessSettings;

typedef struct {
  id<MTLDevice> device;

  // Render targets
  id<MTLTexture> scene_color;
  id<MTLTexture> scene_depth;
  id<MTLTexture> velocity_buffer;
  id<MTLTexture> temp_buffer[4]; // Ping-pong buffers

  // TAA history
  id<MTLTexture> taa_history;
  u32 frame_index;

  // Pipelines
  id<MTLComputePipelineState> taa_pipeline;
  id<MTLComputePipelineState> ssr_pipeline;
  id<MTLComputePipelineState> ao_pipeline;
  id<MTLComputePipelineState> bloom_downsample;
  id<MTLComputePipelineState> bloom_upsample;
  id<MTLComputePipelineState> dof_pipeline;
  id<MTLComputePipelineState> motion_blur_pipeline;
  id<MTLComputePipelineState> tonemap_pipeline;

  PostProcessSettings settings;

} PostProcessSystem;

#ifdef __cplusplus
extern "C" {
#endif

// System lifecycle
PostProcessSystem *post_process_create(id<MTLDevice> device, u32 width,
                                       u32 height);
void post_process_destroy(PostProcessSystem *system);
void post_process_resize(PostProcessSystem *system, u32 width, u32 height);

// Settings
void post_process_set_settings(PostProcessSystem *system,
                               const PostProcessSettings *settings);

// Execution
void post_process_execute(PostProcessSystem *system, id<MTLCommandBuffer> cmd,
                          id<MTLTexture> input_color,
                          id<MTLTexture> input_depth, id<MTLTexture> velocity,
                          id<MTLTexture> output);

// Individual effects
void post_process_taa(PostProcessSystem *system,
                      id<MTLComputeCommandEncoder> compute,
                      id<MTLTexture> current, id<MTLTexture> output);
void post_process_ssr(PostProcessSystem *system,
                      id<MTLComputeCommandEncoder> compute,
                      id<MTLTexture> scene, id<MTLTexture> depth,
                      id<MTLTexture> normal, id<MTLTexture> output);
void post_process_ao(PostProcessSystem *system,
                     id<MTLComputeCommandEncoder> compute, id<MTLTexture> depth,
                     id<MTLTexture> normal, id<MTLTexture> output);
void post_process_bloom(PostProcessSystem *system,
                        id<MTLComputeCommandEncoder> compute,
                        id<MTLTexture> input, id<MTLTexture> output);
void post_process_dof(PostProcessSystem *system,
                      id<MTLComputeCommandEncoder> compute,
                      id<MTLTexture> color, id<MTLTexture> depth,
                      id<MTLTexture> output);
void post_process_motion_blur(PostProcessSystem *system,
                              id<MTLComputeCommandEncoder> compute,
                              id<MTLTexture> color, id<MTLTexture> velocity,
                              id<MTLTexture> output);
void post_process_color_grade(PostProcessSystem *system,
                              id<MTLComputeCommandEncoder> compute,
                              id<MTLTexture> input, id<MTLTexture> output);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESS_H
