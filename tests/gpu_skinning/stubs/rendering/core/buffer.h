// Test-only stand-in for rendering/core/buffer.h. gpu_skinning.c uses
// BufferDesc and a scalar BufferID handle. The current header declares
// BufferCreateInfo, Buffer *buffer_create(const BufferCreateInfo *) and a
// struct BufferID, so the unit does not compile against it.
#ifndef GPU_SKINNING_TEST_BUFFER_H
#define GPU_SKINNING_TEST_BUFFER_H

#include "core/math/types.h"

typedef u32 BufferID;

typedef struct BufferDesc {
  usize size;
  u32 usage;
  u32 memory_type;
} BufferDesc;

enum {
  BUFFER_USAGE_VERTEX = BIT(0),
  BUFFER_USAGE_INDEX = BIT(1),
  BUFFER_USAGE_UNIFORM = BIT(2),
  BUFFER_USAGE_STORAGE = BIT(3),
  BUFFER_USAGE_DYNAMIC = BIT(4),
};

enum {
  MEMORY_TYPE_DEVICE_LOCAL = 0,
  MEMORY_TYPE_HOST_VISIBLE = 1,
};

BufferID buffer_create(const BufferDesc *desc);
void buffer_destroy(BufferID buffer);
void buffer_upload_data(BufferID buffer, const void *data, usize size);
void buffer_bind_compute(BufferID buffer, u32 binding);
void buffer_bind_image_compute(BufferID buffer, u32 binding);

#endif
