#include "backend/vertex_format_reflect.h"
#include <core/logger.h>
#include <string.h>

const VertexAttribute* vertex_format_get_attribute(const VertexFormat* fmt, VertexAttributeSemantic semantic) {
    if (!fmt) return NULL;
    
    for (int i = 0; i < fmt->attribute_count; i++) {
        if (fmt->attributes[i].semantic == semantic) {
            return &fmt->attributes[i];
        }
    }
    return NULL;
}

u32 vertex_format_get_attribute_count(const VertexFormat* fmt) {
    return fmt ? fmt->attribute_count : 0;
}

u32 vertex_format_get_stride(const VertexFormat* fmt) {
    return fmt ? fmt->stride : 0;
}

u16 vertex_format_get_offset(const VertexFormat* fmt, VertexAttributeSemantic semantic) {
    const VertexAttribute* attr = vertex_format_get_attribute(fmt, semantic);
    return attr ? attr->offset : 0;
}

bool vertex_format_has_semantic(const VertexFormat* fmt, VertexAttributeSemantic semantic) {
    return vertex_format_get_attribute(fmt, semantic) != NULL;
}

u32 vertex_format_get_size(const VertexFormat* fmt) {
    return vertex_format_get_stride(fmt);
}

bool vertex_format_is_compatible(const VertexFormat* a, const VertexFormat* b) {
    if (!a || !b) return false;
    if (a->attribute_count != b->attribute_count) return false;
    
    // Check that all semantics match (order may differ)
    for (int i = 0; i < a->attribute_count; i++) {
        VertexAttributeSemantic sem = a->attributes[i].semantic;
        if (!vertex_format_has_semantic(b, sem)) {
            return false;
        }
        
        // Also check format matches
        const VertexAttribute* attr_a = &a->attributes[i];
        const VertexAttribute* attr_b = vertex_format_get_attribute(b, sem);
        if (attr_a->format != attr_b->format) {
            return false;
        }
    }
    
    return true;
}

const char* vertex_semantic_to_string(VertexAttributeSemantic semantic) {
    switch (semantic) {
        case VERTEX_SEMANTIC_POSITION:   return "Position";
        case VERTEX_SEMANTIC_NORMAL:     return "Normal";
        case VERTEX_SEMANTIC_TANGENT:    return "Tangent";
        case VERTEX_SEMANTIC_BITANGENT:  return "Bitangent";
        case VERTEX_SEMANTIC_UV0:        return "UV0";
        case VERTEX_SEMANTIC_UV1:        return "UV1";
        case VERTEX_SEMANTIC_COLOR:      return "Color";
        case VERTEX_SEMANTIC_JOINTS:     return "Joints";
        case VERTEX_SEMANTIC_WEIGHTS:    return "Weights";
        case VERTEX_SEMANTIC_CUSTOM:     return "Custom";
        default:                         return "Unknown";
    }
}

const char* vertex_attribute_format_to_string(VertexAttributeFormat format) {
    switch (format) {
        case VERTEX_ATTRIB_F32:         return "Float";
        case VERTEX_ATTRIB_F32_2:       return "Float2";
        case VERTEX_ATTRIB_F32_3:       return "Float3";
        case VERTEX_ATTRIB_F32_4:       return "Float4";
        case VERTEX_ATTRIB_U8_4:        return "UByte4";
        case VERTEX_ATTRIB_U8_4_NORM:   return "UByte4Norm";
        case VERTEX_ATTRIB_S8_4_NORM:   return "Byte4Norm";
        case VERTEX_ATTRIB_U16_2:       return "UShort2";
        case VERTEX_ATTRIB_U16_4:       return "UShort4";
        case VERTEX_ATTRIB_U16_2_NORM:  return "UShort2Norm";
        case VERTEX_ATTRIB_U16_4_NORM:  return "UShort4Norm";
        case VERTEX_ATTRIB_S16_2_NORM:  return "Short2Norm";
        case VERTEX_ATTRIB_S16_4_NORM:  return "Short4Norm";
        default:                        return "Unknown";
    }
}

void vertex_format_print(const VertexFormat* fmt) {
    if (!fmt) {
        LOG_INFO("VertexFormat: NULL");
        return;
    }
    
    LOG_INFO("VertexFormat (Hash: 0x%08X):", fmt->hash);
    LOG_INFO("  Stride: %u bytes", fmt->stride);
    LOG_INFO("  Attributes: %u", fmt->attribute_count);
    
    for (int i = 0; i < fmt->attribute_count; i++) {
        const VertexAttribute* attr = &fmt->attributes[i];
        LOG_INFO("    [%d] %s (%s) @ offset %u, buffer %u, location %u",
                 i,
                 vertex_semantic_to_string(attr->semantic),
                 vertex_attribute_format_to_string(attr->format),
                 attr->offset,
                 attr->buffer_index,
                 attr->location);
    }
}
