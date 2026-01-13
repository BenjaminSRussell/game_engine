// UI-SYSTEM-001: Implement UI rendering batching system.
// UI-SYSTEM-002: Add UI rendering caching system.
// UI-SYSTEM-003: Implement UI rendering optimization.
// UI-SYSTEM-004: Add UI rendering statistics tracking.
// UI-SYSTEM-005: Implement UI rendering debugging visualization.
// UI-SYSTEM-006: Add UI rendering performance profiling.
// UI-SYSTEM-007: Implement UI rendering configuration system.
// UI-SYSTEM-008: Add UI rendering unit testing framework.
// UI-SYSTEM-009: Implement UI rendering documentation system.
// UI-SYSTEM-010: Add UI rendering optimization suggestions.
#include <core/logger.h>
#include <rendering/ui_renderer.h>
#include <rendering/vulkan.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VULKAN_BUILD

#define MAX_UI_VERTICES 4096

struct UIRenderer {
  VulkanRenderer *vk_renderer;

  // Rendering resources
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSet descriptorSet;
  VkSampler sampler;

  // Buffers
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  UIVertex *mappedVertices;

  // Current frame state
  u32 vertexCount;
};

// Helper to create shader modules
static VkShaderModule create_shader_module(VkDevice device,
                                           const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    LOG_ERROR("Failed to open shader file: %s", filename);
    return VK_NULL_HANDLE;
  }

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  u32 *buffer = malloc(fileSize);
  fread(buffer, 1, fileSize, file);
  fclose(file);

  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = fileSize;
  createInfo.pCode = buffer;

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create shader module for %s", filename);
    free(buffer);
    return VK_NULL_HANDLE;
  }

  free(buffer);
  return shaderModule;
}

void ui_renderer_init(UIRenderer **renderer, VulkanRenderer *vk_renderer) {
  *renderer = malloc(sizeof(UIRenderer));
  UIRenderer *ui = *renderer;
  ui->vk_renderer = vk_renderer;
  ui->vertexCount = 0;

  VkDevice device = vk_renderer->device;

  // 1. Create Descriptor Set Layout
  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = NULL;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerLayoutBinding;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL,
                                  &ui->descriptorSetLayout) != VK_SUCCESS) {
    LOG_ERROR("Failed to create UI descriptor set layout");
    return;
  }

  // 2. Create Pipeline Layout
  VkPushConstantRange pushConstantRange = {0};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(float) * 4; // scale (vec2) + translate (vec2)

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &ui->descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                             &ui->pipelineLayout) != VK_SUCCESS) {
    LOG_ERROR("Failed to create UI pipeline layout");
    return;
  }

  // 3. Create Graphics Pipeline
  VkShaderModule vertShaderModule = create_shader_module(
      device,
      "assets/shaders/ui.vert.spv"); // Assuming pre-compiled or will be
                                     // compiled
  VkShaderModule fragShaderModule =
      create_shader_module(device, "assets/shaders/ui.frag.spv");

  if (!vertShaderModule || !fragShaderModule) {
    LOG_ERROR(
        "UI Shaders missing. Make sure to compile assets/shaders/ui.vert and "
        "assets/shaders/ui.frag");
  }

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(UIVertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescriptions[3];
  // Position
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(UIVertex, position);
  // UV
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(UIVertex, uv);
  // Color
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(UIVertex, color);

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 3;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE; // Don't cull for 2D UI
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL; // No depth testing for UI overlay
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = ui->pipelineLayout;
  pipelineInfo.renderPass = vk_renderer->render_pass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
                                &ui->pipeline) != VK_SUCCESS) {
    LOG_ERROR("Failed to create UI graphics pipeline");
    return;
  }

  if (vertShaderModule)
    vkDestroyShaderModule(device, vertShaderModule, NULL);
  if (fragShaderModule)
    vkDestroyShaderModule(device, fragShaderModule, NULL);

  // 4. Create Vertex Buffer
  VkDeviceSize bufferSize = sizeof(UIVertex) * MAX_UI_VERTICES;
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, NULL, &ui->vertexBuffer) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create UI vertex buffer");
  }

  LOG_WARN("UI Vertex Buffer allocation logic incomplete - pending engine "
           "allocator integration");
  ui->mappedVertices = malloc(bufferSize); // Temporary CPU buffer
}

void ui_renderer_cleanup(UIRenderer *renderer) {
  if (!renderer)
    return;
  VkDevice device = renderer->vk_renderer->device;

  vkDestroyPipeline(device, renderer->pipeline, NULL);
  vkDestroyPipelineLayout(device, renderer->pipelineLayout, NULL);
  vkDestroyDescriptorSetLayout(device, renderer->descriptorSetLayout, NULL);
  vkDestroyBuffer(device, renderer->vertexBuffer, NULL);

  free(renderer->mappedVertices); // Free temp buffer
  free(renderer);
}

void ui_renderer_begin_frame(UIRenderer *renderer) {
  if (!renderer)
    return;
  renderer->vertexCount = 0;
}

void ui_renderer_draw_quad(UIRenderer *renderer, Vec2 position, Vec2 size,
                           Vec2 uv_start, Vec2 uv_end, Vec4 color) {
  if (!renderer || renderer->vertexCount + 6 > MAX_UI_VERTICES)
    return;

  UIVertex *v = &renderer->mappedVertices[renderer->vertexCount];

  // Vertices
  v[0].position = position;
  v[0].uv = uv_start;
  v[0].color = color;

  v[1].position = (Vec2){position.x + size.x, position.y};
  v[1].uv = (Vec2){uv_end.x, uv_start.y};
  v[1].color = color;

  v[2].position = (Vec2){position.x, position.y + size.y};
  v[2].uv = (Vec2){uv_start.x, uv_end.y};
  v[2].color = color;

  v[3].position = (Vec2){position.x + size.x, position.y + size.y};
  v[3].uv = uv_end;
  v[3].color = color;

  // Tri 1
  renderer->mappedVertices[renderer->vertexCount + 0] = v[0];
  renderer->mappedVertices[renderer->vertexCount + 1] = v[2];
  renderer->mappedVertices[renderer->vertexCount + 2] = v[1];

  // Tri 2
  renderer->mappedVertices[renderer->vertexCount + 3] = v[1];
  renderer->mappedVertices[renderer->vertexCount + 4] = v[2];
  renderer->mappedVertices[renderer->vertexCount + 5] = v[3];

  renderer->vertexCount += 6;
}

void ui_renderer_end_frame(UIRenderer *renderer) {
  if (!renderer || renderer->vertexCount == 0)
    return;
}

#else // Stubs for non-Vulkan builds

void ui_renderer_init(UIRenderer **renderer, VulkanRenderer *vk_renderer) {
  (void)vk_renderer;
  *renderer = NULL;
}

void ui_renderer_cleanup(UIRenderer *renderer) { (void)renderer; }

void ui_renderer_begin_frame(UIRenderer *renderer) { (void)renderer; }

void ui_renderer_draw_quad(UIRenderer *renderer, Vec2 position, Vec2 size,
                           Vec2 uv_start, Vec2 uv_end, Vec4 color) {
  (void)renderer;
  (void)position;
  (void)size;
  (void)uv_start;
  (void)uv_end;
  (void)color;
}

void ui_renderer_end_frame(UIRenderer *renderer) { (void)renderer; }

#endif // VULKAN_BUILD
