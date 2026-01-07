/*
 * sw_rasterizer.c
 * Software Rasterizer for Occlusion Culling
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#include "sw_rasterizer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SWR_WIDTH 256
#define SWR_HEIGHT 128

typedef struct sw_rasterizer_ctx {
    float* depth_buffer;
    uint32_t width;
    uint32_t height;
} sw_rasterizer_ctx_t;

static sw_rasterizer_ctx_t g_swr = {0};

int sw_rasterizer_init(void) {
    g_swr.width = SWR_WIDTH;
    g_swr.height = SWR_HEIGHT;
    g_swr.depth_buffer = (float*)malloc(SWR_WIDTH * SWR_HEIGHT * sizeof(float));
    return 0;
}

void sw_rasterizer_clear(void) {
    if (g_swr.depth_buffer) {
        // Init to 1.0 (far plane)
        for(int i=0; i<g_swr.width * g_swr.height; i++) g_swr.depth_buffer[i] = 1.0f;
    }
}

static inline void rasterize_triangle(float* v0, float* v1, float* v2) {
    // Bounding box of triangle
    // For each pixel in bbox:
    //   Barycentric coords
    //   Interpolate Z
    //   Depth Test & Write
}

void sw_rasterizer_rasterize_mesh(const float* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count) {
    for (uint32_t i = 0; i < index_count; i += 3) {
        // rasterize_triangle(...)
    }
}

bool sw_rasterizer_test_aabb(float min_x, float min_y, float max_x, float max_y, float min_z) {
    // Check pixels in AABB screen rect
    // If any pixel has depth > min_z, AABB is occluded? No, visibility check
    // If ANY pixel has stored_depth > min_z, then AT LEAST SOME part is visible (assuming standard depth test)
    // Conservative culling: If ALL pixels covered by AABB have stored_depth < min_z, then AABB is occluded.
    
    return true; 
}

void sw_rasterizer_shutdown(void) {
    free(g_swr.depth_buffer);
    g_swr.depth_buffer = NULL;
}
