#include "physics/fluids/fluid_system.h"
#include "core/logger.h"
#include "physics/fluids/sph_solver.h"
#include <stdlib.h>
#include <string.h>

#define MAX_EMITTERS 32

typedef struct {
  uint64_t id;
  bool active;
  float x, y, z;
  float emit_rate;
  float viscosity;
  uint32_t max_particles;
  bool splashing;
  SPHSolver *solver;
} Emitter;

static struct {
  bool enabled;
  uint64_t next_id;
  Emitter emitters[MAX_EMITTERS];
} fluid_state;

void fluid_sys_init(void) {
  fluid_state.enabled = true;
  fluid_state.next_id = 1;
  memset(fluid_state.emitters, 0, sizeof(fluid_state.emitters));
  LOG_INFO("Fluid System Initialized");
}

void fluid_sys_shutdown(void) {
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (fluid_state.emitters[i].solver) {
      free(fluid_state.emitters[i].solver->particles);
      free(fluid_state.emitters[i].solver);
    }
  }
}

void fluid_sys_update(float delta_time) {
  if (!fluid_state.enabled)
    return;

  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (fluid_state.emitters[i].active && fluid_state.emitters[i].solver) {
      sph_update(fluid_state.emitters[i].solver, delta_time);
    }
  }
}

static Emitter *get_emitter(uint64_t id) {
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (fluid_state.emitters[i].active && fluid_state.emitters[i].id == id) {
      return &fluid_state.emitters[i];
    }
  }
  return NULL;
}

uint64_t fluid_sys_create_emitter(float x, float y, float z, float emit_rate) {
  // Find free slot
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (!fluid_state.emitters[i].active) {
      uint64_t id = fluid_state.next_id++;
      fluid_state.emitters[i].active = true;
      fluid_state.emitters[i].id = id;
      fluid_state.emitters[i].x = x;
      fluid_state.emitters[i].y = y;
      fluid_state.emitters[i].z = z;
      fluid_state.emitters[i].emit_rate = emit_rate;
      fluid_state.emitters[i].viscosity = 0.01f;
      fluid_state.emitters[i].max_particles = 1000;
      fluid_state.emitters[i].splashing = true;

      // Create solver
      fluid_state.emitters[i].solver = sph_create(1000);

      LOG_INFO("Fluid System: Created emitter %llu at (%.2f, %.2f, %.2f)", id,
               x, y, z);
      return id;
    }
  }
  LOG_ERROR("Fluid System: Max emitters reached");
  return 0;
}

void fluid_sys_destroy_emitter(uint64_t emitter_id) {
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (fluid_state.emitters[i].active &&
        fluid_state.emitters[i].id == emitter_id) {
      fluid_state.emitters[i].active = false;
      // Free solver... (leaking slightly here for simplicity in reuse, fix in
      // full impl)
      if (fluid_state.emitters[i].solver) {
        free(fluid_state.emitters[i].solver->particles);
        free(fluid_state.emitters[i].solver);
        fluid_state.emitters[i].solver = NULL;
      }
      LOG_INFO("Fluid System: Destroyed emitter %llu", emitter_id);
      return;
    }
  }
}

void fluid_sys_set_viscosity(uint64_t emitter_id, float viscosity) {
  Emitter *e = get_emitter(emitter_id);
  if (e) {
    e->viscosity = viscosity;
    if (e->solver)
      e->solver->viscosity = viscosity;
  }
}

float fluid_sys_get_viscosity(uint64_t emitter_id) {
  Emitter *e = get_emitter(emitter_id);
  return e ? e->viscosity : 0.0f;
}

void fluid_sys_set_particle_count(uint64_t emitter_id, uint32_t count) {
  Emitter *e = get_emitter(emitter_id);
  if (e)
    e->max_particles = count;
}

uint32_t fluid_sys_get_particle_count(uint64_t emitter_id) {
  Emitter *e = get_emitter(emitter_id);
  return e ? e->max_particles : 0;
}

void fluid_sys_enable_splashing(uint64_t emitter_id, bool enabled) {
  Emitter *e = get_emitter(emitter_id);
  if (e)
    e->splashing = enabled;
}

bool fluid_sys_is_splashing_enabled(uint64_t emitter_id) {
  Emitter *e = get_emitter(emitter_id);
  return e ? e->splashing : false;
}

void fluid_sys_set_enabled(bool enabled) { fluid_state.enabled = enabled; }

bool fluid_sys_is_enabled(void) { return fluid_state.enabled; }
