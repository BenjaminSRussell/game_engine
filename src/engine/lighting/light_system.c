#include "light_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Internal state
static struct {
    Light lights[MAX_LIGHTS];          // Storage for all lights
    Light* active_lights[MAX_LIGHTS];  // Packed array of pointers to active lights
    uint32_t active_count;
    uint32_t next_id;
    bool initialized;
} g_light_system;

void LightSystem_Init(void) {
    memset(&g_light_system, 0, sizeof(g_light_system));
    g_light_system.next_id = 1; // ID 0 is reserved for invalid
    g_light_system.initialized = true;
    printf("Light System Initialized\n");
}

void LightSystem_Shutdown(void) {
    memset(&g_light_system, 0, sizeof(g_light_system));
    g_light_system.initialized = false;
    printf("Light System Shutdown\n");
}

static void RebuildActiveList(void) {
    g_light_system.active_count = 0;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (g_light_system.lights[i].base.id != 0 && g_light_system.lights[i].base.enabled) {
            g_light_system.active_lights[g_light_system.active_count++] = &g_light_system.lights[i];
        }
    }
}

void LightSystem_Update(float delta_time) {
    if (!g_light_system.initialized) return;
    // Future: animate lights, update shadow maps, etc.
    (void)delta_time;
}

static Light* FindFreeSlot(void) {
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (g_light_system.lights[i].base.id == 0) {
            return &g_light_system.lights[i];
        }
    }
    return NULL;
}

static Light* FindLightById(uint32_t id) {
    if (id == 0) return NULL;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (g_light_system.lights[i].base.id == id) {
            return &g_light_system.lights[i];
        }
    }
    return NULL;
}

static uint32_t GetNextId(void) {
    uint32_t id = g_light_system.next_id++;
    if (g_light_system.next_id == 0) g_light_system.next_id = 1; // Wrap around safely
    return id;
}

uint32_t LightSystem_AddDirectionalLight(Vec3 direction, Vec3 color, float intensity, bool cast_shadows) {
    if (!g_light_system.initialized) return 0;

    Light* slot = FindFreeSlot();
    if (!slot) {
        printf("Error: Max lights reached\n");
        return 0;
    }

    uint32_t id = GetNextId();
    slot->base.id = id;
    slot->base.type = LIGHT_TYPE_DIRECTIONAL;
    slot->base.enabled = true;
    slot->base.cast_shadows = cast_shadows;
    slot->base.color = color;
    slot->base.intensity = intensity;

    slot->base.shadow_bias = 0.005f;
    slot->base.shadow_normal_bias = 0.005f;
    slot->base.shadow_map_resolution = 2048;

    slot->directional.direction = vec3_normalize(direction);

    RebuildActiveList();
    return id;
}

uint32_t LightSystem_AddPointLight(Vec3 position, float range, Vec3 color, float intensity, bool cast_shadows) {
    if (!g_light_system.initialized) return 0;

    Light* slot = FindFreeSlot();
    if (!slot) return 0;

    uint32_t id = GetNextId();
    slot->base.id = id;
    slot->base.type = LIGHT_TYPE_POINT;
    slot->base.enabled = true;
    slot->base.cast_shadows = cast_shadows;
    slot->base.color = color;
    slot->base.intensity = intensity;

    slot->base.shadow_bias = 0.001f;
    slot->base.shadow_normal_bias = 0.001f;
    slot->base.shadow_map_resolution = 1024;

    slot->point.position = position;
    slot->point.range = range;
    slot->point.constant_attenuation = 1.0f;
    slot->point.linear_attenuation = 0.09f;
    slot->point.quadratic_attenuation = 0.032f;

    RebuildActiveList();
    return id;
}

uint32_t LightSystem_AddSpotLight(Vec3 position, Vec3 direction, float range,
                                  float inner_angle, float outer_angle,
                                  Vec3 color, float intensity, bool cast_shadows) {
    if (!g_light_system.initialized) return 0;

    Light* slot = FindFreeSlot();
    if (!slot) return 0;

    uint32_t id = GetNextId();
    slot->base.id = id;
    slot->base.type = LIGHT_TYPE_SPOT;
    slot->base.enabled = true;
    slot->base.cast_shadows = cast_shadows;
    slot->base.color = color;
    slot->base.intensity = intensity;

    slot->base.shadow_bias = 0.001f;
    slot->base.shadow_normal_bias = 0.001f;
    slot->base.shadow_map_resolution = 1024;

    slot->spot.position = position;
    slot->spot.direction = vec3_normalize(direction);
    slot->spot.range = range;
    slot->spot.inner_cone_angle = inner_angle;
    slot->spot.outer_cone_angle = outer_angle;
    slot->spot.constant_attenuation = 1.0f;
    slot->spot.linear_attenuation = 0.09f;
    slot->spot.quadratic_attenuation = 0.032f;
    slot->spot.cookie_texture_id = 0;

    RebuildActiveList();
    return id;
}

void LightSystem_RemoveLight(uint32_t light_id) {
    if (!g_light_system.initialized) return;

    Light* light = FindLightById(light_id);
    if (light) {
        memset(light, 0, sizeof(Light)); // Clear slot
        RebuildActiveList();
    }
}

Light* LightSystem_GetLight(uint32_t light_id) {
    if (!g_light_system.initialized) return NULL;
    return FindLightById(light_id);
}

void LightSystem_SetLightEnabled(uint32_t light_id, bool enabled) {
    Light* light = LightSystem_GetLight(light_id);
    if (light) {
        light->base.enabled = enabled;
        RebuildActiveList();
    }
}

uint32_t LightSystem_GetActiveLightCount(void) {
    return g_light_system.active_count;
}

const Light** LightSystem_GetActiveLights(uint32_t* out_count) {
    if (!g_light_system.initialized) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    if (out_count) *out_count = g_light_system.active_count;
    return (const Light**)g_light_system.active_lights;
}
