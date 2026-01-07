/*
 * vertex_format.c
 * Flexible vertex attribute layout definition and management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 *
 * Supports:
 * - Dynamic vertex format descriptors
 * - Interleaved and separate vertex streams
 * - Multiple component types and semantic layouts
 * - Format validation and stride calculation
 * - Query interfaces for attribute discovery
 */

#include "geometry/vertex/vertex_format.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GEOMETRY_VERTEX_FORMAT_MAX_COUNT 4096
#define GEOMETRY_VERTEX_FORMAT_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_vertex_format_internal {
    geometry_vertex_format_handle_t handle;
    geometry_vertex_format_desc_t desc;
    vertex_stream_binding_t* streams;  // Copied streams
    uint32_t stream_count;
    uint32_t total_attribute_count;
    uint32_t flags;
    bool initialized;
    uint64_t frame_updated;
} geometry_vertex_format_internal_t;

typedef struct geometry_vertex_format_context {
    geometry_vertex_format_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_vertex_format_context_t;

static geometry_vertex_format_context_t g_vertex_format_ctx = {0};

/* ============================================================================
 * COMPONENT TYPE UTILITIES
 * ============================================================================ */

uint32_t geometry_vertex_component_get_size(vertex_component_type_t type) {
    static const uint32_t sizes[VERTEX_TYPE_COUNT] = {
        4,  // VERTEX_TYPE_FLOAT32
        2,  // VERTEX_TYPE_FLOAT16
        4,  // VERTEX_TYPE_INT32
        4,  // VERTEX_TYPE_UINT32
        2,  // VERTEX_TYPE_INT16
        2,  // VERTEX_TYPE_UINT16
        1,  // VERTEX_TYPE_INT8
        1,  // VERTEX_TYPE_UINT8
        1,  // VERTEX_TYPE_SNORM8
        1,  // VERTEX_TYPE_UNORM8
        2,  // VERTEX_TYPE_SNORM16
        2,  // VERTEX_TYPE_UNORM16
    };
    if (type >= VERTEX_TYPE_COUNT) return 0;
    return sizes[type];
}

bool geometry_vertex_component_is_normalized_type(vertex_component_type_t type) {
    return type == VERTEX_TYPE_SNORM8 || type == VERTEX_TYPE_UNORM8 ||
           type == VERTEX_TYPE_SNORM16 || type == VERTEX_TYPE_UNORM16;
}

const char* geometry_vertex_component_get_name(vertex_component_type_t type) {
    static const char* names[VERTEX_TYPE_COUNT] = {
        "FLOAT32",
        "FLOAT16",
        "INT32",
        "UINT32",
        "INT16",
        "UINT16",
        "INT8",
        "UINT8",
        "SNORM8",
        "UNORM8",
        "SNORM16",
        "UNORM16",
    };
    if (type >= VERTEX_TYPE_COUNT) return "UNKNOWN";
    return names[type];
}

const char* geometry_vertex_semantic_get_name(vertex_attribute_semantic_t semantic) {
    static const char* names[VERTEX_SEMANTIC_COUNT] = {
        "POSITION",
        "NORMAL",
        "TANGENT",
        "BINORMAL",
        "TEXCOORD",
        "COLOR",
        "BONE_INDEX",
        "BONE_WEIGHT",
        "CUSTOM",
    };
    if (semantic >= VERTEX_SEMANTIC_COUNT) return "UNKNOWN";
    return names[semantic];
}

uint32_t geometry_vertex_format_calculate_stride(const vertex_attribute_t* attributes, uint32_t attribute_count) {
    if (!attributes || attribute_count == 0) return 0;

    uint32_t max_offset = 0;
    uint32_t max_size = 0;

    for (uint32_t i = 0; i < attribute_count; i++) {
        uint32_t attr_size = geometry_vertex_component_get_size(attributes[i].component_type);
        uint32_t attr_end = attributes[i].offset + attr_size * attributes[i].component_count;
        if (attr_end > max_offset) {
            max_offset = attr_end;
        }
    }

    return max_offset;
}

/* ============================================================================
 * COMMON FORMAT HELPERS
 * ============================================================================ */

// Metal Vertex Format values (hardcoded to avoid Obj-C dependency in C file)
#define VF_FLOAT2 29
#define VF_FLOAT3 30
#define VF_FLOAT4 31
#define VF_UCHAR4 3
#define VF_UCHAR4_NORM 9
#define VF_USHORT4_NORM 15

// Common format: Position + Normal + UV
vertex_format_t vertex_format_p3n3t2(void) {
    vertex_format_t fmt = {0};
    fmt.attribute_mask = (1 << VERTEX_ATTR_POSITION) | (1 << VERTEX_ATTR_NORMAL) | (1 << VERTEX_ATTR_TEXCOORD0);
    fmt.stride = sizeof(float) * 8; // 3+3+2 floats = 32 bytes
    
    fmt.formats[VERTEX_ATTR_POSITION] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_POSITION] = 0;
    
    fmt.formats[VERTEX_ATTR_NORMAL] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_NORMAL] = 12;
    
    fmt.formats[VERTEX_ATTR_TEXCOORD0] = VF_FLOAT2;
    fmt.offsets[VERTEX_ATTR_TEXCOORD0] = 24;
    
    return fmt;
}

// Common format: Position + Normal + UV + Tangent
vertex_format_t vertex_format_p3n3t2t4(void) {
    vertex_format_t fmt = {0};
    fmt.attribute_mask = (1 << VERTEX_ATTR_POSITION) | (1 << VERTEX_ATTR_NORMAL) | 
                         (1 << VERTEX_ATTR_TEXCOORD0) | (1 << VERTEX_ATTR_TANGENT);
    fmt.stride = sizeof(float) * 12; // 3+3+2+4 floats = 48 bytes
    
    fmt.formats[VERTEX_ATTR_POSITION] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_POSITION] = 0;
    
    fmt.formats[VERTEX_ATTR_NORMAL] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_NORMAL] = 12;
    
    fmt.formats[VERTEX_ATTR_TEXCOORD0] = VF_FLOAT2;
    fmt.offsets[VERTEX_ATTR_TEXCOORD0] = 24;
    
    fmt.formats[VERTEX_ATTR_TANGENT] = VF_FLOAT4; // Tangent usually requires 4 components (xyz + handedness)
    fmt.offsets[VERTEX_ATTR_TANGENT] = 32;
    
    return fmt;
}

// Common format: Position + Normal + UV + Weights + Bone Indices (Skinned)
vertex_format_t vertex_format_p3n3t2w4b4(void) {
    vertex_format_t fmt = {0};
    fmt.attribute_mask = (1 << VERTEX_ATTR_POSITION) | (1 << VERTEX_ATTR_NORMAL) | 
                         (1 << VERTEX_ATTR_TEXCOORD0) | (1 << VERTEX_ATTR_BONE_WEIGHTS) | 
                         (1 << VERTEX_ATTR_BONE_INDICES);
    
    // Position (3f) = 12
    // Normal (3f) = 12
    // UV (2f) = 8
    // Weights (4f or 4us_norm or 4uc_norm) - Assuming Float4 for high quality or UChar4/UShort4? 
    // Usually weights are float4 or normalized types. Let's assume Float4 for simplicity unless specified otherwise in requested P3N3T2W4B4
    // "W4" usually implies 4 floats or similar. "B4" indices usually int/byte.
    // Let's stick to standard floats for consistency with "P3N3".
    
    fmt.formats[VERTEX_ATTR_POSITION] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_POSITION] = 0;
    
    fmt.formats[VERTEX_ATTR_NORMAL] = VF_FLOAT3;
    fmt.offsets[VERTEX_ATTR_NORMAL] = 12;
    
    fmt.formats[VERTEX_ATTR_TEXCOORD0] = VF_FLOAT2;
    fmt.offsets[VERTEX_ATTR_TEXCOORD0] = 24;
    
    fmt.formats[VERTEX_ATTR_BONE_WEIGHTS] = VF_FLOAT4;
    fmt.offsets[VERTEX_ATTR_BONE_WEIGHTS] = 32;
    
    // Bone indices - usually UShort4 or UChar4. Let's use UChar4 (4 bytes) if < 256 bones, or UShort4. 
    // "B4" might imply 4 bytes? Or 4 components. 
    // Given the stride calculation of P3N3T2 = 32 bytes (3+3+2 floats), using Float4 for weights adds 16 bytes.
    // Let's use USHORT4 for indices (common for > 256 bones) or UCHAR4. 
    // Let's assume Float4 for indices too? No, indices are ints.
    // I'll use VF_UCHAR4 (4 bytes) for indices to keep alignment simple if possible, or VF_USHORT4 (8 bytes).
    // Let's check stride: 12+12+8+16 = 48. 
    // If indices are UChar4 (4 bytes), total = 52. Padding to 52?
    // If indices are UShort4 (8 bytes), total = 56.
    // Use FLOAT4 for indices is rare but possible in some engines (bad practice).
    // I'll pick USHORT4 for indices (safe for bone count) => VF_USHORT4?
    // Metal: MTLVertexFormatUShort4 = 19?
    // UShort4 = 12 (vertex_component_type enum?) No, Metal Enum.
    // MTLVertexFormatUShort4 = 21 (from docs: UShort4 = 21, UShort4Normalized = 22). 
    // I'll use a define for it.
    
    // Wait, let's verify standard practices or if "B4" implies 4 bytes.
    // Often "W4B4" means 4 weights (bytes/shorts) and 4 bones (bytes/shorts).
    // But "P3N3" are floats.
    // I will use Float4 for weights (16 bytes) and UChar4 for indices (4 bytes) => stride 48+4 = 52.
    // Or Ushort4 for indices (8 bytes) => stride 56.
    // Let's use standard Float4 for weights and UChar4 (4 bytes) for indices, aligned to 4 bytes.
    
    fmt.formats[VERTEX_ATTR_BONE_INDICES] = VF_UCHAR4; // 4 bytes
    fmt.offsets[VERTEX_ATTR_BONE_INDICES] = 48;
    
    fmt.stride = 52;
    
    return fmt;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static geometry_vertex_format_internal_t* geometry_vertex_format_get_internal(geometry_vertex_format_handle_t handle) {
    if (handle.id >= g_vertex_format_ctx.count) return NULL;
    geometry_vertex_format_internal_t* item = &g_vertex_format_ctx.items[handle.id];
    if (!item->initialized) return NULL;
    return item;
}

static bool geometry_vertex_format_validate_desc(const geometry_vertex_format_desc_t* desc) {
    if (!desc) return false;
    if (desc->stream_count == 0 || desc->stream_count > 8) return false;
    if (!desc->streams) return false;

    uint32_t total_attrs = 0;
    for (uint32_t s = 0; s < desc->stream_count; s++) {
        if (desc->streams[s].stride == 0) return false;
        if (desc->streams[s].attribute_count == 0) return false;
        if (!desc->streams[s].attributes) return false;
        total_attrs += desc->streams[s].attribute_count;

        // Validate each attribute
        for (uint32_t a = 0; a < desc->streams[s].attribute_count; a++) {
            const vertex_attribute_t* attr = &desc->streams[s].attributes[a];
            if (attr->component_count == 0 || attr->component_count > 4) return false;
            if (attr->component_type >= VERTEX_TYPE_COUNT) return false;

            uint32_t attr_size = geometry_vertex_component_get_size(attr->component_type);
            uint32_t attr_end = attr->offset + attr_size * attr->component_count;
            if (attr_end > desc->streams[s].stride) return false;
        }
    }

    return total_attrs > 0 && total_attrs <= 32;  // Max 32 attributes
}

static void geometry_vertex_format_cleanup_internal(geometry_vertex_format_internal_t* item) {
    if (!item) return;
    if (item->streams) {
        for (uint32_t s = 0; s < item->stream_count; s++) {
            if (item->streams[s].attributes) {
                free(item->streams[s].attributes);
                item->streams[s].attributes = NULL;
            }
        }
        free(item->streams);
        item->streams = NULL;
    }
    item->initialized = false;
}

static int geometry_vertex_format_copy_desc(geometry_vertex_format_internal_t* item,
                                            const geometry_vertex_format_desc_t* desc) {
    if (!item || !desc) return -1;

    // Allocate streams
    item->streams = malloc(desc->stream_count * sizeof(vertex_stream_binding_t));
    if (!item->streams) return -2;

    item->stream_count = desc->stream_count;
    item->total_attribute_count = 0;

    // Copy each stream
    for (uint32_t s = 0; s < desc->stream_count; s++) {
        const vertex_stream_binding_t* src_stream = &desc->streams[s];
        vertex_stream_binding_t* dst_stream = &item->streams[s];

        dst_stream->stride = src_stream->stride;
        dst_stream->attribute_count = src_stream->attribute_count;
        item->total_attribute_count += src_stream->attribute_count;

        // Allocate and copy attributes
        size_t attr_size = src_stream->attribute_count * sizeof(vertex_attribute_t);
        dst_stream->attributes = malloc(attr_size);
        if (!dst_stream->attributes) {
            geometry_vertex_format_cleanup_internal(item);
            return -3;
        }
        memcpy(dst_stream->attributes, src_stream->attributes, attr_size);
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_vertex_format_init(void) {
    if (g_vertex_format_ctx.initialized) {
        return 0;
    }

    g_vertex_format_ctx.capacity = GEOMETRY_VERTEX_FORMAT_DEFAULT_CAPACITY;
    g_vertex_format_ctx.items = calloc(g_vertex_format_ctx.capacity, sizeof(geometry_vertex_format_internal_t));
    if (!g_vertex_format_ctx.items) {
        return -1;
    }

    g_vertex_format_ctx.count = 0;
    g_vertex_format_ctx.initialized = true;

    return 0;
}

void geometry_vertex_format_shutdown(void) {
    if (!g_vertex_format_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_vertex_format_ctx.count; i++) {
        geometry_vertex_format_cleanup_internal(&g_vertex_format_ctx.items[i]);
    }

    free(g_vertex_format_ctx.items);
    g_vertex_format_ctx.items = NULL;
    g_vertex_format_ctx.count = 0;
    g_vertex_format_ctx.capacity = 0;
    g_vertex_format_ctx.initialized = false;
}

int geometry_vertex_format_create(geometry_vertex_format_handle_t* out_handle,
                                   const geometry_vertex_format_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_vertex_format_ctx.initialized) {
        return -2;
    }

    if (!geometry_vertex_format_validate_desc(desc)) {
        return -3;
    }

    if (g_vertex_format_ctx.count >= g_vertex_format_ctx.capacity) {
        return -4;
    }

    uint32_t index = g_vertex_format_ctx.count++;
    geometry_vertex_format_internal_t* item = &g_vertex_format_ctx.items[index];

    memset(item, 0, sizeof(geometry_vertex_format_internal_t));
    item->handle.id = index;
    item->flags = desc->flags;

    int res = geometry_vertex_format_copy_desc(item, desc);
    if (res != 0) {
        g_vertex_format_ctx.count--;
        return res;
    }

    item->initialized = true;
    item->frame_updated = 0;

    *out_handle = item->handle;
    return 0;
}

void geometry_vertex_format_destroy(geometry_vertex_format_handle_t handle) {
    geometry_vertex_format_internal_t* item = geometry_vertex_format_get_internal(handle);
    if (!item) return;

    geometry_vertex_format_cleanup_internal(item);
}

bool geometry_vertex_format_is_valid(geometry_vertex_format_handle_t handle) {
    return geometry_vertex_format_get_internal(handle) != NULL;
}

int geometry_vertex_format_get_info(geometry_vertex_format_handle_t handle,
                                     geometry_vertex_format_info_t* out_info) {
    geometry_vertex_format_internal_t* item = geometry_vertex_format_get_internal(handle);
    if (!item || !out_info) return -1;

    out_info->id = handle.id;
    out_info->stream_count = item->stream_count;
    out_info->total_attribute_count = item->total_attribute_count;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->vertex_stride = item->streams[0].stride;  // Interleaved stride

    return 0;
}

uint32_t geometry_vertex_format_get_stream_stride(geometry_vertex_format_handle_t handle,
                                                  uint32_t stream_index) {
    geometry_vertex_format_internal_t* item = geometry_vertex_format_get_internal(handle);
    if (!item || stream_index >= item->stream_count) return 0;

    return item->streams[stream_index].stride;
}

uint32_t geometry_vertex_format_get_attribute_count(geometry_vertex_format_handle_t handle,
                                                    uint32_t stream_index) {
    geometry_vertex_format_internal_t* item = geometry_vertex_format_get_internal(handle);
    if (!item || stream_index >= item->stream_count) return 0;

    return item->streams[stream_index].attribute_count;
}

int geometry_vertex_format_find_attribute(geometry_vertex_format_handle_t handle,
                                           vertex_attribute_semantic_t semantic,
                                           uint32_t semantic_index,
                                           uint32_t* out_stream_index,
                                           vertex_attribute_t* out_attribute) {
    geometry_vertex_format_internal_t* item = geometry_vertex_format_get_internal(handle);
    if (!item || !out_stream_index || !out_attribute) return -1;

    for (uint32_t s = 0; s < item->stream_count; s++) {
        const vertex_stream_binding_t* stream = &item->streams[s];
        for (uint32_t a = 0; a < stream->attribute_count; a++) {
            const vertex_attribute_t* attr = &stream->attributes[a];
            if (attr->semantic == semantic && attr->semantic_index == semantic_index) {
                *out_stream_index = s;
                memcpy(out_attribute, attr, sizeof(vertex_attribute_t));
                return 0;
            }
        }
    }

    return -2;  // Not found
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t geometry_vertex_format_get_count(void) {
    return g_vertex_format_ctx.count;
}

size_t geometry_vertex_format_get_memory_usage(void) {
    size_t total = 0;

    for (uint32_t i = 0; i < g_vertex_format_ctx.count; i++) {
        geometry_vertex_format_internal_t* item = &g_vertex_format_ctx.items[i];
        if (!item->initialized) continue;

        // Base structure
        total += sizeof(geometry_vertex_format_internal_t);

        // Streams
        if (item->streams) {
            total += item->stream_count * sizeof(vertex_stream_binding_t);
            for (uint32_t s = 0; s < item->stream_count; s++) {
                if (item->streams[s].attributes) {
                    total += item->streams[s].attribute_count * sizeof(vertex_attribute_t);
                }
            }
        }
    }

    return total;
}

void geometry_vertex_format_debug_print(void) {
    if (!g_vertex_format_ctx.initialized) {
        printf("Vertex Format System: Not initialized\n");
        return;
    }

    printf("=== Vertex Format System Debug ===\n");
    printf("Count: %u / %u\n", g_vertex_format_ctx.count, g_vertex_format_ctx.capacity);
    printf("Memory usage: %zu bytes\n", geometry_vertex_format_get_memory_usage());
    printf("\nFormats:\n");

    for (uint32_t i = 0; i < g_vertex_format_ctx.count; i++) {
        geometry_vertex_format_internal_t* item = &g_vertex_format_ctx.items[i];
        if (!item->initialized) continue;

        printf("  Format #%u:\n", item->handle.id);
        printf("    Streams: %u\n", item->stream_count);
        printf("    Total attributes: %u\n", item->total_attribute_count);
        printf("    Flags: 0x%x\n", item->flags);

        for (uint32_t s = 0; s < item->stream_count; s++) {
            const vertex_stream_binding_t* stream = &item->streams[s];
            printf("    Stream %u: stride=%u, attrs=%u\n", s, stream->stride, stream->attribute_count);

            for (uint32_t a = 0; a < stream->attribute_count; a++) {
                const vertex_attribute_t* attr = &stream->attributes[a];
                printf("      Attr %u: %s[%u] @%u, type=%s(%u), count=%u, norm=%d\n",
                       a, geometry_vertex_semantic_get_name(attr->semantic), attr->semantic_index,
                       attr->offset, geometry_vertex_component_get_name(attr->component_type),
                       attr->component_type, attr->component_count, attr->normalized);
            }
        }
    }
}
