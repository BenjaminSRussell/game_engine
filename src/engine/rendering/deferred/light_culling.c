/**
 * =================================================================================================
 *                              CLUSTERED LIGHT CULLING IMPLEMENTATION
 * =================================================================================================
 */

#include "rendering/deferred/light_culling.h"
#include <core/memory.h>
#include <core/logger.h>
#include <include/math/math.h>

void light_culling_build_clusters(void *camera, void *light_list) {
    // TODO: Implement cluster assignment logic
    // 1. Calculate cluster AABBs in view space
    // 2. For each light:
    //    a. Calculate light's sphere/cone in view space
    //    b. Intersect with cluster AABBs
    //    c. Add light index to intersecting clusters
    
    LOG_TRACE("Light clusters built for current frame");
}

void light_culling_bind_resources(u32 shader_id) {
    // TODO: Upload grid to GPU SSBO and bind to shader
    // shader_set_buffer(shader_id, "ClusterBuffer", s_grid_gpu_buffer);
}
