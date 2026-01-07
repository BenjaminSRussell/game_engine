// swift_bridge_stubs.c
// Stub implementations for Swift bridge functions
// These provide default behavior until full implementations are ready

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// OCCLUSION CULLING API
// =============================================================================

static bool occlusion_enabled = false;
static int32_t occlusion_mode = 2; // hardware
static uint32_t occlusion_visible = 0;
static uint32_t occlusion_culled = 0;

void occlusion_set_enabled(bool enabled) { occlusion_enabled = enabled; }

bool occlusion_is_enabled(void) { return occlusion_enabled; }

void occlusion_set_mode(int32_t mode) { occlusion_mode = mode; }

int32_t occlusion_get_mode(void) { return occlusion_mode; }

uint32_t occlusion_get_visible_count(void) { return occlusion_visible; }

uint32_t occlusion_get_culled_count(void) { return occlusion_culled; }

void occlusion_add_occluder(uint64_t entity_id) {
  // Stub: would register entity as occluder
}

void occlusion_remove_occluder(uint64_t entity_id) {
  // Stub: would unregister entity as occluder
}

// =============================================================================
// VFX OPTIMIZATION API
// =============================================================================

static bool vfx_culling_enabled = true;
static float vfx_culling_distance = 100.0f;
static float vfx_lod_bias = 1.0f;
static bool vfx_instancing_enabled = true;
static uint32_t vfx_max_particles = 10000;
static uint32_t vfx_active_particles = 0;

void vfx_opt_set_culling_enabled(bool enabled) {
  vfx_culling_enabled = enabled;
}

bool vfx_opt_is_culling_enabled(void) { return vfx_culling_enabled; }

void vfx_opt_set_culling_distance(float distance) {
  vfx_culling_distance = distance;
}

float vfx_opt_get_culling_distance(void) { return vfx_culling_distance; }

void vfx_opt_set_lod_bias(float bias) { vfx_lod_bias = bias; }

float vfx_opt_get_lod_bias(void) { return vfx_lod_bias; }

void vfx_opt_set_instancing_enabled(bool enabled) {
  vfx_instancing_enabled = enabled;
}

bool vfx_opt_is_instancing_enabled(void) { return vfx_instancing_enabled; }

void vfx_opt_set_max_particles(uint32_t count) { vfx_max_particles = count; }

uint32_t vfx_opt_get_max_particles(void) { return vfx_max_particles; }

uint32_t vfx_opt_get_active_particles(void) { return vfx_active_particles; }

// =============================================================================
// WEATHER VFX API
// =============================================================================

static int32_t weather_type = 0; // clear
static float weather_rain_intensity = 0.0f;
static float weather_snow_intensity = 0.0f;
static float weather_wind_strength = 0.0f;
static float weather_fog_density = 0.0f;

void weather_set_type(int32_t type) { weather_type = type; }

int32_t weather_get_type(void) { return weather_type; }

void weather_set_rain_intensity(float intensity) {
  weather_rain_intensity = intensity;
}

float weather_get_rain_intensity(void) { return weather_rain_intensity; }

void weather_set_snow_intensity(float intensity) {
  weather_snow_intensity = intensity;
}

float weather_get_snow_intensity(void) { return weather_snow_intensity; }

void weather_trigger_lightning(float x, float y, float z) {
  // Stub: would create lightning effect at position
}

void weather_set_wind_strength(float strength) {
  weather_wind_strength = strength;
}

float weather_get_wind_strength(void) { return weather_wind_strength; }

void weather_set_fog_density(float density) { weather_fog_density = density; }

float weather_get_fog_density(void) { return weather_fog_density; }

// =============================================================================
// WATER API
// =============================================================================

static bool water_enabled = true;
static int32_t water_quality = 2; // high
static bool water_reflections_enabled = true;
static bool water_refractions_enabled = true;
static float water_wave_amplitude = 0.5f;
static float water_wave_frequency = 1.0f;

void water_set_enabled(bool enabled) { water_enabled = enabled; }

bool water_is_enabled(void) { return water_enabled; }

void water_set_quality(int32_t quality) { water_quality = quality; }

int32_t water_get_quality(void) { return water_quality; }

void water_set_reflections_enabled(bool enabled) {
  water_reflections_enabled = enabled;
}

bool water_get_reflections_enabled(void) { return water_reflections_enabled; }

void water_set_refractions_enabled(bool enabled) {
  water_refractions_enabled = enabled;
}

bool water_get_refractions_enabled(void) { return water_refractions_enabled; }

void water_set_wave_amplitude(float amplitude) {
  water_wave_amplitude = amplitude;
}

float water_get_wave_amplitude(void) { return water_wave_amplitude; }

void water_set_wave_frequency(float frequency) {
  water_wave_frequency = frequency;
}

float water_get_wave_frequency(void) { return water_wave_frequency; }

// =============================================================================
// VEGETATION API
// =============================================================================

static bool vegetation_enabled = true;
static float vegetation_density = 1.0f;
static float vegetation_lod_bias = 1.0f;
static float vegetation_wind_strength = 0.5f;

void vegetation_set_enabled(bool enabled) { vegetation_enabled = enabled; }

bool vegetation_is_enabled(void) { return vegetation_enabled; }

void vegetation_set_density(float density) { vegetation_density = density; }

float vegetation_get_density(void) { return vegetation_density; }

void vegetation_set_lod_bias(float bias) { vegetation_lod_bias = bias; }

float vegetation_get_lod_bias(void) { return vegetation_lod_bias; }

void vegetation_set_wind_strength(float strength) {
  vegetation_wind_strength = strength;
}

float vegetation_get_wind_strength(void) { return vegetation_wind_strength; }

// =============================================================================
// UNDO/REDO API
// =============================================================================

void undo_perform(void) {
  // Stub: would undo last operation
}

void undo_redo(void) {
  // Stub: would redo last undone operation
}

void undo_clear(void) {
  // Stub: would clear undo stack
}

bool undo_can_undo(void) {
  return false; // Stub
}

bool undo_can_redo(void) {
  return false; // Stub
}

uint32_t undo_get_stack_size(void) {
  return 0; // Stub
}

void undo_set_max_stack_size(uint32_t size) {
  // Stub: would set max undo stack size
}

// =============================================================================
// UV VISUALIZATION API
// =============================================================================

void uv_vis_set_enabled(bool enabled, uint64_t entity_id, uint32_t channel) {
  // Stub: would enable UV visualization for entity
}

bool uv_vis_is_enabled(uint64_t entity_id) {
  return false; // Stub
}

void uv_vis_set_checkerboard_scale(float scale) {
  // Stub: would set checkerboard pattern scale
}

float uv_vis_get_checkerboard_scale(void) {
  return 1.0f; // Stub
}

uint32_t uv_vis_get_channel(uint64_t entity_id) {
  return 0; // Stub
}

// =============================================================================
// VERTEX COLOR DISPLAY API
// =============================================================================

void vcol_display_set_enabled(bool enabled, uint64_t entity_id) {
  // Stub: would enable vertex color display
}

bool vcol_display_is_enabled(uint64_t entity_id) {
  return false; // Stub
}

void vcol_display_set_channel(int32_t mode, uint64_t entity_id) {
  // Stub: would set which color channel to display
}

int32_t vcol_display_get_channel(uint64_t entity_id) {
  return 0; // Stub: RGB
}
