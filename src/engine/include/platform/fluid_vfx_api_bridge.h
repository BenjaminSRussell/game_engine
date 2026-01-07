// Fluid Simulation VFX API Bridge
// Exposes SPH fluid particles to VoxelForgeStudio

#ifndef FLUID_VFX_API_BRIDGE_H
#define FLUID_VFX_API_BRIDGE_H

#include "include/common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Fluid Simulation VFX API
// ============================================================================

/// Create a fluid emitter
/// @param x, y, z Position
/// @param emit_rate Particles per second
/// @return Emitter ID
uint64_t fluid_create_emitter(float x, float y, float z, float emit_rate);

/// Destroy a fluid emitter
void fluid_destroy_emitter(uint64_t emitter_id);

/// Set fluid viscosity (0.0 = water, 1.0 = honey)
void fluid_set_viscosity(uint64_t emitter_id, float viscosity);

/// Get viscosity
float fluid_get_viscosity(uint64_t emitter_id);

/// Set maximum particle count for emitter
void fluid_set_particle_count(uint64_t emitter_id, uint32_t count);

/// Get particle count
uint32_t fluid_get_particle_count(uint64_t emitter_id);

/// Enable/disable splashing effects
void fluid_enable_splashing(uint64_t emitter_id, bool enabled);

/// Check if splashing is enabled
bool fluid_is_splashing_enabled(uint64_t emitter_id);

/// Set global fluid simulation enabled
void fluid_set_enabled(bool enabled);

/// Check if fluid simulation is enabled
bool fluid_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // FLUID_VFX_API_BRIDGE_H
