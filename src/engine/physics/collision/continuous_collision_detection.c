#include "continuous_collision_detection.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct ccd_body_t {
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
    uint32_t id;
};

struct ccd_context_t {
    bool initialized;
    ccd_body_t** bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    float high_speed_threshold;
    uint32_t next_id;
};

static float vec3_length(const vec3_t* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static void vec3_normalize(vec3_t* v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        v->x /= len;
        v->y /= len;
        v->z /= len;
    }
}

static float vec3_dot(const vec3_t* a, const vec3_t* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static void vec3_sub(vec3_t* result, const vec3_t* a, const vec3_t* b) {
    result->x = a->x - b->x;
    result->y = a->y - b->y;
    result->z = a->z - b->z;
}

static void vec3_add_scaled(vec3_t* result, const vec3_t* a, const vec3_t* b, float scale) {
    result->x = a->x + b->x * scale;
    result->y = a->y + b->y * scale;
    result->z = a->z + b->z * scale;
}

static bool aabb_intersects(const aabb_t* a, const aabb_t* b) {
    return (a->min.x <= b->max.x && a->max.x >= b->min.x) &&
           (a->min.y <= b->max.y && a->max.y >= b->min.y) &&
           (a->min.z <= b->max.z && a->max.z >= b->min.z);
}

static bool sphere_sweep(const sphere_t* s1, const vec3_t* v1, 
                        const sphere_t* s2, const vec3_t* v2, 
                        float max_time, float* hit_time) {
    vec3_t relative_vel;
    vec3_sub(&relative_vel, v1, v2);
    
    vec3_t relative_pos;
    vec3_sub(&relative_pos, &s1->center, &s2->center);
    
    float radius_sum = s1->radius + s2->radius;
    float a = vec3_dot(&relative_vel, &relative_vel);
    float b = 2.0f * vec3_dot(&relative_pos, &relative_vel);
    float c = vec3_dot(&relative_pos, &relative_pos) - radius_sum * radius_sum;
    
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) return false;
    
    float sqrt_disc = sqrtf(discriminant);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);
    
    if (t1 < 0.0f && t2 < 0.0f) return false;
    
    *hit_time = (t1 >= 0.0f) ? t1 : t2;
    return *hit_time <= max_time;
}

ccd_error_t ccd_init(ccd_context_t** context) {
    if (!context) return CCD_ERROR_INVALID_PARAM;
    
    ccd_context_t* ctx = (ccd_context_t*)calloc(1, sizeof(ccd_context_t));
    if (!ctx) return CCD_ERROR_OUT_OF_MEMORY;
    
    ctx->body_capacity = 1024;
    ctx->bodies = (ccd_body_t**)calloc(ctx->body_capacity, sizeof(ccd_body_t*));
    if (!ctx->bodies) {
        free(ctx);
        return CCD_ERROR_OUT_OF_MEMORY;
    }
    
    ctx->high_speed_threshold = 10.0f;
    ctx->initialized = true;
    *context = ctx;
    return CCD_SUCCESS;
}

void ccd_shutdown(ccd_context_t* context) {
    if (!context) return;
    
    for (uint32_t i = 0; i < context->body_count; i++) {
        free(context->bodies[i]);
    }
    
    free(context->bodies);
    free(context);
}

ccd_error_t ccd_body_create(ccd_context_t* context, const ccd_body_desc_t* desc, ccd_body_t** body) {
    if (!context || !desc || !body) return CCD_ERROR_INVALID_PARAM;
    if (!context->initialized) return CCD_ERROR_NOT_INITIALIZED;
    
    ccd_body_t* new_body = (ccd_body_t*)calloc(1, sizeof(ccd_body_t));
    if (!new_body) return CCD_ERROR_OUT_OF_MEMORY;
    
    new_body->position = desc->position;
    new_body->velocity = desc->velocity;
    new_body->acceleration = desc->acceleration;
    new_body->aabb = desc->aabb;
    new_body->sphere = desc->sphere;
    new_body->mass = desc->mass;
    new_body->restitution = desc->restitution;
    new_body->friction = desc->friction;
    new_body->is_static = desc->is_static;
    new_body->high_speed = vec3_length(&desc->velocity) > context->high_speed_threshold;
    new_body->id = context->next_id++;
    
    if (context->body_count >= context->body_capacity) {
        return CCD_ERROR_OUT_OF_MEMORY;
    }
    
    context->bodies[context->body_count++] = new_body;
    *body = new_body;
    return CCD_SUCCESS;
}

ccd_error_t ccd_sweep_test(ccd_context_t* context, ccd_body_t* body, float time_step, ccd_sweep_result_t* result) {
    if (!context || !body || !result) return CCD_ERROR_INVALID_PARAM;
    
    memset(result, 0, sizeof(ccd_sweep_result_t));
    result->time_of_impact = time_step;
    
    for (uint32_t i = 0; i < context->body_count; i++) {
        ccd_body_t* other = context->bodies[i];
        if (other == body) continue;
        
        float hit_time;
        if (sphere_sweep(&body->sphere, &body->velocity, 
                        &other->sphere, &other->velocity, 
                        time_step, &hit_time)) {
            if (hit_time < result->time_of_impact) {
                result->collision = true;
                result->time_of_impact = hit_time;
                result->body1 = body;
                result->body2 = other;
                
                vec3_sub(&result->contact_normal, &other->sphere.center, &body->sphere.center);
                vec3_normalize(&result->contact_normal);
                
                vec3_add_scaled(&result->contact_point, &body->sphere.center, &body->velocity, hit_time);
            }
        }
    }
    
    return CCD_SUCCESS;
}

ccd_error_t ccd_update(ccd_context_t* context, float time_step) {
    if (!context) return CCD_ERROR_INVALID_PARAM;
    
    for (uint32_t i = 0; i < context->body_count; i++) {
        ccd_body_t* body = context->bodies[i];
        if (body->is_static) continue;
        
        vec3_add_scaled(&body->position, &body->position, &body->velocity, time_step);
        vec3_add_scaled(&body->velocity, &body->velocity, &body->acceleration, time_step);
        
        body->sphere.center = body->position;
        body->high_speed = vec3_length(&body->velocity) > context->high_speed_threshold;
    }
    
    return CCD_SUCCESS;
}
