#pragma once

#include <common.h>
#include <core/types.h>

/**
 * =================================================================================================
 *                              VERTEX FORMAT DEFINITIONS
 * =================================================================================================
 *
 * Purpose: Defines flexible vertex formats for GPU binding.
 */

/* Max attributes per vertex format */
#define VERTEX_FORMAT_MAX_ATTRIBUTES 16

/* Attribute formats */
typedef enum {
    VERTEX_ATTRIB_F32,      // float
    VERTEX_ATTRIB_F32_2,    // vec2
    VERTEX_ATTRIB_F32_3,    // vec3
    VERTEX_ATTRIB_F32_4,    // vec4
    VERTEX_ATTRIB_U8_4,     // ubyte4 (unnormalized)
    VERTEX_ATTRIB_U8_4_NORM,// ubyte4 (normalized 0..1)
    VERTEX_ATTRIB_S8_4_NORM,// byte4 (normalized -1..1)
    VERTEX_ATTRIB_U16_2,    // ushort2
    VERTEX_ATTRIB_U16_4,    // ushort4
    VERTEX_ATTRIB_U16_2_NORM, // ushort2 normalized
    VERTEX_ATTRIB_U16_4_NORM, // ushort4 normalized
    VERTEX_ATTRIB_S16_2_NORM, // short2 normalized
    VERTEX_ATTRIB_S16_4_NORM, // short4 normalized
} VertexAttributeFormat;

/* Attribute Semantics (what the data represents) */
typedef enum {
    VERTEX_SEMANTIC_POSITION,
    VERTEX_SEMANTIC_NORMAL,
    VERTEX_SEMANTIC_TANGENT,
    VERTEX_SEMANTIC_BITANGENT,
    VERTEX_SEMANTIC_UV0,
    VERTEX_SEMANTIC_UV1,
    VERTEX_SEMANTIC_COLOR,
    VERTEX_SEMANTIC_JOINTS,
    VERTEX_SEMANTIC_WEIGHTS,
    VERTEX_SEMANTIC_CUSTOM,
} VertexAttributeSemantic;

/* Input Rate */
typedef enum {
    VERTEX_INPUT_RATE_VERTEX,   // Per-vertex data
    VERTEX_INPUT_RATE_INSTANCE, // Per-instance data
} VertexInputRate;

/* Single Attribute Definition */
typedef struct {
    VertexAttributeSemantic semantic;
    VertexAttributeFormat format;
    u16 offset;
    u8 buffer_index;  // Which buffer binding slot this comes from
    u8 location;      // Shader location index
} VertexAttribute;

/* Complete Vertex Format Descriptor */
typedef struct VertexFormat {
    VertexAttribute attributes[VERTEX_FORMAT_MAX_ATTRIBUTES];
    u8 attribute_count;
    u32 stride;
    VertexInputRate input_rate;
    u32 hash;         // Cached hash for quick comparison
} VertexFormat;

/* Core Functions */
void vertex_format_init(VertexFormat* fmt);
void vertex_format_add_attribute(VertexFormat* fmt, VertexAttributeSemantic semantic, VertexAttributeFormat format, u8 buffer_index);
void vertex_format_calc_stride(VertexFormat* fmt);
u32 vertex_format_size_of_attribute(VertexAttributeFormat format);
bool vertex_format_equals(const VertexFormat* a, const VertexFormat* b);

/* Standard Format Builders */
void vertex_format_make_p3(VertexFormat* out_fmt);
void vertex_format_make_p3n3(VertexFormat* out_fmt);
void vertex_format_make_p3n3u2(VertexFormat* out_fmt);
void vertex_format_make_p3n3t3u2(VertexFormat* out_fmt); // Common PBR

/* Specialized Formats */
void vertex_format_make_animated(VertexFormat* out_fmt);     // P3N3U2 + 4 bones
void vertex_format_make_terrain(VertexFormat* out_fmt);      // P3N3U2 + 4 blend weights
void vertex_format_make_particle(VertexFormat* out_fmt);     // P3 + Velocity + Color + Life

