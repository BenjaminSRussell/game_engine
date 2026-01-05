/**
 * @file render_raytracing.h
 * @brief Ray tracing support for advanced rendering
 * @details Manages ray tracing pipelines, acceleration structures, and queries
 */

#ifndef RENDER_RAYTRACING_H
#define RENDER_RAYTRACING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Ray tracing geometry type
 */
typedef enum {
    RAYTRACING_GEOMETRY_TRIANGLES = 0,
    RAYTRACING_GEOMETRY_AABBS = 1,
} RayTracingGeometryType;

/**
 * @brief Ray payload structure
 */
typedef struct {
    float hit_distance;
    float hit_normal[3];
    uint32_t hit_primitive_id;
    bool hit;
} RayPayload;

/**
 * @brief Ray tracing instance
 */
typedef struct {
    uint32_t acceleration_structure_id;
    float transform[16];
    uint32_t instance_id;
    uint32_t custom_index;
    uint32_t mask;
} RayTracingInstance;

/**
 * @brief Ray tracing acceleration structure
 */
typedef struct {
    uint32_t id;
    RayTracingGeometryType geometry_type;
    uint32_t primitive_count;
    void* api_handle;
} RayTracingAccelerationStructure;

/**
 * @brief Ray tracing pipeline
 */
typedef struct {
    uint32_t id;
    uint32_t max_recursion_depth;
    uint32_t payload_size;
    void* api_handle;
} RayTracingPipeline;

/**
 * @brief Ray tracing manager
 */
typedef struct RenderRayTracingManager RenderRayTracingManager;

/**
 * @brief Create ray tracing manager
 * @param[in] max_structures Maximum acceleration structures
 * @param[in] max_pipelines Maximum ray tracing pipelines
 * @return Pointer to manager, NULL on failure
 */
RenderRayTracingManager* render_raytracing_manager_create(uint32_t max_structures,
                                                          uint32_t max_pipelines);

/**
 * @brief Destroy ray tracing manager
 * @param[in] manager Manager to destroy
 */
void render_raytracing_manager_destroy(RenderRayTracingManager* manager);

/**
 * @brief Create bottom-level acceleration structure (geometry)
 * @param[in] manager Manager instance
 * @param[in] geometry_type Type of geometry
 * @param[in] primitives Primitive data
 * @param[in] primitive_count Number of primitives
 * @return Structure ID, 0 on failure
 */
uint32_t render_raytracing_create_blas(RenderRayTracingManager* manager,
                                       RayTracingGeometryType geometry_type,
                                       const void* primitives,
                                       uint32_t primitive_count);

/**
 * @brief Create top-level acceleration structure (instances)
 * @param[in] manager Manager instance
 * @param[in] instances Array of instances
 * @param[in] instance_count Number of instances
 * @return Structure ID, 0 on failure
 */
uint32_t render_raytracing_create_tlas(RenderRayTracingManager* manager,
                                       const RayTracingInstance* instances,
                                       uint32_t instance_count);

/**
 * @brief Create ray tracing pipeline
 * @param[in] manager Manager instance
 * @param[in] raygen_shader Ray generation shader
 * @param[in] hit_shaders Array of hit shaders
 * @param[in] hit_shader_count Number of hit shaders
 * @param[in] miss_shaders Array of miss shaders
 * @param[in] miss_shader_count Number of miss shaders
 * @param[in] max_recursion Maximum recursion depth
 * @return Pipeline ID, 0 on failure
 */
uint32_t render_raytracing_create_pipeline(RenderRayTracingManager* manager,
                                           const void* raygen_shader,
                                           const void** hit_shaders,
                                           uint32_t hit_shader_count,
                                           const void** miss_shaders,
                                           uint32_t miss_shader_count,
                                           uint32_t max_recursion);

/**
 * @brief Trace rays
 * @param[in] manager Manager instance
 * @param[in] pipeline_id Pipeline ID
 * @param[in] tlas_id Top-level acceleration structure ID
 * @param[in] rays Ray data
 * @param[in] ray_count Number of rays
 * @param[out] payloads Ray payloads
 * @return true on success
 */
bool render_raytracing_trace(RenderRayTracingManager* manager,
                            uint32_t pipeline_id,
                            uint32_t tlas_id,
                            const float* rays,
                            uint32_t ray_count,
                            RayPayload* payloads);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_RAYTRACING_H */
