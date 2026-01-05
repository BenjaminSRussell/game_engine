/**
 * @file render_culling.h
 * @brief Visibility culling system (frustum, occlusion, etc.)
 * @details Manages culling of invisible geometry to improve performance
 */

#ifndef RENDER_CULLING_H
#define RENDER_CULLING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Culling method
 */
typedef enum {
    CULL_METHOD_NONE = 0,
    CULL_METHOD_FRUSTUM = 1,
    CULL_METHOD_OCCLUSION = 2,
    CULL_METHOD_DISTANCE = 3,
    CULL_METHOD_HIERARCHICAL_Z = 4,
} CullingMethod;

/**
 * @brief Frustum plane
 */
typedef struct {
    float normal[3];
    float distance;
} FrustumPlane;

/**
 * @brief View frustum
 */
typedef struct {
    FrustumPlane planes[6];
    float corners[8][3];
} ViewFrustum;

/**
 * @brief Culling context
 */
typedef struct RenderCullingContext RenderCullingContext;

/**
 * @brief Create culling context
 * @param[in] max_objects Maximum objects to cull
 * @param[in] methods Enabled culling methods
 * @return Pointer to context, NULL on failure
 */
RenderCullingContext* render_culling_context_create(uint32_t max_objects, uint32_t methods);

/**
 * @brief Destroy culling context
 * @param[in] context Context to destroy
 */
void render_culling_context_destroy(RenderCullingContext* context);

/**
 * @brief Build frustum from view-projection matrix
 * @param[in] context Culling context
 * @param[in] view_projection 4x4 matrix
 * @return true on success
 */
bool render_culling_build_frustum(RenderCullingContext* context, const float* view_projection);

/**
 * @brief Test if sphere is visible in frustum
 * @param[in] context Culling context
 * @param[in] center Sphere center
 * @param[in] radius Sphere radius
 * @return true if visible
 */
bool render_culling_test_sphere(RenderCullingContext* context,
                               const float center[3],
                               float radius);

/**
 * @brief Test if AABB is visible in frustum
 * @param[in] context Culling context
 * @param[in] min_bounds Minimum bounds
 * @param[in] max_bounds Maximum bounds
 * @return true if visible
 */
bool render_culling_test_aabb(RenderCullingContext* context,
                             const float min_bounds[3],
                             const float max_bounds[3]);

/**
 * @brief Cull objects against frustum
 * @param[in] context Culling context
 * @param[in] centers Array of object centers
 * @param[in] radii Array of object radii
 * @param[in] count Number of objects
 * @param[out] visible_indices Indices of visible objects
 * @param[in] max_visible Maximum visible objects to return
 * @return Number of visible objects
 */
uint32_t render_culling_cull_objects(RenderCullingContext* context,
                                    const float* centers,
                                    const float* radii,
                                    uint32_t count,
                                    uint32_t* visible_indices,
                                    uint32_t max_visible);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_CULLING_H */
