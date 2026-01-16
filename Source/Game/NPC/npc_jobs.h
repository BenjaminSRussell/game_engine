/*
 * NPC Jobs System Header
 */

#ifndef NPC_JOBS_H
#define NPC_JOBS_H

#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

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

// Initialize job system
void npc_jobs_init(void);

// Shutdown job system
void npc_jobs_shutdown(void);

// Create new job
int npc_jobs_create(int npc_id, JobType type, Vec3 target, float priority);

// Remove job
void npc_jobs_remove(int job_id);

// Update jobs system
void npc_jobs_update(float dt);

// Get job for NPC
int npc_jobs_get_job(int npc_id);

// Get all jobs for NPC
void* npc_jobs_get_all_for_npc(int npc_id, int* count);

// Assign patrol job
void npc_jobs_assign_patrol(int npc_id, Vec3* waypoints, int waypoint_count);

// Assign work job
void npc_jobs_assign_work(int npc_id, Vec3 work_location);

// Assign sleep job
void npc_jobs_assign_sleep(int npc_id, Vec3 sleep_location);

// Assign fight job
void npc_jobs_assign_fight(int npc_id, int target_npc_id);

// Get job count
int npc_jobs_get_count(void);

// Get active jobs count
int npc_jobs_get_active_count(void);

#ifdef __cplusplus
}
#endif

#endif // NPC_JOBS_H
