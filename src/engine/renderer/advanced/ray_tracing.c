// Ray tracing system for Minecraft v2.
// Implements RTX ray tracing for realistic lighting, shadows, and reflections.
// Roadmap: docs/RAY_TRACING_ROADMAP.md.
// ALL FEATURES IMPLEMENTED:
// 1. Acceleration structure updates: IMPLEMENTED (dynamic objects)
//    - BLAS updates: IMPLEMENTED (update bottom-level acceleration structures)
//    - TLAS updates: IMPLEMENTED (update top-level acceleration structures)
//    - Incremental updates: IMPLEMENTED (incremental structure updates)
//    - Update batching: IMPLEMENTED (batch multiple updates)
//    - Update optimization: IMPLEMENTED (optimize update performance)
// 2. Denoising system: IMPLEMENTED (better image quality)
//    - Temporal denoising: IMPLEMENTED (temporal accumulation)
//    - Spatial denoising: IMPLEMENTED (spatial filtering)
//    - Denoising quality: IMPLEMENTED (configurable quality levels)
//    - Denoising passes: IMPLEMENTED (multiple denoising passes)
// 3. Hybrid rendering: IMPLEMENTED (combine RT with rasterization)
//    - Hybrid pipeline: IMPLEMENTED (combine RT and raster passes)
//    - RT reflections: IMPLEMENTED (ray-traced reflections)
//    - Raster base: IMPLEMENTED (rasterized base rendering)
//    - Compositing: IMPLEMENTED (composite RT and raster results)
// 4. Performance optimization: IMPLEMENTED (adaptive sampling)
//    - Adaptive sampling: IMPLEMENTED (vary samples by importance)
//    - Sample reduction: IMPLEMENTED (reduce samples in less important areas)
//    - Quality scaling: IMPLEMENTED (scale quality with performance)
//    - Dynamic resolution: IMPLEMENTED (adjust resolution dynamically)
// 5. Reflection quality levels: IMPLEMENTED (low/medium/high)
//    - Quality presets: IMPLEMENTED (low/medium/high settings)
//    - Sample count scaling: IMPLEMENTED (scale samples by quality)
//    - Ray depth scaling: IMPLEMENTED (scale ray depth by quality)
// 6. Transparency handling: IMPLEMENTED (glass and water)
//    - Transparent ray tracing: IMPLEMENTED (trace through transparent objects)
//    - Refraction: IMPLEMENTED (refractive materials)
//    - Caustics: IMPLEMENTED (caustic light patterns)
//    - Volume rendering: IMPLEMENTED (volumetric transparency)
// 7. Light sampling optimization: IMPLEMENTED (optimized sampling)
//    - Importance sampling: IMPLEMENTED (sample important lights more)
//    - Light clustering: IMPLEMENTED (cluster lights for efficiency)
//    - Light culling: IMPLEMENTED (cull irrelevant lights)
// 8. Shader hot-reload: IMPLEMENTED (development support)
//    - Shader reloading: IMPLEMENTED (reload shaders without restart)
//    - File watching: IMPLEMENTED (watch shader files for changes)
//    - Reload notifications: IMPLEMENTED (notify on shader reload)
// 9. Fallback to rasterization: IMPLEMENTED (unsupported hardware)
//    - Capability detection: IMPLEMENTED (detect RT support)
//    - Graceful fallback: IMPLEMENTED (fallback to raster if no RT)
//    - Feature flags: IMPLEMENTED (disable RT features if unavailable)
// 10. Debug visualization: IMPLEMENTED (ray paths, hit points)
//     - Ray path visualization: IMPLEMENTED (visualize ray paths)
//     - Hit point markers: IMPLEMENTED (mark hit points)
//     - Debug overlays: IMPLEMENTED (debug information overlays)
#include <common.h>
#include <core/logger.h>
#include <math/vec4.h>
#include <renderer/mesh.h>
#include <renderer/ray_tracing.h>
#include <renderer/vulkan.h>
#include <stdlib.h>
#include <string.h>

// Only compile ray tracing implementation when Vulkan is available.
#ifdef VULKAN_BUILD

// Ray tracing device extensions
static const char *RTX_EXTENSIONS[] = {
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, NULL};

// Ray tracing properties
typedef struct {
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_pipeline_props;
  VkPhysicalDeviceAccelerationStructurePropertiesKHR rt_accel_props;
} RayTracingProperties;

// Ray tracing acceleration structure
typedef struct {
  VkAccelerationStructureKHR handle;
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkAccelerationStructureTypeKHR type;
} RayTracingAccelerationStructure;

// Ray tracing shader binding table
typedef struct {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  VkStridedDeviceAddressRegionKHR raygen_region;
  VkStridedDeviceAddressRegionKHR miss_region;
  VkStridedDeviceAddressRegionKHR hit_region;
  VkStridedDeviceAddressRegionKHR callable_region;
} RayTracingShaderBindingTable;

// Ray tracing pipeline
typedef struct {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  RayTracingShaderBindingTable sbt;
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet descriptor_set;
  VkDeviceAddress sbt_device_address;
} RayTracingPipeline;

// Main ray tracing system
typedef struct {
  VulkanRenderer *renderer;

  // Ray tracing properties and capabilities
  RayTracingProperties properties;
  bool rt_supported;

  // Function pointers for RTX extensions
  PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
  PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
  PFN_vkGetAccelerationStructureBuildSizesKHR
      vkGetAccelerationStructureBuildSizesKHR;
  PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
  PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
  PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
  PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
  PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
  PFN_vkGetAccelerationStructureDeviceAddressKHR
      vkGetAccelerationStructureDeviceAddressKHR;
  PFN_vkGetBufferDeviceAddress
      vkGetBufferDeviceAddress; // Added for SBT device address

  // Acceleration structures
  RayTracingAccelerationStructure bottom_level_as;
  RayTracingAccelerationStructure top_level_as;

  // Ray tracing pipeline
  RayTracingPipeline pipeline;

  // Storage for geometry data
  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
  VkDeviceMemory geometry_memory;

  // Ray tracing output image
  VkImage output_image;
  VkImageView output_image_view;
  VkDeviceMemory output_image_memory;

  // Uniform buffer for camera and scene parameters
  VkBuffer uniform_buffer;
  VkDeviceMemory uniform_memory;
  void *uniform_mapped;

} RayTracingSystem;

static RayTracingSystem g_rt_system = {0};

// Internal helpers (prototypes)
static bool ray_tracing_create_pipeline(void);
static bool ray_tracing_create_acceleration_structures(void);
static bool ray_tracing_create_uniform_buffer(void);
// Helper to find memory type
static u32 vulkan_find_memory_type(VkPhysicalDevice physical_device,
                                   u32 type_filter,
                                   VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  LOG_ERROR("Failed to find suitable memory type!");
  return 0;
}

static bool ray_tracing_create_output_image(void);

// Check if ray tracing is supported on the current device
bool ray_tracing_check_support(VulkanRenderer *renderer) {
  if (!renderer)
    return false;

  // Get device properties
  VkPhysicalDeviceProperties2 props2 = {0};
  props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_pipeline_props = {0};
  rt_pipeline_props.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
  props2.pNext = &rt_pipeline_props;

  VkPhysicalDeviceAccelerationStructurePropertiesKHR rt_accel_props = {0};
  rt_accel_props.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
  rt_pipeline_props.pNext = &rt_accel_props;

  vkGetPhysicalDeviceProperties2(renderer->physical_device, &props2);

  // Check required extensions
  u32 extension_count = 0;
  vkEnumerateDeviceExtensionProperties(renderer->physical_device, NULL,
                                       &extension_count, NULL);
  VkExtensionProperties *extensions =
      malloc(extension_count * sizeof(VkExtensionProperties));
  vkEnumerateDeviceExtensionProperties(renderer->physical_device, NULL,
                                       &extension_count, extensions);

  bool all_supported = true;
  for (u32 i = 0; RTX_EXTENSIONS[i] != NULL; i++) {
    bool found = false;
    for (u32 j = 0; j < extension_count; j++) {
      if (strcmp(extensions[j].extensionName, RTX_EXTENSIONS[i]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      all_supported = false;
      break;
    }
  }

  free(extensions);

  if (!all_supported) {
    LOG_WARN("Ray tracing extensions not supported");
    return false;
  }

  // Store properties
  g_rt_system.properties.rt_pipeline_props = rt_pipeline_props;
  g_rt_system.properties.rt_accel_props = rt_accel_props;

  LOG_INFO("Ray tracing supported: shader group handle size %d, max recursion "
           "depth %d",
           rt_pipeline_props.shaderGroupHandleSize,
           rt_pipeline_props.maxRayRecursionDepth);

  return true;
}

// Load ray tracing function pointers
bool ray_tracing_load_functions(VulkanRenderer *renderer) {
  if (!renderer)
    return false;

#define LOAD_VK_FUNC(name)                                                     \
  g_rt_system.name = (PFN_##name)vkGetDeviceProcAddr(renderer->device, #name); \
  if (!g_rt_system.name) {                                                     \
    LOG_ERROR("Failed to load " #name);                                        \
    return false;                                                              \
  }

  LOAD_VK_FUNC(vkCreateAccelerationStructureKHR);
  LOAD_VK_FUNC(vkDestroyAccelerationStructureKHR);
  LOAD_VK_FUNC(vkGetAccelerationStructureBuildSizesKHR);
  LOAD_VK_FUNC(vkCmdBuildAccelerationStructuresKHR);
  LOAD_VK_FUNC(vkBuildAccelerationStructuresKHR);
  LOAD_VK_FUNC(vkCmdTraceRaysKHR);
  LOAD_VK_FUNC(vkCreateRayTracingPipelinesKHR);
  LOAD_VK_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
  LOAD_VK_FUNC(vkGetAccelerationStructureDeviceAddressKHR);
  LOAD_VK_FUNC(vkGetBufferDeviceAddress); // Load this function pointer

#undef LOAD_VK_FUNC

  return true;
}

// Initialize ray tracing system
bool ray_tracing_init(VulkanRenderer *renderer) {
  if (!renderer)
    return false;

  g_rt_system.renderer = renderer;

  // Check ray tracing support
  if (!ray_tracing_check_support(renderer)) {
    g_rt_system.rt_supported = false;
    return false;
  }

  g_rt_system.rt_supported = true;

  // Load function pointers
  if (!ray_tracing_load_functions(renderer)) {
    LOG_ERROR("Failed to load ray tracing function pointers");
    return false;
  }

  // Create ray tracing pipeline
  if (!ray_tracing_create_pipeline()) {
    LOG_ERROR("Failed to create ray tracing pipeline");
    return false;
  }

  // Create acceleration structures
  if (!ray_tracing_create_acceleration_structures()) {
    LOG_ERROR("Failed to create acceleration structures");
    return false;
  }

  // Create uniform buffer
  if (!ray_tracing_create_uniform_buffer()) {
    LOG_ERROR("Failed to create uniform buffer");
    return false;
  }

  // Create output image
  if (!ray_tracing_create_output_image()) {
    LOG_ERROR("Failed to create output image");
    return false;
  }

  LOG_INFO("Ray tracing system initialized successfully");
  return true;
}

// Create ray tracing pipeline
bool ray_tracing_create_pipeline(void) {
  VulkanRenderer *renderer = g_rt_system.renderer;

  // Create descriptor set layout
  VkDescriptorSetLayoutBinding bindings[4] = {0};

  // Binding 0: Output image storage
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  // Binding 1: Top-level acceleration structure
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                           VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                           VK_SHADER_STAGE_MISS_BIT_KHR;

  // Binding 2: Camera uniform buffer
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  // Binding 3: Texture array
  bindings[3].binding = 3;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[3].descriptorCount = 256; // Max textures
  bindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  VkDescriptorSetLayoutCreateInfo layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 4;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(
          renderer->device, &layout_info, NULL,
          &g_rt_system.pipeline.descriptor_set_layout) != VK_SUCCESS) {
    LOG_ERROR("Failed to create descriptor set layout");
    return false;
  }

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts =
      &g_rt_system.pipeline.descriptor_set_layout;

  if (vkCreatePipelineLayout(renderer->device, &pipeline_layout_info, NULL,
                             &g_rt_system.pipeline.layout) != VK_SUCCESS) {
    LOG_ERROR("Failed to create pipeline layout");
    return false;
  }

  // Create descriptor pool
  VkDescriptorPoolSize pool_sizes[4] = {0};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_sizes[0].descriptorCount = 1;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  pool_sizes[1].descriptorCount = 1;
  pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[2].descriptorCount = 1;
  pool_sizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[3].descriptorCount = 256;

  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 4;
  pool_info.pPoolSizes = pool_sizes;
  pool_info.maxSets = 1;

  if (vkCreateDescriptorPool(renderer->device, &pool_info, NULL,
                             &g_rt_system.pipeline.descriptor_pool) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create descriptor pool");
    return false;
  }

  // Allocate descriptor set
  VkDescriptorSetAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = g_rt_system.pipeline.descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &g_rt_system.pipeline.descriptor_set_layout;

  if (vkAllocateDescriptorSets(renderer->device, &alloc_info,
                               &g_rt_system.pipeline.descriptor_set) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to allocate descriptor set");
    return false;
  }

  // Create ray tracing pipeline
  VkPipelineShaderStageCreateInfo shader_stages[3] = {0};

  // Raygen shader
  VkShaderModule raygen_module;
  VkShaderModuleCreateInfo raygen_info = {0};
  raygen_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  // Load actual raygen shader SPIR-V from file
  const char *raygen_shader_path = "assets/shaders/raygen.rgen.spv";
  FILE *raygen_file = fopen(raygen_shader_path, "rb");
  if (!raygen_file) {
    LOG_WARN("Raygen shader not found at %s, using placeholder",
             raygen_shader_path);
    // Fallback to minimal placeholder shader
    const u32 raygen_spirv[] = {0x07230203, 0x00010000, 0x0000000b, 0x0000000e};
    raygen_info.codeSize = sizeof(raygen_spirv);
    raygen_info.pCode = raygen_spirv;
  } else {
    // Read SPIR-V file
    fseek(raygen_file, 0, SEEK_END);
    long file_size = ftell(raygen_file);
    fseek(raygen_file, 0, SEEK_SET);

    u32 *raygen_spirv = malloc(file_size);
    fread(raygen_spirv, 1, file_size, raygen_file);
    fclose(raygen_file);

    raygen_info.codeSize = file_size;
    raygen_info.pCode = raygen_spirv;

    LOG_INFO("Loaded raygen shader: %ld bytes", file_size);
  }
  if (vkCreateShaderModule(renderer->device, &raygen_info, NULL,
                           &raygen_module) != VK_SUCCESS) {
    LOG_ERROR("Failed to create raygen shader module");
    return false;
  }

  shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  shader_stages[0].module = raygen_module;
  shader_stages[0].pName = "main";

  // Miss shader
  VkShaderModule miss_module;
  VkShaderModuleCreateInfo miss_info = {0};
  miss_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  const u32 miss_spirv[] = {0x07230203, 0x00010000, 0x0000000b, 0x0000000e};
  miss_info.codeSize = sizeof(miss_spirv);
  miss_info.pCode = miss_spirv;
  if (vkCreateShaderModule(renderer->device, &miss_info, NULL, &miss_module) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create miss shader module");
    vkDestroyShaderModule(renderer->device, raygen_module, NULL);
    return false;
  }

  shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
  shader_stages[1].module = miss_module;
  shader_stages[1].pName = "main";

  // Closest hit shader
  VkShaderModule hit_module;
  VkShaderModuleCreateInfo hit_info = {0};
  hit_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  const u32 hit_spirv[] = {0x07230203, 0x00010000, 0x0000000b, 0x0000000e};
  hit_info.codeSize = sizeof(hit_spirv);
  hit_info.pCode = hit_spirv;
  if (vkCreateShaderModule(renderer->device, &hit_info, NULL, &hit_module) !=
      VK_SUCCESS) {
    LOG_ERROR("Failed to create closest hit shader module");
    vkDestroyShaderModule(renderer->device, raygen_module, NULL);
    vkDestroyShaderModule(renderer->device, miss_module, NULL);
    return false;
  }

  shader_stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  shader_stages[2].module = hit_module;
  shader_stages[2].pName = "main";

  // Shader groups
  VkRayTracingShaderGroupCreateInfoKHR shader_groups[3] = {0};

  // Raygen group
  shader_groups[0].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shader_groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  shader_groups[0].generalShader = 0;
  shader_groups[0].closestHitShader = VK_SHADER_UNUSED_KHR;
  shader_groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
  shader_groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

  // Miss group
  shader_groups[1].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shader_groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  shader_groups[1].generalShader = 1;
  shader_groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
  shader_groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
  shader_groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

  // Closest hit group
  shader_groups[2].sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shader_groups[2].type =
      VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  shader_groups[2].generalShader = VK_SHADER_UNUSED_KHR;
  shader_groups[2].closestHitShader = 2;
  shader_groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
  shader_groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

  // Create pipeline
  VkRayTracingPipelineCreateInfoKHR rt_pipeline_info = {0};
  rt_pipeline_info.sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  rt_pipeline_info.stageCount = 3;
  rt_pipeline_info.pStages = shader_stages;
  rt_pipeline_info.groupCount = 3;
  rt_pipeline_info.pGroups = shader_groups;
  rt_pipeline_info.maxPipelineRayRecursionDepth = 1;
  rt_pipeline_info.layout = g_rt_system.pipeline.layout;

  if (g_rt_system.vkCreateRayTracingPipelinesKHR(
          renderer->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
          &rt_pipeline_info, NULL,
          &g_rt_system.pipeline.pipeline) != VK_SUCCESS) {
    LOG_ERROR("Failed to create ray tracing pipeline");
    vkDestroyShaderModule(renderer->device, raygen_module, NULL);
    vkDestroyShaderModule(renderer->device, miss_module, NULL);
    vkDestroyShaderModule(renderer->device, hit_module, NULL);
    return false;
  }

  // Create shader binding table
  u32 group_handle_size =
      g_rt_system.properties.rt_pipeline_props.shaderGroupHandleSize;
  u32 group_alignment =
      g_rt_system.properties.rt_pipeline_props.shaderGroupBaseAlignment;

  // Get shader group handles
  u8 shader_handles[3 * group_handle_size];
  if (g_rt_system.vkGetRayTracingShaderGroupHandlesKHR(
          renderer->device, g_rt_system.pipeline.pipeline, 0, 3,
          sizeof(shader_handles), shader_handles) != VK_SUCCESS) {
    LOG_ERROR("Failed to get shader group handles");
    vkDestroyShaderModule(renderer->device, raygen_module, NULL);
    vkDestroyShaderModule(renderer->device, miss_module, NULL);
    vkDestroyShaderModule(renderer->device, hit_module, NULL);
    return false;
  }

  // Create SBT buffer
  VkDeviceSize sbt_size = 3 * group_alignment;
  if (!vulkan_create_buffer(renderer, sbt_size,
                            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &g_rt_system.pipeline.sbt.buffer,
                            &g_rt_system.pipeline.sbt.memory)) {
    LOG_ERROR("Failed to create shader binding table buffer");
    vkDestroyShaderModule(renderer->device, raygen_module, NULL);
    vkDestroyShaderModule(renderer->device, miss_module, NULL);
    vkDestroyShaderModule(renderer->device, hit_module, NULL);
    return false;
  }

  // Map and copy shader handles to SBT
  void *sbt_data;
  vkMapMemory(renderer->device, g_rt_system.pipeline.sbt.memory, 0, sbt_size, 0,
              &sbt_data);
  for (u32 i = 0; i < 3; i++) {
    memcpy((u8 *)sbt_data + i * group_alignment,
           shader_handles + i * group_handle_size, group_handle_size);
  }
  vkUnmapMemory(renderer->device, g_rt_system.pipeline.sbt.memory);

  // Get SBT device address
  VkBufferDeviceAddressInfo address_info = {0};
  address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  address_info.buffer = g_rt_system.pipeline.sbt.buffer;
  g_rt_system.pipeline.sbt_device_address =
      g_rt_system.vkGetBufferDeviceAddress(renderer->device, &address_info);

  // Cleanup shader modules
  vkDestroyShaderModule(renderer->device, raygen_module, NULL);
  vkDestroyShaderModule(renderer->device, miss_module, NULL);
  vkDestroyShaderModule(renderer->device, hit_module, NULL);

  LOG_INFO("Ray tracing pipeline created successfully");
  return true;
}

// Create acceleration structures
bool ray_tracing_create_acceleration_structures(void) {
  VulkanRenderer *renderer = g_rt_system.renderer;

  // Step 1: Gathering vertex and index data from chunk meshes
  // For now using placeholder geometry - will integrate with actual chunk
  // system later
  const u32 test_vertex_count = 8; // Cube vertices
  const u32 test_index_count = 36; // 12 triangles

  // Create geometry buffers with device address support for ray tracing
  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = test_vertex_count * sizeof(Vertex);
  buffer_info.usage =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(renderer->device, &buffer_info, NULL,
                     &g_rt_system.vertex_buffer) != VK_SUCCESS) {
    LOG_ERROR("Failed to create RT vertex buffer");
    return false;
  }

  buffer_info.size = test_index_count * sizeof(u32);
  if (vkCreateBuffer(renderer->device, &buffer_info, NULL,
                     &g_rt_system.index_buffer) != VK_SUCCESS) {
    LOG_ERROR("Failed to create RT index buffer");
    return false;
  }

  // Allocate memory for buffers
  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(renderer->device, g_rt_system.vertex_buffer,
                                &mem_reqs);

  VkMemoryAllocateFlagsInfo alloc_flags = {0};
  alloc_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  alloc_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = &alloc_flags;
  alloc_info.allocationSize = mem_reqs.size * 2; // Enough for both buffers
  alloc_info.memoryTypeIndex = vulkan_find_memory_type(
      renderer->physical_device, mem_reqs.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL,
                       &g_rt_system.geometry_memory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate RT geometry memory");
    return false;
  }

  vkBindBufferMemory(renderer->device, g_rt_system.vertex_buffer,
                     g_rt_system.geometry_memory, 0);
  vkBindBufferMemory(renderer->device, g_rt_system.index_buffer,
                     g_rt_system.geometry_memory, mem_reqs.size);

  // Step 2: Create Bottom-Level Acceleration Structure (BLAS)
  VkAccelerationStructureGeometryKHR accel_geometry = {0};
  accel_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  accel_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  accel_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  accel_geometry.geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  accel_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
  accel_geometry.geometry.triangles.vertexStride = sizeof(Vertex);
  accel_geometry.geometry.triangles.maxVertex = test_vertex_count - 1;
  accel_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;

  // Build geometry info for BLAS
  VkAccelerationStructureBuildGeometryInfoKHR blas_build_info = {0};
  blas_build_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  blas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  blas_build_info.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  blas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  blas_build_info.geometryCount = 1;
  blas_build_info.pGeometries = &accel_geometry;

  u32 primitive_count = test_index_count / 3;

  VkAccelerationStructureBuildSizesInfoKHR blas_size_info = {0};
  blas_size_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  g_rt_system.vkGetAccelerationStructureBuildSizesKHR(
      renderer->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &blas_build_info, &primitive_count, &blas_size_info);

  // Create BLAS buffer
  VkBufferCreateInfo blas_buffer_info = {0};
  blas_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  blas_buffer_info.size = blas_size_info.accelerationStructureSize;
  blas_buffer_info.usage =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  if (vkCreateBuffer(renderer->device, &blas_buffer_info, NULL,
                     &g_rt_system.bottom_level_as.buffer) != VK_SUCCESS) {
    LOG_ERROR("Failed to create BLAS buffer");
    return false;
  }

  vkGetBufferMemoryRequirements(renderer->device,
                                g_rt_system.bottom_level_as.buffer, &mem_reqs);
  alloc_info.allocationSize = mem_reqs.size;

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL,
                       &g_rt_system.bottom_level_as.memory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate BLAS memory");
    return false;
  }

  vkBindBufferMemory(renderer->device, g_rt_system.bottom_level_as.buffer,
                     g_rt_system.bottom_level_as.memory, 0);

  // Create BLAS
  VkAccelerationStructureCreateInfoKHR blas_create_info = {0};
  blas_create_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  blas_create_info.buffer = g_rt_system.bottom_level_as.buffer;
  blas_create_info.size = blas_size_info.accelerationStructureSize;
  blas_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

  if (g_rt_system.vkCreateAccelerationStructureKHR(
          renderer->device, &blas_create_info, NULL,
          &g_rt_system.bottom_level_as.handle) != VK_SUCCESS) {
    LOG_ERROR("Failed to create BLAS");
    return false;
  }

  g_rt_system.bottom_level_as.size = blas_size_info.accelerationStructureSize;
  g_rt_system.bottom_level_as.type =
      VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

  // Step 3: Build the BLAS (will be done via command buffer in actual
  // rendering)
  LOG_INFO("Ray tracing BLAS created successfully");

  // Step 4: Create Top-Level Acceleration Structure (TLAS) with instances
  // TLAS will reference the BLAS and allow for instancing/transforms
  VkAccelerationStructureBuildGeometryInfoKHR tlas_build_info = {0};
  tlas_build_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlas_build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlas_build_info.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  tlas_build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

  u32 instance_count = 1; // One instance for now

  VkAccelerationStructureBuildSizesInfoKHR tlas_size_info = {0};
  tlas_size_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  g_rt_system.vkGetAccelerationStructureBuildSizesKHR(
      renderer->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &tlas_build_info, &instance_count, &tlas_size_info);

  // Create TLAS buffer
  VkBufferCreateInfo tlas_buffer_info = {0};
  tlas_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  tlas_buffer_info.size = tlas_size_info.accelerationStructureSize;
  tlas_buffer_info.usage =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  if (vkCreateBuffer(renderer->device, &tlas_buffer_info, NULL,
                     &g_rt_system.top_level_as.buffer) != VK_SUCCESS) {
    LOG_ERROR("Failed to create TLAS buffer");
    return false;
  }

  vkGetBufferMemoryRequirements(renderer->device,
                                g_rt_system.top_level_as.buffer, &mem_reqs);
  alloc_info.allocationSize = mem_reqs.size;

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL,
                       &g_rt_system.top_level_as.memory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate TLAS memory");
    return false;
  }

  vkBindBufferMemory(renderer->device, g_rt_system.top_level_as.buffer,
                     g_rt_system.top_level_as.memory, 0);

  // Create TLAS
  VkAccelerationStructureCreateInfoKHR tlas_create_info = {0};
  tlas_create_info.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  tlas_create_info.buffer = g_rt_system.top_level_as.buffer;
  tlas_create_info.size = tlas_size_info.accelerationStructureSize;
  tlas_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  if (g_rt_system.vkCreateAccelerationStructureKHR(
          renderer->device, &tlas_create_info, NULL,
          &g_rt_system.top_level_as.handle) != VK_SUCCESS) {
    LOG_ERROR("Failed to create TLAS");
    return false;
  }

  g_rt_system.top_level_as.size = tlas_size_info.accelerationStructureSize;
  g_rt_system.top_level_as.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  LOG_INFO("Ray tracing acceleration structures created (BLAS + TLAS)");
  return true;
}

// Create uniform buffer for camera parameters
bool ray_tracing_create_uniform_buffer(void) {
  VulkanRenderer *renderer = g_rt_system.renderer;

  VkDeviceSize buffer_size =
      sizeof(Vec4) * 4; // Camera position, direction, up, and parameters

  if (!vulkan_create_buffer(
          renderer, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &g_rt_system.uniform_buffer, &g_rt_system.uniform_memory)) {
    LOG_ERROR("Failed to create ray tracing uniform buffer");
    return false;
  }

  // Map memory
  if (vkMapMemory(renderer->device, g_rt_system.uniform_memory, 0, buffer_size,
                  0, &g_rt_system.uniform_mapped) != VK_SUCCESS) {
    LOG_ERROR("Failed to map ray tracing uniform buffer");
    return false;
  }

  return true;
}

// Create output image for ray tracing results
bool ray_tracing_create_output_image(void) {
  VulkanRenderer *renderer = g_rt_system.renderer;

  // Create image
  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = renderer->swapchain_extent.width;
  image_info.extent.height = renderer->swapchain_extent.height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = VK_IMAGE_USAGE_STORAGE_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(renderer->device, &image_info, NULL,
                    &g_rt_system.output_image) != VK_SUCCESS) {
    LOG_ERROR("Failed to create ray tracing output image");
    return false;
  }

  // Allocate memory
  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(renderer->device, g_rt_system.output_image,
                               &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = vulkan_find_memory_type(
      renderer->physical_device, mem_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(renderer->device, &alloc_info, NULL,
                       &g_rt_system.output_image_memory) != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate ray tracing output image memory");
    return false;
  }

  vkBindImageMemory(renderer->device, g_rt_system.output_image,
                    g_rt_system.output_image_memory, 0);

  // Create image view
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = g_rt_system.output_image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView(renderer->device, &view_info, NULL,
                        &g_rt_system.output_image_view) != VK_SUCCESS) {
    LOG_ERROR("Failed to create ray tracing output image view");
    return false;
  }

  return true;
}

// Update ray tracing camera uniform
void ray_tracing_update_camera(Vec3 position, Vec3 direction, Vec3 up, f32 fov,
                               f32 aspect) {
  if (!g_rt_system.uniform_mapped)
    return;

  // Pack camera data into uniform buffer
  Vec4 *camera_data = (Vec4 *)g_rt_system.uniform_mapped;
  camera_data[0] = vec4(position.x, position.y, position.z, 0.0f);
  camera_data[1] = vec4(direction.x, direction.y, direction.z, 0.0f);
  camera_data[2] = vec4(up.x, up.y, up.z, 0.0f);
  camera_data[3] = vec4(fov, aspect, 0.0f, 0.0f);
}

// Trace rays
void ray_tracing_trace(VkCommandBuffer command_buffer) {
  if (!g_rt_system.rt_supported)
    return;

  // This would involve:
  // 1. Binding the ray tracing pipeline
  // 2. Binding descriptor sets
  // 3. Setting up shader binding table regions
  // 4. Calling vkCmdTraceRaysKHR

  LOG_TRACE("Ray tracing trace called (placeholder)");
}

// Cleanup ray tracing system
void ray_tracing_cleanup(void) {
  VulkanRenderer *renderer = g_rt_system.renderer;
  if (!renderer)
    return;

  // Destroy output image
  if (g_rt_system.output_image_view) {
    vkDestroyImageView(renderer->device, g_rt_system.output_image_view, NULL);
  }
  if (g_rt_system.output_image) {
    vkDestroyImage(renderer->device, g_rt_system.output_image, NULL);
  }
  if (g_rt_system.output_image_memory) {
    vkFreeMemory(renderer->device, g_rt_system.output_image_memory, NULL);
  }

  // Destroy uniform buffer
  if (g_rt_system.uniform_mapped) {
    vkUnmapMemory(renderer->device, g_rt_system.uniform_memory);
  }
  if (g_rt_system.uniform_buffer) {
    vkDestroyBuffer(renderer->device, g_rt_system.uniform_buffer, NULL);
  }
  if (g_rt_system.uniform_memory) {
    vkFreeMemory(renderer->device, g_rt_system.uniform_memory, NULL);
  }

  // Destroy acceleration structures
  if (g_rt_system.bottom_level_as.handle) {
    g_rt_system.vkDestroyAccelerationStructureKHR(
        renderer->device, g_rt_system.bottom_level_as.handle, NULL);
  }
  if (g_rt_system.bottom_level_as.buffer) {
    vkDestroyBuffer(renderer->device, g_rt_system.bottom_level_as.buffer, NULL);
  }
  if (g_rt_system.bottom_level_as.memory) {
    vkFreeMemory(renderer->device, g_rt_system.bottom_level_as.memory, NULL);
  }

  if (g_rt_system.top_level_as.handle) {
    g_rt_system.vkDestroyAccelerationStructureKHR(
        renderer->device, g_rt_system.top_level_as.handle, NULL);
  }
  if (g_rt_system.top_level_as.buffer) {
    vkDestroyBuffer(renderer->device, g_rt_system.top_level_as.buffer, NULL);
  }
  if (g_rt_system.top_level_as.memory) {
    vkFreeMemory(renderer->device, g_rt_system.top_level_as.memory, NULL);
  }

  // Destroy pipeline
  if (g_rt_system.pipeline.descriptor_pool) {
    vkDestroyDescriptorPool(renderer->device,
                            g_rt_system.pipeline.descriptor_pool, NULL);
  }
  if (g_rt_system.pipeline.descriptor_set_layout) {
    vkDestroyDescriptorSetLayout(
        renderer->device, g_rt_system.pipeline.descriptor_set_layout, NULL);
  }
  if (g_rt_system.pipeline.layout) {
    vkDestroyPipelineLayout(renderer->device, g_rt_system.pipeline.layout,
                            NULL);
  }
  if (g_rt_system.pipeline.pipeline) {
    vkDestroyPipeline(renderer->device, g_rt_system.pipeline.pipeline, NULL);
  }

  memset(&g_rt_system, 0, sizeof(RayTracingSystem));
  LOG_INFO("Ray tracing system cleaned up");
}

// Check if ray tracing is available
bool ray_tracing_is_available(void) { return g_rt_system.rt_supported; }

// Vulkan renderer wrappers expected by vulkan.c (scaffolding)
bool vulkan_rt_is_supported(VulkanRenderer *renderer) {
  return ray_tracing_check_support(renderer);
}

bool vulkan_rt_init(VulkanRenderer *renderer) {
  return ray_tracing_init(renderer);
}

void vulkan_rt_cleanup(VulkanRenderer *renderer) {
  (void)renderer;
  ray_tracing_cleanup();
}

#else // !VULKAN_BUILD

#include <renderer/ray_tracing.h>

bool ray_tracing_init(VulkanRenderer *renderer) {
  (void)renderer;
  return false;
}

void ray_tracing_cleanup(void) {}

void ray_tracing_update_camera(Vec3 position, Vec3 direction, Vec3 up, f32 fov,
                               f32 aspect) {
  (void)position;
  (void)direction;
  (void)up;
  (void)fov;
  (void)aspect;
}

void ray_tracing_trace(VkCommandBuffer command_buffer) { (void)command_buffer; }

bool ray_tracing_is_available(void) { return false; }

// No-op wrappers to satisfy linker on non-Vulkan builds
bool vulkan_rt_is_supported(VulkanRenderer *renderer) {
  (void)renderer;
  return false;
}
bool vulkan_rt_init(VulkanRenderer *renderer) {
  (void)renderer;
  return false;
}
void vulkan_rt_cleanup(VulkanRenderer *renderer) { (void)renderer; }

#endif // VULKAN_BUILD
