// include/render/instancing.h
//
// Purpose: Defines the mesh instancing system for efficiently rendering multiple
// copies of the same mesh with different transforms. This is critical for
// performance when rendering many identical blocks, trees, or entities.
//
// Public APIs:
// - `instancing_system_init`: Initialize the instancing system
// - `instancing_system_cleanup`: Free all instancing resources
// - `instancing_add_instance`: Add an instance of a mesh
// - `instancing_remove_instance`: Remove an instance
// - `instancing_update_instance`: Update instance transform
// - `instancing_render`: Render all instances efficiently
//
// Ownership: The instancing system manages instance data buffers and transform matrices.
//
// Invariants:
// - Each mesh can have up to MAX_INSTANCES_PER_MESH instances
// - Instance transforms are stored in GPU buffers for efficient rendering
// - Instances are grouped by mesh ID for optimal batching
//
#ifndef INSTANCING_H
#define INSTANCING_H

#include <common.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include "mesh.h"
#include <renderer/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum instances per mesh
#define MAX_INSTANCES_PER_MESH 4096
#define MAX_INSTANCED_MESHES 256

// Instance data (per-instance attributes)
typedef struct {
    Mat4 transform;     // Model transform matrix
    Vec4 color_tint;    // Color tint/modulation
    u32 texture_offset; // Texture atlas offset
    u32 flags;          // Instance-specific flags
} InstanceData;

// Instanced mesh group
typedef struct {
    u64 mesh_id;                     // Reference to base mesh
    Mesh* base_mesh;                 // Pointer to base mesh
    InstanceData* instances;         // Array of instance data
    u32 instance_count;              // Number of active instances
    u32 instance_capacity;           // Allocated capacity

    // GPU resources
    // Note: Always define these structure members to avoid ABI issues if usage flags differ
    VkBuffer instance_buffer;        // GPU buffer for instance data
    VkDeviceMemory instance_memory;  // GPU memory
    bool buffer_needs_update;        // Flag for buffer updates

    // GPU-Driven specific resources
    u32 gpu_data_id;                 // ID/Handle for geometry_gpu_instance_data
    u32 indirect_batch_id;           // ID/Handle for geometry_indirect_instancing
    VkBuffer visible_indices_buffer; // SSBO for culled indices
    VkDeviceMemory visible_indices_memory;

    bool active;                     // Is this group active?
} InstancedMeshGroup;

// Instancing system state
typedef struct {
    InstancedMeshGroup* groups;
    u32 group_count;
    u32 group_capacity;

    VulkanRenderer* renderer;
    VkDescriptorSetLayout instance_descriptor_layout;
    VkDescriptorPool instance_descriptor_pool;

    // GPU-Driven Culling
    VkPipeline cull_pipeline;
    VkPipelineLayout cull_pipeline_layout;

    bool enabled;
    bool gpu_driven; // Use GPU-driven culling and indirect draw
} InstancingSystem;

// Initialize instancing system
void instancing_system_init(InstancingSystem* system);

// Cleanup instancing system
void instancing_system_cleanup(InstancingSystem* system);

// Create an instanced mesh group for a base mesh
u64 instancing_create_group(InstancingSystem* system, Mesh* base_mesh);

// Add an instance to a mesh group
u32 instancing_add_instance(InstancingSystem* system, u64 group_id, Mat4 transform);

// Remove an instance
void instancing_remove_instance(InstancingSystem* system, u64 group_id, u32 instance_id);

// Update instance transform
void instancing_update_instance(InstancingSystem* system, u64 group_id, u32 instance_id, Mat4 transform);

// Update instance buffer on GPU
void instancing_update_buffers(InstancingSystem* system);

// Dispatch compute culling (GPU-driven mode only)
void instancing_dispatch_culling(InstancingSystem* system, VkCommandBuffer cmd_buffer);

// Render all instances (called per frame)
void instancing_render_all(InstancingSystem* system, VkCommandBuffer cmd_buffer);

// Get instance count for a group
u32 instancing_get_count(InstancingSystem* system, u64 group_id);

#ifdef __cplusplus
}
#endif

#endif // INSTANCING_H
