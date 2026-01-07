#pragma once

#include "vertex_format.h"

/**
 * =================================================================================================
 *                          VERTEX FORMAT REFLECTION & QUERIES
 * =================================================================================================
 */

/**
 * Get attribute by semantic. Returns NULL if not found.
 */
const VertexAttribute* vertex_format_get_attribute(const VertexFormat* fmt, VertexAttributeSemantic semantic);

/**
 * Get attribute count
 */
u32 vertex_format_get_attribute_count(const VertexFormat* fmt);

/**
 * Get total stride
 */
u32 vertex_format_get_stride(const VertexFormat* fmt);

/**
 * Get attribute offset by semantic
 */
u16 vertex_format_get_offset(const VertexFormat* fmt, VertexAttributeSemantic semantic);

/**
 * Check if format has a specific semantic
 */
bool vertex_format_has_semantic(const VertexFormat* fmt, VertexAttributeSemantic semantic);

/**
 * Get size of entire vertex
 */
u32 vertex_format_get_size(const VertexFormat* fmt);

/**
 * Format compatibility check (can be used interchangeably)
 */
bool vertex_format_is_compatible(const VertexFormat* a, const VertexFormat* b);

/**
 * Print format to log (debug)
 */
void vertex_format_print(const VertexFormat* fmt);

/**
 * Get human-readable semantic name
 */
const char* vertex_semantic_to_string(VertexAttributeSemantic semantic);

/**
 * Get human-readable format name
 */
const char* vertex_attribute_format_to_string(VertexAttributeFormat format);
