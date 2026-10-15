// src/engine/rendering/post_processing/taa_compute_backend.h
//
// Purpose: GPU operations that taa_compute.c issues. No compute backend
// provides these yet. tests/taa_compute_test.c defines fakes with these exact
// prototypes, so the TAA logic can be tested without a device.

#ifndef TAA_COMPUTE_BACKEND_H
#define TAA_COMPUTE_BACKEND_H

#include "core/math/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compute programs are u32 handles.
u32 shader_load_compute(const char *path);
void shader_destroy_compute(u32 program);
void shader_bind_compute(u32 program);
void shader_set_uniform_compute(u32 program, const char *name, const void *data,
                                size_t size);
void shader_dispatch_compute(u32 program, u32 x, u32 y, u32 z);
void shader_memory_barrier_compute(void);

// Textures are bound by TextureID. Slot numbers follow the shader layout.
void texture_bind_compute(TextureID texture, u32 slot);
void texture_bind_image_compute(TextureID texture, u32 slot);
void texture_clear(TextureID texture, vec4 color);

#ifdef __cplusplus
}
#endif

#endif // TAA_COMPUTE_BACKEND_H
