#ifndef MATERIAL_DEFAULTS_H
#define MATERIAL_DEFAULTS_H

#include "materials/material_instance.h"
#include "include/core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create standard PBR master material (Opaque)
MaterialMaster *material_create_default_pbr(void);

// Create transparent master material (Alpha Blend)
MaterialMaster *material_create_transparent(void);

// Create emissive master material
MaterialMaster *material_create_emissive(void);

// Create foliage master material (Masked/Alpha Test)
MaterialMaster *material_create_foliage(void);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_DEFAULTS_H
