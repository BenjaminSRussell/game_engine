/*
 * resident_mips.h
 * Resident mip tracking
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_RESIDENT_MIPS_H
#define TEXTURE_RESIDENT_MIPS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Residency Management */
int texture_residency_set_mip_status(uint32_t handle, uint32_t mip, bool resident);
int texture_residency_get_highest_resident_mip(uint32_t handle);
bool texture_residency_is_mip_resident(uint32_t handle, uint32_t mip);

/* Memory Tracking */
size_t texture_residency_get_mip_memory_size(uint32_t handle, uint32_t mip);
size_t texture_residency_get_total_memory_usage(void);

/* Batch Operations */
void texture_residency_update_all(void);

/* Original stub compatibility */
int texture_resident_mips_init(void);
void texture_resident_mips_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_RESIDENT_MIPS_H */

