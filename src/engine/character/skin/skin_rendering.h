#ifndef SKIN_RENDERING_H
#define SKIN_RENDERING_H

#include "math/vec3.h"
#include "assets/resources/resource_management/resource_handle.h"

/**
 * @struct skin_params
 * @brief Parameters for skin subsurface scattering and detail rendering
 */
typedef struct skin_params {
    vec3_t scatter_color;            /**< Base scatter color (usually reddish/bloody) */
    float scatter_radius;            /**< Radius of diffusion in millimeters */
    float detail_normal_strength;    /**< Strength of pore/wrinkle detail maps */
    float translucency;              /**< Thin-surface translucency factor (ears/nose) */
    texture_handle_t diffusion_profile; /**< Pre-integrated diffusion profile lookup */
    texture_handle_t detail_normal;  /**< High-frequency pore detail map */
    texture_handle_t wrinkle_map;    /**< Wrinkle/expression map */
    float wrinkle_blend;             /**< Blend factor for wrinkles */
} skin_params_t;

/**
 * @brief Initialize skin rendering subsystem
 * @return true if successful
 */
bool skin_rendering_init(void);

/**
 * @brief Shutdown skin rendering subsystem
 */
void skin_rendering_shutdown(void);

/**
 * @brief Render skin to G-Buffer with SSS mask
 * @param mesh Pointer to mesh data
 * @param params Pointer to skin material parameters
 */
void render_gbuffer_skin(void* mesh, skin_params_t* params);

/**
 * @brief Apply separable subsurface scattering blur
 * @param skin_target Handle to the texture containing skin reflectance
 * @param profile Handle to the diffusion profile texture
 */
void sss_blur_horizontal(texture_handle_t skin_target, texture_handle_t profile);
void sss_blur_vertical(texture_handle_t skin_target, texture_handle_t profile);

/**
 * @brief Composite skin result into final light buffer
 * @param final_target Destination buffer
 * @param skin_target Sources SSS buffer
 */
void composite_skin(texture_handle_t final_target, texture_handle_t skin_target);

#endif // SKIN_RENDERING_H
