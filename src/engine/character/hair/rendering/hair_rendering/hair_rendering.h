#ifndef HAIR_RENDERING_H
#define HAIR_RENDERING_H

#include "math/vec3.h"
#include "math/mat4.h"
#include "assets/resources/resource_management/resource_handle.h"

/**
 * @struct hair_transparency_params
 * @brief Parameters for hair transparency and OIT
 */
typedef struct hair_transparency_params {
    float alpha_threshold;           /**< Alpha knockout threshold for hair */
    float shadow_opacity;            /**< Opacity of hair for shadow casting */
    bool use_oit;                    /**< Enable Order-Independent Transparency */
    u32 oit_max_fragments;           /**< Max fragments per pixel for OIT */
} hair_transparency_params_t;

/**
 * @struct deep_opacity_map
 * @brief Structure for hair self-shadowing via deep opacity maps
 */
typedef struct deep_opacity_map {
    texture_handle_t layers[4];      /**< Multiple opacity layers for DOM */
    mat4_t light_view_proj;          /**< Light space projection matrix */
} deep_opacity_map_t;

/**
 * @brief Render hair with transparency
 * @param mesh Hair mesh data
 * @param params Transparency parameters
 */
void render_hair_transparent(void* mesh, hair_transparency_params_t* params);

/**
 * @brief Generate deep opacity map for hair shadows
 * @param mesh Hair mesh data
 * @param dom Pointer to DOM structure to populate
 */
void generate_hair_shadow_map(void* mesh, deep_opacity_map_t* dom);

#endif // HAIR_RENDERING_H
