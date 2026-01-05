/**
 * @file render_geometry.h
 * @brief Geometry and mesh management
 * @details Manages meshes, vertices, indices, and geometry processing
 */

#ifndef RENDER_GEOMETRY_H
#define RENDER_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Vertex attribute semantic
 */
typedef enum {
    VERTEX_ATTRIBUTE_POSITION = 0,
    VERTEX_ATTRIBUTE_NORMAL = 1,
    VERTEX_ATTRIBUTE_TANGENT = 2,
    VERTEX_ATTRIBUTE_BITANGENT = 3,
    VERTEX_ATTRIBUTE_TEXCOORD0 = 4,
    VERTEX_ATTRIBUTE_TEXCOORD1 = 5,
    VERTEX_ATTRIBUTE_COLOR = 6,
    VERTEX_ATTRIBUTE_BONES = 7,
    VERTEX_ATTRIBUTE_WEIGHTS = 8,
} VertexAttributeSemantic;

/**
 * @brief Vertex attribute format
 */
typedef enum {
    VERTEX_FORMAT_FLOAT = 0,
    VERTEX_FORMAT_FLOAT2 = 1,
    VERTEX_FORMAT_FLOAT3 = 2,
    VERTEX_FORMAT_FLOAT4 = 3,
    VERTEX_FORMAT_INT = 4,
    VERTEX_FORMAT_INT2 = 5,
    VERTEX_FORMAT_INT3 = 6,
    VERTEX_FORMAT_INT4 = 7,
    VERTEX_FORMAT_UINT = 8,
    VERTEX_FORMAT_UINT2 = 9,
    VERTEX_FORMAT_UINT3 = 10,
    VERTEX_FORMAT_UINT4 = 11,
    VERTEX_FORMAT_HALF_FLOAT = 12,
    VERTEX_FORMAT_HALF_FLOAT2 = 13,
    VERTEX_FORMAT_HALF_FLOAT3 = 14,
    VERTEX_FORMAT_HALF_FLOAT4 = 15,
} VertexFormat;

/**
 * @brief Vertex attribute binding
 */
typedef struct {
    VertexAttributeSemantic semantic;
    VertexFormat format;
    uint32_t offset;
} VertexAttribute;

/**
 * @brief Vertex buffer layout
 */
typedef struct {
    VertexAttribute* attributes;
    uint32_t attribute_count;
    uint32_t stride;
} VertexLayout;

/**
 * @brief Index format
 */
typedef enum {
    INDEX_FORMAT_UINT16 = 0,
    INDEX_FORMAT_UINT32 = 1,
} IndexFormat;

/**
 * @brief Mesh structure
 */
typedef struct {
    uint32_t id;
    uint32_t vertex_buffer_id;
    uint32_t index_buffer_id;
    uint32_t vertex_count;
    uint32_t index_count;
    IndexFormat index_format;
    VertexLayout vertex_layout;
    float bounding_sphere_center[3];
    float bounding_sphere_radius;
    float bounding_box_min[3];
    float bounding_box_max[3];
} Mesh;

/**
 * @brief Sub-mesh for LOD or material
 */
typedef struct {
    uint32_t start_index;
    uint32_t index_count;
    uint32_t material_id;
} SubMesh;

/**
 * @brief Mesh geometry LOD
 */
typedef struct {
    Mesh* meshes;
    uint32_t mesh_count;
    float* lod_distances;
    uint32_t lod_count;
} MeshLOD;

/**
 * @brief Mesh manager structure
 */
typedef struct RenderMeshManager RenderMeshManager;

/**
 * @brief Create mesh manager
 * @param[in] max_meshes Maximum number of meshes
 * @return Pointer to manager, NULL on failure
 */
RenderMeshManager* render_mesh_manager_create(uint32_t max_meshes);

/**
 * @brief Destroy mesh manager
 * @param[in] manager Manager to destroy
 */
void render_mesh_manager_destroy(RenderMeshManager* manager);

/**
 * @brief Create a mesh from vertex/index data
 * @param[in] manager Mesh manager
 * @param[in] vertices Vertex data
 * @param[in] vertex_count Number of vertices
 * @param[in] vertex_stride Size of each vertex in bytes
 * @param[in] vertex_layout Vertex attribute layout
 * @param[in] indices Index data
 * @param[in] index_count Number of indices
 * @param[in] index_format Format of indices
 * @return Mesh ID, 0 on failure
 */
uint32_t render_mesh_create(RenderMeshManager* manager,
                            const void* vertices,
                            uint32_t vertex_count,
                            uint32_t vertex_stride,
                            const VertexLayout* vertex_layout,
                            const void* indices,
                            uint32_t index_count,
                            IndexFormat index_format);

/**
 * @brief Get mesh by ID
 * @param[in] manager Mesh manager
 * @param[in] mesh_id Mesh ID
 * @return Pointer to mesh, NULL if not found
 */
const Mesh* render_mesh_get(RenderMeshManager* manager, uint32_t mesh_id);

/**
 * @brief Update mesh vertex data
 * @param[in] manager Mesh manager
 * @param[in] mesh_id Mesh ID
 * @param[in] vertices New vertex data
 * @param[in] vertex_count Number of vertices
 * @param[in] vertex_stride Size of each vertex
 * @return true on success
 */
bool render_mesh_update_vertices(RenderMeshManager* manager,
                                 uint32_t mesh_id,
                                 const void* vertices,
                                 uint32_t vertex_count,
                                 uint32_t vertex_stride);

/**
 * @brief Calculate mesh bounds
 * @param[in] manager Mesh manager
 * @param[in] mesh_id Mesh ID
 * @param[out] center Bounding sphere center
 * @param[out] radius Bounding sphere radius
 * @return true on success
 */
bool render_mesh_calculate_bounds(RenderMeshManager* manager,
                                  uint32_t mesh_id,
                                  float center[3],
                                  float* radius);

/**
 * @brief Delete a mesh
 * @param[in] manager Mesh manager
 * @param[in] mesh_id Mesh ID
 */
void render_mesh_delete(RenderMeshManager* manager, uint32_t mesh_id);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_GEOMETRY_H */
