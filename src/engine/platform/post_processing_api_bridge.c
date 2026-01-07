#include "../include/platform/post_processing_api_bridge.h"
#include "core/logger.h"
#include "rendering/post_processing.h"
#include <string.h>

// Global configuration instance that the renderer can query
// In a proper ECS/System architecture, this would be in a component or system.
static PostProcessingConfig g_pp_config;
static bool g_initialized = false;

static void ensure_init(void) {
  if (!g_initialized) {
    memset(&g_pp_config, 0, sizeof(PostProcessingConfig));
    // Set defaults
    g_pp_config.enabledEffects = 0;
    g_pp_config.motionBlurStrength = 0.5f;
    g_pp_config.motionBlurSamples = 8;
    g_pp_config.chromaticAberrationStrength = 0.5f;
    g_pp_config.filmGrainStrength =
        0.2f; // bridge uses "intensity", config uses "strength"?
    g_pp_config.filmGrainStrength = 0.2f;
    g_pp_config.vignetteStrength = 0.3f;
    g_initialized = true;
  }
}

// Function exposed to the renderer to get the current config
// (Add declaration in a shared header or use extern)
const PostProcessingConfig *post_process_bridge_get_config(void) {
  ensure_init();
  return &g_pp_config;
}

void postfx_set_motion_blur_enabled(bool enabled) {
  ensure_init();
  if (enabled)
    g_pp_config.enabledEffects |= POST_EFFECT_MOTION_BLUR;
  else
    g_pp_config.enabledEffects &= ~POST_EFFECT_MOTION_BLUR;
  g_pp_config.motionBlurEnabled =
      enabled; // Redundant in struct but good for safety
}

bool postfx_is_motion_blur_enabled(void) {
  ensure_init();
  return (g_pp_config.enabledEffects & POST_EFFECT_MOTION_BLUR) != 0;
}

void postfx_set_motion_blur_strength(float strength) {
  ensure_init();
  g_pp_config.motionBlurStrength = strength;
}

float postfx_get_motion_blur_strength(void) {
  ensure_init();
  return g_pp_config.motionBlurStrength;
}

void postfx_set_chromatic_aberration_enabled(bool enabled) {
  ensure_init();
  if (enabled)
    g_pp_config.enabledEffects |= POST_EFFECT_CHROMATIC_ABERRATION;
  else
    g_pp_config.enabledEffects &= ~POST_EFFECT_CHROMATIC_ABERRATION;
  g_pp_config.chromaticAberrationEnabled = enabled;
}

bool postfx_is_chromatic_aberration_enabled(void) {
  ensure_init();
  return (g_pp_config.enabledEffects & POST_EFFECT_CHROMATIC_ABERRATION) != 0;
}

void postfx_set_chromatic_aberration_intensity(float intensity) {
  ensure_init();
  g_pp_config.chromaticAberrationStrength = intensity;
}

float postfx_get_chromatic_aberration_intensity(void) {
  ensure_init();
  return g_pp_config.chromaticAberrationStrength;
}

void postfx_set_film_grain_enabled(bool enabled) {
  ensure_init();
  if (enabled)
    g_pp_config.enabledEffects |= POST_EFFECT_FILM_GRAIN;
  else
    g_pp_config.enabledEffects &= ~POST_EFFECT_FILM_GRAIN;
  g_pp_config.filmGrainEnabled = enabled;
}

bool postfx_is_film_grain_enabled(void) {
  ensure_init();
  return (g_pp_config.enabledEffects & POST_EFFECT_FILM_GRAIN) != 0;
}

void postfx_set_film_grain_intensity(float intensity) {
  ensure_init();
  g_pp_config.filmGrainStrength = intensity;
}

float postfx_get_film_grain_intensity(void) {
  ensure_init();
  return g_pp_config.filmGrainStrength;
}

void postfx_set_vignette_enabled(bool enabled) {
  ensure_init();
  if (enabled)
    g_pp_config.enabledEffects |= POST_EFFECT_VIGNETTE;
  else
    g_pp_config.enabledEffects &= ~POST_EFFECT_VIGNETTE;
  g_pp_config.vignetteEnabled = enabled;
}

bool postfx_is_vignette_enabled(void) {
  ensure_init();
  return (g_pp_config.enabledEffects & POST_EFFECT_VIGNETTE) != 0;
}

void postfx_set_vignette_intensity(float intensity) {
  ensure_init();
  g_pp_config.vignetteStrength = intensity;
}

float postfx_get_vignette_intensity(void) {
  ensure_init();
  return g_pp_config.vignetteStrength;
}
