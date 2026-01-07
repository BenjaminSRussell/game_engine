/*
 * pcss_filter.c
 * PCSS (Percentage Closer Soft Shadows) logic
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/shadows/pcss_filter.h"
#include <include/math/math.h>

/*
 * PCSS Algorithm steps:
 * 1. Blocker Search: Find average depth of blockers in a search region.
 * 2. Penumbra Estimation: Calculate penumbra size based on blocker depth.
 * 3. Filtering: Perform PCF with kernel size dependent on penumbra.
 */

float pcss_blocker_search_radius(float light_size, float receiver_depth) {
    // Search radius depends on light size and depth
    // r_search = w_light * (d_receiver - d_near) / d_receiver
    // Simplified:
    return light_size * 0.1f / receiver_depth; 
}

float pcss_penumbra_size(float receiver_depth, float blocker_depth, float light_size) {
    if (blocker_depth < 0.001f) return 0.0f; // No blocker
    
    // w_penumbra = (d_receiver - d_blocker) * w_light / d_blocker
    return (receiver_depth - blocker_depth) * light_size / blocker_depth;
}
