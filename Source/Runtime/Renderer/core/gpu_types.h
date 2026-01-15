#ifndef RENDERING_GPU_TYPES_H
#define RENDERING_GPU_TYPES_H

#include <stdint.h>

#ifdef __METAL_VERSION__
#define ALIGN(x)
#else
#define ALIGN(x) __attribute__((aligned(x)))
#endif

// ============================================================================
// GPU INSTANCE DATA
// ============================================================================
// 128 bytes per instance - designed for optimal GPU cache coherence
// Aligned to 128 bytes for cache line efficiency

typedef struct ALIGN(128) {
    // Transform: 4x4 column-major matrix (64 bytes)
    float transform[16];

    // Bounds: AABB in local space (32 bytes)
    float bounds_min[4];     // xyz + padding
    float bounds_max[4];     // xyz + padding

    // Instance identifiers and flags (32 bytes)
    uint32_t material_id;           // Index into material buffer
    uint32_t visibility_flags;      // Cull flags, LOD bits
    uint32_t mesh_id;               // Mesh identifier
    uint32_t instance_custom_data;  // App-specific data (custom shader param index, etc)
} GPUInstanceData;

// Ensure correct size
_Static_assert(sizeof(GPUInstanceData) == 128, "GPUInstanceData must be 128 bytes");

// ============================================================================
// GPU MATERIAL DATA
// ============================================================================
// 64 bytes per material - optimized for memory bandwidth

typedef struct ALIGN(64) {
    // PBR Base: 16 bytes
    float base_color[4];                // RGB + alpha

    // PBR Properties: 16 bytes
    float metallic_roughness_ao[4];     // Metallic, Roughness, AO, reserved

    // Texture indices for bindless rendering: 32 bytes
    uint32_t texture_indices[8];        // [0]=albedo, [1]=normal, [2]=roughness, etc
} GPUMaterialData;

_Static_assert(sizeof(GPUMaterialData) == 64, "GPUMaterialData must be 64 bytes");

// ============================================================================
// INDIRECT DRAW ARGUMENTS
// ============================================================================
// Matches Metal's MTLDrawIndexedPrimitivesIndirectArguments exactly

typedef struct {
    uint32_t indexCount;      // Number of indices to draw
    uint32_t instanceCount;   // Number of instances
    uint32_t indexStart;      // Offset into index buffer
    int32_t  baseVertex;      // Bias applied to vertex indices
    uint32_t baseInstance;    // Offset into instance buffer
} IndirectDrawArgs;

_Static_assert(sizeof(IndirectDrawArgs) == 20, "IndirectDrawArgs must be 20 bytes");

// ============================================================================
// GPU CULLING DATA
// ============================================================================

typedef struct ALIGN(16) {
    // Frustum plane equations: Ax + By + Cz + D = 0
    float planes[6][4];       // 6 planes, each with (A, B, C, D)
    float camera_pos[3];
    float padding;
} FrustumData;

typedef struct ALIGN(16) {
    // Distance thresholds for LOD levels 0-4
    float lod_distances[5];   // Distance at which to switch to each LOD
    float lod_hysteresis;     // Hysteresis to prevent LOD popping
    uint32_t max_lod_level;   // Maximum LOD available (0-4)
    uint32_t padding;
} LODSelectionData;

// ============================================================================
// CULLING RESULTS
// ============================================================================
// Per-instance culling result, populated by GPU compute shaders

typedef struct ALIGN(16) {
    uint32_t visible_flag;    // 1 if visible after all culling, 0 otherwise
    uint32_t lod_selected;    // Selected LOD level (0-4)
    uint32_t draw_index;      // Index in compacted indirect args array
    uint32_t padding;
} CullingResult;

// ============================================================================
// GPU SCENE CONFIGURATION
// ============================================================================

#define MAX_INSTANCES_PER_SCENE 1000000
#define MAX_MATERIALS_PER_SCENE 4096
#define MAX_DRAW_CALLS_PER_FRAME 100000
#define MAX_VISIBLE_INSTANCES_PER_FRAME 1000000

// Buffer sizes in bytes
#define INSTANCE_BUFFER_SIZE (MAX_INSTANCES_PER_SCENE * sizeof(GPUInstanceData))
#define MATERIAL_BUFFER_SIZE (MAX_MATERIALS_PER_SCENE * sizeof(GPUMaterialData))
#define INDIRECT_BUFFER_SIZE (MAX_DRAW_CALLS_PER_FRAME * sizeof(IndirectDrawArgs))
#define CULLING_RESULT_BUFFER_SIZE (MAX_INSTANCES_PER_SCENE * sizeof(CullingResult))

// ============================================================================
// GPU COUNTERS
// ============================================================================
// Atomic counters for GPU-side operations

typedef struct ALIGN(16) {
    uint32_t visible_instance_count;      // Number of visible instances after culling
    uint32_t draw_call_count;             // Number of draw calls generated
    uint32_t compaction_offset;           // Offset for next compaction write
    uint32_t frame_number;                // Current frame counter
} GPUCounters;

// ============================================================================
// BATCH INFORMATION
// ============================================================================
// CPU-side batch metadata

typedef struct {
    uint32_t mesh_id;
    uint32_t material_id;
    uint8_t lod_level;
    uint8_t padding[3];
    uint32_t instance_start;              // Start index in GPU instance buffer
    uint32_t instance_count;              // Number of instances in this batch
    uint32_t indirect_args_index;         // Index in indirect args array
} BatchInfo;

#endif // RENDERING_GPU_TYPES_H
