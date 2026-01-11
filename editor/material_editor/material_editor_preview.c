#include "../editor_common.h"
#include "core/logger.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "rendering/vulkan.h"
#include <stdio.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265359f
#endif

// Preview uniform buffer structure
typedef struct {
    Mat4 model_matrix;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    f32 time;
    Vec3 light_direction;
    Vec3 light_color;
    f32 ambient_strength;
} PreviewUniforms;

/**
 * =================================================================================================
 *                           MATERIAL EDITOR PREVIEW
 * =================================================================================================
 * 
 * Handles rendering the real-time preview of the material being edited.
 */

// Extended render state for Vulkan preview
static struct {
    u32 preview_shader;
    u32 sphere_mesh;
    u32 cube_mesh;
    u32 current_mesh;
    f32 rotation;
    
    // Vulkan rendering objects
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorSet descriptor_set;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_memory;
    u32 index_count;
} preview_state = {0};

void material_editor_preview_init(EditorContext* ctx) {
    // Generate primitive meshes for preview
    LOG_INFO("Initializing material preview system");
    
    // In a real implementation, this would:
    /*
    VulkanRenderer* vk_renderer = (VulkanRenderer*)ctx->renderer;
    if (!vk_renderer) {
        LOG_ERROR("No Vulkan renderer available for material preview");
        return;
    }
    
    // Create sphere mesh for preview
    preview_state.sphere_mesh = create_sphere_mesh(vk_renderer, 1.0f, 32, 32);
    if (preview_state.sphere_mesh == 0) {
        LOG_ERROR("Failed to create sphere mesh for preview");
    }
    
    // Create cube mesh for preview
    preview_state.cube_mesh = create_cube_mesh(vk_renderer, 1.0f);
    if (preview_state.cube_mesh == 0) {
        LOG_ERROR("Failed to create cube mesh for preview");
    }
    
    // Create preview shader pipeline
    VkShaderModule vertex_shader = load_shader_module(vk_renderer, "shaders/material_preview.vert.spv");
    VkShaderModule fragment_shader = load_shader_module(vk_renderer, "shaders/material_preview.frag.spv");
    
    if (vertex_shader != VK_NULL_HANDLE && fragment_shader != VK_NULL_HANDLE) {
        // Create pipeline for material preview
        VkPipelineShaderStageCreateInfo shader_stages[] = {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader,
                .pName = "main"
            }
        };
        
        // Create pipeline layout with descriptor sets for material parameters
        VkDescriptorSetLayoutBinding bindings[] = {
            {
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                .binding = 1,
                .descriptorCount = 4,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
            }
        };
        
        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 2,
            .pBindings = bindings
        };
        
        vkCreateDescriptorSetLayout(vk_renderer->device, &layout_info, NULL, &preview_state.descriptor_layout);
        
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipeline_layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &preview_state.descriptor_layout
        };
        
        vkCreatePipelineLayout(vk_renderer->device, &pipeline_layout_info, NULL, &preview_state.pipeline_layout);
        
        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipeline_info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shader_stages,
            .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                // Vertex binding and attribute descriptions
            },
            .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
            },
            .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
            },
            .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
            },
            .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_LESS
            },
            .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo){
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .attachmentCount = 1,
                .pAttachments = &(VkPipelineColorBlendAttachmentState){
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
                }
            },
            .layout = preview_state.pipeline_layout,
            .renderPass = vk_renderer->render_pass
        };
        
        vkCreateGraphicsPipelines(vk_renderer->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &preview_state.pipeline);
        
        // Clean up shader modules
        vkDestroyShaderModule(vk_renderer->device, vertex_shader, NULL);
        vkDestroyShaderModule(vk_renderer->device, fragment_shader, NULL);
    }
    
    // Create uniform buffer for preview parameters
    VkBufferCreateInfo ubo_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(PreviewUniforms),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    
    vulkan_create_buffer(vk_renderer, ubo_info.size, ubo_info.usage,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &preview_state.uniform_buffer, &preview_state.uniform_memory);
    */
    
    preview_state.current_mesh = preview_state.sphere_mesh;
    preview_state.rotation = 0.0f;
    
    LOG_INFO("Material preview initialized");
}

void material_editor_preview_update(f32 delta_time) {
    preview_state.rotation += delta_time * 0.5f;
}

void material_editor_preview_render(EditorContext* ctx, MaterialGraph* graph) {
    if (!graph) return;
    
    // In a real implementation, this would render the material preview using Vulkan
    /*
    VulkanRenderer* vk_renderer = (VulkanRenderer*)ctx->renderer;
    if (!vk_renderer || preview_state.pipeline == VK_NULL_HANDLE) {
        LOG_ERROR("No valid renderer or pipeline for material preview");
        return;
    }
    
    // 1. Check if we need to recompile shader
    if (graph->needs_recompile && graph->generated_fragment_shader) {
        // Create new shader module from generated GLSL
        VkShaderModule new_fragment_shader = compile_glsl_to_spirv(vk_renderer, graph->generated_fragment_shader);
        
        if (new_fragment_shader != VK_NULL_HANDLE) {
            // Recreate pipeline with new fragment shader
            // This would involve updating the shader stage create info and recreating the pipeline
            LOG_INFO("Updated preview shader from material graph");
            
            // Update descriptor sets for material parameters
            update_material_descriptors(vk_renderer, graph);
        }
        
        graph->needs_recompile = false;
    }
    
    // 2. Setup Preview Viewport and Render Pass
    VkCommandBuffer cmd = begin_preview_render_pass(vk_renderer);
    
    // 3. Bind Material Preview Pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, preview_state.pipeline);
    
    // 4. Update and Bind Uniform Buffers
    PreviewUniforms uniforms = {
        .model_matrix = mat4_rotate_y(preview_state.rotation),
        .view_matrix = mat4_look_at((Vec3){2.0f, 2.0f, 2.0f}, (Vec3){0, 0, 0}, (Vec3){0, 1, 0}),
        .projection_matrix = mat4_perspective(45.0f * PI / 180.0f, 1.0f, 0.1f, 100.0f),
        .time = get_time(),
        .light_direction = {0.5f, 1.0f, 0.3f},
        .light_color = {1.0f, 1.0f, 1.0f},
        .ambient_strength = 0.2f
    };
    
    void* uniform_data;
    vkMapMemory(vk_renderer->device, preview_state.uniform_memory, 0, sizeof(uniforms), 0, &uniform_data);
    memcpy(uniform_data, &uniforms, sizeof(uniforms));
    vkUnmapMemory(vk_renderer->device, preview_state.uniform_memory);
    
    VkDescriptorSet descriptor_sets[] = {preview_state.descriptor_set};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, preview_state.pipeline_layout,
                           0, 1, descriptor_sets, 0, NULL);
    
    // 5. Draw Preview Mesh
    if (preview_state.current_mesh != 0) {
        // Bind vertex and index buffers
        VkBuffer vertex_buffers[] = {preview_state.vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(cmd, preview_state.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        
        // Draw mesh
        vkCmdDrawIndexed(cmd, preview_state.index_count, 1, 0, 0, 0);
    }
    
    // 6. End Render Pass
    end_preview_render_pass(vk_renderer, cmd);
    */
    
    // For now, just log the rendering intent
    LOG_DEBUG("Rendering material preview: rotation=%.2f, mesh=%u", 
             preview_state.rotation, preview_state.current_mesh);
    
    // Placeholder visualization - in a real implementation this would be handled by the render pass
    // draw_rect((Vec2){10, 10}, (Vec2){200, 200}, (Vec4){0.1f, 0.1f, 0.1f, 1.0f}); // Background
}
