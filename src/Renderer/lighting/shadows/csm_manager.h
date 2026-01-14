/*
 * csm_manager.h
 * Cascaded Shadow Mapping Manager
 * High-level orchestration of shadow systems
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CSM_MANAGER_H
#define LIGHTING_CSM_MANAGER_H

#include "lighting/shadows/shadow_atlas.h"
#include "lighting/cascades/cascade_resolution.h"
#include "lighting/shadows/cascade_splits.h"
#include "lighting/cascades/cascade_stabilization.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct csm_config {
    uint32_t cascade_count;         // Typically 4
    float shadow_distance;          // Max shadow distance
    float cascade_split_lambda;     // 0.5 = hybrid log/uniform
    float shadow_bias_const;        // Constant bias
    float shadow_bias_slope;        // Slope-scale bias
    float blend_region;             // 0.0-0.5 fraction
    cascade_quality_preset_t quality;
    bool stabilize_cascades;
    bool debug_visualize;
} csm_config_t;

typedef struct csm_manager {
    csm_config_t config;
    lighting_shadow_atlas_handle_t atlas;
    
    // Runtime data
    cascade_split_info_t splits[4];  // Limited to 4 cascades for now
    float cascade_resolutions[4];
    float cascade_distances[4];
    
    bool initialized;
} csm_manager_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int csm_manager_init(csm_manager_t* manager, const csm_config_t* config);
void csm_manager_shutdown(csm_manager_t* manager);

/* Update Logic */
void csm_manager_update(csm_manager_t* manager, 
                        const cascade_camera_t* camera, 
                        const float* light_dir);

/* Render Access */
void csm_manager_get_cascade_info(const csm_manager_t* manager, 
                                 uint32_t cascade_index, 
                                 float* out_view_proj,
                                 float* out_split_near, 
                                 float* out_split_far);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CSM_MANAGER_H */
