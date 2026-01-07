/*
 * probe_parallax.c
 * Probe parallax correction implementation
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/probes/probe_parallax.h"
#include <include/math/math.h>

simd_float3 probe_parallax_intersect_aabb(simd_float3 ray_origin, simd_float3 ray_dir, simd_float3 box_min, simd_float3 box_max) {
    // Standard SLAB intersection algorithm
    simd_float3 inv_dir = 1.0f / ray_dir;
    simd_float3 t_bottom = inv_dir * (box_min - ray_origin);
    simd_float3 t_top = inv_dir * (box_max - ray_origin);
    
    simd_float3 t_min = simd_min(t_top, t_bottom);
    simd_float3 t_max = simd_max(t_top, t_bottom);
    
    // Final t_max of intersection
    float t = simd_min(simd_min(t_max.x, t_max.y), t_max.z);
    
    return ray_origin + ray_dir * t;
}

simd_float3 probe_parallax_box_projection(simd_float3 world_pos, simd_float3 reflection_dir, simd_float3 probe_pos, simd_float3 box_min, simd_float3 box_max) {
    // 1. Find the intersection of the ray (from world_pos along reflection_dir) with the bounding box
    simd_float3 intersect_pos = probe_parallax_intersect_aabb(world_pos, reflection_dir, box_min, box_max);
    
    // 2. The corrected sampling vector is from the probe center to the intersection point
    simd_float3 corrected_vector = intersect_pos - probe_pos;
    
    return corrected_vector;
}
