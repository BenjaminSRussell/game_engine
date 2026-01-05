/*
 * cloth_simulation.h
 * Cloth Physics Simulation
 *
 * Part of the Physics/Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SIMULATION_H
#define CLOTH_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

int cloth_simulation_init(void);
void cloth_simulation_shutdown(void);
void cloth_simulation_update(float dt);

/* Object Management */
// uint32_t cloth_create(...);
// void cloth_destroy(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SIMULATION_H */
