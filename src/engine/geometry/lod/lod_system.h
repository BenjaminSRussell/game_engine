/*
 * lod_system.h
 * LOD system management and selection
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_SYSTEM_H
#define GEOMETRY_LOD_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "geometry/geometry_types.h"
#include "geometry/lod/lod_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SYSTEM MANAGEMENT
 * ============================================================================ */

/* Initialization and shutdown */
int lod_system_init(void);
void lod_system_shutdown(void);

/* Mesh registration */
uint32_t lod_system_register_mesh(const mesh_t* mesh, const char* name);
bool lod_system_unregister_mesh(uint32_t mesh_id);

/* LOD generation */
bool lod_system_generate_lods(uint32_t mesh_id, const lod_generation_config_t* config);
bool lod_system_generate_lods_simple(uint32_t mesh_id, const float* ratios, uint32_t count);

/* LOD selection */
mesh_t* lod_system_select_lod(uint32_t mesh_id, float screen_size);
uint32_t lod_system_get_current_lod(uint32_t mesh_id);

/* LOD chain management */
bool lod_system_set_lod_chain(uint32_t mesh_id, lod_chain_t* chain);
lod_chain_t* lod_system_get_lod_chain(uint32_t mesh_id);

/* ============================================================================
 * GLOBAL SETTINGS
 * ============================================================================ */

/* Hysteresis prevents rapid LOD switching */
void lod_system_set_global_hysteresis(float hysteresis);

/* Bias adjusts LOD selection globally */
void lod_system_set_global_bias(float bias);

/* Force specific LOD level for debugging */
void lod_system_force_lod_level(uint32_t level);
void lod_system_disable_force_lod(void);

/* ============================================================================
 * STATISTICS AND DEBUGGING
 * ============================================================================ */

/* Get system statistics */
void lod_system_get_statistics(uint32_t* total_meshes, uint32_t* meshes_with_lods, uint32_t* total_lod_switches);

/* Print statistics to console */
void lod_system_print_statistics(void);

/* Print detailed information about a specific mesh */
void lod_system_print_mesh_info(uint32_t mesh_id);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_SYSTEM_H */
