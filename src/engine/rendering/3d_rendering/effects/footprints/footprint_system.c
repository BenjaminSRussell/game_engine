#include "footprint_system.h"
#include <stdlib.h>
#include <string.h>

typedef struct footprint_entry {
    float x, z;
    float life;
    uint32_t type;
} footprint_entry_t;

static footprint_entry_t* g_footprints = NULL;
static uint32_t g_footprint_count = 0;

int effects_footprint_init(void) {
    g_footprints = calloc(1024, sizeof(footprint_entry_t));
    return g_footprints ? 0 : -1;
}

void effects_footprint_shutdown(void) {
    free(g_footprints);
    g_footprints = NULL;
}

void effects_footprint_apply(float x, float z, uint32_t type) {
    uint32_t id = g_footprint_count % 1024;
    g_footprints[id].x = x;
    g_footprints[id].z = z;
    g_footprints[id].life = 1.0f;
    g_footprints[id].type = type;
    g_footprint_count++;
}

void effects_footprint_update(float dt) {
    for (uint32_t i = 0; i < 1024; i++) {
        if (g_footprints[i].life > 0.0f) {
            g_footprints[i].life -= dt * 0.1f;
        }
    }
}
