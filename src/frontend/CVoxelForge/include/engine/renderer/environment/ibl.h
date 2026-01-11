#ifndef IBL_H
#define IBL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Image-Based Lighting (IBL) System
 * Environment probes for realistic ambient lighting
 */

// IBL probe
typedef struct {
    uint32_t id;
    
    // Position and bounds
    float position[3];
    float box_min[3];  // For parallax correction
    float box_max[3];
    bool use_parallax_correction;
    
    // Cubemaps
    uint32_t irradiance_map;        // Diffuse convolution
    uint32_t prefiltered_env_map;   // Specular with mip levels
    uint32_t brdf_lut;              // BRDF integration lookup
    
    // Settings
    float intensity;
    float max_mip_level;  // For specular
    
    // Blending
    float blend_distance;  // Distance over which to blend with other probes
    float priority;        // Higher priority probes override lower ones
} IBLProbe;

// IBL system
typedef struct IBLSystem IBLSystem;

// Initialization
IBLSystem* ibl_system_create(void);
void ibl_system_destroy(IBLSystem* system);

// Probe management
uint32_t ibl_system_add_probe(IBLSystem* system);
void ibl_system_remove_probe(IBLSystem* system, uint32_t probe_id);
IBLProbe* ibl_system_get_probe(IBLSystem* system, uint32_t probe_id);

// Probe configuration
void ibl_probe_set_position(IBLProbe* probe, float x, float y, float z);
void ibl_probe_set_box(IBLProbe* probe, const float* min, const float* max);
void ibl_probe_set_cubemaps(IBLProbe* probe, uint32_t irradiance, uint32_t prefiltered, uint32_t brdf_lut);
void ibl_probe_set_intensity(IBLProbe* probe, float intensity);

// Probe capture (generate cubemaps)
void ibl_probe_capture(IBLProbe* probe, const float* position);
void ibl_probe_convolve_diffuse(IBLProbe* probe);   // Generate irradiance map
void ibl_probe_prefilter_specular(IBLProbe* probe); // Generate prefiltered env map

// Probe blending
void ibl_system_update_blending(IBLSystem* system, const float* camera_pos);
void ibl_system_get_blended_probes(const IBLSystem* system, const float* position,
                                   IBLProbe** out_probes, float* out_weights,
                                   uint32_t max_probes, uint32_t* out_count);

// Global fallback probe
void ibl_system_set_global_probe(IBLSystem* system, uint32_t probe_id);
IBLProbe* ibl_system_get_global_probe(IBLSystem* system);

// Rendering helpers
void ibl_apply_lighting(float* color, const float* normal, const float* view_dir,
                       float roughness, float metallic, const float* f0,
                       const IBLProbe* probe);

#endif // IBL_H
