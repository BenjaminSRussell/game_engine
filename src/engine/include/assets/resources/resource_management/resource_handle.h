#ifndef RESOURCE_HANDLE_H
#define RESOURCE_HANDLE_H

#include <core/types.h>

// Resource handle system
typedef struct {
    u64 id;
    u32 type;
    void *data;
    u32 ref_count;
    bool is_valid;
} ResourceHandle;

ResourceHandle resource_handle_create(u64 id, u32 type, void *data);
void resource_handle_destroy(ResourceHandle *handle);
void* resource_handle_get_data(ResourceHandle *handle);
bool resource_handle_is_valid(ResourceHandle *handle);

#endif // RESOURCE_HANDLE_H
