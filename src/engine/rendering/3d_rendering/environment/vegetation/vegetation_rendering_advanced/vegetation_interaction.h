/*
 * vegetation_interaction.h
 * Vegetation Interaction System
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VEGETATION_INTERACTION_H
#define VEGETATION_INTERACTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int vegetation_interaction_init(void);
void vegetation_interaction_shutdown(void);

/* Operations */
uint32_t vegetation_interaction_add(float x, float y, float z, float radius);
void vegetation_interaction_remove(uint32_t id);
void vegetation_interaction_update(uint32_t id, float x, float y, float z);
void vegetation_interaction_render_map(void);

#ifdef __cplusplus
}
#endif

#endif /* VEGETATION_INTERACTION_H */
