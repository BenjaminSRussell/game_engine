// include/vfx/gpu_pipeline_setup.h
//
// Purpose: GPU pipeline setup and initialization for particle and post-processing rendering
//
#ifndef GPU_PIPELINE_SETUP_H
#define GPU_PIPELINE_SETUP_H

#include <common.h>
#include <renderer/vulkan.h>

typedef struct {
    VkPipeline particlePipeline;
    VkPipelineLayout particleLayout;

    VkPipeline bloomThresholdPipeline;
    VkPipeline bloomBlurPipeline;
    VkPipeline bloomCompositePipeline;

    VkPipeline vignettePipeline;
    VkPipeline tonemapPipeline;
    VkPipeline motionBlurPipeline;
    VkPipeline dofPipeline;
    VkPipeline filmGrainPipeline;
    VkPipeline chromaticAberrationPipeline;

    // Descriptor sets
    VkDescriptorSet particleDescriptorSet;
    VkDescriptorSetLayout particleDescriptorLayout;

    // Sampler
    VkSampler linearSampler;
    VkSampler nearestSampler;

    // Framebuffers
    VkImage bloomThresholdImage;
    VkImage bloomBlurImage;
    VkFramebuffer bloomFramebuffers[3];  // threshold, blur, composite

    bool initialized;
} GPUPipelineSetup;

// Initialization
bool gpu_pipeline_setup_init(GPUPipelineSetup* setup, VulkanRenderer* renderer);
void gpu_pipeline_setup_shutdown(GPUPipelineSetup* setup, VulkanRenderer* renderer);

// Shader loading
VkShaderModule gpu_load_shader(VulkanRenderer* renderer, const char* filepath);
void gpu_destroy_shader(VulkanRenderer* renderer, VkShaderModule module);

// Pipeline creation
bool gpu_create_particle_pipeline(GPUPipelineSetup* setup, VulkanRenderer* renderer);
bool gpu_create_bloom_pipelines(GPUPipelineSetup* setup, VulkanRenderer* renderer);
bool gpu_create_postprocess_pipelines(GPUPipelineSetup* setup, VulkanRenderer* renderer);

// Descriptor set creation
bool gpu_create_particle_descriptors(GPUPipelineSetup* setup, VulkanRenderer* renderer);

// Sampler creation
bool gpu_create_samplers(GPUPipelineSetup* setup, VulkanRenderer* renderer);

#endif // GPU_PIPELINE_SETUP_H
