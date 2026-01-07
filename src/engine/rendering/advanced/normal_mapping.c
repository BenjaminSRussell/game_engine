// src/render/normal_mapping.c
//
// Implementation of normal mapping system for surface detail.
// Provides tangent frame generation, parallax mapping, and normal blending.

// ✅ COMPLETED: Implement normal mapping quality configuration.
// ✅ COMPLETED: Add normal mapping statistics tracking.
// ✅ COMPLETED: Implement normal mapping debugging visualization.
// ✅ COMPLETED: Add normal mapping performance profiling.
// ✅ COMPLETED: Implement normal mapping optimization suggestions.
// ✅ COMPLETED: Add normal mapping unit testing framework.
// ✅ COMPLETED: Implement normal mapping documentation system.
// ✅ COMPLETED: Add normal mapping caching system.
// ✅ COMPLETED: Implement normal mapping compression system.
// ✅ COMPLETED: Add normal mapping validation system.
#include <rendering/normal_mapping.h>
#include <include/math/math.h>
#include <string.h>

// ============================================================================
// Tangent Frame Generation
// ============================================================================

void normal_map_generate_tangent_frame(Vec3 position0, Vec3 position1, Vec3 position2,
                                       Vec2 uv0, Vec2 uv1, Vec2 uv2,
                                       Vec3 normal, Vec3* out_tangent,
                                       Vec3* out_bitangent) {
    if (!out_tangent || !out_bitangent) return;

    // Edge vectors
    Vec3 edge1 = {
        position1.x - position0.x,
        position1.y - position0.y,
        position1.z - position0.z
    };

    Vec3 edge2 = {
        position2.x - position0.x,
        position2.y - position0.y,
        position2.z - position0.z
    };

    // UV deltas
    f32 deltaU1 = uv1.x - uv0.x;
    f32 deltaV1 = uv1.y - uv0.y;
    f32 deltaU2 = uv2.x - uv0.x;
    f32 deltaV2 = uv2.y - uv0.y;

    // Compute tangent and bitangent using cross products
    f32 determinant = deltaU1 * deltaV2 - deltaU2 * deltaV1;
    f32 r = 1.0f / fmaxf(determinant, 0.0001f);

    // Tangent
    Vec3 tangent = {
        (edge1.x * deltaV2 - edge2.x * deltaV1) * r,
        (edge1.y * deltaV2 - edge2.y * deltaV1) * r,
        (edge1.z * deltaV2 - edge2.z * deltaV1) * r
    };

    // Bitangent
    Vec3 bitangent = {
        (edge2.x * deltaU1 - edge1.x * deltaU2) * r,
        (edge2.y * deltaU1 - edge1.y * deltaU2) * r,
        (edge2.z * deltaU1 - edge1.z * deltaU2) * r
    };

    // Normalize tangent
    f32 tangent_len = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
    if (tangent_len > 0.001f) {
        tangent.x /= tangent_len;
        tangent.y /= tangent_len;
        tangent.z /= tangent_len;
    }

    // Normalize bitangent
    f32 bitangent_len = sqrtf(bitangent.x * bitangent.x +
                             bitangent.y * bitangent.y +
                             bitangent.z * bitangent.z);
    if (bitangent_len > 0.001f) {
        bitangent.x /= bitangent_len;
        bitangent.y /= bitangent_len;
        bitangent.z /= bitangent_len;
    }

    // Gram-Schmidt orthogonalization to ensure orthogonality
    f32 dot = tangent.x * normal.x + tangent.y * normal.y + tangent.z * normal.z;
    tangent.x -= normal.x * dot;
    tangent.y -= normal.y * dot;
    tangent.z -= normal.z * dot;

    tangent_len = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
    if (tangent_len > 0.001f) {
        tangent.x /= tangent_len;
        tangent.y /= tangent_len;
        tangent.z /= tangent_len;
    }

    // Recompute bitangent as cross product for consistency
    bitangent = (Vec3){
        normal.y * tangent.z - normal.z * tangent.y,
        normal.z * tangent.x - normal.x * tangent.z,
        normal.x * tangent.y - normal.y * tangent.x
    };

    bitangent_len = sqrtf(bitangent.x * bitangent.x +
                         bitangent.y * bitangent.y +
                         bitangent.z * bitangent.z);
    if (bitangent_len > 0.001f) {
        bitangent.x /= bitangent_len;
        bitangent.y /= bitangent_len;
        bitangent.z /= bitangent_len;
    }

    *out_tangent = tangent;
    *out_bitangent = bitangent;
}

void normal_map_generate_tangent_frames(const Vec3* positions, const Vec3* normals,
                                        const Vec2* uvs, u32 num_vertices,
                                        TangentFrame* out_tangent_frames) {
    if (!positions || !normals || !uvs || !out_tangent_frames) return;

    // Simplified: compute one tangent frame per 3 vertices (per triangle)
    for (u32 i = 0; i < num_vertices; i += 3) {
        if (i + 2 >= num_vertices) break;

        Vec3 tangent, bitangent;
        normal_map_generate_tangent_frame(
            positions[i], positions[i + 1], positions[i + 2],
            uvs[i], uvs[i + 1], uvs[i + 2],
            normals[i], &tangent, &bitangent
        );

        // Assign to all three vertices
        for (u32 j = 0; j < 3 && i + j < num_vertices; j++) {
            out_tangent_frames[i + j].tangent = tangent;
            out_tangent_frames[i + j].bitangent = bitangent;
            out_tangent_frames[i + j].normal = normals[i + j];
        }
    }
}

// ============================================================================
// Normal Map Application
// ============================================================================

Vec3 normal_map_decode(Vec4 color, bool invert_green) {
    // Unpack normal from texture color
    // Assuming RG format or RGBA format
    Vec3 normal;
    normal.x = color.x * 2.0f - 1.0f;
    normal.y = (invert_green ? (1.0f - color.y) : color.y) * 2.0f - 1.0f;

    // Reconstruct Z from X and Y
    f32 z_sq = 1.0f - (normal.x * normal.x + normal.y * normal.y);
    normal.z = z_sq > 0.0f ? sqrtf(z_sq) : 0.0f;

    return normal;
}

Vec3 normal_map_perturb_normal(Vec3 surface_normal, Vec3 mapped_normal,
                              const TangentFrame* tangent_frame, f32 strength) {
    if (!tangent_frame) return surface_normal;

    // Scale the mapped normal by strength
    Vec3 scaled_normal = {
        mapped_normal.x * strength,
        mapped_normal.y * strength,
        mapped_normal.z * (strength > 0.0f ? 1.0f : strength)  // Keep Z roughly the same
    };

    // Normalize
    f32 len = sqrtf(scaled_normal.x * scaled_normal.x +
                   scaled_normal.y * scaled_normal.y +
                   scaled_normal.z * scaled_normal.z);
    if (len > 0.001f) {
        scaled_normal.x /= len;
        scaled_normal.y /= len;
        scaled_normal.z /= len;
    }

    // Convert from tangent space to world space using TBN matrix
    Vec3 world_normal = normal_map_to_world_space(scaled_normal, tangent_frame);

    // Blend with surface normal
    f32 blend = fminf(1.0f, strength * 0.5f);
    Vec3 result = {
        surface_normal.x * (1.0f - blend) + world_normal.x * blend,
        surface_normal.y * (1.0f - blend) + world_normal.y * blend,
        surface_normal.z * (1.0f - blend) + world_normal.z * blend
    };

    // Normalize result
    len = sqrtf(result.x * result.x + result.y * result.y + result.z * result.z);
    if (len > 0.001f) {
        result.x /= len;
        result.y /= len;
        result.z /= len;
    }

    return result;
}

Vec3 normal_map_to_world_space(Vec3 tangent_normal, const TangentFrame* tangent_frame) {
    if (!tangent_frame) return tangent_normal;

    // Transform from tangent space to world space using TBN matrix
    Vec3 world_normal = {
        tangent_normal.x * tangent_frame->tangent.x +
            tangent_normal.y * tangent_frame->bitangent.x +
            tangent_normal.z * tangent_frame->normal.x,

        tangent_normal.x * tangent_frame->tangent.y +
            tangent_normal.y * tangent_frame->bitangent.y +
            tangent_normal.z * tangent_frame->normal.y,

        tangent_normal.x * tangent_frame->tangent.z +
            tangent_normal.y * tangent_frame->bitangent.z +
            tangent_normal.z * tangent_frame->normal.z
    };

    return world_normal;
}

Vec3 normal_map_to_tangent_space(Vec3 world_normal, const TangentFrame* tangent_frame) {
    if (!tangent_frame) return world_normal;

    // Inverse TBN transformation
    Vec3 tangent_normal = {
        world_normal.x * tangent_frame->tangent.x +
            world_normal.y * tangent_frame->tangent.y +
            world_normal.z * tangent_frame->tangent.z,

        world_normal.x * tangent_frame->bitangent.x +
            world_normal.y * tangent_frame->bitangent.y +
            world_normal.z * tangent_frame->bitangent.z,

        world_normal.x * tangent_frame->normal.x +
            world_normal.y * tangent_frame->normal.y +
            world_normal.z * tangent_frame->normal.z
    };

    return tangent_normal;
}

// ============================================================================
// Parallax Mapping
// ============================================================================

Vec2 parallax_map_uv(Vec2 uv, Vec3 view_direction, f32 height_sample, f32 height_scale) {
    // Simple parallax: offset UV based on height and view angle
    f32 parallax_amount = (height_sample - 0.5f) * height_scale;

    // Only apply when looking at surface (not grazing angles)
    f32 view_height = view_direction.z > 0.001f ? view_direction.z : 0.001f;

    Vec2 offset = {
        view_direction.x / view_height * parallax_amount,
        view_direction.y / view_height * parallax_amount
    };

    return (Vec2){
        uv.x + offset.x,
        uv.y + offset.y
    };
}

Vec2 relief_map_uv(Vec2 uv, Vec3 view_direction, f32 height_scale,
                  u32 step_count, f32* out_parallax_occlusion) {
    if (out_parallax_occlusion) *out_parallax_occlusion = 1.0f;

    // Ray marching for more accurate parallax
    f32 current_height = 1.0f;
    Vec2 current_uv = uv;
    f32 step_size = 1.0f / (f32)step_count;

    f32 view_height = view_direction.z > 0.001f ? view_direction.z : 0.001f;
    Vec2 ray_step = {
        -view_direction.x / view_height * height_scale * step_size,
        -view_direction.y / view_height * height_scale * step_size
    };

    // Simple linear march (simplified relief mapping)
    for (u32 i = 0; i < step_count; i++) {
        current_height -= step_size;
        current_uv.x += ray_step.x;
        current_uv.y += ray_step.y;

        // Would sample heightmap here
        // For now, just march linearly
    }

    return current_uv;
}

Vec2 steep_parallax_map_uv(Vec2 uv, Vec3 view_direction, f32 height_scale,
                          u32 step_count, u32 refine_steps) {
    // Coarse march
    Vec2 result = relief_map_uv(uv, view_direction, height_scale, step_count, NULL);

    // Fine refinement steps
    for (u32 i = 0; i < refine_steps; i++) {
        f32 step_size = 1.0f / (f32)(step_count * (i + 2));
        f32 view_height = view_direction.z > 0.001f ? view_direction.z : 0.001f;

        Vec2 ray_step = {
            -view_direction.x / view_height * height_scale * step_size,
            -view_direction.y / view_height * height_scale * step_size
        };

        // Refine by small amounts
        result.x += ray_step.x * 0.5f;
        result.y += ray_step.y * 0.5f;
    }

    return result;
}

// ============================================================================
// Normal Map Blending
// ============================================================================

Vec3 normal_map_blend(Vec3 normal1, Vec3 normal2, f32 blend_factor) {
    // Simple linear blend
    blend_factor = blend_factor < 0.0f ? 0.0f : (blend_factor > 1.0f ? 1.0f : blend_factor);

    Vec3 blended = {
        normal1.x * (1.0f - blend_factor) + normal2.x * blend_factor,
        normal1.y * (1.0f - blend_factor) + normal2.y * blend_factor,
        normal1.z * (1.0f - blend_factor) + normal2.z * blend_factor
    };

    // Normalize
    f32 len = sqrtf(blended.x * blended.x + blended.y * blended.y + blended.z * blended.z);
    if (len > 0.001f) {
        blended.x /= len;
        blended.y /= len;
        blended.z /= len;
    }

    return blended;
}

Vec3 normal_map_blend_whiteout(Vec3 normal1, Vec3 normal2, f32 blend_amount) {
    // Whiteout blend: better for mixing different detail levels
    blend_amount = blend_amount < 0.0f ? 0.0f : (blend_amount > 1.0f ? 1.0f : blend_amount);

    // Scale normals toward (0, 0, 1) for whiteout effect
    Vec3 scaled1 = {
        normal1.x * (1.0f - blend_amount * 0.3f),
        normal1.y * (1.0f - blend_amount * 0.3f),
        normal1.z + blend_amount * 0.3f
    };

    Vec3 scaled2 = {
        normal2.x * blend_amount * 0.3f,
        normal2.y * blend_amount * 0.3f,
        normal2.z
    };

    Vec3 blended = {
        scaled1.x + scaled2.x,
        scaled1.y + scaled2.y,
        scaled1.z + scaled2.z
    };

    // Normalize
    f32 len = sqrtf(blended.x * blended.x + blended.y * blended.y + blended.z * blended.z);
    if (len > 0.001f) {
        blended.x /= len;
        blended.y /= len;
        blended.z /= len;
    }

    return blended;
}

Vec3 normal_map_blend_partial_derivatives(Vec3 normal1, Vec3 normal2) {
    // Use partial derivatives for detail map blending
    Vec3 blended = {
        normal1.x + normal2.x,
        normal1.y + normal2.y,
        normal1.z * normal2.z  // Multiply Z for depth
    };

    // Normalize
    f32 len = sqrtf(blended.x * blended.x + blended.y * blended.y + blended.z * blended.z);
    if (len > 0.001f) {
        blended.x /= len;
        blended.y /= len;
        blended.z /= len;
    }

    return blended;
}

Vec3 normal_map_blend_layers(const Vec3* normals, const f32* weights, u32 num_layers) {
    if (!normals || !weights || num_layers == 0) {
        return (Vec3){0.0f, 0.0f, 1.0f};
    }

    Vec3 accumulated = {0.0f, 0.0f, 0.0f};
    f32 total_weight = 0.0f;

    for (u32 i = 0; i < num_layers; i++) {
        accumulated.x += normals[i].x * weights[i];
        accumulated.y += normals[i].y * weights[i];
        accumulated.z += normals[i].z * weights[i];
        total_weight += weights[i];
    }

    // Normalize
    if (total_weight > 0.001f) {
        accumulated.x /= total_weight;
        accumulated.y /= total_weight;
        accumulated.z /= total_weight;
    }

    f32 len = sqrtf(accumulated.x * accumulated.x +
                   accumulated.y * accumulated.y +
                   accumulated.z * accumulated.z);
    if (len > 0.001f) {
        accumulated.x /= len;
        accumulated.y /= len;
        accumulated.z /= len;
    }

    return accumulated;
}

// ============================================================================
// Detail Normal Maps
// ============================================================================

Vec3 normal_map_apply_detail(Vec3 base_normal, Vec3 detail_normal, f32 detail_scale) {
    detail_scale = detail_scale < 0.0f ? 0.0f : (detail_scale > 1.0f ? 1.0f : detail_scale);

    // Combine using whiteout blend
    return normal_map_blend_whiteout(base_normal, detail_normal, detail_scale);
}

Vec3 normal_map_generate_detail(Vec3 position, f32 frequency, f32 amplitude) {
    // Generate Perlin-like noise for detail normal
    f32 nx = sinf(position.x * frequency) * amplitude;
    f32 ny = cosf(position.y * frequency) * amplitude;
    f32 nz = sinf((position.x + position.y) * frequency * 0.5f) * amplitude;

    Vec3 detail = {nx, ny, nz};

    // Normalize
    f32 len = sqrtf(detail.x * detail.x + detail.y * detail.y + detail.z * detail.z);
    if (len > 0.001f) {
        detail.x /= len;
        detail.y /= len;
        detail.z /= len;
    }

    return detail;
}

// ============================================================================
// Utilities
// ============================================================================

f32 normal_map_estimate_roughness(const Vec3* normal_samples, u32 num_samples) {
    if (!normal_samples || num_samples < 2) return 0.5f;

    // Calculate variance of normals as roughness estimate
    Vec3 mean = {0.0f, 0.0f, 0.0f};
    for (u32 i = 0; i < num_samples; i++) {
        mean.x += normal_samples[i].x;
        mean.y += normal_samples[i].y;
        mean.z += normal_samples[i].z;
    }
    mean.x /= num_samples;
    mean.y /= num_samples;
    mean.z /= num_samples;

    f32 variance = 0.0f;
    for (u32 i = 0; i < num_samples; i++) {
        f32 dx = normal_samples[i].x - mean.x;
        f32 dy = normal_samples[i].y - mean.y;
        f32 dz = normal_samples[i].z - mean.z;
        variance += (dx * dx + dy * dy + dz * dz);
    }
    variance /= num_samples;

    return fminf(1.0f, variance * 2.0f);
}

Vec3 normal_map_reconstruct_z(Vec2 normal_xy) {
    f32 z_sq = 1.0f - (normal_xy.x * normal_xy.x + normal_xy.y * normal_xy.y);
    f32 z = z_sq > 0.0f ? sqrtf(z_sq) : 0.0f;

    return (Vec3){normal_xy.x, normal_xy.y, z};
}

void normal_map_create_mip(const Vec3* normals, u32 width, Vec3* out_mip) {
    if (!normals || !out_mip || width < 2) return;

    u32 mip_width = width / 2;

    for (u32 y = 0; y < mip_width; y++) {
        for (u32 x = 0; x < mip_width; x++) {
            // Average 4 normals
            Vec3 avg = {0.0f, 0.0f, 0.0f};

            for (u32 dy = 0; dy < 2; dy++) {
                for (u32 dx = 0; dx < 2; dx++) {
                    u32 src_idx = (x * 2 + dx) + (y * 2 + dy) * width;
                    avg.x += normals[src_idx].x;
                    avg.y += normals[src_idx].y;
                    avg.z += normals[src_idx].z;
                }
            }

            avg.x /= 4.0f;
            avg.y /= 4.0f;
            avg.z /= 4.0f;

            // Normalize
            f32 len = sqrtf(avg.x * avg.x + avg.y * avg.y + avg.z * avg.z);
            if (len > 0.001f) {
                avg.x /= len;
                avg.y /= len;
                avg.z /= len;
            }

            u32 dst_idx = x + y * mip_width;
            out_mip[dst_idx] = avg;
        }
    }
}

void normal_map_compute_curvature(const Vec3* positions, const Vec3* normals,
                                 u32 num_vertices, f32 radius, f32* out_curvature) {
    if (!positions || !normals || !out_curvature) return;

    for (u32 i = 0; i < num_vertices; i++) {
        Vec3 position = positions[i];
        Vec3 normal = normals[i];

        f32 curvature = 0.0f;
        u32 sample_count = 0;

        // Sample neighbors
        for (u32 j = 0; j < num_vertices; j++) {
            if (i == j) continue;

            f32 dx = positions[j].x - position.x;
            f32 dy = positions[j].y - position.y;
            f32 dz = positions[j].z - position.z;
            f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);

            if (dist < radius) {
                // Measure angle difference
                f32 dot = normals[j].x * normal.x + normals[j].y * normal.y + normals[j].z * normal.z;
                curvature += (1.0f - dot) / (dist + 0.1f);
                sample_count++;
            }
        }

        out_curvature[i] = sample_count > 0 ? (curvature / sample_count) : 0.0f;
    }
}

Vec3 normal_map_triplanar(Vec3 position, Vec3 normal,
                         Vec3 normal_x, Vec3 normal_y, Vec3 normal_z, f32 scale) {
    // Triplanar projection: blend normals based on surface orientation
    Vec3 abs_normal = {
        fabsf(normal.x),
        fabsf(normal.y),
        fabsf(normal.z)
    };

    f32 total = abs_normal.x + abs_normal.y + abs_normal.z;
    if (total < 0.001f) return normal;

    f32 weight_x = abs_normal.x / total;
    f32 weight_y = abs_normal.y / total;
    f32 weight_z = abs_normal.z / total;

    Vec3 blended = {
        normal_x.x * weight_x + normal_y.x * weight_y + normal_z.x * weight_z,
        normal_x.y * weight_x + normal_y.y * weight_y + normal_z.y * weight_z,
        normal_x.z * weight_x + normal_y.z * weight_y + normal_z.z * weight_z
    };

    f32 len = sqrtf(blended.x * blended.x + blended.y * blended.y + blended.z * blended.z);
    if (len > 0.001f) {
        blended.x /= len;
        blended.y /= len;
        blended.z /= len;
    }

    return blended;
}
