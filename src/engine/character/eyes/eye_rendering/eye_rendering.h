#ifndef EYE_RENDERING_H
#define EYE_RENDERING_H

#include "../math/vec3.h"
#include "../math/vec2.h"
#include "assets/resources/resource_management/resource_handle.h"

/**
 * @struct eye_params
 * @brief Parameters for dual-layer eye rendering (cornea + iris)
 */
typedef struct eye_params {
    float iris_depth;                /**< Distance from cornea surface to iris plane */
    float pupil_size;               /**< Pupil dilation factor [0, 1] */
    float cornea_ior;               /**< Index of refraction for cornea (default ~1.376) */
    vec3_t iris_color;              /**< Base color tint for the iris */
    vec3_t eye_center;              /**< World/Object space center of the eyeball */
    vec3_t eye_forward;             /**< Forward direction of the eye */
    texture_handle_t iris_texture;   /**< High-detail iris texture */
    texture_handle_t sclera_texture; /**< Sclera texture with veins */
} eye_params_t;

/**
 * @brief Initialize eye rendering subsystem
 * @return true if successful
 */
bool eye_rendering_init(void);

/**
 * @brief Shading function for eye material
 * @param V View vector
 * @param N Surface normal
 * @param params Pointer to eye parameters
 * @return Computed color
 */
vec3_t shade_eye(vec3_t V, vec3_t N, eye_params_t* params);

/**
 * @brief Calculate refracted UVs for iris sampling
 * @param refracted Refracted view ray
 * @param params Eye parameters
 * @return UV coordinates for iris sampling
 */
vec2_t calculate_iris_uv(vec3_t refracted, eye_params_t* params);

#endif // EYE_RENDERING_H
