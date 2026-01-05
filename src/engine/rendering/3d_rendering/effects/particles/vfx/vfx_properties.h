/*
 * vfx_properties.h
 * VFX Graph Property System
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_PROPERTIES_H
#define EFFECTS_VFX_PROPERTIES_H

#include <stdint.h>
#include "../../math/vec3.h"
#include "../../math/vec4.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vfx_property_type {
    VFX_PROPERTY_FLOAT,
    VFX_PROPERTY_INT,
    VFX_PROPERTY_VEC3,
    VFX_PROPERTY_VEC4,
    VFX_PROPERTY_CURVE,
    VFX_PROPERTY_GRADIENT
} vfx_property_type_t;

typedef struct vfx_property {
    uint32_t id;
    vfx_property_type_t type;
    union {
        float f;
        int i;
        vec3_t v3;
        vec4_t v4;
        // Curve/Gradient handle
    } value;
} vfx_property_t;

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_PROPERTIES_H */
