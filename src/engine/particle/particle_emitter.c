#include "particle/particle_emitter.h"
#include <stdlib.h>

void particle_emitter_init() {}

void *particle_emitter_create() {
    return NULL;
}

void particle_emitter_emit(void *emitter, int count) {}

void particle_emitter_update(void *emitter, float dt) {
    // Update particle positions, velocities, lifetimes
}

void particle_emitter_render(void *emitter, void *camera) {}

void particle_emitter_set_rate(void *emitter, float particles_per_second) {}

void particle_emitter_destroy(void *emitter) {}
