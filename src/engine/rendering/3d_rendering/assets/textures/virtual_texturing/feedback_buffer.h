/*
 * feedback_buffer.h
 * GPU feedback buffer management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FEEDBACK_BUFFER_H
#define TEXTURE_FEEDBACK_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct feedback_request {
    uint32_t texture_handle;
    uint16_t x, y;
    uint8_t mip;
    uint8_t flags;
} feedback_request_t;

typedef struct feedback_buffer {
    uint32_t gpu_buffer_handle;
    uint32_t resolve_target_handle;
    uint32_t buffer_size;
    bool active;
} feedback_buffer_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int feedback_buffer_init(feedback_buffer_t* fb, uint32_t width, uint32_t height);
void feedback_buffer_shutdown(feedback_buffer_t* fb);

/* Operations */
void feedback_buffer_begin(feedback_buffer_t* fb);
void feedback_buffer_end(feedback_buffer_t* fb);
int feedback_buffer_read(feedback_buffer_t* fb, void* out_data, uint32_t* out_count);

/* Original stub compatibility */
int texture_feedback_buffer_init(void);
void texture_feedback_buffer_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FEEDBACK_BUFFER_H */

