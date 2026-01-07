#include "effects/fire/fire_effects.h"
#include <stdio.h>

// Distortion parameters
#define DISTORTION_RADIUS_SCALE 1.5f

void apply_heat_distortion(Vec3 position, float intensity) {
    // In a deferred renderer or post-process chain, this would invoke a compute shader
    // or draw a distortion volume into a low-res buffer.
    
    // TODO: Submit distortion volume to render queue
    // render_queue_push_distortion(position, intensity * DISTORTION_RADIUS_SCALE);
    
    // Placeholder logic
    // printf("Applying heat distortion at (%.2f, %.2f, %.2f) with intensity %.2f\n", 
    //        position.x, position.y, position.z, intensity);
}
