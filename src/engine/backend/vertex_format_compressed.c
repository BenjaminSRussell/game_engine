#include "vertex_format_compressed.h"

void vertex_format_make_p3n3t3u2_compressed(VertexFormat* fmt) {
    vertex_format_init(fmt);
    
    // Position: 3x u16 (quantized)
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_U16_4, 0);
    
    // Normal: Oct16 (2 bytes packed in u16)
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_U16_2, 0);
    
    // Tangent: Oct16 + sign (4 bytes packed in u32, but using u16x2)
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_TANGENT, VERTEX_ATTRIB_U16_2, 0);
    
    // UV: 16-bit per component
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_U16_2, 0);
}

void vertex_format_make_p3n3u2_compressed(VertexFormat* fmt) {
    vertex_format_init(fmt);
    
    // Position: 3x u16 (quantized)
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_U16_4, 0);
    
    // Normal: Oct16
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_NORMAL, VERTEX_ATTRIB_U16_2, 0);
    
    // UV: 16-bit per component
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_UV0, VERTEX_ATTRIB_U16_2, 0);
}

void vertex_format_make_particle_compressed(VertexFormat* fmt) {
    vertex_format_init(fmt);
    
    // Position: Full precision needed for particles
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_POSITION, VERTEX_ATTRIB_F32_3, 0);
    
    // Velocity: Quantized to 16-bit (custom semantic)
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_CUSTOM, VERTEX_ATTRIB_U16_4, 0);
    
    // Color: RGBA8
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_COLOR, VERTEX_ATTRIB_U8_4_NORM, 0);
    
    // Lifetime: Float
    vertex_format_add_attribute(fmt, VERTEX_SEMANTIC_CUSTOM, VERTEX_ATTRIB_F32, 0);
}
