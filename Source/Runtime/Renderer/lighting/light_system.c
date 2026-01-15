#include "light_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_LIGHTS 1024

typedef struct {
    Light lights[MAX_LIGHTS];
    bool active[MAX_LIGHTS];
    uint32_t active_count;
    uint32_t next_id;
    bool initialized;
} LightSystemState;

static LightSystemState state = {0};

bool light_system_init(void) {
    if (state.initialized) return true;

    memset(&state, 0, sizeof(LightSystemState));
    state.next_id = 1; // Start IDs at 1
    state.initialized = true;

    printf("[LightSystem] Initialized. Max lights: %d\n", MAX_LIGHTS);
    return true;
}

void light_system_shutdown(void) {
    if (!state.initialized) return;

    memset(&state, 0, sizeof(LightSystemState));
    printf("[LightSystem] Shutdown.\n");
}

void light_system_update(float delta_time) {
    if (!state.initialized) return;

    // Future: Animate lights, update shadow maps, etc.
    (void)delta_time;
}

uint32_t light_system_create_light(LightType type) {
    if (!state.initialized) return 0;

    // Find a free slot
    int slot = -1;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (!state.active[i]) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        printf("[LightSystem] Error: Max lights reached (%d)\n", MAX_LIGHTS);
        return 0;
    }

    // Initialize light
    Light* light = &state.lights[slot];
    memset(light, 0, sizeof(Light));

    light->base.id = state.next_id++;
    light->base.type = type;
    light->base.enabled = true;
    light->base.cast_shadows = false;
    light->base.intensity = 1.0f;
    light->base.color = (Vec3){1.0f, 1.0f, 1.0f};
    light->base.shadow_bias = 0.005f;
    light->base.shadow_normal_bias = 0.005f;
    light->base.shadow_map_resolution = 1024;

    // Set type specific defaults
    switch (type) {
        case LIGHT_TYPE_DIRECTIONAL:
            light->directional.direction = (Vec3){0.0f, -1.0f, 0.0f};
            break;
        case LIGHT_TYPE_POINT:
            light->point.range = 10.0f;
            light->point.constant_attenuation = 1.0f;
            light->point.linear_attenuation = 0.09f;
            light->point.quadratic_attenuation = 0.032f;
            break;
        case LIGHT_TYPE_SPOT:
            light->spot.range = 10.0f;
            light->spot.direction = (Vec3){0.0f, -1.0f, 0.0f};
            light->spot.inner_cone_angle = 0.5f; // ~28 degrees
            light->spot.outer_cone_angle = 0.7f; // ~40 degrees
            light->spot.constant_attenuation = 1.0f;
            light->spot.linear_attenuation = 0.09f;
            light->spot.quadratic_attenuation = 0.032f;
            break;
        default:
            break;
    }

    state.active[slot] = true;
    state.active_count++;

    return light->base.id;
}

void light_system_destroy_light(uint32_t id) {
    if (!state.initialized || id == 0) return;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (state.active[i] && state.lights[i].base.id == id) {
            state.active[i] = false;
            state.active_count--;
            // Optional: Clear data, but not strictly necessary if we rely on active flag
            return;
        }
    }

    printf("[LightSystem] Warning: distinct light %u not found.\n", id);
}

Light* light_system_get_light(uint32_t id) {
    if (!state.initialized || id == 0) return NULL;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (state.active[i] && state.lights[i].base.id == id) {
            return &state.lights[i];
        }
    }

    return NULL;
}

const Light* light_system_get_all_lights(uint32_t* out_count) {
    if (!state.initialized) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    // For now, return the internal array. In a real engine, we might pack active lights
    // into a contiguous buffer for the GPU. Since we return 'const Light*', the user
    // must iterate up to MAX_LIGHTS and check .enabled or rely on us compacting it.

    // To implement "get all active", we should compact them into a temporary buffer or
    // ensure the internal array is packed. For simplicity in this step, I'll return a
    // static buffer that is repopulated on call, or just return the raw array and
    // let the user filter.

    // Better approach: We have `active_count`. Let's assume we want to return a contiguous list.
    // We can use a static buffer for this.

    static Light packed_lights[MAX_LIGHTS];
    uint32_t count = 0;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (state.active[i]) {
            packed_lights[count++] = state.lights[i];
        }
    }

    if (out_count) *out_count = count;
    return packed_lights;
}

void light_system_prune(void) {
    // Implementation for pruning... mostly irrelevant with the current slot system
    // unless we want to defragment the slots.
}
