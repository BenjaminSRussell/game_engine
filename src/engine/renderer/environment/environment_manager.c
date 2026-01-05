#include "environment/environment_manager.h"
#include "environment/skybox.h"
#include "environment/ibl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Create environment manager
EnvironmentManager* environment_manager_create(void) {
    EnvironmentManager* env = calloc(1, sizeof(EnvironmentManager));
    if (!env) return NULL;
    
    env->skybox = skybox_create();
    env->ibl_system = ibl_system_create();
    
    env->state.time_of_day = 12.0f;
    env->state.time_speed = 0.0f;
    env->state.ambient_intensity = 0.3f;
    
    printf("[EnvironmentManager] Created\n");
    return env;
}

void environment_manager_destroy(EnvironmentManager* env) {
    if (!env) return;
    if (env->skybox) skybox_destroy(env->skybox);
    if (env->ibl_system) ibl_system_destroy(env->ibl_system);
    free(env);
}

void environment_set_time_of_day(EnvironmentManager* env, float hours) {
    if (!env) return;
    env->state.time_of_day = fmodf(hours, 24.0f);
    if (env->skybox) skybox_set_time_of_day(env->skybox, env->state.time_of_day);
}

void environment_update(EnvironmentManager* env, float delta_time) {
    if (!env) return;
    env->state.time_of_day += delta_time * env->state.time_speed;
    env->state.time_of_day = fmodf(env->state.time_of_day, 24.0f);
}

Skybox* environment_get_skybox(EnvironmentManager* env) {
    return env ? env->skybox : NULL;
}

IBLSystem* environment_get_ibl_system(EnvironmentManager* env) {
    return env ? env->ibl_system : NULL;
}
