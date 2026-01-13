/*
 * npc_jobs.c
 * Job assignment and task management for NPCs
 *
 * Part of the NPC subsystem
 * Advanced 3D Rendering Engine
 */

#include "npc/npc_jobs.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

// Simple Vec3 structure for NPC jobs
typedef struct {
    float x, y, z;
} SimpleVec3;

// Job types
typedef enum {
    JOB_TYPE_IDLE = 0,
    JOB_TYPE_WANDER = 1,
    JOB_TYPE_PATROL = 2,
    JOB_TYPE_WORK = 3,
    JOB_TYPE_SLEEP = 4,
    JOB_TYPE_EAT = 5,
    JOB_TYPE_FIGHT = 6,
    JOB_TYPE_FLEE = 7
} JobType;

// Job structure
typedef struct {
    int id;
    int npc_id;
    JobType type;
    SimpleVec3 target_position;
    int target_npc_id;
    float priority;
    int active;
    uint32_t start_time;
    uint32_t duration;
} Job;

#define MAX_JOBS 128
static Job g_jobs[MAX_JOBS];
static int g_job_count = 0;
static int g_next_job_id = 1;

// Initialize job system
void npc_jobs_init(void) {
    memset(g_jobs, 0, sizeof(g_jobs));
    g_job_count = 0;
    g_next_job_id = 1;
    
    LOG_INFO("NPC jobs system initialized");
}

// Shutdown job system
void npc_jobs_shutdown(void) {
    memset(g_jobs, 0, sizeof(g_jobs));
    g_job_count = 0;
    g_next_job_id = 1;
    
    LOG_INFO("NPC jobs system shutdown");
}

// Create new job
int npc_jobs_create(int npc_id, JobType type, Vec3 target, float priority) {
    if (g_job_count >= MAX_JOBS) {
        LOG_ERROR("NPC jobs: Maximum jobs reached");
        return -1;
    }
    
    Job* job = &g_jobs[g_job_count];
    job->id = g_next_job_id++;
    job->npc_id = npc_id;
    job->type = type;
    job->target_position = target;
    job->target_npc_id = -1;
    job->priority = priority;
    job->active = 1;
    job->start_time = (uint32_t)time(NULL);
    job->duration = 5000 + (rand() % 10000); // 5-15 seconds
    
    g_job_count++;
    LOG_INFO("Job created: NPC %d, Type %d, Priority %.2f", npc_id, type, priority);
    
    return job->id;
}

// Remove job
void npc_jobs_remove(int job_id) {
    for (int i = 0; i < g_job_count; i++) {
        if (g_jobs[i].active && g_jobs[i].id == job_id) {
            g_jobs[i].active = 0;
            
            // Move last job to this slot
            if (i < g_job_count - 1) {
                g_jobs[i] = g_jobs[g_job_count - 1];
            }
            
            g_job_count--;
            LOG_INFO("Job removed: ID %d", job_id);
            return;
        }
    }
    
    LOG_WARNING("Job not found for removal: ID %d", job_id);
}

// Update jobs system
void npc_jobs_update(float dt) {
    uint32_t current_time = (uint32_t)time(NULL);
    
    for (int i = 0; i < g_job_count; i++) {
        if (!g_jobs[i].active) {
            continue;
        }
        
        Job* job = &g_jobs[i];
        
        // Check if job is complete
        if (current_time - job->start_time > job->duration) {
            LOG_DEBUG("Job completed: ID %d for NPC %d", job->id, job->npc_id);
            job->active = 0;
            
            // Create new idle job
            npc_jobs_create(job->npc_id, JOB_TYPE_IDLE, job->target_position, 0.1f);
        }
    }
}

// Get job for NPC
int npc_jobs_get_job(int npc_id) {
    for (int i = 0; i < g_job_count; i++) {
        if (g_jobs[i].active && g_jobs[i].npc_id == npc_id) {
            return g_jobs[i].id;
        }
    }
    
    return -1;
}

// Get all jobs for NPC
void* npc_jobs_get_all_for_npc(int npc_id, int* count) {
    static Job npc_jobs[MAX_JOBS];
    int found_count = 0;
    
    for (int i = 0; i < g_job_count; i++) {
        if (g_jobs[i].active && g_jobs[i].npc_id == npc_id) {
            npc_jobs[found_count] = g_jobs[i];
            found_count++;
        }
    }
    
    if (count) {
        *count = found_count;
    }
    
    return npc_jobs;
}

// Assign patrol job
void npc_jobs_assign_patrol(int npc_id, Vec3* waypoints, int waypoint_count) {
    if (waypoint_count > 0) {
        Vec3 target = waypoints[0]; // Start with first waypoint
        npc_jobs_create(npc_id, JOB_TYPE_PATROL, target, 1.0f);
        LOG_INFO("Patrol job assigned to NPC %d with %d waypoints", npc_id, waypoint_count);
    }
}

// Assign work job
void npc_jobs_assign_work(int npc_id, Vec3 work_location) {
    npc_jobs_create(npc_id, JOB_TYPE_WORK, work_location, 2.0f);
    LOG_INFO("Work job assigned to NPC %d at (%.2f, %.2f, %.2f)", 
             npc_id, work_location.x, work_location.y, work_location.z);
}

// Assign sleep job
void npc_jobs_assign_sleep(int npc_id, Vec3 sleep_location) {
    npc_jobs_create(npc_id, JOB_TYPE_SLEEP, sleep_location, 0.5f);
    LOG_INFO("Sleep job assigned to NPC %d", npc_id);
}

// Assign fight job
void npc_jobs_assign_fight(int npc_id, int target_npc_id) {
    // Get target NPC position
    extern Vec3 npc_get_position(int npc_id);
    Vec3 target_pos = npc_get_position(target_npc_id);
    
    Job* job = NULL;
    for (int i = 0; i < g_job_count; i++) {
        if (g_jobs[i].active && g_jobs[i].npc_id == npc_id) {
            job = &g_jobs[i];
            break;
        }
    }
    
    if (job) {
        job->type = JOB_TYPE_FIGHT;
        job->target_position = target_pos;
        job->target_npc_id = target_npc_id;
        job->priority = 3.0f;
        LOG_INFO("Fight job assigned: NPC %d targeting NPC %d", npc_id, target_npc_id);
    }
}

// Get job count
int npc_jobs_get_count(void) {
    return g_job_count;
}

// Get active jobs count
int npc_jobs_get_active_count(void) {
    int count = 0;
    for (int i = 0; i < g_job_count; i++) {
        if (g_jobs[i].active) {
            count++;
        }
    }
    return count;
}
