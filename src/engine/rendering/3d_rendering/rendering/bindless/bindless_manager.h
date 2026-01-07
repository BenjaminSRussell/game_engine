/*
 * bindless_manager.h
 * Global bindless resource management
 */

#ifndef BINDLESS_MANAGER_H
#define BINDLESS_MANAGER_H

#include "../resource_management/resource_handle.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration for the bindless heap
typedef struct {
    uint32_t max_textures;
    uint32_t max_buffers;
    uint32_t texture_binding; // Binding index in the global set
    uint32_t buffer_binding;  // Binding index in the global set
} bindless_config_t;

typedef struct bindless_heap_t bindless_heap_t;

// Initialization
bindless_heap_t* bindless_manager_create(const bindless_config_t* config);
void bindless_manager_destroy(bindless_heap_t* heap);

// Registration
// Returns the index in the bindless array to be used in shaders
// Returns UINT32_MAX on failure
uint32_t bindless_register_texture(bindless_heap_t* heap, texture_handle_t texture);
uint32_t bindless_register_buffer(bindless_heap_t* heap, buffer_handle_t buffer);

// Unregistration
// Frees the index for reuse
void bindless_unregister_texture(bindless_heap_t* heap, uint32_t index);
void bindless_unregister_buffer(bindless_heap_t* heap, uint32_t index);

// Update
// Flushes pending updates to the GPU
void bindless_manager_update(bindless_heap_t* heap);

// Get internal descriptor set (for pipeline binding)
// In reality returns VkDescriptorSet or similar opaque handle
void* bindless_manager_get_descriptor_set(bindless_heap_t* heap);

#ifdef __cplusplus
}
#endif

#endif // BINDLESS_MANAGER_H
