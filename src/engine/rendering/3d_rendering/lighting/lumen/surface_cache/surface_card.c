/*
 * surface_card.c
 * Surface Card Representation
 *
 * Part of the Lumen GI subsystem
 * Advanced 3D Rendering Engine
 */

#include "surface_card.h"
#include "surface_atlas.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"

/*
 * A Surface Card is a simplified representation of mesh geometry (a plane) 
 * captured from a specific view, stored in the Surface Atlas.
 */

typedef struct surface_card {
    vec3_t center;
    vec3_t normal;
    vec3_t axis_u;
    vec3_t axis_v;
    float extent_u;
    float extent_v;
    
    uint32_t atlas_x;
    uint32_t atlas_y;
    uint32_t resolution_x;
    uint32_t resolution_y;
    
    bool valid;
} surface_card_t;

// Card logic (construction, projection, updating)
// ...

void surface_card_create(vec3_t center, vec3_t normal, float size, surface_card_t* out_card) {
    out_card->center = center;
    out_card->normal = normal;
    // Define basis U, V
    
    out_card->resolution_x = 32; // Default res
    out_card->resolution_y = 32;
    
    // Allocate in atlas
    if (surface_atlas_allocate(32, 32, &out_card->atlas_x, &out_card->atlas_y)) {
        out_card->valid = true;
    } else {
        out_card->valid = false;
    }
}
