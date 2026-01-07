/*
 * descriptor_set_layout.h
 * Descriptor set layout creation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_DESCRIPTOR_SET_LAYOUT_H
#define CORE_DESCRIPTOR_SET_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    uint32_t id;
} descriptor_set_layout_handle_t;

typedef enum {
    DESCRIPTOR_TYPE_SAMPLER = 0,
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10
} descriptor_type_t;

typedef enum {
    SHADER_STAGE_VERTEX_BIT = 0x00000001,
    SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
    SHADER_STAGE_COMPUTE_BIT = 0x00000020,
    SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
    SHADER_STAGE_ALL = 0x7FFFFFFF
} shader_stage_flags_t;

typedef struct {
    uint32_t binding;
    descriptor_type_t descriptor_type;
    uint32_t descriptor_count;
    shader_stage_flags_t stage_flags;
    const void* p_immutable_samplers; // Optional
} descriptor_binding_t;

typedef struct {
    const descriptor_binding_t* bindings;
    uint32_t binding_count;
} descriptor_layout_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
void descriptor_set_layout_init_system(void);
void descriptor_set_layout_shutdown_system(void);

/* layout creation (cached) */
descriptor_set_layout_handle_t descriptor_set_layout_get(const descriptor_layout_info_t* info);

/* Lifecycle - primarily handled by system shutdown, but specific destroy available */
void descriptor_set_layout_destroy(descriptor_set_layout_handle_t handle);

/* Access */
bool descriptor_set_layout_is_valid(descriptor_set_layout_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DESCRIPTOR_SET_LAYOUT_H */
