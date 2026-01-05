#include "bindless_manager.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Simple free list implementation
typedef struct {
    uint32_t* indices;
    uint32_t count;
    uint32_t capacity;
} free_list_t;

struct bindless_heap_t {
    bindless_config_t config;
    
    // Free lists for recycling indices
    free_list_t texture_free_list;
    free_list_t buffer_free_list;
    
    // High water marks
    uint32_t texture_count;
    uint32_t buffer_count;
    
    // Internal Vulkan/API state (placeholders)
    void* descriptor_pool;
    void* descriptor_set;
    void* descriptor_layout;
};

static void free_list_init(free_list_t* list, uint32_t capacity) {
    list->capacity = capacity;
    list->count = 0;
    list->indices = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    // Initially empty free list means we allocate sequentially from 0
    // Or we could pre-fill it.
    // Strategy: Use a counter (high water mark). If free list is empty, increment counter.
    // If free list has items, pop one.
}

static void free_list_destroy(free_list_t* list) {
    if (list->indices) free(list->indices);
    list->indices = NULL;
}

static void free_list_push(free_list_t* list, uint32_t index) {
    if (list->count < list->capacity) {
        list->indices[list->count++] = index;
    }
}

static bool free_list_pop(free_list_t* list, uint32_t* out_index) {
    if (list->count > 0) {
        *out_index = list->indices[--list->count];
        return true;
    }
    return false;
}

bindless_heap_t* bindless_manager_create(const bindless_config_t* config) {
    if (!config) return NULL;
    
    bindless_heap_t* heap = (bindless_heap_t*)calloc(1, sizeof(bindless_heap_t));
    if (!heap) return NULL;
    
    heap->config = *config;
    
    free_list_init(&heap->texture_free_list, config->max_textures);
    free_list_init(&heap->buffer_free_list, config->max_buffers);
    
    heap->texture_count = 0;
    heap->buffer_count = 0;
    
    // TODO: Create Descriptor Pool and Set Layout
    // TODO: Allocate descriptor set
    
    return heap;
}

void bindless_manager_destroy(bindless_heap_t* heap) {
    if (!heap) return;
    
    free_list_destroy(&heap->texture_free_list);
    free_list_destroy(&heap->buffer_free_list);
    
    // TODO: Destroy Vulkan resources
    
    free(heap);
}

uint32_t bindless_register_texture(bindless_heap_t* heap, texture_handle_t texture) {
    if (!heap) return UINT32_MAX;
    
    uint32_t index;
    if (free_list_pop(&heap->texture_free_list, &index)) {
        // Reusing index
    } else {
        if (heap->texture_count >= heap->config.max_textures) return UINT32_MAX;
        index = heap->texture_count++;
    }
    
    // TODO: Write to descriptor set at 'index' for texture binding
    // descriptor_writer_write_image(...)
    
    return index;
}

uint32_t bindless_register_buffer(bindless_heap_t* heap, buffer_handle_t buffer) {
    if (!heap) return UINT32_MAX;
    
    uint32_t index;
    if (free_list_pop(&heap->buffer_free_list, &index)) {
        // Reusing index
    } else {
        if (heap->buffer_count >= heap->config.max_buffers) return UINT32_MAX;
        index = heap->buffer_count++;
    }
    
    // TODO: Write to descriptor set at 'index' for buffer binding
    
    return index;
}

void bindless_unregister_texture(bindless_heap_t* heap, uint32_t index) {
    if (!heap || index >= heap->config.max_textures) return;
    
    // TODO: Write dummy/null descriptor to index to prevent use-after-free crashes on GPU?
    
    free_list_push(&heap->texture_free_list, index);
}

void bindless_unregister_buffer(bindless_heap_t* heap, uint32_t index) {
    if (!heap || index >= heap->config.max_buffers) return;
    
    free_list_push(&heap->buffer_free_list, index);
}

void bindless_manager_update(bindless_heap_t* heap) {
    // Determine if we need explicit flush or if writes were immediate.
    // If using descriptor_writer, we might trigger a build here.
}

void* bindless_manager_get_descriptor_set(bindless_heap_t* heap) {
    return heap ? heap->descriptor_set : NULL;
}
