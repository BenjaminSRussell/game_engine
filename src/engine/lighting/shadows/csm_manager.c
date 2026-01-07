/*
 * csm_manager.c
 * Cascaded Shadow Mapping Manager
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/shadows/csm_manager.h"
#include "lighting/cascades/cascade_selection.h"
#include "lighting/cascades/cascade_blending.h"
#include <string.h>

int csm_manager_init(csm_manager_t* manager, const csm_config_t* config) {
    if (!manager || !config) return -1;
    
    manager->config = *config;
    
    // Create/Configure Atlas
    lighting_shadow_atlas_init();
    
    lighting_shadow_atlas_desc_t atlas_desc = {0};
    atlas_desc.width = 4096; // Default to 4K for pool
    atlas_desc.height = 4096;
    atlas_desc.format = LIGHTING_SHADOW_FORMAT_DEPTH32;
    atlas_desc.cascade_count = config->cascade_count;
    
    int result = lighting_shadow_atlas_create(&manager->atlas, &atlas_desc);
    if (result != 0) return result;
    
    manager->initialized = true;
    return 0;
}

void csm_manager_shutdown(csm_manager_t* manager) {
    if (!manager || !manager->initialized) return;
    
    lighting_shadow_atlas_destroy(manager->atlas);
    // lighting_shadow_atlas_shutdown(); // Global shutdown usually handled by system
    
    manager->initialized = false;
}

void csm_manager_update(csm_manager_t* manager, 
                        const cascade_camera_t* camera, 
                        const float* light_dir) {
    if (!manager || !manager->initialized || !camera || !light_dir) return;
    
    // 1. Determine resolutions based on quality
    uint32_t resolutions[4];
    cascade_apply_quality_preset(manager->config.quality, resolutions, manager->config.cascade_count);
    
    // 2. Calculate Splits
    cascade_splits_calculate(camera, 
                            light_dir, 
                            manager->config.cascade_count, 
                            manager->config.shadow_distance, 
                            manager->config.cascade_split_lambda, 
                            manager->splits);
    
    // 3. Stabilize Cascades
    if (manager->config.stabilize_cascades) {
        for (uint32_t i = 0; i < manager->config.cascade_count; i++) {
            cascade_stabilize_matrix(manager->splits[i].view_proj, resolutions[i]);
        }
    }
    
    // 4. Update Internal State
    for (uint32_t i = 0; i < manager->config.cascade_count; i++) {
        manager->cascade_resolutions[i] = (float)resolutions[i];
    }
}

void csm_manager_get_cascade_info(const csm_manager_t* manager, 
                                 uint32_t cascade_index, 
                                 float* out_view_proj,
                                 float* out_split_near, 
                                 float* out_split_far) {
    if (!manager || !manager->initialized || cascade_index >= manager->config.cascade_count) return;
    
    if (out_view_proj) {
        memcpy(out_view_proj, manager->splits[cascade_index].view_proj, sizeof(float) * 16);
    }
    
    if (out_split_near) *out_split_near = manager->splits[cascade_index].split_near;
    if (out_split_far) *out_split_far = manager->splits[cascade_index].split_far;
}
