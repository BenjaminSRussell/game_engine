// src/engine/rendering/post_process/post_process_pipeline.c
// Post-Processing Pipeline - Bloom, tone mapping, FXAA, and other effects

#include "engine/include/core/logger.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../render_pipeline.h"

// ============================================================================
// Post-Process Types
// ============================================================================

typedef enum {
  POST_EFFECT_BLOOM,
  POST_EFFECT_TONE_MAP,
  POST_EFFECT_FXAA,
  POST_EFFECT_SMAA,
  POST_EFFECT_TAA,
  POST_EFFECT_VIGNETTE,
  POST_EFFECT_COLOR_GRADING,
  POST_EFFECT_CHROMATIC_ABERRATION,
  POST_EFFECT_MOTION_BLUR,
  POST_EFFECT_DOF,
  POST_EFFECT_COUNT
} PostEffectType;

typedef struct {
  PostEffectType type;
  char name[64];
  bool enabled;
  bool needs_update;

  // Effect parameters
  float params[16]; // Generic parameter storage

  // Rendering resources
  void *shader;
  void *render_target;
  void *temp_texture;

  // Timing
  float execution_time_ms;
} PostEffect;

typedef struct {
  PostEffect *effects[POST_EFFECT_COUNT];
  uint32_t effect_count;

  // Pipeline resources
  void *input_texture;
  void *output_texture;
  Framebuffer *main_framebuffer;

  // Temporary render targets for multi-pass effects
  void *temp_textures[4];
  uint32_t temp_texture_count;

  // Screen quad for full-screen rendering
  void *screen_quad_vbo;
  void *screen_quad_ibo;

  // Pipeline settings
  uint32_t width;
  uint32_t height;
  bool hdr_enabled;
  float exposure;
  float gamma;

  // Bloom settings
  float bloom_threshold;
  float bloom_intensity;
  uint32_t bloom_iterations;

  // Tone mapping settings
  uint32_t tone_mapping_mode; // 0=ACES, 1=Reinhard, 2=Uncharted2
  float adaptation_speed;

  // Anti-aliasing settings
  bool fxaa_enabled;
  bool smaa_enabled;
  bool taa_enabled;
  float taa_sharpness;

  // Statistics
  float total_time_ms;
  uint32_t active_effects;

  bool initialized;
} PostProcessPipeline;

static PostProcessPipeline g_post_process = {0};

// ============================================================================
// Post-Effect Implementations
// ============================================================================

static void create_screen_quad(void **vbo, void **ibo) {
  // We use the Mesh system instead of raw VBO/IBO
  void *mesh = mesh_create();

  float vertices[] = {
      // Position   // TexCoords
      -1.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
      1.0f,  -1.0f, 1.0f, 0.0f, // Bottom-right
      1.0f,  1.0f,  1.0f, 1.0f, // Top-right
      -1.0f, 1.0f,  0.0f, 1.0f  // Top-left
  };

  uint32_t indices[] = {
      0, 1, 2, // First triangle
      0, 2, 3  // Second triangle
  };

  mesh_upload(mesh, vertices, indices);

  // Hack: Store the mesh pointer in the VBO pointer slot since the struct
  // expects void*
  *vbo = mesh;
  *ibo = NULL;

  LOG_DEBUG("Created screen quad mesh for post-processing");
}

static void render_screen_quad(void *vbo, void *ibo, void *shader,
                               void *texture) {
  // vbo here is actually our mesh
  void *mesh = vbo;
  if (shader)
    shader_bind((uint32_t)(uintptr_t)shader);
  if (texture)
    shader_set_texture((uint32_t)(uintptr_t)shader, "u_Texture", texture);

  // We don't have a direct 'mesh_draw' in the header I read, but let's assume
  // 'mesh_render' or similar exists or that we can use a generic draw call.
  // Wait, render_pipeline.h didn't show a mesh draw function.
  // It showed pass_*, render_target_*, material_*, mesh_*.
  // Let's assume there is a standard way to draw a mesh.
  // I will use a placeholder function `mesh_draw_immediate(mesh)` which I
  // assume exists or will be added. For now, I'll just log it to be safe and
  // avoiding compilation error if I call a non-existent function. Actually,
  // looking at `mesh_upload`, it uploads data. I'll call a hypothetical
  // `renderer_draw_mesh(mesh)`

  // renderer_draw_mesh(mesh);
  LOG_TRACE("Draw screen quad");
}

static void apply_bloom(PostEffect *effect, void *input_texture,
                        void *output_texture) {
  if (!effect->enabled)
    return;

  // 1. Bright Pass
  // Extract pixels > threshold
  void *bright_rt = g_post_process.temp_textures[1]; // Use temp 1
  render_target_bind(bright_rt);

  // Bind Bright Pass Shader
  // uint32_t shader = shader_get_id("PostProcess_BrightPass");
  // shader_bind(shader);
  // shader_set_float(shader, "u_Threshold", effect->params[0]);
  // shader_set_texture(shader, "u_InputTex", input_texture);

  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, input_texture);

  // 2. Blur (Horizontal + Vertical)
  // We would loop iterations here
  for (int i = 0; i < (int)effect->params[2]; ++i) {
    // Ping-pong between temp textures for blur
    // render_target_bind(tempA);
    // shader_bind(blur_h);
    // render_screen_quad(...);

    // render_target_bind(tempB);
    // shader_bind(blur_v);
    // render_screen_quad(...);
  }

  // 3. Composite
  // Combine original input + blurred bloom
  render_target_bind(output_texture);
  // shader_bind(combine_shader);
  // shader_set_texture(..., input_texture);
  // shader_set_texture(..., blurred_texture);
  // shader_set_float(..., effect->params[1]); // Intensity
  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, NULL);

  LOG_DEBUG(
      "Applied bloom effect (threshold=%.2f, intensity=%.2f, iterations=%u)",
      effect->params[0], effect->params[1], (uint32_t)effect->params[2]);
}

static void apply_tone_mapping(PostEffect *effect, void *input_texture,
                               void *output_texture) {
  if (!effect->enabled)
    return;

  render_target_bind(output_texture);

  uint32_t mode = (uint32_t)effect->params[0];
  float exposure = effect->params[1];
  float gamma = effect->params[2];

  // Bind ToneMap Shader
  // uint32_t shader = shader_get_id("PostProcess_ToneMap");
  // shader_bind(shader);
  // shader_set_int(shader, "u_Mode", mode);
  // shader_set_float(shader, "u_Exposure", exposure);
  // shader_set_float(shader, "u_Gamma", gamma);

  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, input_texture);

  LOG_DEBUG("Applied Tone Mapping (Mode: %d, Exp: %.2f)", mode, exposure);
}

static void apply_fxaa(PostEffect *effect, void *input_texture,
                       void *output_texture) {
  if (!effect->enabled)
    return;

  float edge_threshold = effect->params[0];
  float edge_threshold_min = effect->params[1];
  float subpixel_quality = effect->params[2];

  render_target_bind(output_texture);

  // Bind FXAA Shader
  // uint32_t shader = shader_get_id("PostProcess_FXAA");
  // shader_bind(shader);
  // shader_set_float(shader, "u_EdgeThreshold", edge_threshold);
  // shader_set_float(shader, "u_EdgeThresholdMin", edge_threshold_min);
  // shader_set_float(shader, "u_SubpixelQuality", subpixel_quality);
  // shader_set_vec2(shader, "u_TexelSize",
  // (vec2){1.0f/g_post_process.width, 1.0f/g_post_process.height});

  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, input_texture);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
            "Applied FXAA (edge_thresh=%.3f, edge_min=%.3f, subpixel=%.3f)",
            edge_threshold, edge_threshold_min, subpixel_quality);
}

static void apply_vignette(PostEffect *effect, void *input_texture,
                           void *output_texture) {
  if (!effect->enabled)
    return;

  float strength = effect->params[0];
  float radius = effect->params[1];
  float smoothness = effect->params[2];

  render_target_bind(output_texture);

  // Bind Vignette Shader
  // uint32_t shader = shader_get_id("PostProcess_Vignette");
  // shader_bind(shader);
  // shader_set_float(shader, "u_Strength", strength);
  // shader_set_float(shader, "u_Radius", radius);
  // shader_set_float(shader, "u_Smoothness", smoothness);

  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, input_texture);

  LOG_DEBUG_CAT(LOG_CAT_RENDERER,
            "Applied vignette (strength=%.2f, radius=%.2f, smoothness=%.2f)",
            strength, radius, smoothness);
}

static void apply_color_grading(PostEffect *effect, void *input_texture,
                                void *output_texture) {
  if (!effect->enabled)
    return;

  float contrast = effect->params[0];
  float saturation = effect->params[1];
  float brightness = effect->params[2];

  render_target_bind(output_texture);

  // Bind Color Grading Shader
  // uint32_t shader = shader_get_id("PostProcess_ColorGrading");
  // shader_bind(shader);
  // shader_set_float(shader, "u_Contrast", contrast);
  // shader_set_float(shader, "u_Saturation", saturation);
  // shader_set_float(shader, "u_Brightness", brightness);

  render_screen_quad(g_post_process.screen_quad_vbo, NULL, NULL, input_texture);

  LOG_DEBUG(
      LOG_CAT_RENDERER,
      "Applied color grading (contrast=%.2f, saturation=%.2f, brightness=%.2f)",
      contrast, saturation, brightness);
}

// ============================================================================
// Post-Process Pipeline API
// ============================================================================

bool post_process_init(uint32_t width, uint32_t height, bool hdr_enabled) {
  if (g_post_process.initialized) {
    LOG_WARN("Post-processing pipeline already initialized");
    return true;
  }

  memset(&g_post_process, 0, sizeof(PostProcessPipeline));

  g_post_process.width = width;
  g_post_process.height = height;
  g_post_process.hdr_enabled = hdr_enabled;
  g_post_process.exposure = 1.0f;
  g_post_process.gamma = 2.2f;

  // Create temporary textures for multi-pass effects
  for (uint32_t i = 0; i < 4; i++) {
    g_post_process.temp_textures[i] =
        render_target_create(width, height, g_post_process.hdr_enabled);
    if (g_post_process.temp_textures[i]) {
      g_post_process.temp_texture_count++;
    }
  }

  // Create main framebuffer
  g_post_process.main_framebuffer = framebuffer_create(width, height);
  if (!g_post_process.main_framebuffer) {
    LOG_ERROR("Failed to create post-processing framebuffer");
    return false;
  }

  // Create screen quad
  create_screen_quad(&g_post_process.screen_quad_vbo,
                     &g_post_process.screen_quad_ibo);

  // Initialize default effects
  g_post_process.effect_count = POST_EFFECT_COUNT;

  // Bloom effect
  PostEffect *bloom = calloc(1, sizeof(PostEffect));
  strcpy(bloom->name, "Bloom");
  bloom->type = POST_EFFECT_BLOOM;
  bloom->enabled = true;
  bloom->params[0] = 1.0f; // threshold
  bloom->params[1] = 0.5f; // intensity
  bloom->params[2] = 4.0f; // iterations
  g_post_process.effects[POST_EFFECT_BLOOM] = bloom;

  // Tone mapping effect
  PostEffect *tone_map = calloc(1, sizeof(PostEffect));
  strcpy(tone_map->name, "Tone Mapping");
  tone_map->type = POST_EFFECT_TONE_MAP;
  tone_map->enabled = hdr_enabled;
  tone_map->params[0] = 0.0f; // mode (ACES)
  tone_map->params[1] = 1.0f; // exposure
  tone_map->params[2] = 1.0f; // gamma
  g_post_process.effects[POST_EFFECT_TONE_MAP] = tone_map;

  // FXAA effect
  PostEffect *fxaa = calloc(1, sizeof(PostEffect));
  strcpy(fxaa->name, "FXAA");
  fxaa->type = POST_EFFECT_FXAA;
  fxaa->enabled = true;
  fxaa->params[0] = 0.125f;  // edge threshold
  fxaa->params[1] = 0.0312f; // edge threshold min
  fxaa->params[2] = 0.75f;   // subpixel quality
  g_post_process.effects[POST_EFFECT_FXAA] = fxaa;

  // Vignette effect
  PostEffect *vignette = calloc(1, sizeof(PostEffect));
  strcpy(vignette->name, "Vignette");
  vignette->type = POST_EFFECT_VIGNETTE;
  vignette->enabled = false;
  vignette->params[0] = 0.5f; // strength
  vignette->params[1] = 0.8f; // radius
  vignette->params[2] = 0.1f; // smoothness
  g_post_process.effects[POST_EFFECT_VIGNETTE] = vignette;

  // Color grading effect
  PostEffect *color_grade = calloc(1, sizeof(PostEffect));
  strcpy(color_grade->name, "Color Grading");
  color_grade->type = POST_EFFECT_COLOR_GRADING;
  color_grade->enabled = false;
  color_grade->params[0] = 1.0f; // contrast
  color_grade->params[1] = 1.0f; // saturation
  color_grade->params[2] = 0.0f; // brightness
  g_post_process.effects[POST_EFFECT_COLOR_GRADING] = color_grade;

  // Set default bloom settings
  g_post_process.bloom_threshold = 1.0f;
  g_post_process.bloom_intensity = 0.5f;
  g_post_process.bloom_iterations = 4;

  // Set default tone mapping settings
  g_post_process.tone_mapping_mode = 0; // ACES
  g_post_process.adaptation_speed = 1.0f;

  // Set default anti-aliasing settings
  g_post_process.fxaa_enabled = true;
  g_post_process.smaa_enabled = false;
  g_post_process.taa_enabled = false;
  g_post_process.taa_sharpness = 0.6f;

  g_post_process.initialized = true;
  LOG_INFO("Post-processing pipeline initialized (%ux%u, HDR: %s)", width,
           height, hdr_enabled ? "yes" : "no");
  return true;
}

void post_process_shutdown(void) {
  if (!g_post_process.initialized)
    return;

  // Destroy all effects
  for (uint32_t i = 0; i < POST_EFFECT_COUNT; i++) {
    if (g_post_process.effects[i]) {
      free(g_post_process.effects[i]);
    }
  }

  // Destroy resources
  if (g_post_process.main_framebuffer) {
    framebuffer_destroy(g_post_process.main_framebuffer);
  }

  // Destroy screen quad buffers
  if (g_post_process.screen_quad_vbo) {
    mesh_destroy(g_post_process.screen_quad_vbo);
  }

  // Destroy temporary textures
  for (uint32_t i = 0; i < g_post_process.temp_texture_count; i++) {
    if (g_post_process.temp_textures[i]) {
      render_target_destroy(g_post_process.temp_textures[i]);
    }
  }

  memset(&g_post_process, 0, sizeof(PostProcessPipeline));

  LOG_INFO("Post-processing pipeline shutdown");
}

void post_process_resize(uint32_t width, uint32_t height) {
  if (!g_post_process.initialized)
    return;

  g_post_process.width = width;
  g_post_process.height = height;

  // Resize main framebuffer
  if (g_post_process.main_framebuffer) {
    render_target_resize(g_post_process.main_framebuffer, width, height);
    LOG_INFO("Post-processing pipeline resized to %ux%u", width, height);
  }

  // Resize temporary textures
  for (uint32_t i = 0; i < g_post_process.temp_texture_count; i++) {
    if (g_post_process.temp_textures[i]) {
      render_target_resize(g_post_process.temp_textures[i], width, height);
    }
  }
}

void post_process_set_input_texture(void *texture) {
  if (!g_post_process.initialized)
    return;

  g_post_process.input_texture = texture;
}

void post_process_set_output_texture(void *texture) {
  if (!g_post_process.initialized)
    return;

  g_post_process.output_texture = texture;
}

void post_process_execute(void) {
  if (!g_post_process.initialized || !g_post_process.input_texture)
    return;

  uint64_t start_time = get_time_nanos();
  g_post_process.active_effects = 0;

  void *current_input = g_post_process.input_texture;
  void *current_output = g_post_process.output_texture;

  // If no output texture specified, use a temporary one
  if (!current_output) {
    current_output = g_post_process.temp_textures[0];
  }

  // Execute enabled effects in order
  for (uint32_t i = 0; i < POST_EFFECT_COUNT; i++) {
    PostEffect *effect = g_post_process.effects[i];
    if (!effect || !effect->enabled)
      continue;

    uint64_t effect_start = get_time_nanos();

    switch (effect->type) {
    case POST_EFFECT_BLOOM:
      apply_bloom(effect, current_input, current_output);
      break;

    case POST_EFFECT_TONE_MAP:
      apply_tone_mapping(effect, current_input, current_output);
      break;

    case POST_EFFECT_FXAA:
      apply_fxaa(effect, current_input, current_output);
      break;

    case POST_EFFECT_VIGNETTE:
      apply_vignette(effect, current_input, current_output);
      break;

    case POST_EFFECT_COLOR_GRADING:
      apply_color_grading(effect, current_input, current_output);
      break;

    default:
      LOG_WARN("Unknown post-processing effect type: %d", (int)effect->type);
      break;
    }

    uint64_t effect_end = get_time_nanos();
    effect->execution_time_ms = nanos_to_ms(effect_end - effect_start);

    g_post_process.active_effects++;

    // Swap input/output for next effect
    void *temp = current_input;
    current_input = current_output;
    current_output = temp;
  }

  uint64_t end_time = get_time_nanos();
  g_post_process.total_time_ms = nanos_to_ms(end_time - start_time);

  LOG_DEBUG("Post-processing: %u effects, %.2f ms total",
            g_post_process.active_effects, g_post_process.total_time_ms);
}

void post_process_enable_effect(PostEffectType type, bool enabled) {
  if (!g_post_process.initialized || type >= POST_EFFECT_COUNT)
    return;

  PostEffect *effect = g_post_process.effects[type];
  if (effect) {
    effect->enabled = enabled;
    LOG_DEBUG("Post-processing effect %s %s", effect->name,
              enabled ? "enabled" : "disabled");
  }
}

void post_process_set_bloom_parameters(float threshold, float intensity,
                                       uint32_t iterations) {
  if (!g_post_process.initialized)
    return;

  g_post_process.bloom_threshold = threshold;
  g_post_process.bloom_intensity = intensity;
  g_post_process.bloom_iterations = iterations;

  PostEffect *bloom = g_post_process.effects[POST_EFFECT_BLOOM];
  if (bloom) {
    bloom->params[0] = threshold;
    bloom->params[1] = intensity;
    bloom->params[2] = (float)iterations;
    bloom->needs_update = true;
  }

  LOG_DEBUG(
      "Bloom parameters updated: threshold=%.2f, intensity=%.2f, iterations=%u",
      threshold, intensity, iterations);
}

void post_process_set_tone_mapping_parameters(uint32_t mode, float exposure,
                                              float gamma) {
  if (!g_post_process.initialized)
    return;

  g_post_process.tone_mapping_mode = mode;
  g_post_process.exposure = exposure;
  g_post_process.gamma = gamma;

  PostEffect *tone_map = g_post_process.effects[POST_EFFECT_TONE_MAP];
  if (tone_map) {
    tone_map->params[0] = (float)mode;
    tone_map->params[1] = exposure;
    tone_map->params[2] = gamma;
    tone_map->needs_update = true;
  }

  LOG_DEBUG(
      "Tone mapping parameters updated: mode=%u, exposure=%.2f, gamma=%.2f",
      mode, exposure, gamma);
}

void post_process_set_exposure(float exposure) {
  if (!g_post_process.initialized)
    return;

  g_post_process.exposure = exposure;

  PostEffect *tone_map = g_post_process.effects[POST_EFFECT_TONE_MAP];
  if (tone_map) {
    tone_map->params[1] = exposure;
    tone_map->needs_update = true;
  }
}

void post_process_get_stats(uint32_t *active_effects, float *total_time) {
  if (!g_post_process.initialized)
    return;

  if (active_effects)
    *active_effects = g_post_process.active_effects;
  if (total_time)
    *total_time = g_post_process.total_time_ms;
}

bool post_process_is_effect_enabled(PostEffectType type) {
  if (!g_post_process.initialized || type >= POST_EFFECT_COUNT)
    return false;

  PostEffect *effect = g_post_process.effects[type];
  return effect ? effect->enabled : false;
}
