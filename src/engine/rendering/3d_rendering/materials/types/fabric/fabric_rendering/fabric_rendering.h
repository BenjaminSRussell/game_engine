#ifndef FABRIC_RENDERING_H
#define FABRIC_RENDERING_H

#include "../math/vec3.h"
#include "../resource_management/resource_handle.h"

/**
 * @struct fabric_params
 * @brief Parameters for fabric rendering including weave detail and sheen
 */
typedef struct fabric_params {
    vec3_t base_color;               /**< Fabric albedo color */
    vec3_t sheen_color;              /**< Color of the grazing sheen layer */
    float sheen_intensity;           /**< Intensity of the Charlie BRDF sheen */
    float weave_scale;               /**< Tiling scale for weave detail maps */
    float roughness;                 /**< Base surface roughness */
    texture_handle_t weave_normal;   /**< Detail normal map for weave pattern */
    texture_handle_t fuzz_mask;      /**< Mask for surface fuzz/threading */
} fabric_params_t;

/**
 * @brief Shading function for fabric material
 * @param V View vector
 * @param N Surface normal
 * @param L Light vector
 * @param params Pointer to fabric parameters
 * @return Computed color
 */
vec3_t shade_fabric(vec3_t V, vec3_t N, vec3_t L, fabric_params_t* params);

/**
 * @brief Calculate Charlie BRDF sheen component
 * @param V View vector
 * @param N Surface normal
 * @param L Light vector
 * @param roughness Sheen roughness
 * @return Sheen contribution factor
 */
float calculate_fabric_sheen(vec3_t V, vec3_t N, vec3_t L, float roughness);

#endif // FABRIC_RENDERING_H
