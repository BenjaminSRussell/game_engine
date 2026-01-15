// PBR Material Template Library
// Standard materials for common use cases

#ifndef MATERIAL_TEMPLATES_H
#define MATERIAL_TEMPLATES_H

#include "rendering/materials/material_instance.h"

#ifdef __cplusplus
extern "C" {
#endif

// === Standard PBR Master Materials ===

// M_PBR_Standard - Full-featured PBR material
MaterialMaster *material_template_pbr_standard(void);

// M_Unlit - No lighting, perfect for UI/effects
MaterialMaster *material_template_unlit(void);

// M_Transparent - Alpha blending support
MaterialMaster *material_template_transparent(void);

// M_TwoSided - Disable backface culling
MaterialMaster *material_template_two_sided(void);

// M_Foliage - Wind animation + subsurface scattering
MaterialMaster *material_template_foliage(void);

// M_Water - Refraction, reflection, flow
MaterialMaster *material_template_water(void);

// === Instance Creation Helpers ===

// Create common material instances
MaterialInstance *material_create_grass(MaterialMaster *pbr);
MaterialInstance *material_create_metal(MaterialMaster *pbr);
MaterialInstance *material_create_wood(MaterialMaster *pbr);
MaterialInstance *material_create_stone(MaterialMaster *pbr);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_TEMPLATES_H
