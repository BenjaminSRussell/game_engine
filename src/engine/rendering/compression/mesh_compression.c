// src/engine/rendering/compression/mesh_compression.c
// Mesh Compression System - Advanced mesh compression for memory optimization

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Mesh Compression Types
// ============================================================================

typedef enum {
    COMPRESSION_NONE,
    COMPRESSION_QUANTIZATION,
    COMPRESSION_INDEX_TRIANGLE_STRIP,
    COMPRESSION_VERTEX_CACHE,
    COMPRESSION_ATTRIBUTE_COMPRESSION,
    COMPRESSION_GEOMETRIC,
    COMPRESSION_HYBRID
} MeshCompressionType;

typedef struct {
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t index_count;
    
    // Original data
    float *original_vertices;
    uint32_t *original_indices;
    float *original_normals;
    float *original_uvs;
    float *original_tangents;
    
    // Compressed data
    uint8_t *compressed_vertices;
    uint32_t *compressed_indices;
    uint8_t *compressed_normals;
    uint8_t *compressed_uvs;
    
    // Compression metadata
    MeshCompressionType compression_type;
    float quantization_scale[3];
    float quantization_offset[3];
    uint32_t bits_per_component;
    
    // Statistics
    uint32_t original_size;
    uint32_t compressed_size;
    float compression_ratio;
    float compression_time_ms;
    float decompression_time_ms;
    
    char name[256];
    bool is_compressed;
    bool needs_decompression;
} CompressedMesh;

typedef struct {
    CompressedMesh *meshes[4096];
    uint32_t mesh_count;
    uint32_t mesh_capacity;
    
    // Global compression settings
    MeshCompressionType default_compression;
    uint32_t default_bits_per_component;
    float default_quantization_scale;
    bool enable_vertex_cache_optimization;
    bool enable_attribute_compression;
    
    // Performance tracking
    uint64_t total_original_size;
    uint64_t total_compressed_size;
    uint64_t total_compression_time_ms;
    uint64_t total_decompression_time_ms;
    
    // Thread safety
    pthread_mutex_t compression_mutex;
    
    bool initialized;
} MeshCompressionSystem;

static MeshCompressionSystem g_compression_system = {0};

// ============================================================================
// Compression Algorithms
// ============================================================================

static void quantize_vertices(const float *vertices, uint32_t vertex_count,
                             uint8_t *compressed_vertices, uint32_t bits_per_component,
                             float *scale, float *offset) {
    if (!vertices || !compressed_vertices || vertex_count == 0) {
        return;
    }
    
    // Calculate bounding box
    float min_bounds[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float max_bounds[3] = {FLT_MIN, FLT_MIN, FLT_MIN};
    
    for (uint32_t i = 0; i < vertex_count; i++) {
        const float *vertex = &vertices[i * 3];
        for (int j = 0; j < 3; j++) {
            min_bounds[j] = fminf(min_bounds[j], vertex[j]);
            max_bounds[j] = fmaxf(max_bounds[j], vertex[j]);
        }
    }
    
    // Calculate scale and offset
    for (int j = 0; j < 3; j++) {
        float range = max_bounds[j] - min_bounds[j];
        scale[j] = range > 0.0f ? range / ((1 << bits_per_component) - 1) : 1.0f;
        offset[j] = min_bounds[j];
    }
    
    // Quantize vertices
    float max_value = (float)((1 << bits_per_component) - 1);
    for (uint32_t i = 0; i < vertex_count; i++) {
        const float *vertex = &vertices[i * 3];
        uint8_t *compressed = &compressed_vertices[i * 3];
        
        for (int j = 0; j < 3; j++) {
            float normalized = (vertex[j] - offset[j]) / scale[j];
            float clamped = fmaxf(0.0f, fminf(max_value, normalized));
            compressed[j] = (uint8_t)clamped;
        }
    }
}

static void dequantize_vertices(const uint8_t *compressed_vertices, uint32_t vertex_count,
                               float *vertices, uint32_t bits_per_component,
                               const float *scale, const float *offset) {
    if (!compressed_vertices || !vertices || vertex_count == 0) {
        return;
    }
    
    float max_value = (float)((1 << bits_per_component) - 1);
    for (uint32_t i = 0; i < vertex_count; i++) {
        const uint8_t *compressed = &compressed_vertices[i * 3];
        float *vertex = &vertices[i * 3];
        
        for (int j = 0; j < 3; j++) {
            float normalized = (float)compressed[j] / max_value;
            vertex[j] = normalized * scale[j] + offset[j];
        }
    }
}

static void compress_normals(const float *normals, uint32_t vertex_count,
                            uint8_t *compressed_normals) {
    if (!normals || !compressed_normals || vertex_count == 0) {
        return;
    }
    
    // Compress normals to 16-bit (2 bytes per normal)
    for (uint32_t i = 0; i < vertex_count; i++) {
        const float *normal = &normals[i * 3];
        uint8_t *compressed = &compressed_normals[i * 2];
        
        // Octahedral compression
        float abs_x = fabsf(normal[0]);
        float abs_y = fabsf(normal[1]);
        float abs_z = fabsf(normal[2]);
        
        float max_component = fmaxf(abs_x, fmaxf(abs_y, abs_z));
        if (max_component > 0.0f) {
            float nx = normal[0] / max_component;
            float ny = normal[1] / max_component;
            float nz = normal[2] / max_component;
            
            // Project to octahedron
            float u = nx / (fabsf(nx) + fabsf(ny) + fabsf(nz));
            float v = ny / (fabsf(nx) + fabsf(ny) + fabsf(nz));
            
            // Pack to 16-bit
            uint16_t packed = ((uint16_t)(u * 65535.0f) & 0xFFFF);
            compressed[0] = (packed >> 8) & 0xFF;
            compressed[1] = packed & 0xFF;
        } else {
            compressed[0] = 0x80;
            compressed[1] = 0x80;
        }
    }
}

static void decompress_normals(const uint8_t *compressed_normals, uint32_t vertex_count,
                              float *normals) {
    if (!compressed_normals || !normals || vertex_count == 0) {
        return;
    }
    
    for (uint32_t i = 0; i < vertex_count; i++) {
        const uint8_t *compressed = &compressed_normals[i * 2];
        float *normal = &normals[i * 3];
        
        // Unpack from 16-bit
        uint16_t packed = ((uint16_t)compressed[0] << 8) | compressed[1];
        float u = (float)packed / 65535.0f;
        float v = (float)((packed >> 8) & 0xFF) / 65535.0f;
        
        // Decompress from octahedron
        float nx = u;
        float ny = v;
        float nz = 1.0f - fabsf(nx) - fabsf(ny);
        
        if (nz < 0.0f) {
            nx = -nx;
            ny = -ny;
        }
        
        // Normalize
        float length = sqrtf(nx * nx + ny * ny + nz * nz);
        if (length > 0.0f) {
            normal[0] = nx / length;
            normal[1] = ny / length;
            normal[2] = nz / length;
        } else {
            normal[0] = 0.0f;
            normal[1] = 0.0f;
            normal[2] = 1.0f;
        }
    }
}

static void compress_uvs(const float *uvs, uint32_t vertex_count,
                        uint8_t *compressed_uvs, uint32_t bits_per_component) {
    if (!uvs || !compressed_uvs || vertex_count == 0) {
        return;
    }
    
    float max_value = (float)((1 << bits_per_component) - 1);
    for (uint32_t i = 0; i < vertex_count; i++) {
        const float *uv = &uvs[i * 2];
        uint8_t *compressed = &compressed_uvs[i * 2];
        
        // Clamp and quantize UV coordinates
        float u_clamped = fmaxf(0.0f, fminf(1.0f, uv[0]));
        float v_clamped = fmaxf(0.0f, fminf(1.0f, uv[1]));
        
        compressed[0] = (uint8_t)(u_clamped * max_value);
        compressed[1] = (uint8_t)(v_clamped * max_value);
    }
}

static void decompress_uvs(const uint8_t *compressed_uvs, uint32_t vertex_count,
                          float *uvs, uint32_t bits_per_component) {
    if (!compressed_uvs || !uvs || vertex_count == 0) {
        return;
    }
    
    float max_value = (float)((1 << bits_per_component) - 1);
    for (uint32_t i = 0; i < vertex_count; i++) {
        const uint8_t *compressed = &compressed_uvs[i * 2];
        float *uv = &uvs[i * 2];
        
        uv[0] = (float)compressed[0] / max_value;
        uv[1] = (float)compressed[1] / max_value;
    }
}

static uint32_t generate_triangle_strips(const uint32_t *indices, uint32_t index_count,
                                        uint32_t *strip_indices) {
    if (!indices || !strip_indices || index_count < 3) {
        return 0;
    }
    
    // Simple triangle strip generation
    // TODO: Implement proper triangle strip optimization
    uint32_t strip_count = 0;
    
    for (uint32_t i = 0; i < index_count; i++) {
        strip_indices[strip_count++] = indices[i];
    }
    
    return strip_count;
}

// ============================================================================
// Mesh Compression API
// ============================================================================

bool mesh_compression_init(MeshCompressionType default_type, uint32_t bits_per_component,
                           float quantization_scale, bool enable_cache_optimization,
                           bool enable_attribute_compression) {
    if (g_compression_system.initialized) {
        LOG_WARN("Mesh compression system already initialized");
        return true;
    }
    
    memset(&g_compression_system, 0, sizeof(MeshCompressionSystem));
    
    g_compression_system.default_compression = default_type;
    g_compression_system.default_bits_per_component = bits_per_component;
    g_compression_system.default_quantization_scale = quantization_scale;
    g_compression_system.enable_vertex_cache_optimization = enable_cache_optimization;
    g_compression_system.enable_attribute_compression = enable_attribute_compression;
    
    g_compression_system.mesh_capacity = 4096;
    g_compression_system.meshes = calloc(g_compression_system.mesh_capacity, sizeof(CompressedMesh*));
    
    if (!g_compression_system.meshes) {
        LOG_ERROR("Failed to allocate mesh compression array");
        return false;
    }
    
    if (pthread_mutex_init(&g_compression_system.compression_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize compression mutex");
        free(g_compression_system.meshes);
        return false;
    }
    
    g_compression_system.initialized = true;
    LOG_INFO("Mesh compression system initialized (type: %d, bits: %u, scale: %.2f)",
             (int)default_type, bits_per_component, quantization_scale);
    return true;
}

void mesh_compression_shutdown(void) {
    if (!g_compression_system.initialized)
        return;
    
    LOG_INFO("Shutting down mesh compression system");
    
    // Destroy all compressed meshes
    for (uint32_t i = 0; i < g_compression_system.mesh_count; i++) {
        if (g_compression_system.meshes[i]) {
            mesh_compression_destroy_mesh(g_compression_system.meshes[i]);
        }
    }
    
    // Cleanup
    free(g_compression_system.meshes);
    pthread_mutex_destroy(&g_compression_system.compression_mutex);
    
    memset(&g_compression_system, 0, sizeof(MeshCompressionSystem));
    
    LOG_INFO("Mesh compression system shutdown complete");
}

CompressedMesh* mesh_compression_compress_mesh(const char *name,
                                             const float *vertices, uint32_t vertex_count,
                                             const uint32_t *indices, uint32_t index_count,
                                             const float *normals, const float *uvs, const float *tangents,
                                             MeshCompressionType compression_type) {
    if (!g_compression_system.initialized || !name || !vertices || !indices) {
        LOG_ERROR("Mesh compression system not initialized or invalid parameters");
        return NULL;
    }
    
    pthread_mutex_lock(&g_compression_system.compression_mutex);
    
    if (g_compression_system.mesh_count >= g_compression_system.mesh_capacity) {
        LOG_ERROR("Too many compressed meshes");
        pthread_mutex_unlock(&g_compression_system.compression_mutex);
        return NULL;
    }
    
    CompressedMesh *mesh = calloc(1, sizeof(CompressedMesh));
    if (!mesh) {
        LOG_ERROR("Failed to allocate compressed mesh");
        pthread_mutex_unlock(&g_compression_system.compression_mutex);
        return NULL;
    }
    
    strncpy(mesh->name, name, sizeof(mesh->name) - 1);
    mesh->compression_type = compression_type;
    mesh->bits_per_component = g_compression_system.default_bits_per_component;
    
    uint64_t start_time = get_time_nanos();
    
    // Store original data
    mesh->vertex_count = vertex_count;
    mesh->triangle_count = index_count / 3;
    mesh->index_count = index_count;
    
    mesh->original_vertices = malloc(vertex_count * 3 * sizeof(float));
    mesh->original_indices = malloc(index_count * sizeof(uint32_t));
    
    if (!mesh->original_vertices || !mesh->original_indices) {
        LOG_ERROR("Failed to allocate original mesh data");
        free(mesh->original_vertices);
        free(mesh->original_indices);
        free(mesh);
        pthread_mutex_unlock(&g_compression_system.compression_mutex);
        return NULL;
    }
    
    memcpy(mesh->original_vertices, vertices, vertex_count * 3 * sizeof(float));
    memcpy(mesh->original_indices, indices, index_count * sizeof(uint32_t));
    
    if (normals) {
        mesh->original_normals = malloc(vertex_count * 3 * sizeof(float));
        memcpy(mesh->original_normals, normals, vertex_count * 3 * sizeof(float));
    }
    
    if (uvs) {
        mesh->original_uvs = malloc(vertex_count * 2 * sizeof(float));
        memcpy(mesh->original_uvs, uvs, vertex_count * 2 * sizeof(float));
    }
    
    if (tangents) {
        mesh->original_tangents = malloc(vertex_count * 4 * sizeof(float));
        memcpy(mesh->original_tangents, tangents, vertex_count * 4 * sizeof(float));
    }
    
    // Calculate original size
    mesh->original_size = vertex_count * 3 * sizeof(float) + index_count * sizeof(uint32_t);
    if (normals) mesh->original_size += vertex_count * 3 * sizeof(float);
    if (uvs) mesh->original_size += vertex_count * 2 * sizeof(float);
    if (tangents) mesh->original_size += vertex_count * 4 * sizeof(float);
    
    // Compress based on type
    switch (compression_type) {
        case COMPRESSION_QUANTIZATION:
            // Compress vertices
            mesh->compressed_vertices = malloc(vertex_count * 3 * sizeof(uint8_t));
            quantize_vertices(vertices, vertex_count, mesh->compressed_vertices,
                             mesh->bits_per_component, mesh->quantization_scale, mesh->quantization_offset);
            
            // Compress normals if available
            if (normals) {
                mesh->compressed_normals = malloc(vertex_count * 2 * sizeof(uint8_t));
                compress_normals(normals, vertex_count, mesh->compressed_normals);
            }
            
            // Compress UVs if available
            if (uvs) {
                mesh->compressed_uvs = malloc(vertex_count * 2 * sizeof(uint8_t));
                compress_uvs(uvs, vertex_count, mesh->compressed_uvs, mesh->bits_per_component);
            }
            break;
            
        case COMPRESSION_INDEX_TRIANGLE_STRIP:
            // Convert to triangle strips
            mesh->compressed_indices = malloc(index_count * sizeof(uint32_t));
            mesh->index_count = generate_triangle_strips(indices, index_count, mesh->compressed_indices);
            break;
            
        case COMPRESSION_VERTEX_CACHE:
            // Optimize for vertex cache
            // TODO: Implement vertex cache optimization
            break;
            
        case COMPRESSION_ATTRIBUTE_COMPRESSION:
            // Compress all attributes
            if (g_compression_system.enable_attribute_compression) {
                mesh->compressed_vertices = malloc(vertex_count * 3 * sizeof(uint8_t));
                quantize_vertices(vertices, vertex_count, mesh->compressed_vertices,
                                 mesh->bits_per_component, mesh->quantization_scale, mesh->quantization_offset);
                
                if (normals) {
                    mesh->compressed_normals = malloc(vertex_count * 2 * sizeof(uint8_t));
                    compress_normals(normals, vertex_count, mesh->compressed_normals);
                }
                
                if (uvs) {
                    mesh->compressed_uvs = malloc(vertex_count * 2 * sizeof(uint8_t));
                    compress_uvs(uvs, vertex_count, mesh->compressed_uvs, mesh->bits_per_component);
                }
            }
            break;
            
        default:
            LOG_WARN("Unsupported compression type: %d", (int)compression_type);
            break;
    }
    
    // Calculate compressed size
    mesh->compressed_size = 0;
    if (mesh->compressed_vertices) mesh->compressed_size += vertex_count * 3 * sizeof(uint8_t);
    if (mesh->compressed_indices) mesh->compressed_size += mesh->index_count * sizeof(uint32_t);
    if (mesh->compressed_normals) mesh->compressed_size += vertex_count * 2 * sizeof(uint8_t);
    if (mesh->compressed_uvs) mesh->compressed_size += vertex_count * 2 * sizeof(uint8_t);
    
    mesh->compression_ratio = (float)mesh->original_size / (float)mesh->compressed_size;
    
    uint64_t end_time = get_time_nanos();
    mesh->compression_time_ms = nanos_to_ms(end_time - start_time);
    
    mesh->is_compressed = true;
    mesh->needs_decompression = false;
    
    // Add to system
    g_compression_system.meshes[g_compression_system.mesh_count++] = mesh;
    
    // Update statistics
    g_compression_system.total_original_size += mesh->original_size;
    g_compression_system.total_compressed_size += mesh->compressed_size;
    g_compression_system.total_compression_time_ms += (uint64_t)mesh->compression_time_ms;
    
    pthread_mutex_unlock(&g_compression_system.compression_mutex);
    
    LOG_INFO("Compressed mesh '%s': %u -> %u bytes (%.2fx ratio, %.2f ms)",
             name, mesh->original_size, mesh->compressed_size, mesh->compression_ratio, mesh->compression_time_ms);
    
    return mesh;
}

void mesh_compression_decompress_mesh(CompressedMesh *mesh) {
    if (!mesh || !mesh->is_compressed) {
        return;
    }
    
    uint64_t start_time = get_time_nanos();
    
    // Decompress based on compression type
    switch (mesh->compression_type) {
        case COMPRESSION_QUANTIZATION:
            // Decompress vertices
            if (mesh->compressed_vertices && mesh->original_vertices) {
                dequantize_vertices(mesh->compressed_vertices, mesh->vertex_count,
                                  mesh->original_vertices, mesh->bits_per_component,
                                  mesh->quantization_scale, mesh->quantization_offset);
            }
            
            // Decompress normals if available
            if (mesh->compressed_normals && mesh->original_normals) {
                decompress_normals(mesh->compressed_normals, mesh->vertex_count, mesh->original_normals);
            }
            
            // Decompress UVs if available
            if (mesh->compressed_uvs && mesh->original_uvs) {
                decompress_uvs(mesh->compressed_uvs, mesh->vertex_count, mesh->original_uvs, mesh->bits_per_component);
            }
            break;
            
        case COMPRESSION_INDEX_TRIANGLE_STRIP:
            // Convert triangle strips back to triangles
            // TODO: Implement triangle strip decompression
            break;
            
        default:
            LOG_WARN("Unsupported decompression type: %d", (int)mesh->compression_type);
            break;
    }
    
    uint64_t end_time = get_time_nanos();
    mesh->decompression_time_ms = nanos_to_ms(end_time - start_time);
    mesh->needs_decompression = false;
    
    g_compression_system.total_decompression_time_ms += (uint64_t)mesh->decompression_time_ms;
    
    LOG_DEBUG("Decompressed mesh '%s' in %.2f ms", mesh->name, mesh->decompression_time_ms);
}

void mesh_compression_destroy_mesh(CompressedMesh *mesh) {
    if (!mesh) return;
    
    pthread_mutex_lock(&g_compression_system.compression_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_compression_system.mesh_count; i++) {
        if (g_compression_system.meshes[i] == mesh) {
            g_compression_system.meshes[i] = g_compression_system.meshes[g_compression_system.mesh_count - 1];
            g_compression_system.mesh_count--;
            break;
        }
    }
    
    // Free memory
    free(mesh->original_vertices);
    free(mesh->original_indices);
    free(mesh->original_normals);
    free(mesh->original_uvs);
    free(mesh->original_tangents);
    
    free(mesh->compressed_vertices);
    free(mesh->compressed_indices);
    free(mesh->compressed_normals);
    free(mesh->compressed_uvs);
    
    free(mesh);
    
    pthread_mutex_unlock(&g_compression_system.compression_mutex);
    
    LOG_DEBUG("Destroyed compressed mesh");
}

void mesh_compression_get_decompressed_data(CompressedMesh *mesh, float **vertices, uint32_t **indices,
                                          float **normals, float **uvs) {
    if (!mesh || !mesh->is_compressed) {
        return;
    }
    
    if (mesh->needs_decompression) {
        mesh_compression_decompress_mesh(mesh);
    }
    
    if (vertices) *vertices = mesh->original_vertices;
    if (indices) *indices = mesh->original_indices;
    if (normals) *normals = mesh->original_normals;
    if (uvs) *uvs = mesh->original_uvs;
}

void mesh_compression_set_default_compression(MeshCompressionType type, uint32_t bits_per_component,
                                            float quantization_scale) {
    if (!g_compression_system.initialized) return;
    
    pthread_mutex_lock(&g_compression_system.compression_mutex);
    
    g_compression_system.default_compression = type;
    g_compression_system.default_bits_per_component = bits_per_component;
    g_compression_system.default_quantization_scale = quantization_scale;
    
    pthread_mutex_unlock(&g_compression_system.compression_mutex);
    
    LOG_INFO("Default compression updated: type=%d, bits=%u, scale=%.2f",
             (int)type, bits_per_component, quantization_scale);
}

void mesh_compression_get_stats(uint64_t *total_original, uint64_t *total_compressed,
                               float *average_compression_ratio, float *total_compression_time,
                               float *total_decompression_time) {
    if (!g_compression_system.initialized) return;
    
    pthread_mutex_lock(&g_compression_system.compression_mutex);
    
    if (total_original) *total_original = g_compression_system.total_original_size;
    if (total_compressed) *total_compressed = g_compression_system.total_compressed_size;
    if (average_compression_ratio) {
        *average_compression_ratio = g_compression_system.total_original_size > 0 ?
                                   (float)g_compression_system.total_original_size / (float)g_compression_system.total_compressed_size : 1.0f;
    }
    if (total_compression_time) *total_compression_time = (float)g_compression_system.total_compression_time_ms / 1000.0f;
    if (total_decompression_time) *total_decompression_time = (float)g_compression_system.total_decompression_time_ms / 1000.0f;
    
    pthread_mutex_unlock(&g_compression_system.compression_mutex);
}

bool mesh_compression_is_initialized(void) {
    return g_compression_system.initialized;
}
