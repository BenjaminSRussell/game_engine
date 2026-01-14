// lighting/lumen/lumen_gi.h
// Lumen Global Illumination - Real-time GI with probe-based radiance cache
#ifndef LUMEN_GI_H
#define LUMEN_GI_H

#include "../../common.h"
#include "../../math/vec3.h"
#include "../../rendering/metal_raytracing.h"
#include <Metal/Metal.h>

#define LUMEN_PROBE_GRID_RESOLUTION 64
#define LUMEN_PROBE_RAYS_PER_PROBE 256
#define LUMEN_RADIANCE_CACHE_SIZE 128

// Radiance probe for caching indirect lighting
typedef struct {
  Vec3 position;
  f32 radius;
  Vec3 irradiance[6]; // Spherical harmonics coefficients (simplified)
  f32 last_update_time;
} LumenProbe;

// Surface cache entry for secondary bounces
typedef struct {
  Vec3 position;
  Vec3 normal;
  Vec3 albedo;
  Vec3 emissive;
  f32 roughness;
  f32 metallic;
} LumenSurfaceCache;

typedef struct LumenGISystem LumenGISystem;

#ifdef __cplusplus
extern "C" {
#endif

// System lifecycle
LumenGISystem *lumen_create(id<MTLDevice> device, u32 scene_width,
                            u32 scene_height, u32 scene_depth);
void lumen_destroy(LumenGISystem *lumen);

// Per-frame update
void lumen_update(LumenGISystem *lumen, id<MTLCommandBuffer> cmd,
                  const Vec3 *camera_pos, f32 delta_time);

// Generate GI for a frame
void lumen_trace_gi(LumenGISystem *lumen, id<MTLCommandBuffer> cmd,
                    id<MTLTexture> depth, id<MTLTexture> normals,
                    id<MTLTexture> albedo, id<MTLTexture> output_diffuse,
                    id<MTLTexture> output_specular);

// Surface cache update (for dynamic objects)
void lumen_update_surface_cache(LumenGISystem *lumen, const Vec3 *positions,
                                const Vec3 *normals, const Vec3 *albedos,
                                u32 surface_count);

// Probe placement (automatic based on geometry)
void lumen_place_probes_automatic(LumenGISystem *lumen,
                                  const void *scene_geometry);

// Set hardware ray tracing scene structure
void lumen_set_raytracing_scene(LumenGISystem *lumen,
                                MetalRTAccelStructure *scene);

#ifdef __cplusplus
}
#endif

#endif // LUMEN_GI_H
