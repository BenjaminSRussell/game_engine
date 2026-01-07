#include "rendering/environment/ibl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <include/math/math.h>

/**
 * IBL System Implementation
 */

#define MAX_IBL_PROBES 64
#define MAX_BLENDED_PROBES 4

struct IBLSystem {
    IBLProbe probes[MAX_IBL_PROBES];
    uint32_t probe_count;
    uint32_t next_id;
    
    uint32_t global_probe_id;
};

// Create IBL system
IBLSystem* ibl_system_create(void) {
    IBLSystem* system = calloc(1, sizeof(IBLSystem));
    if (!system) return NULL;
    
    system->probe_count = 0;
    system->next_id = 1;
    system->global_probe_id = 0;
    
    printf("[IBL] Created IBL system\n");
    return system;
}

// Destroy IBL system
void ibl_system_destroy(IBLSystem* system) {
    if (!system) return;
    free(system);
    printf("[IBL] Destroyed IBL system\n");
}

// Add probe
uint32_t ibl_system_add_probe(IBLSystem* system) {
    if (!system || system->probe_count >= MAX_IBL_PROBES) {
        printf("[IBL] ERROR: Cannot add more probes\n");
        return 0;
    }
    
    IBLProbe* probe = &system->probes[system->probe_count];
    memset(probe, 0, sizeof(IBLProbe));
    
    probe->id = system->next_id++;
    probe->intensity = 1.0f;
    probe->max_mip_level = 5.0f;
    probe->blend_distance = 5.0f;
    probe->priority = 0.5f;
    probe->use_parallax_correction = false;
    
    system->probe_count++;
    
    printf("[IBL] Added probe ID: %u\n", probe->id);
    return probe->id;
}

// Remove probe
void ibl_system_remove_probe(IBLSystem* system, uint32_t probe_id) {
    if (!system) return;
    
    for (uint32_t i = 0; i < system->probe_count; i++) {
        if (system->probes[i].id == probe_id) {
            memmove(&system->probes[i], &system->probes[i + 1],
                   (system->probe_count - i - 1) * sizeof(IBLProbe));
            system->probe_count--;
            printf("[IBL] Removed probe ID: %u\n", probe_id);
            return;
        }
    }
}

// Get probe
IBLProbe* ibl_system_get_probe(IBLSystem* system, uint32_t probe_id) {
    if (!system) return NULL;
    
    for (uint32_t i = 0; i < system->probe_count; i++) {
        if (system->probes[i].id == probe_id) {
            return &system->probes[i];
        }
    }
    
    return NULL;
}

// Configuration
void ibl_probe_set_position(IBLProbe* probe, float x, float y, float z) {
    if (!probe) return;
    probe->position[0] = x;
    probe->position[1] = y;
    probe->position[2] = z;
}

void ibl_probe_set_box(IBLProbe* probe, const float* min, const float* max) {
    if (!probe || !min || !max) return;
    
    memcpy(probe->box_min, min, 3 * sizeof(float));
    memcpy(probe->box_max, max, 3 * sizeof(float));
    probe->use_parallax_correction = true;
}

void ibl_probe_set_cubemaps(IBLProbe* probe, uint32_t irradiance, 
                            uint32_t prefiltered, uint32_t brdf_lut) {
    if (!probe) return;
    probe->irradiance_map = irradiance;
    probe->prefiltered_env_map = prefiltered;
    probe->brdf_lut = brdf_lut;
}

void ibl_probe_set_intensity(IBLProbe* probe, float intensity) {
    if (!probe) return;
    probe->intensity = intensity;
}

// Probe capture (stubs - would integrate with actual rendering)
void ibl_probe_capture(IBLProbe* probe, const float* position) {
    if (!probe) return;
    printf("[IBL] Capturing probe %u at (%.2f, %.2f, %.2f)\n",
           probe->id, position[0], position[1], position[2]);
    // TODO: Render 6 cube faces from this position
}

void ibl_probe_convolve_diffuse(IBLProbe* probe) {
    if (!probe) return;
    printf("[IBL] Convolving diffuse irradiance for probe %u\n", probe->id);
    // TODO: Generate irradiance map via convolution
}

void ibl_probe_prefilter_specular(IBLProbe* probe) {
    if (!probe) return;
    printf("[IBL] Prefiltering specular for probe %u\n", probe->id);
    // TODO: Generate prefiltered environment map with mip chain
}

// Probe blending
static float probe_weight_for_position(const IBLProbe* probe, const float* position) {
    // Calculate distance from position to probe
    float dx = position[0] - probe->position[0];
    float dy = position[1] - probe->position[1];
    float dz = position[2] - probe->position[2];
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    
    // Weight based on distance and blend distance
    if (dist > probe->blend_distance) {
        return 0.0f;
    }
    
    float weight = 1.0f - (dist / probe->blend_distance);
    return weight * probe->priority;
}

void ibl_system_get_blended_probes(const IBLSystem* system, const float* position,
                                   IBLProbe** out_probes, float* out_weights,
                                   uint32_t max_probes, uint32_t* out_count) {
    if (!system || !position || !out_probes || !out_weights || !out_count) return;
    
    *out_count = 0;
    
    // Calculate weights for all probes
    for (uint32_t i = 0; i < system->probe_count && *out_count < max_probes; i++) {
        const IBLProbe* probe = &system->probes[i];
        float weight = probe_weight_for_position(probe, position);
        
        if (weight > 0.0f) {
            out_probes[*out_count] = (IBLProbe*)probe;
            out_weights[*out_count] = weight;
            (*out_count)++;
        }
    }
    
    // Normalize weights
    if (*out_count > 0) {
        float total_weight = 0.0f;
        for (uint32_t i = 0; i < *out_count; i++) {
            total_weight += out_weights[i];
        }
        
        if (total_weight > 0.0f) {
            for (uint32_t i = 0; i < *out_count; i++) {
                out_weights[i] /= total_weight;
            }
        }
    }
}

// Global probe
void ibl_system_set_global_probe(IBLSystem* system, uint32_t probe_id) {
    if (!system) return;
    system->global_probe_id = probe_id;
    printf("[IBL] Set global probe: %u\n", probe_id);
}

IBLProbe* ibl_system_get_global_probe(IBLSystem* system) {
    if (!system || system->global_probe_id == 0) return NULL;
    return ibl_system_get_probe(system, system->global_probe_id);
}

// Apply IBL lighting (simplified - would be done in shader)
void ibl_apply_lighting(float* color, const float* normal, const float* view_dir,
                       float roughness, float metallic, const float* f0,
                       const IBLProbe* probe) {
    if (!color || !normal || !view_dir || !probe) return;
    
    // This is a simplified CPU version
    // Real implementation would sample cubemaps in shader
    
    // Diffuse IBL (irradiance)
    float ambient_diffuse[3] = {0.3f, 0.3f, 0.4f}; // Placeholder
    
    // Specular IBL (prefiltered environment)
    float ambient_specular[3] = {0.2f, 0.2f, 0.3f}; // Placeholder
    
    // Apply intensity
    for (int i = 0; i < 3; i++) {
        color[i] += (ambient_diffuse[i] + ambient_specular[i]) * probe->intensity;
    }
}
