#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan graphics pipeline creation system
#define VK_MAX_GRAPHICS_PIPELINES 256
#define VK_MAX_SHADER_STAGES 8
#define VK_MAX_VERTEX_BINDINGS 16
#define VK_MAX_VERTEX_ATTRIBUTES 32

typedef struct vk_vertex_input_binding {
    u32 binding;
    u32 stride;
    VkVertexInputRate input_rate;
} vk_vertex_input_binding_t;

typedef struct vk_vertex_input_attribute {
    u32 location;
    u32 binding;
    VkFormat format;
    u32 offset;
} vk_vertex_input_attribute_t;

typedef struct vk_graphics_pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptor_layout;
    
    VkPipelineShaderStageCreateInfo shader_stages[VK_MAX_SHADER_STAGES];
    u32 shader_stage_count;
    
    VkPipelineVertexInputStateCreateInfo vertex_input;
    vk_vertex_input_binding_t vertex_bindings[VK_MAX_VERTEX_BINDINGS];
    u32 vertex_binding_count;
    vk_vertex_input_attribute_t vertex_attributes[VK_MAX_VERTEX_ATTRIBUTES];
    u32 vertex_attribute_count;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineColorBlendStateCreateInfo color_blend;
    VkPipelineDynamicStateCreateInfo dynamic_state;
    
    VkRenderPass render_pass;
    u32 subpass;
    
    char name[256];
    bool is_valid;
} vk_graphics_pipeline_t;

typedef struct vk_graphics_pipeline_manager {
    VkDevice device;
    
    vk_graphics_pipeline_t pipelines[VK_MAX_GRAPHICS_PIPELINES];
    u32 pipeline_count;
    u32 next_pipeline_id;
    
    // Statistics
    u32 total_pipelines_created;
    u32 total_pipelines_destroyed;
} vk_graphics_pipeline_manager_t;

static vk_graphics_pipeline_manager_t g_pipeline_manager = {0};

// Initialize pipeline manager
bool vk_graphics_pipeline_manager_init(VkDevice device) {
    if (!device) {
        printf("Error: Invalid device for graphics pipeline manager initialization\n");
        return false;
    }
    
    g_pipeline_manager.device = device;
    
    printf("Vulkan graphics pipeline manager initialized\n");
    return true;
}

// Cleanup pipeline manager
void vk_graphics_pipeline_manager_cleanup(void) {
    if (!g_pipeline_manager.device) {
        return;
    }
    
    // Destroy all pipelines
    for (u32 i = 0; i < g_pipeline_manager.pipeline_count; i++) {
        if (g_pipeline_manager.pipelines[i].is_valid) {
            vk_graphics_pipeline_destroy(g_pipeline_manager.pipelines[i].id);
        }
    }
    
    memset(&g_pipeline_manager, 0, sizeof(g_pipeline_manager));
    
    printf("Vulkan graphics pipeline manager cleaned up\n");
}

// Create pipeline layout
static VkPipelineLayout create_pipeline_layout(VkDevice device, VkDescriptorSetLayout descriptor_layout) {
    VkPipelineLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_layout;
    
    VkPipelineLayout layout;
    VkResult result = vkCreatePipelineLayout(device, &layout_info, NULL, &layout);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create pipeline layout\n");
        return VK_NULL_HANDLE;
    }
    
    return layout;
}

// Create graphics pipeline
u32 vk_graphics_pipeline_create(const char* name, 
                                const VkPipelineShaderStageCreateInfo* shader_stages, u32 shader_stage_count,
                                const vk_vertex_input_binding_t* vertex_bindings, u32 vertex_binding_count,
                                const vk_vertex_input_attribute_t* vertex_attributes, u32 vertex_attribute_count,
                                VkPrimitiveTopology topology,
                                VkPolygonMode polygon_mode,
                                VkCullModeFlags cull_mode,
                                VkFrontFace front_face,
                                bool depth_test_enable,
                                bool depth_write_enable,
                                VkCompareOp depth_compare_op,
                                bool blend_enable,
                                VkBlendFactor src_color_blend,
                                VkBlendFactor dst_color_blend,
                                VkRenderPass render_pass,
                                u32 subpass) {
    if (!name || !g_pipeline_manager.device || !shader_stages || shader_stage_count == 0) {
        return 0;
    }
    
    if (g_pipeline_manager.pipeline_count >= VK_MAX_GRAPHICS_PIPELINES) {
        printf("Error: Maximum graphics pipelines reached\n");
        return 0;
    }
    
    // Find free pipeline slot
    u32 pipeline_id = g_pipeline_manager.next_pipeline_id++;
    if (pipeline_id >= VK_MAX_GRAPHICS_PIPELINES) {
        printf("Error: No free graphics pipeline slots available\n");
        return 0;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    // Copy shader stages
    for (u32 i = 0; i < shader_stage_count && i < VK_MAX_SHADER_STAGES; i++) {
        pipeline->shader_stages[i] = shader_stages[i];
    }
    pipeline->shader_stage_count = shader_stage_count;
    
    // Setup vertex input
    VkVertexInputBindingDescription vertex_binding_descs[VK_MAX_VERTEX_BINDINGS];
    for (u32 i = 0; i < vertex_binding_count && i < VK_MAX_VERTEX_BINDINGS; i++) {
        vertex_binding_descs[i].binding = vertex_bindings[i].binding;
        vertex_binding_descs[i].stride = vertex_bindings[i].stride;
        vertex_binding_descs[i].inputRate = vertex_bindings[i].input_rate;
        
        pipeline->vertex_bindings[i] = vertex_bindings[i];
    }
    pipeline->vertex_binding_count = vertex_binding_count;
    
    VkVertexInputAttributeDescription vertex_attribute_descs[VK_MAX_VERTEX_ATTRIBUTES];
    for (u32 i = 0; i < vertex_attribute_count && i < VK_MAX_VERTEX_ATTRIBUTES; i++) {
        vertex_attribute_descs[i].location = vertex_attributes[i].location;
        vertex_attribute_descs[i].binding = vertex_attributes[i].binding;
        vertex_attribute_descs[i].format = vertex_attributes[i].format;
        vertex_attribute_descs[i].offset = vertex_attributes[i].offset;
        
        pipeline->vertex_attributes[i] = vertex_attributes[i];
    }
    pipeline->vertex_attribute_count = vertex_attribute_count;
    
    pipeline->vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline->vertex_input.vertexBindingDescriptionCount = vertex_binding_count;
    pipeline->vertex_input.pVertexBindingDescriptions = vertex_binding_descs;
    pipeline->vertex_input.vertexAttributeDescriptionCount = vertex_attribute_count;
    pipeline->vertex_input.pVertexAttributeDescriptions = vertex_attribute_descs;
    
    // Setup input assembly
    pipeline->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline->input_assembly.topology = topology;
    pipeline->input_assembly.primitiveRestartEnable = VK_FALSE;
    
    // Setup rasterization
    pipeline->rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline->rasterization.depthClampEnable = VK_FALSE;
    pipeline->rasterization.rasterizerDiscardEnable = VK_FALSE;
    pipeline->rasterization.polygonMode = polygon_mode;
    pipeline->rasterization.cullMode = cull_mode;
    pipeline->rasterization.frontFace = front_face;
    pipeline->rasterization.depthBiasEnable = VK_FALSE;
    pipeline->rasterization.depthBiasConstantFactor = 0.0f;
    pipeline->rasterization.depthBiasClamp = 0.0f;
    pipeline->rasterization.depthBiasSlopeFactor = 0.0f;
    pipeline->rasterization.lineWidth = 1.0f;
    
    // Setup multisampling
    pipeline->multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline->multisample.sampleShadingEnable = VK_FALSE;
    pipeline->multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline->multisample.minSampleShading = 1.0f;
    pipeline->multisample.pSampleMask = NULL;
    pipeline->multisample.alphaToCoverageEnable = VK_FALSE;
    pipeline->multisample.alphaToOneEnable = VK_FALSE;
    
    // Setup depth stencil
    pipeline->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline->depth_stencil.depthTestEnable = depth_test_enable;
    pipeline->depth_stencil.depthWriteEnable = depth_write_enable;
    pipeline->depth_stencil.depthCompareOp = depth_compare_op;
    pipeline->depth_stencil.depthBoundsTestEnable = VK_FALSE;
    pipeline->depth_stencil.minDepthBounds = 0.0f;
    pipeline->depth_stencil.maxDepthBounds = 1.0f;
    pipeline->depth_stencil.stencilTestEnable = VK_FALSE;
    pipeline->depth_stencil.front.failOp = VK_STENCIL_OP_KEEP;
    pipeline->depth_stencil.front.passOp = VK_STENCIL_OP_KEEP;
    pipeline->depth_stencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
    pipeline->depth_stencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
    pipeline->depth_stencil.front.compareMask = 0;
    pipeline->depth_stencil.front.writeMask = 0;
    pipeline->depth_stencil.front.reference = 0;
    pipeline->depth_stencil.back = pipeline->depth_stencil.front;
    
    // Setup color blend
    pipeline->color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline->color_blend.logicOpEnable = VK_FALSE;
    pipeline->color_blend.logicOp = VK_LOGIC_OP_COPY;
    pipeline->color_blend.attachmentCount = 1;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.blendEnable = blend_enable;
    color_blend_attachment.srcColorBlendFactor = src_color_blend;
    color_blend_attachment.dstColorBlendFactor = dst_color_blend;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    pipeline->color_blend.pAttachments = &color_blend_attachment;
    pipeline->color_blend.blendConstants[0] = 0.0f;
    pipeline->color_blend.blendConstants[1] = 0.0f;
    pipeline->color_blend.blendConstants[2] = 0.0f;
    pipeline->color_blend.blendConstants[3] = 0.0f;
    
    // Setup dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };
    
    pipeline->dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline->dynamic_state.dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
    pipeline->dynamic_state.pDynamicStates = dynamic_states;
    
    // Create descriptor set layout
    VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {0};
    descriptor_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout_info.bindingCount = 0;
    descriptor_layout_info.pBindings = NULL;
    
    VkResult result = vkCreateDescriptorSetLayout(g_pipeline_manager.device, &descriptor_layout_info, NULL, &pipeline->descriptor_layout);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create descriptor set layout\n");
        return 0;
    }
    
    // Create pipeline layout
    pipeline->layout = create_pipeline_layout(g_pipeline_manager.device, pipeline->descriptor_layout);
    if (pipeline->layout == VK_NULL_HANDLE) {
        printf("Error: Failed to create pipeline layout\n");
        vkDestroyDescriptorSetLayout(g_pipeline_manager.device, pipeline->descriptor_layout, NULL);
        return 0;
    }
    
    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = shader_stage_count;
    pipeline_info.pStages = pipeline->shader_stages;
    pipeline_info.pVertexInputState = &pipeline->vertex_input;
    pipeline_info.pInputAssemblyState = &pipeline->input_assembly;
    pipeline_info.pRasterizationState = &pipeline->rasterization;
    pipeline_info.pMultisampleState = &pipeline->multisample;
    pipeline_info.pDepthStencilState = &pipeline->depth_stencil;
    pipeline_info.pColorBlendState = &pipeline->color_blend;
    pipeline_info.pDynamicState = &pipeline->dynamic_state;
    pipeline_info.layout = pipeline->layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = subpass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    
    result = vkCreateGraphicsPipelines(g_pipeline_manager.device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create graphics pipeline\n");
        vkDestroyPipelineLayout(g_pipeline_manager.device, pipeline->layout, NULL);
        vkDestroyDescriptorSetLayout(g_pipeline_manager.device, pipeline->descriptor_layout, NULL);
        return 0;
    }
    
    // Store pipeline
    pipeline->render_pass = render_pass;
    pipeline->subpass = subpass;
    strncpy(pipeline->name, name, 255);
    pipeline->name[255] = '\0';
    pipeline->id = pipeline_id;
    pipeline->is_valid = true;
    
    g_pipeline_manager.pipeline_count++;
    g_pipeline_manager.total_pipelines_created++;
    
    printf("Created graphics pipeline '%s' (%u shader stages, %u vertex bindings)\n", 
           name, shader_stage_count, vertex_binding_count);
    
    return pipeline_id;
}

// Destroy graphics pipeline
bool vk_graphics_pipeline_destroy(u32 pipeline_id) {
    if (!g_pipeline_manager.device || pipeline_id == 0) {
        return false;
    }
    
    if (pipeline_id > g_pipeline_manager.next_pipeline_id) {
        printf("Error: Invalid graphics pipeline ID %u\n", pipeline_id);
        return false;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    if (!pipeline->is_valid) {
        return false;
    }
    
    // Destroy pipeline
    if (pipeline->handle != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_pipeline_manager.device, pipeline->handle, NULL);
    }
    
    // Destroy pipeline layout
    if (pipeline->layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(g_pipeline_manager.device, pipeline->layout, NULL);
    }
    
    // Destroy descriptor set layout
    if (pipeline->descriptor_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(g_pipeline_manager.device, pipeline->descriptor_layout, NULL);
    }
    
    memset(pipeline, 0, sizeof(vk_graphics_pipeline_t));
    pipeline->is_valid = false;
    
    g_pipeline_manager.total_pipelines_destroyed++;
    g_pipeline_manager.pipeline_count--;
    
    printf("Destroyed graphics pipeline '%s'\n", pipeline->name);
    return true;
}

// Get pipeline handle
VkPipeline vk_graphics_pipeline_get_handle(u32 pipeline_id) {
    if (!g_pipeline_manager.device || pipeline_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (pipeline_id > g_pipeline_manager.next_pipeline_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    return pipeline->is_valid ? pipeline->handle : VK_NULL_HANDLE;
}

// Get pipeline layout
VkPipelineLayout vk_graphics_pipeline_get_layout(u32 pipeline_id) {
    if (!g_pipeline_manager.device || pipeline_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (pipeline_id > g_pipeline_manager.next_pipeline_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    return pipeline->is_valid ? pipeline->layout : VK_NULL_HANDLE;
}

// Get descriptor set layout
VkDescriptorSetLayout vk_graphics_pipeline_get_descriptor_layout(u32 pipeline_id) {
    if (!g_pipeline_manager.device || pipeline_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (pipeline_id > g_pipeline_manager.next_pipeline_id) {
        return VK_NULL_HANDLE;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    return pipeline->is_valid ? pipeline->descriptor_layout : VK_NULL_HANDLE;
}

// Bind pipeline
bool vk_graphics_pipeline_bind(VkCommandBuffer command_buffer, u32 pipeline_id) {
    if (!command_buffer || pipeline_id == 0) {
        return false;
    }
    
    VkPipeline pipeline = vk_graphics_pipeline_get_handle(pipeline_id);
    if (pipeline == VK_NULL_HANDLE) {
        return false;
    }
    
    VkPipelineLayout layout = vk_graphics_pipeline_get_layout(pipeline_id);
    if (layout == VK_NULL_HANDLE) {
        return false;
    }
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    return true;
}

// Create simple forward rendering pipeline
u32 vk_graphics_pipeline_create_forward(const char* name, VkShaderModule vertex_shader, VkShaderModule fragment_shader, VkRenderPass render_pass) {
    VkPipelineShaderStageCreateInfo shader_stages[2];
    
    // Vertex shader stage
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[0].pSpecializationInfo = NULL;
    
    // Fragment shader stage
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";
    shader_stages[1].pSpecializationInfo = NULL;
    
    // Default vertex binding
    vk_vertex_input_binding_t vertex_binding = {0};
    vertex_binding.binding = 0;
    vertex_binding.stride = sizeof(float) * 8; // position + normal + uv
    vertex_binding.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // Default vertex attributes
    vk_vertex_input_attribute_t vertex_attributes[3];
    vertex_attributes[0].location = 0;
    vertex_attributes[0].binding = 0;
    vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[0].offset = 0;
    
    vertex_attributes[1].location = 1;
    vertex_attributes[1].binding = 0;
    vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[1].offset = sizeof(float) * 3;
    
    vertex_attributes[2].location = 2;
    vertex_attributes[2].binding = 0;
    vertex_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attributes[2].offset = sizeof(float) * 6;
    
    return vk_graphics_pipeline_create(name, shader_stages, 2, &vertex_binding, 1, vertex_attributes, 3,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
                                       VK_FRONT_FACE_COUNTER_CLOCKWISE, true, true, VK_COMPARE_OP_LESS, true,
                                       VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, render_pass, 0);
}

// Create simple deferred rendering pipeline
u32 vk_graphics_pipeline_create_deferred(const char* name, VkShaderModule vertex_shader, VkShaderModule fragment_shader, VkRenderPass render_pass) {
    VkPipelineShaderStageCreateInfo shader_stages[2];
    
    // Vertex shader stage
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";
    shader_stages[0].pSpecializationInfo = NULL;
    
    // Fragment shader stage
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";
    shader_stages[1].pSpecializationInfo = NULL;
    
    // Default vertex binding
    vk_vertex_input_binding_t vertex_binding = {0};
    vertex_binding.binding = 0;
    vertex_binding.stride = sizeof(float) * 8; // position + normal + uv
    vertex_binding.input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // Default vertex attributes
    vk_vertex_input_attribute_t vertex_attributes[3];
    vertex_attributes[0].location = 0;
    vertex_attributes[0].binding = 0;
    vertex_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[0].offset = 0;
    
    vertex_attributes[1].location = 1;
    vertex_attributes[1].binding = 0;
    vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_attributes[1].offset = sizeof(float) * 3;
    
    vertex_attributes[2].location = 2;
    vertex_attributes[2].binding = 0;
    vertex_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_attributes[2].offset = sizeof(float) * 6;
    
    return vk_graphics_pipeline_create(name, shader_stages, 2, &vertex_binding, 1, vertex_attributes, 3,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
                                       VK_FRONT_FACE_COUNTER_CLOCKWISE, true, true, VK_COMPARE_OP_LESS, false,
                                       VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, render_pass, 0);
}

// Get statistics
void vk_graphics_pipeline_get_stats(u32* total_pipelines_created, u32* total_pipelines_destroyed) {
    if (total_pipelines_created) *total_pipelines_created = g_pipeline_manager.total_pipelines_created;
    if (total_pipelines_destroyed) *total_pipelines_destroyed = g_pipeline_manager.total_pipelines_destroyed;
}

// Validate pipeline
bool vk_graphics_pipeline_validate(u32 pipeline_id) {
    if (!g_pipeline_manager.device || pipeline_id == 0) {
        return false;
    }
    
    if (pipeline_id > g_pipeline_manager.next_pipeline_id) {
        return false;
    }
    
    vk_graphics_pipeline_t* pipeline = &g_pipeline_manager.pipelines[pipeline_id - 1];
    
    return pipeline->is_valid;
}
