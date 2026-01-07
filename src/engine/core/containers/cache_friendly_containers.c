#include "core/containers/cache_friendly_containers.h"
#include <stdlib.h>

// Structure of Arrays (SoA) pattern for cache-friendly access
typedef struct EntitySoA {
    float *positions_x;
    float *positions_y;
    float *positions_z;
    float *velocities_x;
    float *velocities_y;
    float *velocities_z;
    size_t count;
    size_t capacity;
} EntitySoA;

EntitySoA* entity_soa_create(size_t capacity) {
    EntitySoA *soa = (EntitySoA*)malloc(sizeof(EntitySoA));
    soa->positions_x = (float*)malloc(capacity * sizeof(float));
    soa->positions_y = (float*)malloc(capacity * sizeof(float));
    soa->positions_z = (float*)malloc(capacity * sizeof(float));
    soa->velocities_x = (float*)malloc(capacity * sizeof(float));
    soa->velocities_y = (float*)malloc(capacity * sizeof(float));
    soa->velocities_z = (float*)malloc(capacity * sizeof(float));
    soa->count = 0;
    soa->capacity = capacity;
    return soa;
}

void entity_soa_destroy(EntitySoA *soa) {
    if (soa) {
        free(soa->positions_x);
        free(soa->positions_y);
        free(soa->positions_z);
        free(soa->velocities_x);
        free(soa->velocities_y);
        free(soa->velocities_z);
        free(soa);
    }
}

void entity_soa_update(EntitySoA *soa, float dt) {
    // Cache-friendly update loop
    for (size_t i = 0; i < soa->count; i++) {
        soa->positions_x[i] += soa->velocities_x[i] * dt;
        soa->positions_y[i] += soa->velocities_y[i] * dt;
        soa->positions_z[i] += soa->velocities_z[i] * dt;
    }
}
