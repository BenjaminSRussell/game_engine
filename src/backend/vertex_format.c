#include "backend/vertex_format.h"
#include <core/logger.h>
#include <string.h>

static u32 format_size_lookup[] = {
    [VERTEX_ATTRIB_F32]        = 4,
    [VERTEX_ATTRIB_F32_2]      = 8,
    [VERTEX_ATTRIB_F32_3]      = 12,
    [VERTEX_ATTRIB_F32_4]      = 16,
    [VERTEX_ATTRIB_U8_4]       = 4,
    [VERTEX_ATTRIB_U8_4_NORM]  = 4,
    [VERTEX_ATTRIB_S8_4_NORM]  = 4,
    [VERTEX_ATTRIB_U16_2]      = 4,
    [VERTEX_ATTRIB_U16_4]      = 8,
    [VERTEX_ATTRIB_U16_2_NORM] = 4,
    [VERTEX_ATTRIB_U16_4_NORM] = 8,
    [VERTEX_ATTRIB_S16_2_NORM] = 4,
    [VERTEX_ATTRIB_S16_4_NORM] = 8,
};

u32 vertex_format_size_of_attribute(VertexAttributeFormat format) {
    if (format < 0 || format >= sizeof(format_size_lookup) / sizeof(u32)) {
        LOG_ERROR("Unknown vertex attribute format: %d", format);
        return 0;
    }
    return format_size_lookup[format];
}

void vertex_format_init(VertexFormat* fmt) {
    memset(fmt, 0, sizeof(VertexFormat));
    fmt->attribute_count = 0;
    fmt->stride = 0;
    fmt->input_rate = VERTEX_INPUT_RATE_VERTEX;
    fmt->hash = 0;
}

void vertex_format_add_attribute(VertexFormat* fmt, VertexAttributeSemantic semantic, VertexAttributeFormat format, u8 buffer_index) {
    if (fmt->attribute_count >= VERTEX_FORMAT_MAX_ATTRIBUTES) {
        LOG_ERROR("Vertex format attribute count exceeded limit of %d", VERTEX_FORMAT_MAX_ATTRIBUTES);
        return;
    }

    u32 idx = fmt->attribute_count;
    VertexAttribute* attr = &fmt->attributes[idx];
    
    attr->semantic = semantic;
    attr->format = format;
    attr->buffer_index = buffer_index;
    attr->location = idx; // Default to sequential locations for now

    // Calculate offset automatically based on previous attributes in the SAME buffer
    u32 offset = 0;
    if (idx > 0) {
        // Simple auto-offset logic: sum sizes of all previous attributes in this buffer
        // Note: This assumes attributes for a buffer are added sequentially. 
        // If interleaved attributes from different buffers are added, this logic needs to be smarter.
        // For now, we assume standard interleaved arrays.
        for (int i = 0; i < idx; i++) {
            if (fmt->attributes[i].buffer_index == buffer_index) {
                offset += vertex_format_size_of_attribute(fmt->attributes[i].format);
            }
        }
    }
    attr->offset = (u16)offset;

    fmt->attribute_count++;
    
    // Recalculate stride
    vertex_format_calc_stride(fmt);
}

void vertex_format_calc_stride(VertexFormat* fmt) {
    // For interleaved (single buffer), stride is the sum of all sizes
    // For now, we calculate the max extent of the attributes
    
    // Assuming mostly single buffer (index 0) for now, or per-buffer stride support later.
    // This function sets the 'primary' stride (buffer 0) often used for allocation.
    
    u32 max_offset = 0;
    for (int i = 0; i < fmt->attribute_count; i++) {
        u32 end = fmt->attributes[i].offset + vertex_format_size_of_attribute(fmt->attributes[i].format);
        if (end > max_offset) {
            max_offset = end;
        }
    }
    
    // Align to 4 bytes? Metal usually requires 4-byte alignment for strides
    u32 alignment = 4;
    u32 remainder = max_offset % alignment;
    if (remainder != 0) {
        max_offset += (alignment - remainder);
    }
    
    fmt->stride = max_offset;
    
    // Simple hash calculation
    u32 hash = 5381;
    hash = ((hash << 5) + hash) + fmt->attribute_count;
    hash = ((hash << 5) + hash) + fmt->stride;
    for (int i = 0; i < fmt->attribute_count; i++) {
        hash = ((hash << 5) + hash) + fmt->attributes[i].format;
        hash = ((hash << 5) + hash) + fmt->attributes[i].semantic;
    }
    fmt->hash = hash;
}

bool vertex_format_equals(const VertexFormat* a, const VertexFormat* b) {
    if (a->hash != b->hash) return false;
    if (a->attribute_count != b->attribute_count) return false;
    if (a->stride != b->stride) return false;
    
    for (int i = 0; i < a->attribute_count; i++) {
        if (a->attributes[i].format != b->attributes[i].format) return false;
        if (a->attributes[i].semantic != b->attributes[i].semantic) return false;
        if (a->attributes[i].offset != b->attributes[i].offset) return false;
        if (a->attributes[i].buffer_index != b->attributes[i].buffer_index) return false;
    }
    
    return true;
}

// Standard Formats

void vertex_format_make_p3(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
}

void vertex_format_make_p3n3(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_F32_3, 0);
}

void vertex_format_make_p3n3u2(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_F32_2, 0);
}

void vertex_format_make_p3n3t3u2(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_TANGENT, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_F32_2, 0);
}

// Animated format: P3N3U2 + bone indices (4xu8) + bone weights (4xf32)
void vertex_format_make_animated(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_F32_2, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_JOINTS, VERTEX_ATTRIB_U8_4, 0);    // Bone indices
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_WEIGHTS, VERTEX_ATTRIB_F32_4, 0);  // Bone weights
}

// Terrain format: P3N3U2 + blend weights (4xu8 normalized)
void vertex_format_make_terrain(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_F32_3, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_F32_2, 0);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_CUSTOM, VERTEX_ATTRIB_U8_4_NORM, 0); // Blend weights for 4 textures
}

// Particle format: Position + Velocity + Color + Lifetime + Size
void vertex_format_make_particle(VertexFormat* fmt) {
    vertex_format_init(fmt);
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);  // Position
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_CUSTOM, VERTEX_ATTRIB_F32_3, 0);    // Velocity
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_COLOR, VERTEX_ATTRIB_U8_4_NORM, 0); // Color
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_CUSTOM, VERTEX_ATTRIB_F32_2, 0);    // Lifetime + Size
}

