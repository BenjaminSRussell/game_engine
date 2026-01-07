#include "cluster_builder.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Morton code (z-order curve) utilities for 3D coordinates
// Expands a 10-bit integer into 30 bits by inserting 2 zeros after each bit.
static uint32_t expand_bits(uint32_t v) {
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

static uint32_t morton_3d(float x, float y, float z) {
    // Quantize 0..1 to 0..1023
    x = fminf(fmaxf(x * 1024.0f, 0.0f), 1023.0f);
    y = fminf(fmaxf(y * 1024.0f, 0.0f), 1023.0f);
    z = fminf(fmaxf(z * 1024.0f, 0.0f), 1023.0f);
    uint32_t xx = expand_bits((uint32_t)x);
    uint32_t yy = expand_bits((uint32_t)y);
    uint32_t zz = expand_bits((uint32_t)z);
    return xx * 4 + yy * 2 + zz;
}

typedef struct {
    uint32_t tri_index;
    uint32_t morton_code;
} tri_sort_entry_t;

static int compare_tri_sort(const void* a, const void* b) {
    const tri_sort_entry_t* ta = (const tri_sort_entry_t*)a;
    const tri_sort_entry_t* tb = (const tri_sort_entry_t*)b;
    return (ta->morton_code < tb->morton_code) ? -1 : (ta->morton_code > tb->morton_code);
}

static void compute_bounds(const vertex_t* vertices, const uint32_t* indices, uint32_t index_offset, uint32_t index_count, simd_float3* out_center, simd_float3* out_extent) {
    simd_float3 min_b = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
    simd_float3 max_b = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};

    for (uint32_t i = 0; i < index_count; i++) {
        uint32_t idx = indices[index_offset + i];
        simd_float3 p = vertices[idx].position; // Assuming vertex_t has position
        min_b = simd_min(min_b, p);
        max_b = simd_max(max_b, p);
    }

    *out_center = (min_b + max_b) * 0.5f;
    *out_extent = (max_b - min_b) * 0.5f;
}

cluster_mesh_t* cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t* vertices, uint32_t vertex_count,
                                   const uint32_t* indices, uint32_t index_count) {
    
    // 1. Compute global bounds to normalize for Morton codes
    simd_float3 global_min = {MAXFLOAT, MAXFLOAT, MAXFLOAT};
    simd_float3 global_max = {-MAXFLOAT, -MAXFLOAT, -MAXFLOAT};
    for(uint32_t i = 0; i < vertex_count; i++) {
        global_min = simd_min(global_min, vertices[i].position);
        global_max = simd_max(global_max, vertices[i].position);
    }
    simd_float3 global_size = global_max - global_min;
    simd_float3 inv_size = 1.0f / (global_size + 1e-6f); // Avoid div by zero

    // 2. Prepare sorting
    uint32_t tri_count = index_count / 3;
    tri_sort_entry_t* sort_entries = (tri_sort_entry_t*)malloc(sizeof(tri_sort_entry_t) * tri_count);
    
    for(uint32_t i = 0; i < tri_count; i++) {
        // Compute centroid
        uint32_t idx0 = indices[i * 3 + 0];
        uint32_t idx1 = indices[i * 3 + 1];
        uint32_t idx2 = indices[i * 3 + 2];
        simd_float3 c = (vertices[idx0].position + vertices[idx1].position + vertices[idx2].position) * 0.333333f;
        
        // Normalize
        simd_float3 norm_c = (c - global_min) * inv_size;
        sort_entries[i].tri_index = i;
        sort_entries[i].morton_code = morton_3d(norm_c.x, norm_c.y, norm_c.z);
    }

    // 3. Sort triangles
    qsort(sort_entries, tri_count, sizeof(tri_sort_entry_t), compare_tri_sort);

    // 4. Cluster generation
    uint32_t cluster_count = (tri_count + CLUSTER_TRIANGLE_COUNT - 1) / CLUSTER_TRIANGLE_COUNT;
    mesh_cluster_t* clusters = (mesh_cluster_t*)calloc(cluster_count, sizeof(mesh_cluster_t));
    
    // Create new index buffer sorted by clusters
    uint32_t* sorted_indices = (uint32_t*)malloc(sizeof(uint32_t) * index_count);
    
    // We could optimize vertex layout too (vertex reuse), but for now just copy indices
    // and rely on the original VB. Or we should build a new Compact VB per cluster?
    // The prompt struct has `vertex_offset` per cluster. This implies vertices are partitioned or we use a global VB with offsets?
    // If we use global VB, `vertex_offset` might mean base vertex.
    // For simplicity, let's keep the global VB and just reorder indices. 
    // Wait, `vertex_offset` usually usually used for `drawIndexedPrimitives: ... baseVertex:`. 
    // If vertices are not reordered, baseVertex is useless unless clusters are disjoint in vertex space.
    // Nanite usually builds local vertex buffers. 
    // Let's assume we just use the global VB for now (V1) or reorder if needed.
    // To match "vertex_offset", we should probably repack vertices.
    
    // Simplified: Just reorder indices. `vertex_offset` = 0.
    
    for(uint32_t c = 0; c < cluster_count; c++) {
        uint32_t tri_start = c * CLUSTER_TRIANGLE_COUNT;
        uint32_t tri_end = tri_start + CLUSTER_TRIANGLE_COUNT;
        if (tri_end > tri_count) tri_end = tri_count;
        
        uint32_t c_tri_count = tri_end - tri_start;
        clusters[c].triangle_count = c_tri_count;
        clusters[c].index_offset = tri_start * 3;
        clusters[c].vertex_offset = 0; // Global buffer
        clusters[c].lod_error = 0.01f; // Placeholder error
        clusters[c].parent_cluster = 0xFFFFFFFF; // Root
        clusters[c].child_count = 0;
        
        // Copy indices
        for(uint32_t t = 0; t < c_tri_count; t++) {
            uint32_t src_tri = sort_entries[tri_start + t].tri_index;
            sorted_indices[(tri_start + t) * 3 + 0] = indices[src_tri * 3 + 0];
            sorted_indices[(tri_start + t) * 3 + 1] = indices[src_tri * 3 + 1];
            sorted_indices[(tri_start + t) * 3 + 2] = indices[src_tri * 3 + 2];
        }
        
        // Compute cluster bounds
        compute_bounds(vertices, sorted_indices, clusters[c].index_offset, c_tri_count * 3, &clusters[c].bounds_center, &clusters[c].bounds_extent);
    }

    // 5. Create GPU buffers
    cluster_mesh_t* mesh = (cluster_mesh_t*)malloc(sizeof(cluster_mesh_t));
    mesh->cluster_count = cluster_count;
    mesh->total_triangles = tri_count;
    
    mesh->vertex_buffer = [device newBufferWithBytes:vertices length:vertex_count * sizeof(vertex_t) options:MTLResourceStorageModeShared];
    mesh->index_buffer = [device newBufferWithBytes:sorted_indices length:index_count * sizeof(uint32_t) options:MTLResourceStorageModeShared];
    mesh->cluster_buffer = [device newBufferWithBytes:clusters length:cluster_count * sizeof(mesh_cluster_t) options:MTLResourceStorageModeShared];

    // Cleanup
    free(sort_entries);
    free(sorted_indices);
    free(clusters);
    
    return mesh;
}

void cluster_mesh_free(cluster_mesh_t* mesh) {
    if (mesh) {
        [mesh->vertex_buffer release];
        [mesh->index_buffer release];
        [mesh->cluster_buffer release];
        free(mesh);
    }
}
