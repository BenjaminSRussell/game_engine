#ifndef GROUP_BEHAVIOR_H
#define GROUP_BEHAVIOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct group_behavior_context_t group_behavior_context_t;
typedef struct flocking_agent_t flocking_agent_t;
typedef struct formation_t formation_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    vec3_t position;
    vec3_t velocity;
    vec3_t acceleration;
    float max_speed;
    float max_force;
    float perception_radius;
    uint32_t id;
    bool active;
} flocking_agent_desc_t;

typedef enum {
    FORMATION_LINE,
    FORMATION_COLUMN,
    FORMATION_WEDGE,
    FORMATION_V,
    FORMATION_CIRCLE,
    FORMATION_BOX,
    FORMATION_CUSTOM
} formation_type_t;

typedef struct {
    formation_type_t type;
    vec3_t center;
    vec3_t direction;
    float spacing;
    uint32_t rows;
    uint32_t columns;
    float radius; // For circle formation
    vec3_t* custom_positions; // For custom formation
    uint32_t custom_count;
} formation_desc_t;

typedef struct {
    vec3_t separation_force;
    vec3_t alignment_force;
    vec3_t cohesion_force;
    vec3_t target_force;
    vec3_t avoidance_force;
    vec3_t total_force;
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float target_weight;
    float avoidance_weight;
} flocking_forces_t;

typedef struct {
    uint32_t agent_count;
    uint32_t active_agents;
    float avg_speed;
    float avg_separation;
    float avg_alignment;
    float avg_cohesion;
    uint32_t formation_changes;
} group_behavior_stats_t;

typedef enum {
    GROUP_SUCCESS = 0,
    GROUP_ERROR_INVALID_PARAM = -1,
    GROUP_ERROR_OUT_OF_MEMORY = -2,
    GROUP_ERROR_NOT_INITIALIZED = -3,
    GROUP_ERROR_AGENT_NOT_FOUND = -4,
    GROUP_ERROR_FORMATION_INVALID = -5
} group_error_t;

group_error_t group_behavior_init(group_behavior_context_t** context, uint32_t max_agents);
void group_behavior_shutdown(group_behavior_context_t* context);

group_error_t flocking_agent_create(group_behavior_context_t* context, const flocking_agent_desc_t* desc, flocking_agent_t** agent);
group_error_t flocking_agent_destroy(group_behavior_context_t* context, flocking_agent_t* agent);
group_error_t flocking_agent_update(group_behavior_context_t* context, flocking_agent_t* agent, float dt);

group_error_t formation_create(group_behavior_context_t* context, const formation_desc_t* desc, formation_t** formation);
group_error_t formation_destroy(group_behavior_context_t* context, formation_t* formation);
group_error_t formation_update(group_behavior_context_t* context, formation_t* formation, flocking_agent_t** agents, uint32_t agent_count);

group_error_t group_behavior_update(group_behavior_context_t* context, float dt);
group_error_t group_behavior_set_target(group_behavior_context_t* context, const vec3_t* target);
group_error_t group_behavior_set_formation(group_behavior_context_t* context, formation_type_t type);

group_error_t group_behavior_get_forces(group_behavior_context_t* context, flocking_agent_t* agent, flocking_forces_t* forces);
group_error_t group_behavior_get_stats(group_behavior_context_t* context, group_behavior_stats_t* stats);

group_error_t group_behavior_enable_separation(group_behavior_context_t* context, bool enabled, float weight);
group_error_t group_behavior_enable_alignment(group_behavior_context_t* context, bool enabled, float weight);
group_error_t group_behavior_enable_cohesion(group_behavior_context_t* context, bool enabled, float weight);
group_error_t group_behavior_enable_target_seeking(group_behavior_context_t* context, bool enabled, float weight);
group_error_t group_behavior_enable_obstacle_avoidance(group_behavior_context_t* context, bool enabled, float weight);

#ifdef __cplusplus
}
#endif

#endif
