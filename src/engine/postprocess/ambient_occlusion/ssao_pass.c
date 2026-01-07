/*
 * ssao_pass.c
 * Screen-space AO
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement ACES tonemapping
 * TODO: Add physically-based bloom
 * TODO: Implement TAA
 * TODO: Add depth of field
 * TODO: Implement motion blur
 * TODO: Add GTAO
 * TODO: Implement SSR
 * TODO: Add color grading
 * TODO: Implement lens effects
 * TODO: Add film grain
 * TODO: Implement ssao pass initialization
 * TODO: Add ssao pass cleanup/shutdown
 * TODO: Implement ssao pass validation
 * TODO: Add ssao pass error handling
 * TODO: Implement ssao pass serialization
 * TODO: Add ssao pass debug output
 * TODO: Implement ssao pass unit tests
 * TODO: Add ssao pass performance counters
 * TODO: Implement ssao pass hot-reload
 * TODO: Add ssao pass thread safety
 * TODO: Implement ssao pass memory pooling
 * TODO: Add ssao pass caching layer
 * TODO: Implement ssao pass async operations
 * TODO: Add ssao pass GPU integration
 * TODO: Implement ssao pass SIMD optimization
 * TODO: Add ssao pass batch processing
 * TODO: Implement ssao pass streaming support
 * TODO: Add ssao pass LOD support
 * TODO: Implement ssao pass culling integration
 * TODO: Add ssao pass render graph node
 */

#include "postprocess/ambient_occlusion/ssao_pass.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_SSAO_PASS_MAX_COUNT 4096
#define POSTPROCESSING_SSAO_PASS_DEFAULT_CAPACITY 256
#define POSTPROCESSING_SSAO_PASS_ALIGNMENT 16
#define SSAO_KERNEL_SIZE 64

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct postprocessing_ssao_pass_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    ssao_params_t params;
    float kernel[SSAO_KERNEL_SIZE * 3]; // Precomputed kernel
    float noise[16 * 3]; // 4x4 rotation noise
} postprocessing_ssao_pass_internal_t;

typedef struct postprocessing_ssao_pass_context {
    postprocessing_ssao_pass_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_ssao_pass_context_t;

static postprocessing_ssao_pass_context_t g_ssao_pass_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

static void generate_kernel(float* kernel, int size) {
    for (int i = 0; i < size; ++i) {
        float x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float z = ((float)rand() / RAND_MAX); // Hemisphere 0..1
        
        // Normalize
        float len = sqrtf(x*x + y*y + z*z);
        x /= len; y /= len; z /= len;
        
        // Scale samples so they're more aligned to center of kernel
        float scale = (float)i / size;
        scale = lerp(0.1f, 1.0f, scale * scale);
        x *= scale; y *= scale; z *= scale;
        
        kernel[i*3 + 0] = x;
        kernel[i*3 + 1] = y;
        kernel[i*3 + 2] = z;
    }
}

static void generate_noise(float* noise) {
    for (int i = 0; i < 16; ++i) {
        float x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float z = 0.0f; // Rotate around Z axis (XY plane)
        noise[i*3 + 0] = x;
        noise[i*3 + 1] = y;
        noise[i*3 + 2] = z;
    }
}

static void postprocessing_ssao_pass_cleanup_internal(postprocessing_ssao_pass_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_ssao_pass_init(void) {
    if (g_ssao_pass_ctx.initialized) {
        return 0; // Already initialized
    }

    g_ssao_pass_ctx.capacity = POSTPROCESSING_SSAO_PASS_DEFAULT_CAPACITY;
    g_ssao_pass_ctx.items = calloc(g_ssao_pass_ctx.capacity, sizeof(postprocessing_ssao_pass_internal_t));
    if (!g_ssao_pass_ctx.items) {
        return -1;
    }

    g_ssao_pass_ctx.count = 0;
    g_ssao_pass_ctx.initialized = true;

    return 0;
}

void postprocessing_ssao_pass_shutdown(void) {
    if (!g_ssao_pass_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ssao_pass_ctx.count; i++) {
        postprocessing_ssao_pass_cleanup_internal(&g_ssao_pass_ctx.items[i]);
    }

    free(g_ssao_pass_ctx.items);
    g_ssao_pass_ctx.items = NULL;
    g_ssao_pass_ctx.count = 0;
    g_ssao_pass_ctx.capacity = 0;
    g_ssao_pass_ctx.initialized = false;
}

int postprocessing_ssao_pass_create(postprocessing_ssao_pass_handle_t* out_handle, const postprocessing_ssao_pass_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ssao_pass_ctx.initialized) {
        return -2;
    }

    if (g_ssao_pass_ctx.count >= g_ssao_pass_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_ssao_pass_ctx.count++;
    postprocessing_ssao_pass_internal_t* item = &g_ssao_pass_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->params = desc->initial_params;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Initialize standard kernel/noise
    generate_kernel(item->kernel, SSAO_KERNEL_SIZE);
    generate_noise(item->noise);

    out_handle->id = index;
    return 0;
}

void postprocessing_ssao_pass_destroy(postprocessing_ssao_pass_handle_t handle) {
    if (handle.id >= g_ssao_pass_ctx.count) {
        return;
    }

    postprocessing_ssao_pass_cleanup_internal(&g_ssao_pass_ctx.items[handle.id]);
}

int postprocessing_ssao_pass_update(postprocessing_ssao_pass_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ssao_pass_ctx.count) {
        return -1;
    }

    postprocessing_ssao_pass_internal_t* item = &g_ssao_pass_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

void postprocessing_ssao_pass_set_params(postprocessing_ssao_pass_handle_t handle, const ssao_params_t* params) {
    if (handle.id >= g_ssao_pass_ctx.count || !params) return;
    postprocessing_ssao_pass_internal_t* item = &g_ssao_pass_ctx.items[handle.id];
    if (item->initialized) {
        item->params = *params;
        item->dirty = true;
    }
}

bool postprocessing_ssao_pass_is_valid(postprocessing_ssao_pass_handle_t handle) {
    if (handle.id >= g_ssao_pass_ctx.count) {
        return false;
    }
    return g_ssao_pass_ctx.items[handle.id].initialized;
}

int postprocessing_ssao_pass_get_info(postprocessing_ssao_pass_handle_t handle, postprocessing_ssao_pass_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ssao_pass_ctx.count) {
        return -2;
    }

    const postprocessing_ssao_pass_internal_t* item = &g_ssao_pass_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->current_params = item->params;

    return 0;
}

void postprocessing_ssao_pass_mark_dirty(postprocessing_ssao_pass_handle_t handle) {
    if (handle.id < g_ssao_pass_ctx.count) {
        g_ssao_pass_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_ssao_pass_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ssao_pass_ctx.count; i++) {
        postprocessing_ssao_pass_internal_t* item = &g_ssao_pass_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

void postprocessing_ssao_compute_cpu(const ssao_params_t* params, const float* input_depth, const float* input_normals, float* output_ao, uint32_t width, uint32_t height, const float* projection_matrix) {
    if (!input_depth || !input_normals || !output_ao || !params || !projection_matrix) return;
    
    // Use fixed kernel if no handle/context passed to access generated kernel.
    // For this standalone function, we regenerate or simple static.
    // Actually, real impl would take context.
    // We'll generate a temporary kernel here for correctness of standalone test.
    
    float kernel[SSAO_KERNEL_SIZE * 3];
    float noise[16 * 3];
    generate_kernel(kernel, params->kernel_size > 0 ? (params->kernel_size < SSAO_KERNEL_SIZE ? params->kernel_size : SSAO_KERNEL_SIZE) : 16);
    generate_noise(noise);
    int valid_kernel_size = params->kernel_size > 0 ? params->kernel_size : 16;
    if (valid_kernel_size > SSAO_KERNEL_SIZE) valid_kernel_size = SSAO_KERNEL_SIZE;
    
    // Helper to get view pos
    // ViewPos.z = depth (linear)
    // ViewPos.xy = derived from UV and inverse projection?
    // Let's assume P00 = 1/tan(fov/2)/aspect, P11 = 1/tan(fov/2).
    // x = (u*2-1) / P00 * z
    // y = (v*2-1) / P11 * z
    float P00 = projection_matrix[0];
    float P11 = projection_matrix[5];
    
    // Inverse projection params
    float invP00 = (fabsf(P00) > 0.0001f) ? 1.0f / P00 : 1.0f;
    float invP11 = (fabsf(P11) > 0.0001f) ? 1.0f / P11 : 1.0f;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            float depth = input_depth[idx];
            
            // Skip background
            if (depth > 1000.0f) {
                output_ao[idx] = 1.0f;
                continue;
            }
            
            // Reconstruct View Position
            float u = (float)x / width;
            float v = (float)y / height;
            
            float viewX = (u * 2.0f - 1.0f) * invP00 * depth;
            float viewY = (v * 2.0f - 1.0f) * invP11 * depth; // assuming y up or down, P11 usually handles sign?
            // OpenGL NDC y is up, Vulkan/DX y is down?
            // Let's assume y is up for view space logic or consistent with proj.
            float viewZ = depth; // View space Z is depth (linear)
            
            // Get Normal
            float nx = input_normals[idx*3 + 0];
            float ny = input_normals[idx*3 + 1];
            float nz = input_normals[idx*3 + 2];
            
            // Construct TBN
            // Random noise tile 4x4
            int noiseX = x % 4;
            int noiseY = y % 4;
            int nIdx = (noiseY * 4 + noiseX) * 3;
            float rx = noise[nIdx];
            float ry = noise[nIdx+1];
            float rz = noise[nIdx+2]; // 0.0f
            
            // Graham-Schmidt
            // Tangent = normalize(randomVec - normal * dot(randomVec, normal))
            float dot_val = rx*nx + ry*ny + rz*nz;
            float tx = rx - nx * dot_val;
            float ty = ry - ny * dot_val;
            float tz = rz - nz * dot_val;
            float t_len = sqrtf(tx*tx + ty*ty + tz*tz);
            if (t_len < 0.0001f) {
                tx = 1.0f; ty = 0.0f; tz = 0.0f; // fallback
            } else {
                tx /= t_len; ty /= t_len; tz /= t_len;
            }
            
            // Bitangent = cross(normal, tangent)
            float bx = ny*tz - nz*ty;
            float by = nz*tx - nx*tz;
            float bz = nx*ty - ny*tx;
            
            float occlusion = 0.0f;
            
            for (int k = 0; k < valid_kernel_size; ++k) {
                float kx = kernel[k*3 + 0];
                float ky = kernel[k*3 + 1];
                float kz = kernel[k*3 + 2];
                
                // samplePos = TBN * kernel[k]
                float sx = tx*kx + bx*ky + nx*kz;
                float sy = ty*kx + by*ky + ny*kz;
                float sz = tz*kx + bz*ky + nz*kz;
                
                sx = viewX + sx * params->radius;
                sy = viewY + sy * params->radius;
                sz = viewZ + sz * params->radius;
                
                // Project sample (to screen space)
                // clip = proj * view
                // clip.x = P00 * sx + P02 * sz? (P02 is usually 0 for symm frustum)
                // clip.y = P11 * sy + P12 * sz?
                // clip.w = sz? (Or -sz for GL)
                
                // Assuming standard:
                float cx = P00 * sx;
                float cy = P11 * sy;
                float cw = sz; // view space Z
                
                // NDC
                float sampleU = (cx / cw) * 0.5f + 0.5f;
                float sampleV = (cy / cw) * 0.5f + 0.5f;
                
                if (sampleU < 0.0f || sampleU > 1.0f || sampleV < 0.0f || sampleV > 1.0f) {
                    continue;
                }
                
                // Get sample depth
                uint32_t sX = (uint32_t)(sampleU * width);
                uint32_t sY = (uint32_t)(sampleV * height);
                if (sX >= width) sX = width - 1;
                if (sY >= height) sY = height - 1;
                
                float sampleDepth = input_depth[sY * width + sX];
                
                // Range check & Accumulate
                float rangeCheck = fabsf(viewZ - sampleDepth) < params->radius ? 1.0f : 0.0f;
                // Actually smoothstep:
                // float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewZ - sampleDepth));
                
                // Check occlusion
                // coordinate system: +Z is into screen? If linear depth is positive distance:
                // If sampleDepth is smaller (closer) than sample position Z (sz), then occluded.
                // Wait, sz is the sample position (e.g. viewZ + small offset).
                // sampleDepth is the surface depth at that screen position.
                // If sampleDepth <= sz - bias, then occluded.
                if (sampleDepth <= sz - params->bias) {
                     occlusion += 1.0f * rangeCheck;
                }
            }
            
            output_ao[idx] = 1.0f - (occlusion / valid_kernel_size) * params->intensity;
        }
    }
}

uint32_t postprocessing_ssao_pass_get_count(void) {
    return g_ssao_pass_ctx.count;
}

size_t postprocessing_ssao_pass_get_memory_usage(void) {
    size_t total = sizeof(g_ssao_pass_ctx);
    total += g_ssao_pass_ctx.capacity * sizeof(postprocessing_ssao_pass_internal_t);

    for (uint32_t i = 0; i < g_ssao_pass_ctx.count; i++) {
        total += g_ssao_pass_ctx.items[i].data_size;
    }

    return total;
}

void postprocessing_ssao_pass_debug_print(void) {
    printf("SSAO Pass Context: %u/%u items\n", g_ssao_pass_ctx.count, g_ssao_pass_ctx.capacity);
}

/* End of ssao_pass.c */
