/**
 * @file render_lighting.h
 * @brief Lighting system for 3D rendering
 * @details Manages light sources, light calculations, shadow mapping, and advanced lighting
 */

#ifndef RENDER_LIGHTING_H
#define RENDER_LIGHTING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Light type enumeration
 */
typedef enum {
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
} LightType;

/**
 * @brief Shadow map technique
 */
typedef enum {
    SHADOW_TECHNIQUE_NONE = 0,
    SHADOW_TECHNIQUE_SHADOW_MAP = 1,
    SHADOW_TECHNIQUE_CASCADE_SHADOW_MAP = 2,
    SHADOW_TECHNIQUE_VARIANCE_SHADOW_MAP = 3,
    SHADOW_TECHNIQUE_RAY_TRACED = 4,
} ShadowTechnique;

/**
 * @brief Light source structure
 */
typedef struct {
    uint32_t id;
    LightType type;
    float color[3];
    float intensity;
    float range;
    float position[3];
    float direction[3];
    float cone_angle;
    float inner_cone_angle;
    bool casts_shadow;
    ShadowTechnique shadow_technique;
    uint32_t shadow_map_resolution;
} Light;

/**
 * @brief Light manager structure
 */
typedef struct RenderLightManager RenderLightManager;

/**
 * @brief Shadow rendering context
 */
typedef struct RenderShadowContext RenderShadowContext;

/**
 * @brief Create a light manager
 * @param[in] max_lights Maximum number of lights
 * @param[in] max_cascades Maximum cascade levels for CSM
 * @return Pointer to created manager, NULL on failure
 */
RenderLightManager* render_light_manager_create(uint32_t max_lights, uint32_t max_cascades);

/**
 * @brief Destroy a light manager
 * @param[in] manager Manager to destroy
 */
void render_light_manager_destroy(RenderLightManager* manager);

/**
 * @brief Add a light to the scene
 * @param[in] manager Light manager
 * @param[in] light Light parameters
 * @return Light ID, 0 on failure
 */
uint32_t render_light_add(RenderLightManager* manager, const Light* light);

/**
 * @brief Remove a light from the scene
 * @param[in] manager Light manager
 * @param[in] light_id ID of light to remove
 * @return true on success
 */
bool render_light_remove(RenderLightManager* manager, uint32_t light_id);

/**
 * @brief Update light parameters
 * @param[in] manager Light manager
 * @param[in] light_id ID of light to update
 * @param[in] light New light parameters
 * @return true on success
 */
bool render_light_update(RenderLightManager* manager, uint32_t light_id, const Light* light);

/**
 * @brief Get light by ID
 * @param[in] manager Light manager
 * @param[in] light_id ID of light
 * @return Pointer to light, NULL if not found
 */
const Light* render_light_get(RenderLightManager* manager, uint32_t light_id);

/**
 * @brief Get all lights in manager
 * @param[in] manager Light manager
 * @param[out] lights Array to fill with lights
 * @param[in] max_lights Maximum lights to return
 * @return Number of lights returned
 */
uint32_t render_light_get_all(RenderLightManager* manager, Light* lights, uint32_t max_lights);

/**
 * @brief Get number of active lights
 * @param[in] manager Light manager
 * @return Number of active lights
 */
uint32_t render_light_get_count(RenderLightManager* manager);

/**
 * @brief Create shadow rendering context
 * @param[in] manager Light manager
 * @param[in] max_shadow_cascades Maximum cascade levels
 * @return Pointer to shadow context, NULL on failure
 */
RenderShadowContext* render_shadow_context_create(RenderLightManager* manager,
                                                  uint32_t max_shadow_cascades);

/**
 * @brief Destroy shadow context
 * @param[in] context Context to destroy
 */
void render_shadow_context_destroy(RenderShadowContext* context);

/**
 * @brief Update shadow cascades for directional light
 * @param[in] context Shadow context
 * @param[in] light_id ID of directional light
 * @param[in] camera_near Near plane of camera
 * @param[in] camera_far Far plane of camera
 * @return true on success
 */
bool render_shadow_update_cascades(RenderShadowContext* context,
                                   uint32_t light_id,
                                   float camera_near,
                                   float camera_far);

/**
 * @brief Render shadow maps for all shadow-casting lights
 * @param[in] context Shadow context
 * @return true on success
 */
bool render_shadow_render_all(RenderShadowContext* context);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_LIGHTING_H */
