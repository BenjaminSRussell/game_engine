// Meshlets System Header
// MESHLETS-001: Meshlet core architecture
// MESHLETS-002: Meshlet generation and clustering
// MESHLETS-003: Mesh shader pipeline
// MESHLETS-004: Culling and optimization
// MESHLETS-005: LOD management
// MESHLETS-006: Compression and streaming
// MESHLETS-007: Debugging and visualization
// MESHLETS-008: Performance optimization
// MESHLETS-009: Cross-platform support
// MESHLETS-010: Integration with existing systems

#ifndef MESHLETS_H
#define MESHLETS_H

#include "engine/include/common.h"
#include "include/rendering/vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

// Meshlet configuration
#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_PRIMITIVES 126
#define MAX_MESHLETS_PER_MESH 65536
#define MAX_MESHLET_GROUPS 1024

// Meshlet primitive types
typedef enum {
    MESHLET_PRIMITIVE_TRIANGLE = 0,
    MESHLET_PRIMITIVE_LINE,
    MESHLET_PRIMITIVE_POINT,
    MESHLET_PRIMITIVE_COUNT
} MeshletPrimitiveType;

// Meshlet culling modes
typedef enum {
    MESHLET_CULL_NONE = 0,
    MESHLET_CULL_FRUSTUM,
    MESHLET_CULL_OCCLUSION,
    MESHLET_CULL_DISTANCE,
    MESHLET_CULL_ALL
} MeshletCullMode;

// Meshlet compression formats
typedef enum {
    MESHLET_COMPRESSION_NONE = 0,
    MESHLET_COMPRESSION_VERTEX_INDEX,
    MESHLET_COMPRESSION_ATTRIBUTE,
    MESHLET_COMPRESSION_GEOMETRY,
    MESHLET_COMPRESSION_COUNT
} MeshletCompressionFormat;

// Meshlet vertex structure
typedef struct {
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 texCoord0;
    vec2 texCoord1;
    vec4 color;
    u32 meshletId;
} MeshletVertex;

// Meshlet primitive structure
typedef struct {
    u32 indices[3]; // Triangle indices
    u32 meshletId;
    float area;
    vec3 normal;
} MeshletPrimitive;

// Meshlet structure
typedef struct {
    u32 vertexOffset;
    u32 vertexCount;
    u32 primitiveOffset;
    u32 primitiveCount;
    vec3 boundingSphereCenter;
    float boundingSphereRadius;
    vec3 boundingBoxMin;
    vec3 boundingBoxMax;
    u32 groupId;
    u32 lodLevel;
    MeshletPrimitiveType primitiveType;
    bool visible;
    bool culled;
} Meshlet;

// Meshlet group structure
typedef struct {
    u32 meshletOffset;
    u32 meshletCount;
    u32 materialId;
    u32 lodBias;
    bool visible;
    bool castShadows;
    bool receiveShadows;
} MeshletGroup;

// Meshlet LOD structure
typedef struct {
    u32 meshletCount;
    u32* meshletIndices;
    float distance;
    float errorMetric;
    bool active;
} MeshletLOD;

// Mesh statistics
typedef struct {
    u64 totalVertices;
    u64 totalPrimitives;
    u64 totalMeshlets;
    u64 totalGroups;
    u64 culledMeshlets;
    u64 drawnMeshlets;
    double averageVerticesPerMeshlet;
    double averagePrimitivesPerMeshlet;
    double cullingTime;
    double drawingTime;
    u64 memoryUsage;
} MeshletStats;

// Meshlet system configuration
typedef struct {
    u32 maxVerticesPerMeshlet;
    u32 maxPrimitivesPerMeshlet;
    u32 maxMeshletsPerMesh;
    MeshletCullMode cullMode;
    MeshletCompressionFormat compressionFormat;
    bool enableFrustumCulling;
    bool enableOcclusionCulling;
    bool enableDistanceCulling;
    bool enableLOD;
    float cullDistance;
    u32 lodCount;
    float lodDistances[8];
    bool enableCompression;
    bool enableStreaming;
    u32 streamingChunkSize;
} MeshletConfig;

// Meshlet system
typedef struct MeshletSystem {
#ifdef VULKAN_BUILD
    // Mesh shader pipeline
    VkPipeline meshShaderPipeline;
    VkPipelineLayout meshShaderLayout;
    VkDescriptorSetLayout meshShaderDescriptorLayout;
    VkDescriptorSet meshShaderDescriptorSet;
    
    // Vertex and index buffers
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    
    // Meshlet data buffers
    VkBuffer meshletBuffer;
    VkDeviceMemory meshletMemory;
    VkBuffer meshletGroupBuffer;
    VkDeviceMemory meshletGroupMemory;
    
    // Indirect draw buffers
    VkBuffer indirectBuffer;
    VkDeviceMemory indirectMemory;
    VkBuffer cullIndirectBuffer;
    VkDeviceMemory cullIndirectMemory;
    
    // Culling compute pipeline
    VkPipeline cullPipeline;
    VkPipelineLayout cullLayout;
    VkDescriptorSetLayout cullDescriptorLayout;
    VkDescriptorSet cullDescriptorSet;
    
    // LOD buffers
    VkBuffer lodBuffer;
    VkDeviceMemory lodMemory;
    
    // Debug visualization
    VkBuffer debugBuffer;
    VkDeviceMemory debugMemory;
    VkImage debugImage;
    VkImageView debugImageView;
    VkDeviceMemory debugImageMemory;
#endif
    bool initialized;
    MeshletConfig config;
    MeshletStats stats;
    
    // CPU-side data
    MeshletVertex* vertices;
    u32 vertexCount;
    MeshletPrimitive* primitives;
    u32 primitiveCount;
    Meshlet* meshlets;
    u32 meshletCount;
    MeshletGroup* groups;
    u32 groupCount;
    MeshletLOD* lods;
    u32 lodCount;
} MeshletSystem;

// Core meshlet system functions
bool meshlets_init(MeshletSystem* system, VulkanRenderer* renderer, const MeshletConfig* config);
void meshlets_shutdown(MeshletSystem* system, VulkanRenderer* renderer);
bool meshlets_is_initialized(const MeshletSystem* system);

// Mesh generation and processing
bool meshlets_generate_from_mesh(MeshletSystem* system, const void* vertices, u32 vertexCount,
                                 const u32* indices, u32 indexCount);
bool meshlets_optimize_meshlets(MeshletSystem* system);
bool meshlets_generate_lods(MeshletSystem* system);
bool meshlets_compress_meshlets(MeshletSystem* system);

// Rendering functions
bool meshlets_render(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer,
                    const mat4* viewMatrix, const mat4* projMatrix);
bool meshlets_render_debug(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer);

// Culling functions
bool meshlets_cull_frustum(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer);
bool meshlets_cull_occlusion(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer);
bool meshlets_cull_distance(MeshletSystem* system, VulkanRenderer* renderer, VkCommandBuffer commandBuffer);

// LOD management
bool meshlets_select_lod(MeshletSystem* system, const vec3* cameraPosition);
bool meshlets_update_lod_transitions(MeshletSystem* system, float deltaTime);

// Configuration and optimization
void meshlets_set_config(MeshletSystem* system, const MeshletConfig* config);
void meshlets_get_config(const MeshletSystem* system, MeshletConfig* outConfig);
void meshlets_optimize_for_gpu(MeshletSystem* system);
void meshlets_enable_culling(MeshletSystem* system, MeshletCullMode cullMode);

// Streaming and compression
bool meshlets_enable_streaming(MeshletSystem* system, bool enable);
bool meshlets_stream_chunk(MeshletSystem* system, u32 chunkIndex);
bool meshlets_compress_data(MeshletSystem* system, MeshletCompressionFormat format);

// Statistics and debugging
void meshlets_get_stats(const MeshletSystem* system, MeshletStats* outStats);
void meshlets_reset_stats(MeshletSystem* system);
void meshlets_debug_print_stats(const MeshletSystem* system);
bool meshlets_validate_system(const MeshletSystem* system);
bool meshlets_debug_draw_bounds(MeshletSystem* system, bool enable);

// Utility functions
u64 meshlets_estimate_memory_usage(const MeshletConfig* config, u32 vertexCount, u32 primitiveCount);
bool meshlets_check_gpu_support(VulkanRenderer* renderer);
u32 meshlets_calculate_optimal_meshlet_size(u32 vertexCount, u32 primitiveCount);
const char* meshlets_get_cull_mode_name(MeshletCullMode mode);

#ifdef __cplusplus
}
#endif

#endif // MESHLETS_H
