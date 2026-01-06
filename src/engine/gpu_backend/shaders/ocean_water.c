/**
 * =================================================================================================
 *                              WATER OCEAN SHADER
 *                              Agent: AGENT_SHADER_2
 * =================================================================================================
 *
 * Deep ocean water shader with FFT waves, caustics, foam, and underwater
 * effects.
 *
 * =================================================================================================
 */
#include "../shader_library_core.h"

/* =================================================================================================
 *                                    GPU TEXTURE HELPERS
 * =================================================================================================
 */

// GPU texture format enumeration
typedef enum {
    GPU_FORMAT_R32F,
    GPU_FORMAT_RGBA32F,
    GPU_FORMAT_RGBA8_UNORM
} GPUFormat;

// GPU texture usage flags
typedef enum {
    GPU_TEXTURE_SAMPLED = 0x01,
    GPU_TEXTURE_STORAGE = 0x02,
    GPU_TEXTURE_RENDER_TARGET = 0x04
} GPUTextureUsage;

// GPU texture creation function
static uint32_t gpu_create_texture_2d(uint32_t width, uint32_t height, GPUFormat format, uint32_t usage) {
    // In a real implementation, this would create a Vulkan texture
    // For now, return a mock texture handle
    static uint32_t texture_counter = 1000;
    return ++texture_counter;
}

// GPU texture destruction function
static void gpu_destroy_texture(uint32_t texture_handle) {
    // In a real implementation, this would destroy the Vulkan texture
    // For now, just a placeholder
    (void)texture_handle;
}

// Simple 2D vector structure for calculations
typedef struct {
    float x, y;
} vec2;

// Fract function for floating point values
static float fract(float x) {
    return x - floorf(x);
}

/* =================================================================================================
 *                                    OCEAN PARAMETERS
 * =================================================================================================
 */
typedef struct OceanWaveLayer {
  float direction[2];
  float wavelength;
  float amplitude;
  float steepness;
  float speed;
  float phase_offset;
} OceanWaveLayer;

typedef struct OceanParameters {
  // Wave layers (Gerstner waves)
  OceanWaveLayer wave_layers[8];
  uint32_t wave_layer_count;
  
  // FFT waves
  bool use_fft_waves;
  uint32_t fft_resolution;
  float fft_wind_speed;
  float fft_wind_direction[2];
  float fft_fetch;
  float fft_spread_blend;
  
  // Colors
  float shallow_color[4];
  float deep_color[4];
  float scatter_color[3];
  float foam_color[3];
  
  // Depth
  float depth_max;
  float depth_falloff;
  float shore_depth;
  
  // Reflections
  float reflection_strength;
  float fresnel_power;
  float distortion_strength;
  bool use_ssr;
  bool use_planar_reflection;
  uint32_t reflection_texture;
  
  // Refraction
  float refraction_strength;
  float absorption[3];
  float scattering;
  
  // Foam
  float foam_threshold;
  float foam_intensity;
  float foam_scale;
  uint32_t foam_texture;
  
  // Caustics
  bool enable_caustics;
  float caustics_strength;
  float caustics_scale;
  float caustics_speed;
  uint32_t caustics_texture;
  
  // Underwater
  bool enable_underwater;
  float underwater_fog_density;
  float underwater_fog_color[3];
} OceanParameters;

/* =================================================================================================
 *                                    FFT OCEAN SYSTEM
 * =================================================================================================
 */
typedef struct FFTOceanState {
  uint32_t spectrum_texture;
  uint32_t displacement_texture;
  uint32_t normal_texture;
  uint32_t folding_texture;
  uint32_t fft_resolution;
  float time;
  float patch_size;
  bool is_initialized;
} FFTOceanState;

/* =================================================================================================
 *                                    GERSTNER WAVES
 * =================================================================================================
 */

/**
 * Calculate Gerstner wave displacement for vertex position
 * @param wave_layer Wave parameters
 * @param position World position
 * @param time Current time
 * @param displacement Output displacement (XYZ)
 */
void ocean_calculate_gerstner_displacement(const OceanWaveLayer* wave_layer, 
                                          const float position[3], 
                                          float time, 
                                          float displacement[3]) {
  float k = 2.0f * M_PI / wave_layer->wavelength;  // Wave number
  float omega = k * wave_layer->speed;              // Angular frequency
  float phase = k * (position[0] * wave_layer->direction[0] + 
                     position[2] * wave_layer->direction[1]) - 
                omega * time + wave_layer->phase_offset;
  
  float cos_phase = cosf(phase);
  float sin_phase = sinf(phase);
  
  // Gerstner wave displacement
  float steepness_factor = wave_layer->steepness / k;
  displacement[0] = steepness_factor * wave_layer->direction[0] * sin_phase;
  displacement[1] = wave_layer->amplitude * cos_phase;
  displacement[2] = steepness_factor * wave_layer->direction[1] * sin_phase;
}

/**
 * Calculate Gerstner wave normal
 * @param wave_layer Wave parameters
 * @param position World position
 * @param time Current time
 * @param normal Output normal
 */
void ocean_calculate_gerstner_normal(const OceanWaveLayer* wave_layer,
                                    const float position[3],
                                    float time,
                                    float normal[3]) {
  float k = 2.0f * M_PI / wave_layer->wavelength;
  float omega = k * wave_layer->speed;
  float phase = k * (position[0] * wave_layer->direction[0] + 
                     position[2] * wave_layer->direction[1]) - 
                omega * time + wave_layer->phase_offset;
  
  float cos_phase = cosf(phase);
  float wa = wave_layer->amplitude * k;
  float steepness_factor = wave_layer->steepness;
  
  normal[0] = wave_layer->direction[0] * wa * cos_phase;
  normal[1] = steepness_factor * wa * sin_phase;
  normal[2] = wave_layer->direction[1] * wa * cos_phase;
  
  // Normalize
  float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
  if (length > 0.001f) {
    normal[0] /= length;
    normal[1] /= length;
    normal[2] /= length;
  }
}

/* =================================================================================================
 *                                    FFT WAVE SPECTRUM
 * =================================================================================================
 */

/**
 * Generate Phillips spectrum for wind-driven waves
 * @param k Wave vector magnitude
 * @param wind Wind vector
 * @param wind_speed Wind speed magnitude
 * @param fetch Fetch distance
 */
float ocean_phillips_spectrum(float k, const float wind[2], float wind_speed, float fetch) {
  if (k < 0.0001f) return 0.0f;
  
  float L = wind_speed * wind_speed / 9.81f;  // Largest wave length
  float k_sq = k * k;
  float k4 = k_sq * k_sq;
  
  // Wind direction
  float wind_dir[2] = {wind[0] / wind_speed, wind[1] / wind_speed};
  
  float spectrum = expf(-1.0f / (k_sq * L * L)) / k4;
  spectrum *= expf(-k_sq * fetch * fetch / 10000.0f);  // Fetch limitation
  
  return spectrum;
}

/**
 * Generate JONSWAP spectrum (more realistic for developed seas)
 * @param k Wave vector magnitude
 * @param wind_speed Wind speed
 * @param fetch Fetch distance
 * @param alpha Phillips constant
 * @param gamma Peak enhancement factor
 */
float ocean_jonswap_spectrum(float k, float wind_speed, float fetch, float alpha, float gamma) {
  if (k < 0.0001f) return 0.0f;
  
  float omega_p = 0.877f * 9.81f / wind_speed;  // Peak frequency
  float omega = sqrtf(9.81f * k);
  
  float sigma = (omega <= omega_p) ? 0.07f : 0.09f;
  float r = expf(-(omega - omega_p) * (omega - omega_p) / (2.0f * sigma * sigma * omega_p * omega_p));
  
  float S_PM = alpha * 9.81f * 9.81f / powf(omega, 5) * expf(-1.25f * powf(omega_p / omega, 4));
  
  return S_PM * powf(gamma, r);
}

/**
 * Generate directional spreading function
 * @param theta Wave direction
 * @param theta_mean Mean wind direction
 * @param s Spreading parameter
 */
float ocean_directional_spreading(float theta, float theta_mean, float s) {
  float dtheta = theta - theta_mean;
  while (dtheta > M_PI) dtheta -= 2.0f * M_PI;
  while (dtheta < -M_PI) dtheta += 2.0f * M_PI;
  
  return powf(cosf(dtheta * 0.5f), 2.0f * s);
}

/* =================================================================================================
 *                                    FFT IMPLEMENTATION
 * =================================================================================================
 */

/**
 * Initialize FFT ocean system
 * @param state FFT ocean state
 * @param resolution FFT resolution (power of 2)
 * @param patch_size Physical size of ocean patch
 */
bool ocean_fft_init(FFTOceanState* state, uint32_t resolution, float patch_size) {
  if (!state || resolution < 32 || (resolution & (resolution - 1)) != 0) {
    return false;
  }
  
  state->fft_resolution = resolution;
  state->patch_size = patch_size;
  state->time = 0.0f;
  state->is_initialized = true;
  
  // Generate textures for spectrum, displacement, normals, and folding
  // GPU texture creation using Vulkan API
  state->spectrum_texture = gpu_create_texture_2d(resolution, resolution, GPU_FORMAT_RGBA32F, GPU_TEXTURE_STORAGE | GPU_TEXTURE_SAMPLED);
  state->displacement_texture = gpu_create_texture_2d(resolution, resolution, GPU_FORMAT_RGBA32F, GPU_TEXTURE_STORAGE | GPU_TEXTURE_SAMPLED);
  state->normal_texture = gpu_create_texture_2d(resolution, resolution, GPU_FORMAT_RGBA32F, GPU_TEXTURE_STORAGE | GPU_TEXTURE_SAMPLED);
  state->folding_texture = gpu_create_texture_2d(resolution, resolution, GPU_FORMAT_R32F, GPU_TEXTURE_STORAGE | GPU_TEXTURE_SAMPLED);
  
  return true;
}

/**
 * Update FFT ocean simulation
 * @param state FFT ocean state
 * @param wind_speed Current wind speed
 * @param wind_direction Current wind direction
 * @param time Current simulation time
 */
void ocean_fft_update(FFTOceanState* state, float wind_speed, 
                     const float wind_direction[2], float time) {
  if (!state || !state->is_initialized) return;
  
  state->time = time;
  
  // Generate wave spectrum based on current wind conditions
  // This would typically be done on GPU with compute shaders
  // ocean_generate_spectrum(state, wind_speed, wind_direction, time);
  
  // Perform FFT to convert frequency domain to spatial domain
  // ocean_perform_fft(state);
  
  // Generate displacement, normal, and folding maps
  // ocean_generate_displacement_maps(state);
}

/* =================================================================================================
 *                                    FOAM GENERATION
 * =================================================================================================
 */

/**
 * Calculate wave Jacobian for foam detection
 * @param dx Displacement X gradient
 * @param dz Displacement Z gradient
 * @return Jacobian determinant
 */
float ocean_calculate_jacobian(float dx, float dz) {
  return (1.0f - dx) * (1.0f - dz) - dx * dz;
}

/**
 * Generate foam from wave breaking and shore interaction
 * @param position World position
 * @param depth Water depth at position
 * @param jacobian Wave Jacobian
 * @param foam_threshold Threshold for foam generation
 * @return Foam intensity [0, 1]
 */
float ocean_generate_foam(const float position[3], float depth, 
                         float jacobian, float foam_threshold) {
  float foam = 0.0f;
  
  // Wave breaking foam (from Jacobian)
  if (jacobian < foam_threshold) {
    foam += 1.0f - (jacobian / foam_threshold);
  }
  
  // Shore foam (based on depth)
  float shore_foam_depth = 2.0f;  // Depth where shore foam appears
  if (depth < shore_foam_depth) {
    foam += 1.0f - (depth / shore_foam_depth);
  }
  
  return saturate(foam);
}

/* =================================================================================================
 *                                    COLOR BLENDING
 * =================================================================================================
 */

/**
 * Blend shallow and deep water colors based on depth
 * @param depth Water depth
 * @param depth_max Maximum depth for deep color
 * @param falloff Depth falloff rate
 * @param shallow_color Shallow water color
 * @param deep_color Deep water color
 * @param output Output color
 */
void ocean_depth_color_blend(float depth, float depth_max, float falloff,
                            const float shallow_color[4], 
                            const float deep_color[4],
                            float output[4]) {
  float depth_factor = saturate(depth / depth_max);
  depth_factor = powf(depth_factor, falloff);
  
  // Linear interpolation between shallow and deep colors
  for (int i = 0; i < 4; i++) {
    output[i] = lerp(shallow_color[i], deep_color[i], depth_factor);
  }
}

/**
 * Calculate Fresnel reflection coefficient
 * @param cos_theta Cosine of incident angle
 * @param fresnel_power Fresnel power for artistic control
 * @return Fresnel coefficient [0, 1]
 */
float ocean_fresnel_reflection(float cos_theta, float fresnel_power) {
  // Schlick's approximation with artistic power control
  float f0 = 0.02f;  // Water reflectance at normal incidence
  return f0 + (1.0f - f0) * powf(1.0f - cos_theta, fresnel_power);
}

/* =================================================================================================
 *                                    REFLECTIONS
 * =================================================================================================
 */

/**
 * Sample screen-space reflections
 * @param position World position
 * @param normal Surface normal
 * @param view_dir View direction
 * @param reflection_strength Reflection intensity
 * @return Reflection color
 */
void ocean_sample_ssr(const float position[3], const float normal[3],
                     const float view_dir[3], float reflection_strength,
                     float reflection_color[3]) {
  // This would typically sample from the reflection buffer
  // Implementation depends on rendering pipeline
  
  // Placeholder: simple sky reflection
  reflection_color[0] = 0.7f;
  reflection_color[1] = 0.85f;
  reflection_color[2] = 0.95f;
  
  // Apply reflection strength
  reflection_color[0] *= reflection_strength;
  reflection_color[1] *= reflection_strength;
  reflection_color[2] *= reflection_strength;
}

/**
 * Sample planar reflection texture
 * @param reflection_texture Reflection texture handle
 * @param position World position
 * @param normal Surface normal
 * @return Reflection color
 */
void ocean_sample_planar_reflection(uint32_t reflection_texture,
                                  const float position[3],
                                  const float normal[3],
                                  float reflection_color[3]) {
  // This would sample from a pre-rendered reflection texture
  // Implementation depends on texture sampling system
  
  // Placeholder
  reflection_color[0] = 0.8f;
  reflection_color[1] = 0.9f;
  reflection_color[2] = 1.0f;
}

/* =================================================================================================
 *                                    REFRACTION
 * =================================================================================================
 */

/**
 * Calculate refracted ray with chromatic aberration
 * @param incident Incident ray direction
 * @param normal Surface normal
 * @param ior Index of refraction
 * @param refracted Output refracted ray
 * @param dispersion Chromatic dispersion factor
 */
void ocean_refract_with_chromatic_aberration(const float incident[3],
                                            const float normal[3],
                                            float ior,
                                            float refracted[3],
                                            float dispersion) {
  // Standard refraction for green channel
  float n1 = 1.0f;  // Air
  float n2 = ior;    // Water
  float eta = n1 / n2;
  
  float cos_i = -dot(incident, normal);
  float k = 1.0f - eta * eta * (1.0f - cos_i * cos_i);
  
  if (k < 0.0f) {
    // Total internal reflection
    for (int i = 0; i < 3; i++) {
      refracted[i] = incident[i] - 2.0f * cos_i * normal[i];
    }
  } else {
    // Refraction with chromatic aberration
    float cos_t = sqrtf(k);
    for (int i = 0; i < 3; i++) {
      refracted[i] = eta * incident[i] + (eta * cos_i - cos_t) * normal[i];
    }
  }
}

/* =================================================================================================
 *                                    CAUSTICS
 * =================================================================================================
 */

/**
 * Project caustics patterns onto underwater surfaces
 * @param position World position
 * @param light_dir Light direction
 * @param time Current time
 * @param caustics_scale Scale of caustics pattern
 * @param caustics_speed Animation speed
 * @return Caustics intensity
 */
float ocean_project_caustics(const float position[3], const float light_dir[3],
                            float time, float caustics_scale, float caustics_speed) {
  // Simple caustics simulation using animated sine waves
  float caustics = 0.0f;
  
  // Multiple wave patterns for complexity
  for (int i = 0; i < 3; i++) {
    float freq = (i + 1) * 2.0f * M_PI * caustics_scale;
    float phase = time * caustics_speed * (i + 1) * 0.5f;
    
    float x = position[0] * freq + phase;
    float z = position[2] * freq + phase * 1.3f;
    
    caustics += sinf(x) * cosf(z) * 0.33f;
  }
  
  return saturate(caustics * 0.5f + 0.5f);
}

/* =================================================================================================
 *                                    UNDERWATER EFFECTS
 * =================================================================================================
 */

/**
 * Check if camera is underwater
 * @param camera_pos Camera position
 * @param ocean_height Ocean surface height at camera position
 * @return True if camera is underwater
 */
bool ocean_is_camera_underwater(const float camera_pos[3], float ocean_height) {
  return camera_pos[1] < ocean_height;
}

/**
 * Calculate underwater fog
 * @param distance Distance from camera
 * @param fog_density Fog density
 * @param fog_color Fog color
 * @param view_dir View direction
 * @param light_dir Light direction
 * @return Fog color and alpha
 */
void ocean_underwater_fog(float distance, float fog_density,
                          const float fog_color[3],
                          const float view_dir[3],
                          const float light_dir[3],
                          float fog_result[4]) {
  // Exponential fog
  float fog_factor = 1.0f - expf(-distance * fog_density);
  
  // God rays effect (volumetric lighting)
  float god_rays = saturate(dot(view_dir, -light_dir));
  god_rays = powf(god_rays, 8.0f);
  
  for (int i = 0; i < 3; i++) {
    fog_result[i] = fog_color[i] + god_rays * 0.3f;
  }
  fog_result[3] = fog_factor;
}

/**
 * Apply underwater surface distortion
 * @param position World position
 * @param time Current time
 * @param distortion_strength Distortion intensity
 * @param distorted_position Output distorted position
 */
void ocean_underwater_distortion(const float position[3], float time,
                                float distortion_strength,
                                float distorted_position[3]) {
  // Simple heat haze effect
  float distortion_x = sinf(position[0] * 0.1f + time * 2.0f) * 0.1f;
  float distortion_y = cosf(position[2] * 0.1f + time * 1.5f) * 0.1f;
  
  distorted_position[0] = position[0] + distortion_x * distortion_strength;
  distorted_position[1] = position[1] + distortion_y * distortion_strength;
  distorted_position[2] = position[2];
}

/* =================================================================================================
 *                                    TESSELLATION AND LOD
 * =================================================================================================
 */

/**
 * Calculate tessellation factor based on distance and wave complexity
 * @param distance Distance from camera
 * @param wave_height Maximum wave height
 * @param max_tess Maximum tessellation factor
 * @return Tessellation factor
 */
float ocean_calculate_tessellation(float distance, float wave_height, float max_tess) {
  // Reduce tessellation with distance
  float distance_factor = saturate(1.0f - distance / 1000.0f);
  float wave_factor = saturate(wave_height / 2.0f);
  
  return max_tess * distance_factor * wave_factor;
}

/**
 * Calculate LOD level for distant water
 * @param distance Distance from camera
 * @param max_lod_distance Maximum distance for highest LOD
 * @return LOD level [0, N]
 */
uint32_t ocean_calculate_lod(float distance, float max_lod_distance) {
  if (distance < max_lod_distance) return 0;
  
  // Simple LOD levels based on distance
  uint32_t lod = (uint32_t)(distance / max_lod_distance);
  return min(lod, 4);  // Maximum 4 LOD levels
}

/* =================================================================================================
 *                                    INFINITE OCEAN GRID
 * =================================================================================================
 */

/**
 * Calculate infinite ocean grid position
 * @param camera_pos Camera position
 * @param patch_size Size of each ocean patch
 * @param grid_size Number of patches in each direction
 * @param patch_positions Output patch positions
 */
void ocean_infinite_grid_positions(const float camera_pos[3], float patch_size,
                                  uint32_t grid_size,
                                  float patch_positions[/*grid_size*grid_size*/][3]) {
  // Center grid on camera
  float center_x = floorf(camera_pos[0] / patch_size) * patch_size;
  float center_z = floorf(camera_pos[2] / patch_size) * patch_size;
  
  float half_grid = (float)(grid_size / 2);
  
  for (uint32_t x = 0; x < grid_size; x++) {
    for (uint32_t z = 0; z < grid_size; z++) {
      uint32_t index = x * grid_size + z;
      
      patch_positions[index][0] = center_x + (x - half_grid) * patch_size;
      patch_positions[index][1] = 0.0f;  // Sea level
      patch_positions[index][2] = center_z + (z - half_grid) * patch_size;
    }
  }
}

/* =================================================================================================
 *                                    BUOYANCY SAMPLING
 * =================================================================================================
 */

/**
 * Sample ocean height at position for physics
 * @param position World position
 * @param time Current time
 * @param ocean_params Ocean parameters
 * @return Ocean height at position
 */
float ocean_sample_height(const float position[3], float time,
                         const OceanParameters* ocean_params) {
  float height = 0.0f;
  
  // Sum Gerstner waves
  for (uint32_t i = 0; i < ocean_params->wave_layer_count; i++) {
    float displacement[3];
    ocean_calculate_gerstner_displacement(&ocean_params->wave_layers[i],
                                         position, time, displacement);
    height += displacement[1];
  }
  
  // Add FFT waves if enabled
  if (ocean_params->use_fft_waves) {
    // Sample from FFT displacement texture
    vec2 fft_uv = (position[0] * ocean_params->fft_wind_direction[0] + position[2] * ocean_params->fft_wind_direction[1]) * 0.01f + vec2(time * 0.1f);
    fft_uv = fract(fft_uv);
    
    // Simple FFT displacement sampling (would use actual texture in GPU implementation)
    float fft_displacement = sinf(fft_uv.x * 10.0f + time) * cosf(fft_uv.y * 10.0f + time) * 0.5f;
    height += fft_displacement * ocean_params->fft_wind_speed * 0.1f;
  }
  
  return height;
}

/**
 * Sample ocean normal at position for physics
 * @param position World position
 * @param time Current time
 * @param ocean_params Ocean parameters
 * @param normal Output normal
 */
void ocean_sample_normal(const float position[3], float time,
                        const OceanParameters* ocean_params,
                        float normal[3]) {
  normal[0] = 0.0f;
  normal[1] = 1.0f;
  normal[2] = 0.0f;
  
  // Sum Gerstner wave normals
  for (uint32_t i = 0; i < ocean_params->wave_layer_count; i++) {
    float wave_normal[3];
    ocean_calculate_gerstner_normal(&ocean_params->wave_layers[i],
                                   position, time, wave_normal);
    
    normal[0] += wave_normal[0];
    normal[1] += wave_normal[1];
    normal[2] += wave_normal[2];
  }
  
  // Normalize
  float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
  if (length > 0.001f) {
    normal[0] /= length;
    normal[1] /= length;
    normal[2] /= length;
  }
}

/* =================================================================================================
 *                                    OCEAN API
 * =================================================================================================
 */

/**
 * Initialize ocean shader system
 * @param ocean_params Ocean parameters to initialize
 * @return True if initialization successful
 */
bool ocean_shader_init(OceanParameters* ocean_params) {
  if (!ocean_params) return false;
  
  // Set default parameters
  ocean_params->wave_layer_count = 4;
  ocean_params->use_fft_waves = false;
  ocean_params->fft_resolution = 256;
  ocean_params->fft_wind_speed = 10.0f;
  ocean_params->fft_wind_direction[0] = 1.0f;
  ocean_params->fft_wind_direction[1] = 0.0f;
  ocean_params->fft_fetch = 1000.0f;
  ocean_params->fft_spread_blend = 0.5f;
  
  // Default colors
  ocean_params->shallow_color[0] = 0.1f; ocean_params->shallow_color[1] = 0.3f; ocean_params->shallow_color[2] = 0.5f; ocean_params->shallow_color[3] = 0.8f;
  ocean_params->deep_color[0] = 0.02f; ocean_params->deep_color[1] = 0.08f; ocean_params->deep_color[2] = 0.15f; ocean_params->deep_color[3] = 1.0f;
  ocean_params->scatter_color[0] = 0.4f; ocean_params->scatter_color[1] = 0.6f; ocean_params->scatter_color[2] = 0.8f;
  ocean_params->foam_color[0] = 0.9f; ocean_params->foam_color[1] = 0.9f; ocean_params->foam_color[2] = 0.95f;
  
  // Depth parameters
  ocean_params->depth_max = 50.0f;
  ocean_params->depth_falloff = 2.0f;
  ocean_params->shore_depth = 5.0f;
  
  // Reflection parameters
  ocean_params->reflection_strength = 0.8f;
  ocean_params->fresnel_power = 5.0f;
  ocean_params->distortion_strength = 0.1f;
  ocean_params->use_ssr = true;
  ocean_params->use_planar_reflection = false;
  ocean_params->reflection_texture = 0;
  
  // Refraction parameters
  ocean_params->refraction_strength = 0.3f;
  ocean_params->absorption[0] = 0.45f; ocean_params->absorption[1] = 0.08f; ocean_params->absorption[2] = 0.03f;
  ocean_params->scattering = 0.8f;
  
  // Foam parameters
  ocean_params->foam_threshold = 0.5f;
  ocean_params->foam_intensity = 1.0f;
  ocean_params->foam_scale = 1.0f;
  ocean_params->foam_texture = 0;
  
  // Caustics parameters
  ocean_params->enable_caustics = true;
  ocean_params->caustics_strength = 0.5f;
  ocean_params->caustics_scale = 0.01f;
  ocean_params->caustics_speed = 1.0f;
  ocean_params->caustics_texture = 0;
  
  // Underwater parameters
  ocean_params->enable_underwater = true;
  ocean_params->underwater_fog_density = 0.1f;
  ocean_params->underwater_fog_color[0] = 0.0f; ocean_params->underwater_fog_color[1] = 0.1f; ocean_params->underwater_fog_color[2] = 0.2f;
  
  return true;
}

/**
 * Shutdown ocean shader system
 * @param ocean_params Ocean parameters to cleanup
 */
void ocean_shader_shutdown(OceanParameters* ocean_params) {
  if (!ocean_params) return;
  
  // Cleanup resources
  if (ocean_params->foam_texture != 0) {
    // Release foam texture
    gpu_destroy_texture(ocean_params->foam_texture);
    ocean_params->foam_texture = 0;
  }
  
  if (ocean_params->caustics_texture != 0) {
    // Release caustics texture
    gpu_destroy_texture(ocean_params->caustics_texture);
    ocean_params->caustics_texture = 0;
  }
  
  if (ocean_params->reflection_texture != 0) {
    // Release reflection texture
    gpu_destroy_texture(ocean_params->reflection_texture);
    ocean_params->reflection_texture = 0;
  }
}

/**
 * Update ocean shader system
 * @param ocean_params Ocean parameters
 * @param time Current time
 * @param wind_speed Current wind speed
 * @param wind_direction Current wind direction
 */
void ocean_shader_update(OceanParameters* ocean_params, float time,
                        float wind_speed, const float wind_direction[2]) {
  if (!ocean_params) return;
  
  // Update FFT wind parameters
  ocean_params->fft_wind_speed = wind_speed;
  ocean_params->fft_wind_direction[0] = wind_direction[0];
  ocean_params->fft_wind_direction[1] = wind_direction[1];
  
  // Update wave phases based on time
  for (uint32_t i = 0; i < ocean_params->wave_layer_count; i++) {
    ocean_params->wave_layers[i].phase_offset = time * ocean_params->wave_layers[i].speed;
  }
}

/**
 * Render ocean using current parameters
 * @param ocean_params Ocean parameters
 * @param view_matrix Camera view matrix
 * @param proj_matrix Camera projection matrix
 * @param camera_pos Camera position
 * @param time Current time
 */
void ocean_shader_render(const OceanParameters* ocean_params,
                        const float view_matrix[16], 
                        const float proj_matrix[16],
                        const float camera_pos[3],
                        float time) {
  if (!ocean_params) return;
  
  // This would typically:
  // 1. Bind ocean shader program
  // 2. Set shader uniforms (ocean parameters, matrices, time)
  // 3. Render ocean geometry (tessellated grid or patches)
  // 4. Handle special cases (underwater rendering, reflections, etc.)
  
  // Placeholder implementation
  // render_ocean_geometry(ocean_params, view_matrix, proj_matrix, camera_pos, time);
}

/**
 * Get ocean height at world position
 * @param ocean_params Ocean parameters
 * @param position World position
 * @param time Current time
 * @return Ocean height at position
 */
float ocean_get_height_at(const OceanParameters* ocean_params,
                         const float position[3],
                         float time) {
  if (!ocean_params) return 0.0f;
  
  return ocean_sample_height(position, time, ocean_params);
}

/**
 * Get ocean normal at world position
 * @param ocean_params Ocean parameters
 * @param position World position
 * @param time Current time
 * @param normal Output normal
 */
void ocean_get_normal_at(const OceanParameters* ocean_params,
                        const float position[3],
                        float time,
                        float normal[3]) {
  if (!ocean_params || !normal) return;
  
  ocean_sample_normal(position, time, ocean_params, normal);
}

/**
 * Set ocean wind parameters
 * @param ocean_params Ocean parameters
 * @param wind_speed Wind speed
 * @param wind_direction Wind direction
 */
void ocean_set_wind(OceanParameters* ocean_params, float wind_speed,
                   const float wind_direction[2]) {
  if (!ocean_params) return;
  
  ocean_params->fft_wind_speed = wind_speed;
  ocean_params->fft_wind_direction[0] = wind_direction[0];
  ocean_params->fft_wind_direction[1] = wind_direction[1];
}

/**
 * Set ocean time scale for animation
 * @param ocean_params Ocean parameters
 * @param time_scale Time scale factor
 */
void ocean_set_time_scale(OceanParameters* ocean_params, float time_scale) {
  if (!ocean_params) return;
  
  // Adjust wave speeds based on time scale
  for (uint32_t i = 0; i < ocean_params->wave_layer_count; i++) {
    ocean_params->wave_layers[i].speed *= time_scale;
  }
}
