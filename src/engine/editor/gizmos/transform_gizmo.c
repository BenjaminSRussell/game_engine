#include <simd/simd.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

typedef enum gizmo_mode {
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE
} gizmo_mode_t;

typedef enum gizmo_space {
    GIZMO_SPACE_LOCAL,
    GIZMO_SPACE_WORLD
} gizmo_space_t;

typedef struct transform_gizmo {
    gizmo_mode_t mode;
    gizmo_space_t space;
    simd_float3 position;
    simd_quatf rotation;
    simd_float3 scale;
    int active_axis;           // -1 = none, 0=X, 1=Y, 2=Z, 3=XY, etc.
    bool is_dragging;
    
    // Internal drag state
    simd_float3 drag_start_pos;
    simd_float3 initial_obj_pos;
    simd_quatf initial_obj_rot;
    simd_float3 initial_obj_scale;
} transform_gizmo_t;

// Forward decls
int gizmo_pick(transform_gizmo_t* gizmo, simd_float3 ray_origin, simd_float3 ray_dir);

void gizmo_render(id encoder_ptr, 
                  transform_gizmo_t* gizmo, 
                  const simd_float4x4* view, 
                  const simd_float4x4* proj) {
#ifdef __OBJC__
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>)encoder_ptr;
    // Implementation would queue 3 axis lines using debug renderer or custom mesh
    // For now we simulate logic hook
    // debug_draw_line(..., gizmo->position, gizmo->position + right...)
#endif
}

// Intersect ray with cylinder/axis
// Returns axis index or -1
int gizmo_pick(transform_gizmo_t* gizmo, simd_float3 ray_origin, simd_float3 ray_dir) {
    if (!gizmo) return -1;
    
    // Simplified sphere test for origin
    simd_float3 diff = gizmo->position - ray_origin;
    float t = simd_dot(diff, ray_dir);
    simd_float3 closest = ray_origin + ray_dir * t;
    float dist = simd_distance(closest, gizmo->position);
    
    if (dist < 0.5f) { // Arbitrary radius
        return 0; // Hit X for example
    }
    
    return -1;
}

void gizmo_drag(transform_gizmo_t* gizmo, simd_float3 ray_origin, simd_float3 ray_dir, simd_float4x4* out_transform) {
    if (!gizmo || !gizmo->is_dragging) return;
    
    // Logic to project ray onto axis/plane and update gizmo->position/rotation/scale
    // Then rebuild matrix
    
    // *out_transform = ...
}
