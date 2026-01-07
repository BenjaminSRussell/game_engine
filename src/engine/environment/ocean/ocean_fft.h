#ifndef OCEAN_FFT_H
#define OCEAN_FFT_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations for Metal types (defined in backend)
typedef struct MTLDevice* MTLDeviceRef;
typedef struct MTLCommandQueue* MTLCommandQueueRef;
typedef struct MTLTexture* MTLTextureRef;
typedef struct MTLComputePipelineState* MTLComputePipelineRef;
typedef struct MTLBuffer* MTLBufferRef;

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Ocean FFT Configuration
// =============================================================================

#define OCEAN_FFT_DEFAULT_RESOLUTION 256
#define OCEAN_FFT_MAX_CASCADES 4
#define OCEAN_GRAVITY 9.81f

typedef struct OceanFFTConfig {
    // FFT resolution (must be power of 2: 128, 256, 512)
    uint32_t fft_resolution;
    
    // Physics parameters
    float gravity;              // Default: 9.81 m/s²
    float time_scale;           // Time multiplier for wave speed
    
    // Cascade configuration
    uint32_t cascade_count;     // Number of cascade levels (1-4)
    float cascade_scales[OCEAN_FFT_MAX_CASCADES];  // Tile size per cascade
    
    // Performance
    bool enable_foam;           // Enable foam generation
    bool enable_normals;        // Enable normal map generation
} OceanFFTConfig;

// =============================================================================
// Phillips Spectrum Parameters
// =============================================================================

typedef struct PhillipsSpectrumParams {
    // Wind parameters
    float wind_speed;           // Wind speed in m/s (typical: 10-30)
    float wind_direction_x;     // Wind direction X component (normalized)
    float wind_direction_z;     // Wind direction Z component (normalized)
    
    // Wave parameters
    float amplitude;            // Overall wave amplitude scaling
    float wave_suppression;     // Small wave suppression (l parameter)
    float fetch;                // Fetch length (affects largest waves)
    
    // Choppy waves
    float choppy_factor;        // Horizontal displacement scale (0-2)
} PhillipsSpectrumParams;

// =============================================================================
// Ocean FFT System State
// =============================================================================

typedef struct OceanFFTCascade {
    // Spectrum textures (complex, RG32F)
    MTLTextureRef h0_texture;           // Initial spectrum h0(k)
    MTLTextureRef h0_conj_texture;      // Conjugate h0*(-k)
    MTLTextureRef omega_texture;        // Dispersion ω(k)
    
    // Time-varying spectrum (complex, RG32F)
    MTLTextureRef ht_height;            // h(k,t) for height
    MTLTextureRef ht_displacement_x;    // h(k,t) for Dx
    MTLTextureRef ht_displacement_z;    // h(k,t) for Dz
    
    // FFT ping-pong buffers (complex, RG32F)
    MTLTextureRef fft_ping;
    MTLTextureRef fft_pong;
    
    // Final displacement outputs (real domain)
    MTLTextureRef displacement_map;     // RGB32F: (Dx, height, Dz)
    MTLTextureRef normal_map;           // RGBA16F: (Nx, Ny, Nz, foam)
    
    // Cascade configuration
    float tile_size;                    // Physical tile size in world units
    float blend_start;                  // Distance to start blending
    float blend_end;                    // Distance to end blending
} OceanFFTCascade;

typedef struct OceanFFTSystem {
    // Configuration
    OceanFFTConfig config;
    PhillipsSpectrumParams spectrum_params;
    
    // Metal resources
    MTLDeviceRef device;
    MTLCommandQueueRef command_queue;
    
    // Compute pipelines
    MTLComputePipelineRef spectrum_init_pipeline;
    MTLComputePipelineRef spectrum_update_pipeline;
    MTLComputePipelineRef fft_horizontal_pipeline;
    MTLComputePipelineRef fft_vertical_pipeline;
    MTLComputePipelineRef displacement_pipeline;
    MTLComputePipelineRef normal_pipeline;
    MTLComputePipelineRef foam_pipeline;
    
    // Uniform buffers
    MTLBufferRef spectrum_params_buffer;
    MTLBufferRef fft_params_buffer;
    
    // Cascades
    uint32_t cascade_count;
    OceanFFTCascade cascades[OCEAN_FFT_MAX_CASCADES];
    
    // Timing
    float time;
    float delta_time;
    
    // Performance tracking
    double last_update_time_ms;
} OceanFFTSystem;

// =============================================================================
// Ocean FFT API
// =============================================================================

/**
 * Create and initialize ocean FFT system
 * 
 * @param device Metal device
 * @param queue Metal command queue
 * @param config Ocean configuration
 * @return Initialized ocean system or NULL on failure
 */
OceanFFTSystem* ocean_fft_create(
    MTLDeviceRef device,
    MTLCommandQueueRef queue,
    const OceanFFTConfig* config
);

/**
 * Destroy ocean FFT system and release resources
 */
void ocean_fft_destroy(OceanFFTSystem* ocean);

/**
 * Update spectrum parameters (wind, amplitude, etc.)
 * Calls are batched; actual update happens on next ocean_fft_update()
 * 
 * @param ocean Ocean system
 * @param params New spectrum parameters
 */
void ocean_fft_set_spectrum_params(
    OceanFFTSystem* ocean,
    const PhillipsSpectrumParams* params
);

/**
 * Update ocean simulation for current frame
 * This runs the FFT pipeline and generates displacement/normal maps
 * 
 * @param ocean Ocean system
 * @param delta_time Time elapsed since last frame (seconds)
 */
void ocean_fft_update(OceanFFTSystem* ocean, float delta_time);

/**
 * Get displacement texture for a specific cascade
 * Format: RGB32F (Dx, height, Dz)
 * 
 * @param ocean Ocean system
 * @param cascade_index Cascade index (0 to cascade_count-1)
 * @return Displacement texture or NULL if invalid index
 */
MTLTextureRef ocean_fft_get_displacement_texture(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index
);

/**
 * Get normal map texture for a specific cascade
 * Format: RGBA16F (Nx, Ny, Nz, foam)
 * 
 * @param ocean Ocean system
 * @param cascade_index Cascade index (0 to cascade_count-1)
 * @return Normal texture or NULL if invalid index
 */
MTLTextureRef ocean_fft_get_normal_texture(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index
);

/**
 * Get cascade configuration for shader binding
 * 
 * @param ocean Ocean system
 * @param cascade_index Cascade index
 * @param out_tile_size Output tile size
 * @param out_blend_start Output blend start distance
 * @param out_blend_end Output blend end distance
 * @return true if valid cascade, false otherwise
 */
bool ocean_fft_get_cascade_info(
    const OceanFFTSystem* ocean,
    uint32_t cascade_index,
    float* out_tile_size,
    float* out_blend_start,
    float* out_blend_end
);

/**
 * Get current simulation time
 */
float ocean_fft_get_time(const OceanFFTSystem* ocean);

/**
 * Get last update performance time in milliseconds
 */
double ocean_fft_get_last_update_time_ms(const OceanFFTSystem* ocean);

// =============================================================================
// Default configurations
// =============================================================================

/**
 * Get default ocean configuration
 * - 256x256 FFT resolution
 * - 3 cascades
 * - Normal + foam enabled
 */
OceanFFTConfig ocean_fft_default_config(void);

/**
 * Get default spectrum parameters
 * - Wind speed: 20 m/s
 * - Wind direction: (1, 0) normalized
 * - Moderate amplitude
 */
PhillipsSpectrumParams ocean_fft_default_spectrum(void);

#ifdef __cplusplus
}
#endif

#endif // OCEAN_FFT_H
