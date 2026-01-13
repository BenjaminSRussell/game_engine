#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan command buffer recording helpers
#define VK_MAX_COMMAND_BUFFERS 64
#define VK_MAX_RENDER_PASS_INSTANCES 32
#define VK_MAX_DRAW_CALLS 1024

typedef struct vk_render_pass_instance {
    VkRenderPass render_pass;
    VkFramebuffer framebuffer;
    u32 width;
    u32 height;
    VkClearValue clear_values[4];
    u32 clear_value_count;
    bool is_active;
} vk_render_pass_instance_t;

typedef struct vk_draw_call {
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    u32 vertex_count;
    u32 index_count;
    u32 first_vertex;
    u32 first_index;
    VkIndexType index_type;
    bool is_indexed;
    VkPrimitiveTopology topology;
} vk_draw_call_t;

typedef struct vk_command_buffer_recorder {
    VkCommandBuffer handle;
    VkCommandBufferLevel level;
    VkCommandBufferUsageFlags usage;
    
    vk_render_pass_instance_t render_passes[VK_MAX_RENDER_PASS_INSTANCES];
    u32 render_pass_count;
    u32 current_render_pass;
    
    vk_draw_call_t draw_calls[VK_MAX_DRAW_CALLS];
    u32 draw_call_count;
    
    VkCommandBufferBeginInfo begin_info;
    VkCommandBufferInheritanceInfo inheritance_info;
    
    char name[256];
    bool is_recording;
    bool is_active;
} vk_command_buffer_recorder_t;

typedef struct vk_command_buffer_manager {
    VkDevice device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;
    
    vk_command_buffer_recorder_t command_buffers[VK_MAX_COMMAND_BUFFERS];
    u32 command_buffer_count;
    u32 next_command_buffer_id;
    
    // Statistics
    u32 total_command_buffers_created;
    u32 total_command_buffers_destroyed;
    u32 total_render_passes_recorded;
    u32 total_draw_calls_recorded;
    u32 total_command_buffers_reset;
} vk_command_buffer_manager_t;

static vk_command_buffer_manager_t g_command_buffer_manager = {0};

// Initialize command buffer manager
bool vk_command_buffer_manager_init(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue) {
    if (!device || !command_pool || !graphics_queue) {
        printf("Error: Invalid parameters for command buffer manager initialization\n");
        return false;
    }
    
    g_command_buffer_manager.device = device;
    g_command_buffer_manager.command_pool = command_pool;
    g_command_buffer_manager.graphics_queue = graphics_queue;
    
    printf("Vulkan command buffer manager initialized\n");
    return true;
}

// Cleanup command buffer manager
void vk_command_buffer_manager_cleanup(void) {
    if (!g_command_buffer_manager.device) {
        return;
    }
    
    // Wait for device idle before cleanup
    vkDeviceWaitIdle(g_command_buffer_manager.device);
    
    // Destroy all command buffers
    for (u32 i = 0; i < g_command_buffer_manager.command_buffer_count; i++) {
        if (g_command_buffer_manager.command_buffers[i].is_active) {
            vk_command_buffer_end(g_command_buffer.command_buffers[i].id);
        }
    }
    
    memset(&g_command_buffer_manager, 0, sizeof(g_command_buffer_manager));
    
    printf("Vulkan command buffer manager cleaned up\n");
}

// Begin command buffer recording
u32 vk_command_buffer_begin(VkCommandBufferLevel level, VkCommandBufferUsageFlags usage, const char* name) {
    if (!g_command_buffer_manager.device || !name) {
        return 0;
    }
    
    if (g_command_buffer_manager.command_buffer_count >= VK_MAX_COMMAND_BUFFERS) {
        printf("Error: Maximum command buffers reached\n");
        return 0;
    }
    
    u32 command_buffer_id = g_command_buffer_manager.next_command_buffer_id++;
    if (command_buffer_id >= VK_MAX_COMMAND_BUFFERS) {
        printf("Error: No free command buffer slots available\n");
        return 0;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_manager.command_buffers[command_buffer_id - 1];
    
    // Allocate command buffer
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = level;
    alloc_info.commandPool = g_command_buffer_manager.command_pool;
    alloc_info.flags = usage;
    
    VkResult result = vkAllocateCommandBuffers(g_command_buffer_manager.device, &alloc_info, NULL, &recorder->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to allocate command buffer\n");
        return 0;
    }
    
    // Begin command buffer
    recorder->begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    recorder->begin_info.flags = 0;
    recorder->begin_info.pInheritanceInfo = NULL;
    
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        recorder->inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        recorder->inheritance_info.pNext = NULL;
        recorder->inheritance_info.framebuffer = VK_NULL_HANDLE;
        recorder->inheritance_info.occlusionQueryEnable = VK_FALSE;
        recorder->inheritance_info.queryFlags = 0;
        recorder->inheritance_info.pipelineStatistics = VK_FALSE;
        recorder->begin_info.pInheritanceInfo = &recorder->inheritance_info;
    }
    
    result = vkBeginCommandBuffer(recorder->handle, &recorder->begin_info);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to begin command buffer\n");
        vkFreeCommandBuffers(g_command_manager.device, 1, &recorder->handle, NULL);
        return 0;
    }
    
    recorder->level = level;
    recorder->usage = usage;
    strncpy(recorder->name, name, 255);
    recorder->name[255] = '\0';
    recorder->is_recording = true;
    recorder->is_active = true;
    recorder->render_pass_count = 0;
    recorder->current_render_pass = 0;
    recorder->draw_call_count = 0;
    
    g_command_buffer_manager.command_buffer_count++;
    g_command_buffer_manager.total_command_buffers_created++;
    
    printf("Began command buffer '%s' (level: %d, usage: %u)\n", name, level, usage);
    
    return command_buffer_id;
}

// End command buffer recording
bool vk_command_buffer_end(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_manager.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    // End all render passes
    for (u32 i = 0; i < recorder->render_pass_count; i++) {
        if (recorder->render_passes[i].is_active) {
            vk_command_buffer_end_render_pass(command_buffer_id, i);
        }
    }
    
    // End command buffer
    VkResult result = vkEndCommandBuffer(recorder->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to end command buffer\n");
        return false;
    }
    
    recorder->is_recording = false;
    g_command_buffer_manager.total_command_buffers_destroyed++;
    
    printf("Ended command buffer '%s'\n", recorder->name);
    
    return true;
}

// Reset command buffer
bool vk_command_buffer_reset(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_manager.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_active) {
        return false;
    }
    
    VkResult result = vkResetCommandBuffer(recorder->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to reset command buffer\n");
        return false;
    }
    
    recorder->render_pass_count = 0;
    recorder->current_render_pass = 0;
    recorder->draw_call_count = 0;
    
    g_command_buffer_manager.total_command_buffers_reset++;
    
    printf("Reset command buffer '%s'\n", recorder->name);
    
    return true;
}

// Begin render pass
bool vk_command_buffer_begin_render_pass(u32 command_buffer_id, u32 render_pass_index, VkRenderPass render_pass, VkFramebuffer framebuffer, u32 width, u32 height) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_manager.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    if (render_pass_index >= VK_MAX_RENDER_PASS_INSTANCES) {
        printf("Error: Maximum render pass instances reached\n");
        return false;
    }
    
    // End current render pass if active
    if (recorder->current_render_pass < recorder->render_pass_count) {
        vk_command_buffer_end_render_pass(command_buffer_id, recorder->current_render_pass);
    }
    
    // Begin new render pass
    vk_render_pass_instance_t* render_pass = &recorder->render_passes[render_pass_index];
    
    render_pass->render_pass = render_pass;
    render_pass->framebuffer = framebuffer;
    render_pass->width = width;
    render_pass->height = height;
    render_pass->clear_value_count = 0;
    render_pass->is_active = true;
    
    // Set default clear values
    render_pass->clear_values[0].color.float32[0] = 0.0f;
    render_pass->clear_values[0].color.float32[1] = 0.0f;
    render_pass->clear_values[0].color.float32[2] = 0.0f;
    render_pass->clear_values[0].color.float32[3] = 1.0f;
    render_pass->clear_values[1].depthStencil.depth = 1.0f;
    render_pass->clear_values[1].stencil = 0;
    
    render_pass->clear_value_count = 2;
    
    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffer;
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.extent.width = width;
    render_pass_info.renderArea.extent.height = height;
    render_pass_info.clearValueCount = render_pass->clear_value_count;
    render_pass_info.pClearValues = render_pass->clear_values;
    
    VkResult result = vkCmdBeginRenderPass(recorder->handle, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE_BIT);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to begin render pass\n");
        return false;
    }
    
    recorder->current_render_pass = render_pass_index;
    recorder->render_pass_count++;
    g_command_buffer_manager.total_render_passes_recorded++;
    
    printf("Began render pass %u in command buffer '%s'\n", render_pass_index, recorder->name);
    
    return true;
}

// End render pass
bool vk_command_buffer_end_render_pass(u32 command_buffer_id, u32 render_pass_index) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    if (render_pass_index >= recorder->render_pass_count) {
        printf("Error: Invalid render pass index %u\n", render_pass_index);
        return false;
    }
    
    vk_render_pass_instance_t* render_pass = &recorder->render_passes[render_pass_index];
    
    if (!render_pass->is_active) {
        return false;
    }
    
    vkCmdEndRenderPass(recorder->handle);
    
    render_pass->is_active = false;
    
    recorder->current_render_pass = (render_pass_index > 0) ? render_pass_index - 1 : 0;
    
    printf("Ended render pass %u in command buffer '%s'\n", render_pass_index, recorder->name);
    
    return true;
}

// Bind pipeline
bool vk_command_buffer_bind_pipeline(u32 command_buffer_id, VkPipeline pipeline) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_manager.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    vkCmdBindPipeline(recorder->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    return true;
}

// Bind vertex buffer
bool vk_command_buffer_bind_vertex_buffer(u32 command_buffer_id, VkBuffer vertex_buffer, VkDeviceSize offset, u32 binding) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    VkDeviceSize buffers[1] = {vertex_buffer};
    VkDeviceSize offsets[1] = {offset};
    VkDeviceSize sizes[1] = {VK_WHOLE_SIZE};
    
    vkCmdBindVertexBuffers(recorder->handle, binding, 1, buffers, offsets, sizes);
    
    return true;
}

// Bind index buffer
bool vk_command_buffer_bind_index_buffer(u32 command_buffer_id, VkBuffer index_buffer, VkDeviceSize offset) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer.command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    VkDeviceSize buffers[1] = {index_buffer};
    VkDeviceSize offsets[1] = {offset};
    VkDeviceSize sizes[1] = {VK_WHOLE_SIZE};
    
    vkCmdBindIndexBuffer(recorder->handle, 0, buffers, offsets, sizes);
    
    return true;
}

// Bind descriptor sets
bool vk_command_buffer_bind_descriptor_sets(u32 command_buffer_id, VkPipelineLayout layout, VkDescriptorSet* descriptor_sets, u32 descriptor_set_count, u32 first_set, u32 dynamic_offset_count, const u32* dynamic_offsets) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    vkCmdBindDescriptorSets(recorder->handle, layout, descriptor_set_count, descriptor_sets, first_set, dynamic_offset_count, dynamic_offsets);
    
    return true;
}

// Set viewport
bool vk_command_buffer_set_viewport(u32 command_buffer, f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    VkViewport viewport = {x, y, width, height, min_depth, max_depth};
    vkCmdSetViewport(recorder->handle, 0, 1, &viewport);
    
    return true;
}

// Set scissor
bool vk_command_buffer_set_scissor(u32 command_buffer, f32 x, f32 y, f32 width, f32 height) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    VkRect2D scissor = {x, y, width, height};
    vkCmdSetScissor(recorder->handle, 0, 1, &scissor);
    
    return true;
}

// Draw vertices
bool vk_command_buffer_draw(u32 command_buffer_id, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    vkCmdDraw(recorder->handle, vertex_count, instance_count, first_vertex, first_instance);
    
    g_command_buffer_manager.total_draw_calls_recorded++;
    
    return true;
}

// Draw indexed
bool vk_command_buffer_draw_indexed(u32 command_buffer_id, u32 index_count, u32 instance_count, u32 first_index, u32 first_instance) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    vkCmdDrawIndexed(recorder->handle, index_count, instance_count, first_index, first_instance);
    
    g_command_buffer_manager.total_draw_calls_recorded++;
    
    return true;
}

// Draw call helper
bool vk_command_buffer_draw_call(u32 command_buffer_id, const vk_draw_call_t* draw_call) {
    if (!g_command_buffer_manager.device || !draw_call || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    // Bind vertex buffer
    if (draw_call->vertex_buffer != VK_NULL_HANDLE) {
        vk_command_buffer_bind_vertex_buffer(command_buffer_id, draw_call->vertex_buffer, draw_call->first_vertex * sizeof(float) * 8, 0);
    }
    
    // Bind index buffer
    if (draw_call->is_indexed && draw_call->index_buffer != VK_NULL_HANDLE) {
        vk_command_buffer_bind_index_buffer(command_buffer_id, draw_call->index_buffer, draw_call->first_index * sizeof(u32));
    }
    
    // Draw
    if (draw_call->is_indexed) {
        vk_command_buffer_draw_indexed(command_buffer_id, draw_call->index_count, 1, draw_call->first_index, 0);
    } else {
        vk_command_buffer_draw(command_buffer_id, draw_call->vertex_count, 1, draw_call->first_vertex, 0);
    }
    
    g_command_buffer.total_draw_calls_recorded++;
    
    return true;
}

// Add draw call
bool vk_command_buffer_add_draw_call(u32 command_buffer_id, const vk_draw_call_t* draw_call) {
    if (!g_command_buffer_manager.device || !draw_call || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    if (recorder->draw_call_count >= VK_MAX_DRAW_CALLS) {
        printf("Error: Maximum draw calls reached\n");
        return false;
    }
    
    vk_draw_call_t* draw_call = &recorder->draw_calls[recorder->draw_call_count++];
    
    *draw_call = *draw_call;
    
    g_command_buffer.total_draw_calls_recorded++;
    
    return true;
}

// Execute draw calls
bool vk_command_buffer_execute_draw_calls(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_recording) {
        return false;
    }
    
    // Execute all draw calls
    for (u32 i = 0; i < recorder->draw_call_count; i++) {
        vk_command_buffer_draw_call(command_buffer_id, &recorder->draw_calls[i]);
    }
    
    recorder->draw_call_count = 0;
    
    return true;
}

// Submit command buffer
bool vk_command_buffer_submit(u32 command_buffer_id, VkSemaphore* wait_semaphores, u32 wait_semaphore_count, VkSemaphore* signal_semaphores, u32 signal_semaphore_count, VkFence fence) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        printf("Error: Invalid command buffer ID %u\n", command_buffer_id);
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (recorder->is_recording) {
        printf("Error: Cannot submit recording command buffer\n");
        return false;
    }
    
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.signalSemaphoreCount = signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &recorder->handle;
    submit_info.pNext = NULL;
    
    VkResult result = vkQueueSubmit2(g_command_manager.graphics_queue, &submit_info, fence);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to submit command buffer\n");
        return false;
    }
    
    printf("Submitted command buffer '%s'\n", recorder->name);
    
    return true;
}

// Get command buffer handle
VkCommandBuffer vk_command_buffer_get_handle(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    return recorder->is_active ? recorder->handle : VK_NULL_HANDLE;
}

// Get command buffer info
bool vk_command_buffer_get_info(u32 command_buffer_id, char* name, size_t name_size, VkCommandBufferLevel* level, VkCommandBufferUsageFlags* usage) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        return false;
    }
    
    vk_command_buffer_recorder_t* recorder = &g_command_buffer_command_buffers[command_buffer_id - 1];
    
    if (!recorder->is_active) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, recorder->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (level) *level = recorder->level;
    if (usage) *usage = recorder->usage;
    
    return true;
}

// Get statistics
void vk_command_buffer_get_stats(u32* total_command_buffers_created, u32* total_command_buffers_destroyed, u32* total_render_passes_recorded, u32* total_draw_calls_recorded, u32* total_command_buffers_reset) {
    if (total_command_buffers_created) *total_command_buffers_created = g_command_buffer_manager.total_command_buffers_created;
    if (total_command_buffers_destroyed) *total_command_buffers_destroyed = g_command_buffer_manager.total_command_buffers_destroyed;
    if (total_render_passes_recorded) *total_render_passes_recorded = g_command_manager.total_render_passes_recorded;
    if (total_draw_calls_recorded) *total_draw_calls_recorded = g_command_manager.total_draw_calls_recorded;
    if (total_command_buffers_reset) *total_command_buffers_reset = g_command_buffer_manager.total_command_buffers_reset;
}

// Check if command buffer is recording
bool vk_command_buffer_is_recording(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        return false;
    }
    
    return g_command_buffer_command_buffers[command_buffer_id - 1].is_recording;
}

// Check if command buffer is active
bool vk_command_buffer_is_active(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return false;
    }
    
    if (command_buffer_id > g_command_buffer_manager.next_command_buffer_id) {
        return false;
    }
    
    return g_command_buffer_command_buffers[command_buffer_id - 1].is_active;
}

// Get active command buffer count
u32 vk_command_buffer_get_active_count(void) {
    u32 count = 0;
    for (u32 i = 0; i < g_command_buffer_manager.command_buffer_count; i++) {
        if (g_command_buffer_command_buffers[i].is_active) {
            count++;
        }
    }
    return count;
}

// Get current render pass
u32 vk_command_buffer_get_current_render_pass(u32 command_buffer_id) {
    if (!g_command_buffer_manager.device || command_buffer_id == 0) {
        return 0;
    }
    
    if (command_buffer_id > g_command_manager.next_command_buffer_id) {
        return 0;
    }
    
    return g_command_buffer_command_buffers[command_buffer_id - 1].current_render_pass;
}
