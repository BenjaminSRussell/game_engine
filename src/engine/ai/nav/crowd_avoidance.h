#pragma once

typedef struct {
    float position[3];
    float velocity[3];
    float desired_velocity[3];
    float radius;
    float max_speed;
} CrowdAgent;

typedef struct {
    CrowdAgent *agents;
    int agent_count;
    float neighbor_distance;
    float time_horizon;
} CrowdAvoidance;

void crowd_avoidance_init(CrowdAvoidance *crowd);
void crowd_avoidance_destroy(CrowdAvoidance *crowd);

void crowd_add_agent(CrowdAvoidance *crowd, const float *pos, float radius);
void crowd_set_desired_velocity(CrowdAvoidance *crowd, int agent_idx, const float *desired_vel);

// RVO (Reciprocal Velocity Obstacles)
void crowd_compute_avoidance(CrowdAvoidance *crowd, float delta_time);
void crowd_update_positions(CrowdAvoidance *crowd, float delta_time);
