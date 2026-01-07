/*
 * surface_capture.c
 * Surface Cache Capture and Update
 *
 * Part of the Lumen GI subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/lumen/surface_cache/surface_capture.h"
#include <stdint.h>

/*
 * Captures the Albedo, Normal, and Emission of the mesh geometry covered by a card
 * into the Surface Atlas. This is essentially "rasterizing" the scene onto the cards.
 */

void surface_capture_execute(void* card_list, uint32_t card_count) {
    // 1. Setup GPU Rasterizer
    // 2. Set RenderTarget -> Surface Atlas
    // 3. For each card:
    //    - Set Viewport (atlas_x, atlas_y, w, h)
    //    - Set View/Proj matrix (looking at card from normal direction)
    //    - Draw Mesh (associated with this card)
}
