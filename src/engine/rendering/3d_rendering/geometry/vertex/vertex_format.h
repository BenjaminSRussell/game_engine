/*
 * vertex_format.h
 * Vertex attribute layout definition
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_VERTEX_FORMAT_H
#define GEOMETRY_VERTEX_FORMAT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES & ENUMS
 * ============================================================================ */

// Vertex attribute semantic type
typedef enum vertex_attribute_semantic {
    VERTEX_SEMANTIC_POSITION = 0,
    VERTEX_SEMANTIC_NORMAL,
    VERTEX_SEMANTIC_TANGENT,
    VERTEX_SEMANTIC_BINORMAL,
    VERTEX_SEMANTIC_TEXCOORD,
    VERTEX_SEMANTIC_COLOR,
    VERTEX_SEMANTIC_BONE_INDEX,
    VERTEX_SEMANTIC_BONE_WEIGHT,
    VERTEX_SEMANTIC_CUSTOM,
    VERTEX_SEMANTIC_COUNT
} vertex_attribute_semantic_t;

// Vertex attribute component type
typedef enum vertex_component_type {
    VERTEX_TYPE_FLOAT32 = 0,  // 32-bit float
    VERTEX_TYPE_FLOAT16,      // 16-bit float (half)
    VERTEX_TYPE_INT32,        // 32-bit signed integer
    VERTEX_TYPE_UINT32,       // 32-bit unsigned integer
    VERTEX_TYPE_INT16,        // 16-bit signed integer
    VERTEX_TYPE_UINT16,       // 16-bit unsigned integer
    VERTEX_TYPE_INT8,         // 8-bit signed integer
    VERTEX_TYPE_UINT8,        // 8-bit unsigned integer
    VERTEX_TYPE_SNORM8,       // 8-bit signed normalized (maps [-128, 127] to [-1.0, 1.0])
    VERTEX_TYPE_UNORM8,       // 8-bit unsigned normalized (maps [0, 255] to [0.0, 1.0])
    VERTEX_TYPE_SNORM16,      // 16-bit signed normalized
    VERTEX_TYPE_UNORM16,      // 16-bit unsigned normalized
    VERTEX_TYPE_COUNT
} vertex_component_type_t;

// Individual vertex attribute descriptor
typedef struct vertex_attribute {
    vertex_attribute_semantic_t semantic;
    uint32_t semantic_index;  // For multiple TEXCOORD/COLOR attributes
    vertex_component_type_t component_type;
    uint32_t component_count; // 1-4
    uint32_t offset;          // Offset within vertex struct (in bytes)
    bool normalized;          // For integer types: normalize to [-1, 1] or [0, 1]
} vertex_attribute_t;

// Stream binding for interleaved or separate streams
typedef struct vertex_stream_binding {
    uint32_t stride;          // Stride of this stream in bytes
    uint32_t attribute_count; // Number of attributes in this stream
    vertex_attribute_t* attributes;
} vertex_stream_binding_t;

// Complete vertex format descriptor
typedef struct geometry_vertex_format_desc {
    uint32_t stream_count;    // Number of vertex streams (1 = interleaved, >1 = separate)
    vertex_stream_binding_t* streams;
    uint32_t flags;
    void* user_data;
} geometry_vertex_format_desc_t;

// Handle for opaque vertex format
typedef struct geometry_vertex_format_handle {
    uint32_t id;
} geometry_vertex_format_handle_t;

// Runtime info about vertex format
typedef struct geometry_vertex_format_info {
    uint32_t id;
    uint32_t stream_count;
    uint32_t total_attribute_count;
    uint32_t flags;
    bool initialized;
    uint32_t vertex_stride;  // Total stride for interleaved (stream 0)
} geometry_vertex_format_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_vertex_format_init(void);
void geometry_vertex_format_shutdown(void);

/* Lifecycle */
int geometry_vertex_format_create(geometry_vertex_format_handle_t* out_handle, const geometry_vertex_format_desc_t* desc);
void geometry_vertex_format_destroy(geometry_vertex_format_handle_t handle);

/* Query & Validation */
bool geometry_vertex_format_is_valid(geometry_vertex_format_handle_t handle);
int geometry_vertex_format_get_info(geometry_vertex_format_handle_t handle, geometry_vertex_format_info_t* out_info);

/* Stream & Attribute Queries */
uint32_t geometry_vertex_format_get_stream_stride(geometry_vertex_format_handle_t handle, uint32_t stream_index);
uint32_t geometry_vertex_format_get_attribute_count(geometry_vertex_format_handle_t handle, uint32_t stream_index);

// Find attribute by semantic
int geometry_vertex_format_find_attribute(geometry_vertex_format_handle_t handle,
                                          vertex_attribute_semantic_t semantic,
                                          uint32_t semantic_index,
                                          uint32_t* out_stream_index,
                                          vertex_attribute_t* out_attribute);

/* Component Type Utilities */
uint32_t geometry_vertex_component_get_size(vertex_component_type_t type);
bool geometry_vertex_component_is_normalized_type(vertex_component_type_t type);
const char* geometry_vertex_component_get_name(vertex_component_type_t type);
const char* geometry_vertex_semantic_get_name(vertex_attribute_semantic_t semantic);

/* Stride Calculation */
uint32_t geometry_vertex_format_calculate_stride(const vertex_attribute_t* attributes, uint32_t attribute_count);

/* Statistics */
uint32_t geometry_vertex_format_get_count(void);
size_t geometry_vertex_format_get_memory_usage(void);
void geometry_vertex_format_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_VERTEX_FORMAT_H */
