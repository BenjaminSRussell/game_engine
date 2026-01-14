#ifndef CONTINUOUS_COLLISION_DETECTION_H
#define CONTINUOUS_COLLISION_DETECTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ccd_context_t ccd_context_t;
typedef struct ccd_body_t ccd_body_t;
typedef struct ccd_sweep_result_t ccd_sweep_result_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    vec3_t min, max;
} aabb_t;

typedef struct {
    vec3_t center;
    float radius;
} sphere_t;

typedef struct {
    vec3_t position;
    vec3_t velocity;
    vec3_t acceleration;
    aabb_t aabb;
    sphere_t sphere;
    float mass;
    float restitution;
    float friction;
    bool is_static;
    bool high_speed;
} ccd_body_desc_t;

typedef struct {
    bool collision;
    float time_of_impact;
    vec3_t contact_point;
    vec3_t contact_normal;
    vec3_t penetration_depth;
    ccd_body_t* body1;
    ccd_body_t* body2;
} ccd_sweep_result_t;

typedef enum {
    CCD_SUCCESS = 0,
    CCD_ERROR_INVALID_PARAM = -1,
    CCD_ERROR_OUT_OF_MEMORY = -2,
    CCD_ERROR_NOT_INITIALIZED = -3
} ccd_error_t;

ccd_error_t ccd_init(ccd_context_t** context);
void ccd_shutdown(ccd_context_t* context);

ccd_error_t ccd_body_create(ccd_context_t* context, const ccd_body_desc_t* desc, ccd_body_t** body);
ccd_error_t ccd_body_destroy(ccd_context_t* context, ccd_body_t* body);

ccd_error_t ccd_sweep_test(ccd_context_t* context, ccd_body_t* body, float time_step, ccd_sweep_result_t* result);
ccd_error_t ccd_ray_cast(ccd_context_t* context, const vec3_t* start, const vec3_t* direction, float max_distance, ccd_sweep_result_t* result);

ccd_error_t ccd_update(ccd_context_t* context, float time_step);
ccd_error_t ccd_set_high_speed_threshold(ccd_context_t* context, float threshold);

#ifdef __cplusplus
}
#endif

#endif
