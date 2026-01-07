#ifndef RENDERER_CORE_BUFFER_DEFINITIONS_H
#define RENDERER_CORE_BUFFER_DEFINITIONS_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Buffer usage flags
typedef enum BufferUsage {
    BUFFER_USAGE_VERTEX = BIT(0),
    BUFFER_USAGE_INDEX = BIT(1),
    BUFFER_USAGE_UNIFORM = BIT(2),
    BUFFER_USAGE_STORAGE = BIT(3),
    BUFFER_USAGE_TRANSFER_SRC = BIT(4),
    BUFFER_USAGE_TRANSFER_DST = BIT(5),
} BufferUsage;

typedef u32 BufferUsageFlags;

// Opaque buffer handle
typedef struct BufferID {
    u32 id;
} BufferID;

// Creation info
typedef struct BufferCreateInfo {
    u64 size;
    BufferUsageFlags usage;
    u32 flags;
    const char *name;
} BufferCreateInfo;

// Buffer object
typedef struct Buffer {
    BufferID id;
    u64 size;
    BufferUsageFlags usage;
} Buffer;

// API
Buffer *buffer_create(const BufferCreateInfo *info);
void buffer_destroy(Buffer *buffer);
BufferID buffer_get_id(Buffer *buffer);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_CORE_BUFFER_H
