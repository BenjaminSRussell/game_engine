/**
 * =================================================================================================
 *                          CROWD AVOIDANCE (RVO)
 * =================================================================================================
 */

#include "ai/nav/crowd_avoidance.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

void crowd_avoidance_init(CrowdAvoidance *crowd) {
    crowd->agents = NULL;
    crowd->agent_count = 0;
    crowd->neighbor_distance = 5.0f;
    crowd->time_horizon = 2.0f;
}

void crowd_avoidance_destroy(CrowdAvoidance *crowd) {
    free(crowd->agents);
}

void crowd_add_agent(CrowdAvoidance *crowd, const float *pos, float radius) {
    crowd->agent_count++;
    crowd->agents = realloc(crowd->agents, sizeof(CrowdAgent) * crowd->agent_count);
    
    CrowdAgent *agent = &crowd->agents[crowd->agent_count - 1];
    memcpy(agent->position, pos, sizeof(float) * 3);
    memset(agent->velocity, 0, sizeof(float) * 3);
    memset(agent->desired_velocity, 0, sizeof(float) * 3);
    agent->radius = radius;
    agent->max_speed = 3.0f;
}

void crowd_set_desired_velocity(CrowdAvoidance *crowd, int agent_idx, const float *desired_vel) {
    if (agent_idx < 0 || agent_idx >= crowd->agent_count) return;
    memcpy(crowd->agents[agent_idx].desired_velocity, desired_vel, sizeof(float) * 3);
}

static float crowd_vec3_dot(const float *a, const float *b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static void crowd_vec3_sub(float *out, const float *a, const float *b) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

void crowd_compute_avoidance(CrowdAvoidance *crowd, float delta_time) {
    // RVO (Reciprocal Velocity Obstacles) algorithm
    
    for (int i = 0; i < crowd->agent_count; i++) {
        CrowdAgent *agent = &crowd->agents[i];
        
        float new_velocity[3];
        memcpy(new_velocity, agent->desired_velocity, sizeof(float) * 3);
        
        // Check neighbors
        for (int j = 0; j < crowd->agent_count; j++) {
            if (i == j) continue;
            
            CrowdAgent *other = &crowd->agents[j];
            
            float rel_pos[3];
            crowd_vec3_sub(rel_pos, other->position, agent->position);
            float dist = sqrtf(crowd_vec3_dot(rel_pos, rel_pos));
            
            if (dist < crowd->neighbor_distance) {
                // Compute avoidance velocity
                float rel_vel[3];
                crowd_vec3_sub(rel_vel, other->velocity, agent->velocity);
                
                // Project onto velocity obstacle cone
                float combined_radius = agent->radius + other->radius;
                
                if (dist < combined_radius) {
                    // Too close, push apart
                    float push_force = (combined_radius - dist) / combined_radius;
                    new_velocity[0] -= rel_pos[0] / dist * push_force;
                    new_velocity[1] -= rel_pos[1] / dist * push_force;
                    new_velocity[2] -= rel_pos[2] / dist * push_force;
                }
            }
        }
        
        // Clamp to max speed
        float speed = sqrtf(crowd_vec3_dot(new_velocity, new_velocity));
        if (speed > agent->max_speed) {
            float scale = agent->max_speed / speed;
            new_velocity[0] *= scale;
            new_velocity[1] *= scale;
            new_velocity[2] *= scale;
        }
        
        memcpy(agent->velocity, new_velocity, sizeof(float) * 3);
    }
}

void crowd_update_positions(CrowdAvoidance *crowd, float delta_time) {
    for (int i = 0; i < crowd->agent_count; i++) {
        CrowdAgent *agent = &crowd->agents[i];
        
        agent->position[0] += agent->velocity[0] * delta_time;
        agent->position[1] += agent->velocity[1] * delta_time;
        agent->position[2] += agent->velocity[2] * delta_time;
    }
}
