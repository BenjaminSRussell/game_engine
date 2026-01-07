#include "../vertex_format.h"
#include "metal_backend.h"
#import <Metal/Metal.h>
#include <core/logger.h>

MTLVertexFormat mtl_translate_vertex_format(VertexAttributeFormat format) {
  switch (format) {
  case VERTEX_ATTRIB_F32:
    return MTLVertexFormatFloat;
  case VERTEX_ATTRIB_F32_2:
    return MTLVertexFormatFloat2;
  case VERTEX_ATTRIB_F32_3:
    return MTLVertexFormatFloat3;
  case VERTEX_ATTRIB_F32_4:
    return MTLVertexFormatFloat4;
  case VERTEX_ATTRIB_U8_4:
    return MTLVertexFormatUChar4;
  case VERTEX_ATTRIB_U8_4_NORM:
    return MTLVertexFormatUChar4Normalized;
  case VERTEX_ATTRIB_S8_4_NORM:
    return MTLVertexFormatChar4Normalized;
  case VERTEX_ATTRIB_U16_2:
    return MTLVertexFormatUShort2;
  case VERTEX_ATTRIB_U16_4:
    return MTLVertexFormatUShort4;
  case VERTEX_ATTRIB_U16_2_NORM:
    return MTLVertexFormatUShort2Normalized;
  case VERTEX_ATTRIB_U16_4_NORM:
    return MTLVertexFormatUShort4Normalized;
  case VERTEX_ATTRIB_S16_2_NORM:
    return MTLVertexFormatShort2Normalized;
  case VERTEX_ATTRIB_S16_4_NORM:
    return MTLVertexFormatShort4Normalized;
  default:
    LOG_ERROR("Unknown vertex format: %d", format);
    return MTLVertexFormatInvalid;
  }
}

// Helper to create a Metal Vertex Descriptor from our generic VertexFormat
void *mtl_create_vertex_descriptor(const VertexFormat *fmt) {
  MTLVertexDescriptor *mtlDesc = [[MTLVertexDescriptor alloc] init];

  // 1. Setup Attributes
  for (int i = 0; i < fmt->attribute_count; i++) {
    const VertexAttribute *attr = &fmt->attributes[i];

    MTLVertexFormat mtlFmt = mtl_translate_vertex_format(attr->format);
    if (mtlFmt == MTLVertexFormatInvalid) {
      LOG_ERROR("Failed to translate vertex attribute at index %d", i);
      continue;
    }

    mtlDesc.attributes[attr->location].format = mtlFmt;
    mtlDesc.attributes[attr->location].offset = attr->offset;
    mtlDesc.attributes[attr->location].bufferIndex = attr->buffer_index;
  }

  // 2. Setup Buffer Layouts
  // Find unique buffer indices and set stride/step function
  // For now, we iterate all attributes to find participating buffers
  // This is slightly inefficient but safe for the small number of buffers
  // (usually 1 or 2)

  bool buffer_seen[31] = {0}; // Metal usually limits to 31 buffers

  for (int i = 0; i < fmt->attribute_count; i++) {
    int buffer_idx = fmt->attributes[i].buffer_index;
    if (buffer_idx < 31 && !buffer_seen[buffer_idx]) {
      buffer_seen[buffer_idx] = true;

      mtlDesc.layouts[buffer_idx].stride =
          fmt->stride; // Currently assumes same stride for a formatted buffer
      mtlDesc.layouts[buffer_idx].stepRate = 1;
      mtlDesc.layouts[buffer_idx].stepFunction =
          (fmt->input_rate == VERTEX_INPUT_RATE_INSTANCE)
              ? MTLVertexStepFunctionPerInstance
              : MTLVertexStepFunctionPerVertex;
    }
  }

  return (__bridge_retained void *)mtlDesc;
}
