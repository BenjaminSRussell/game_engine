#pragma once

#include "backend/vertex_format.h"
#include "backend/vertex_compression.h"

/**
 * =================================================================================================
 *                      COMPRESSED VERTEX FORMAT BUILDERS
 * =================================================================================================
 *
 * Convenience functions to create common compressed vertex formats.
 */

/**
 * Create compressed PBR format:
 * - Position: 16-bit quantized (6 bytes)
 * - Normal: Oct16 (2 bytes)
 * - Tangent: Oct16 + sign (4 bytes)
 * - UV: 16-bit fixed point (4 bytes)
 * Total: 16 bytes vs 32 bytes uncompressed (50% savings)
 */
void vertex_format_make_p3n3t3u2_compressed(VertexFormat* fmt);

/**
 * Create ultra-compressed format for distant LODs:
 * - Position: 16-bit quantized (6 bytes)
 * - Normal: Oct16 (2 bytes)
 * - UV: 16-bit (4 bytes)
 * Total: 12 bytes vs 32 bytes (62.5% savings)
 */
void vertex_format_make_p3n3u2_compressed(VertexFormat* fmt);

/**
 * Create compressed particle format:
 * - Position: Float3 (12 bytes) - full precision needed
 * - Velocity: 16-bit quantized (6 bytes)
 * - Color: RGBA8 (4 bytes)
 * - Lifetime: Float (4 bytes)
 * Total: 26 bytes
 */
void vertex_format_make_particle_compressed(VertexFormat* fmt);
