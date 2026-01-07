/*
 * metallic_roughness.h
 * Metallic-roughness workflow
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_METALLIC_ROUGHNESS_H
#define MATERIALS_METALLIC_ROUGHNESS_H

#include "pbr_types.h"
#include <Metal/Metal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a new metallic-roughness material.
 * @param dev The Metal device to use for creating buffers.
 * @param name Debug name for the material.
 * @return A pointer to the new material, or NULL on failure.
 */
material_t* material_create(metal_device_t* dev, const char* name);

/**
 * Destroys a material instance.
 * @param mat The material to destroy.
 */
void material_destroy(material_t* mat);

/**
 * Updates the GPU buffer with current parameters.
 * Should be called after modifying material parameters.
 * @param mat The material to update.
 */
void material_update_gpu(material_t* mat);

/**
 * Binds the material to the given command encoder.
 * @param mat The material to bind.
 * @param encoder The render command encoder.
 * @param defaults Default textures to use for missing slots.
 */
void material_bind(material_t* mat, id<MTLRenderCommandEncoder> encoder, material_defaults_t* defaults);

/**
 * Helper to create default 1x1 textures.
 */
void material_defaults_init(material_defaults_t* defaults, metal_device_t* dev);

/**
 * Helper to release default textures.
 */
void material_defaults_cleanup(material_defaults_t* defaults);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_METALLIC_ROUGHNESS_H */
