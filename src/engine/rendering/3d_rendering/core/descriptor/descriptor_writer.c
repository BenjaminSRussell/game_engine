#include "descriptor_writer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_WRITERS 16
#define MAX_WRITES_PER_BATCH 64

typedef struct {
    uint32_t binding;
    descriptor_type_t type;
    
    // Union of resource info
    struct {
        buffer_handle_t buffer;
        size_t offset;
        size_t range;
    } buffer_info;
    
    struct {
        texture_handle_t texture;
        sampler_handle_t sampler;
    } image_info;
    
} pending_write_t;

typedef struct {
    pending_write_t writes[MAX_WRITES_PER_BATCH];
    uint32_t write_count;
    bool is_active;
} writer_internal_t;

static struct {
    writer_internal_t writers[MAX_WRITERS];
    bool initialized;
} g_writer_manager = {0};

void descriptor_writer_init_system(void) {
    g_writer_manager.initialized = true;
    memset(g_writer_manager.writers, 0, sizeof(g_writer_manager.writers));
}

void descriptor_writer_shutdown_system(void) {
    g_writer_manager.initialized = false;
}

descriptor_writer_handle_t descriptor_writer_create(void) {
    descriptor_writer_handle_t invalid = {0};
    if (!g_writer_manager.initialized) return invalid;

    int slot = -1;
    for (int i = 0; i < MAX_WRITERS; ++i) {
        if (!g_writer_manager.writers[i].is_active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return invalid;

    writer_internal_t* w = &g_writer_manager.writers[slot];
    w->is_active = true;
    w->write_count = 0;
    
    descriptor_writer_handle_t h = {(uint32_t)slot + 1};
    return h;
}

void descriptor_writer_destroy(descriptor_writer_handle_t handle) {
    if (handle.id == 0 || handle.id > MAX_WRITERS) return;
    g_writer_manager.writers[handle.id - 1].is_active = false;
}

void descriptor_writer_write_buffer(descriptor_writer_handle_t writer, 
                                    uint32_t binding, 
                                    buffer_handle_t buffer, 
                                    size_t offset, 
                                    size_t range, 
                                    descriptor_type_t type) {
    if (writer.id == 0 || writer.id > MAX_WRITERS) return;
    writer_internal_t* w = &g_writer_manager.writers[writer.id - 1];
    
    if (w->write_count >= MAX_WRITES_PER_BATCH) return; // Overflow
    
    pending_write_t* write = &w->writes[w->write_count++];
    write->binding = binding;
    write->type = type;
    write->buffer_info.buffer = buffer;
    write->buffer_info.offset = offset;
    write->buffer_info.range = range;
}

void descriptor_writer_write_image(descriptor_writer_handle_t writer, 
                                   uint32_t binding, 
                                   texture_handle_t texture, 
                                   sampler_handle_t sampler, 
                                   descriptor_type_t type) {
    if (writer.id == 0 || writer.id > MAX_WRITERS) return;
    writer_internal_t* w = &g_writer_manager.writers[writer.id - 1];
    
    if (w->write_count >= MAX_WRITES_PER_BATCH) return; // Overflow
    
    pending_write_t* write = &w->writes[w->write_count++];
    write->binding = binding;
    write->type = type;
    write->image_info.texture = texture;
    write->image_info.sampler = sampler;
}

bool descriptor_writer_build(descriptor_writer_handle_t writer, 
                             descriptor_set_handle_t target_set) {
    if (writer.id == 0 || writer.id > MAX_WRITERS) return false;
    writer_internal_t* w = &g_writer_manager.writers[writer.id - 1];
    
    // In a real implementation:
    // 1. Convert pending_write_t to VkWriteDescriptorSet
    // 2. Call vkUpdateDescriptorSets(device, w->write_count, writes, 0, NULL);
    
    // For now, just clear
    w->write_count = 0;
    return true;
}

void descriptor_writer_clear(descriptor_writer_handle_t writer) {
    if (writer.id == 0 || writer.id > MAX_WRITERS) return;
    g_writer_manager.writers[writer.id - 1].write_count = 0;
}
