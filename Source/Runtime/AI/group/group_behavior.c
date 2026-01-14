#include "group_behavior.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct flocking_agent_t {
    vec3_t position;
    vec3_t velocity;
    vec3_t acceleration;
    float max_speed;
    float max_force;
    float perception_radius;
    uint32_t id;
    bool active;
};

struct formation_t {
    formation_type_t type;
    vec3_t center;
    vec3_t direction;
    float spacing;
    uint32_t rows;
    uint32_t columns;
    float radius;
    vec3_t* positions;
    uint32_t position_count;
};

struct group_behavior_context_t {
    bool initialized;
    
    // Agent management
    flocking_agent_t** agents;
    uint32_t agent_count;
    uint32_t agent_capacity;
    uint32_t next_agent_id;
    
    // Flocking parameters
    bool separation_enabled;
    bool alignment_enabled;
    bool cohesion_enabled;
    bool target_seeking_enabled;
    bool obstacle_avoidance_enabled;
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float target_weight;
    float avoidance_weight;
    
    // Formation
    formation_t* current_formation;
    vec3_t group_target;
    bool has_target;
    
    // Statistics
    group_behavior_stats_t stats;
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

static float vec3_distance(const vec3_t* a, const vec3_t* b) {
    vec3_t diff = {a->x - b->x, a->y - b->y, a->z - b->z};
    return vec3_length(&diff);
}

static void vec3_add(vec3_t* result, const vec3_t* a, const vec3_t* b) {
    result->x = a->x + b->x;
    result->y = a->y + b->y;
    result->z = a->z + b->z;
}

static void vec3_sub(vec3_t* result, const vec3_t* a, const vec3_t* b) {
    result->x = a->x - b->x;
    result->y = a->y - b->y;
    result->z = a->z - b->z;
}

static void vec3_mul_scalar(vec3_t* result, const vec3_t* v, float scalar) {
    result->x = v->x * scalar;
    result->y = v->y * scalar;
    result->z = v->z * scalar;
}

static float vec3_dot(const vec3_t* a, const vec3_t* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static void vec3_limit(vec3_t* v, float max) {
    float len = vec3_length(v);
    if (len > max) {
        vec3_normalize(v);
        v->x *= max;
        v->y *= max;
        v->z *= max;
    }
}

group_error_t group_behavior_init(group_behavior_context_t** context, uint32_t max_agents) {
    if (!context) return GROUP_ERROR_INVALID_PARAM;
    
    group_behavior_context_t* ctx = (group_behavior_context_t*)calloc(1, sizeof(group_behavior_context_t));
    if (!ctx) return GROUP_ERROR_OUT_OF_MEMORY;
    
    ctx->agent_capacity = max_agents;
    ctx->agents = (flocking_agent_t**)calloc(max_agents, sizeof(flocking_agent_t*));
    if (!ctx->agents) {
        free(ctx);
        return GROUP_ERROR_OUT_OF_MEMORY;
    }
    
    // Default flocking parameters
    ctx->separation_enabled = true;
    ctx->alignment_enabled = true;
    ctx->cohesion_enabled = true;
    ctx->target_seeking_enabled = true;
    ctx->obstacle_avoidance_enabled = false;
    ctx->separation_weight = 1.5f;
    ctx->alignment_weight = 1.0f;
    ctx->cohesion_weight = 1.0f;
    ctx->target_weight = 0.5f;
    ctx->avoidance_weight = 2.0f;
    
    ctx->initialized = true;
    *context = ctx;
    return GROUP_SUCCESS;
}

void group_behavior_shutdown(group_behavior_context_t* context) {
    if (!context) return;
    
    for (uint32_t i = 0; i < context->agent_count; i++) {
        free(context->agents[i]);
    }
    
    if (context->current_formation) {
        free(context->current_formation->positions);
        free(context->current_formation);
    }
    
    free(context->agents);
    free(context);
}

group_error_t flocking_agent_create(group_behavior_context_t* context, const flocking_agent_desc_t* desc, flocking_agent_t** agent) {
    if (!context || !desc || !agent) return GROUP_ERROR_INVALID_PARAM;
    if (!context->initialized) return GROUP_ERROR_NOT_INITIALIZED;
    if (context->agent_count >= context->agent_capacity) return GROUP_ERROR_OUT_OF_MEMORY;
    
    flocking_agent_t* new_agent = (flocking_agent_t*)calloc(1, sizeof(flocking_agent_t));
    if (!new_agent) return GROUP_ERROR_OUT_OF_MEMORY;
    
    new_agent->position = desc->position;
    new_agent->velocity = desc->velocity;
    new_agent->acceleration = desc->acceleration;
    new_agent->max_speed = desc->max_speed;
    new_agent->max_force = desc->max_force;
    new_agent->perception_radius = desc->perception_radius;
    new_agent->id = context->next_agent_id++;
    new_agent->active = desc->active;
    
    context->agents[context->agent_count++] = new_agent;
    *agent = new_agent;
    
    return GROUP_SUCCESS;
}

static vec3_t calculate_separation(flocking_agent_t* agent, flocking_agent_t** neighbors, uint32_t neighbor_count) {
    vec3_t steer = {0, 0, 0};
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < neighbor_count; i++) {
        flocking_agent_t* other = neighbors[i];
        if (other == agent || !other->active) continue;
        
        float distance = vec3_distance(&agent->position, &other->position);
        if (distance > 0 && distance < agent->perception_radius) {
            vec3_t diff;
            vec3_sub(&diff, &agent->position, &other->position);
            vec3_normalize(&diff);
            vec3_mul_scalar(&diff, &diff, 1.0f / distance); // Weight by distance
            vec3_add(&steer, &steer, &diff);
            count++;
        }
    }
    
    if (count > 0) {
        vec3_mul_scalar(&steer, &steer, 1.0f / count);
        vec3_normalize(&steer);
        vec3_mul_scalar(&steer, &steer, agent->max_speed);
        vec3_sub(&steer, &steer, &agent->velocity);
        vec3_limit(&steer, agent->max_force);
    }
    
    return steer;
}

static vec3_t calculate_alignment(flocking_agent_t* agent, flocking_agent_t** neighbors, uint32_t neighbor_count) {
    vec3_t sum = {0, 0, 0};
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < neighbor_count; i++) {
        flocking_agent_t* other = neighbors[i];
        if (other == agent || !other->active) continue;
        
        float distance = vec3_distance(&agent->position, &other->position);
        if (distance > 0 && distance < agent->perception_radius) {
            vec3_add(&sum, &sum, &other->velocity);
            count++;
        }
    }
    
    if (count > 0) {
        vec3_mul_scalar(&sum, &sum, 1.0f / count);
        vec3_normalize(&sum);
        vec3_mul_scalar(&sum, &sum, agent->max_speed);
        vec3_sub(&sum, &sum, &agent->velocity);
        vec3_limit(&sum, agent->max_force);
    }
    
    return sum;
}

static vec3_t calculate_cohesion(flocking_agent_t* agent, flocking_agent_t** neighbors, uint32_t neighbor_count) {
    vec3_t sum = {0, 0, 0};
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < neighbor_count; i++) {
        flocking_agent_t* other = neighbors[i];
        if (other == agent || !other->active) continue;
        
        float distance = vec3_distance(&agent->position, &other->position);
        if (distance > 0 && distance < agent->perception_radius) {
            vec3_add(&sum, &sum, &other->position);
            count++;
        }
    }
    
    if (count > 0) {
        vec3_mul_scalar(&sum, &sum, 1.0f / count);
        vec3_t steer;
        vec3_sub(&steer, &sum, &agent->position);
        vec3_normalize(&steer);
        vec3_mul_scalar(&steer, &steer, agent->max_speed);
        vec3_sub(&steer, &steer, &agent->velocity);
        vec3_limit(&steer, agent->max_force);
        return steer;
    }
    
    vec3_t zero = {0, 0, 0};
    return zero;
}

static vec3_t calculate_target_seeking(flocking_agent_t* agent, const vec3_t* target) {
    vec3_t desired;
    vec3_sub(&desired, target, &agent->position);
    vec3_normalize(&desired);
    vec3_mul_scalar(&desired, &desired, agent->max_speed);
    
    vec3_t steer;
    vec3_sub(&steer, &desired, &agent->velocity);
    vec3_limit(&steer, agent->max_force);
    
    return steer;
}

group_error_t flocking_agent_update(group_behavior_context_t* context, flocking_agent_t* agent, float dt) {
    if (!context || !agent || !context->initialized) return GROUP_ERROR_INVALID_PARAM;
    
    // Calculate flocking forces
    vec3_t separation = {0, 0, 0};
    vec3_t alignment = {0, 0, 0};
    vec3_t cohesion = {0, 0, 0};
    vec3_t target = {0, 0, 0};
    
    if (context->separation_enabled) {
        separation = calculate_separation(agent, context->agents, context->agent_count);
    }
    if (context->alignment_enabled) {
        alignment = calculate_alignment(agent, context->agents, context->agent_count);
    }
    if (context->cohesion_enabled) {
        cohesion = calculate_cohesion(agent, context->agents, context->agent_count);
    }
    if (context->target_seeking_enabled && context->has_target) {
        target = calculate_target_seeking(agent, &context->group_target);
    }
    
    // Apply forces
    agent->acceleration.x = 0;
    agent->acceleration.y = 0;
    agent->acceleration.z = 0;
    
    vec3_mul_scalar(&separation, &separation, context->separation_weight);
    vec3_mul_scalar(&alignment, &alignment, context->alignment_weight);
    vec3_mul_scalar(&cohesion, &cohesion, context->cohesion_weight);
    vec3_mul_scalar(&target, &target, context->target_weight);
    
    vec3_add(&agent->acceleration, &agent->acceleration, &separation);
    vec3_add(&agent->acceleration, &agent->acceleration, &alignment);
    vec3_add(&agent->acceleration, &agent->acceleration, &cohesion);
    vec3_add(&agent->acceleration, &agent->acceleration, &target);
    
    // Update velocity and position
    vec3_add(&agent->velocity, &agent->velocity, &agent->acceleration);
    vec3_limit(&agent->velocity, agent->max_speed);
    vec3_add(&agent->position, &agent->position, &agent->velocity);
    
    // Reset acceleration
    agent->acceleration.x = 0;
    agent->acceleration.y = 0;
    agent->acceleration.z = 0;
    
    return GROUP_SUCCESS;
}

group_error_t formation_create(group_behavior_context_t* context, const formation_desc_t* desc, formation_t** formation) {
    if (!context || !desc || !formation) return GROUP_ERROR_INVALID_PARAM;
    
    formation_t* new_formation = (formation_t*)calloc(1, sizeof(formation_t));
    if (!new_formation) return GROUP_ERROR_OUT_OF_MEMORY;
    
    new_formation->type = desc->type;
    new_formation->center = desc->center;
    new_formation->direction = desc->direction;
    new_formation->spacing = desc->spacing;
    new_formation->rows = desc->rows;
    new_formation->columns = desc->columns;
    new_formation->radius = desc->radius;
    
    // Calculate formation positions
    uint32_t position_count = desc->rows * desc->columns;
    new_formation->positions = (vec3_t*)malloc(position_count * sizeof(vec3_t));
    if (!new_formation->positions) {
        free(new_formation);
        return GROUP_ERROR_OUT_OF_MEMORY;
    }
    
    for (uint32_t row = 0; row < desc->rows; row++) {
        for (uint32_t col = 0; col < desc->columns; col++) {
            vec3_t pos = {0, 0, 0};
            
            switch (desc->type) {
                case FORMATION_LINE:
                    pos.x = (float)col * desc->spacing;
                    break;
                case FORMATION_COLUMN:
                    pos.z = (float)row * desc->spacing;
                    break;
                case FORMATION_WEDGE:
                    pos.x = (float)(col - desc->columns/2) * desc->spacing;
                    pos.z = (float)row * desc->spacing;
                    break;
                case FORMATION_V:
                    pos.x = (float)(row - desc->rows/2) * desc->spacing;
                    pos.z = (float)col * desc->spacing;
                    break;
                case FORMATION_CIRCLE:
                    {
                        float angle = 2.0f * M_PI * (row * desc->columns + col) / position_count;
                        pos.x = desc->radius * cosf(angle);
                        pos.z = desc->radius * sinf(angle);
                    }
                    break;
                case FORMATION_BOX:
                    pos.x = (float)(col - desc->columns/2) * desc->spacing;
                    pos.z = (float)(row - desc->rows/2) * desc->spacing;
                    break;
                default:
                    break;
            }
            
            vec3_add(&pos, &pos, &desc->center);
            new_formation->positions[row * desc->columns + col] = pos;
        }
    }
    
    new_formation->position_count = position_count;
    *formation = new_formation;
    
    return GROUP_SUCCESS;
}

group_error_t group_behavior_set_target(group_behavior_context_t* context, const vec3_t* target) {
    if (!context || !target) return GROUP_ERROR_INVALID_PARAM;
    if (!context->initialized) return GROUP_ERROR_NOT_INITIALIZED;
    
    context->group_target = *target;
    context->has_target = true;
    
    return GROUP_SUCCESS;
}

group_error_t group_behavior_get_stats(group_behavior_context_t* context, group_behavior_stats_t* stats) {
    if (!context || !stats) return GROUP_ERROR_INVALID_PARAM;
    if (!context->initialized) return GROUP_ERROR_NOT_INITIALIZED;
    
    stats->agent_count = context->agent_count;
    stats->active_agents = 0;
    
    float total_speed = 0;
    for (uint32_t i = 0; i < context->agent_count; i++) {
        if (context->agents[i]->active) {
            stats->active_agents++;
            total_speed += vec3_length(&context->agents[i]->velocity);
        }
    }
    
    if (stats->active_agents > 0) {
        stats->avg_speed = total_speed / stats->active_agents;
    }
    
    return GROUP_SUCCESS;
}
