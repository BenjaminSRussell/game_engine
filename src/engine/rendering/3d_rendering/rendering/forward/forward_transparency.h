/*
 * forward_transparency.h
 * Forward transparency pass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef FORWARD_TRANSPARENCY_H
#define FORWARD_TRANSPARENCY_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct static_mesh_draw_info static_mesh_draw_info_t;

/**
 * @brief Initialize the transparency pass
 * @return 0 on success
 */
int rendering_forward_transparency_init(void);

/**
 * @brief Shutdown the transparency pass
 */
void rendering_forward_transparency_shutdown(void);

/**
 * @brief Submit a transparent mesh to be drawn
 * @param info Draw information
 */
void rendering_forward_transparency_submit(const static_mesh_draw_info_t* info);

/**
 * @brief Execute the transparency pass
 * @param cmd_buffer Command buffer
 * @param camera_pos Camera position for back-to-front sorting
 */
void rendering_forward_transparency_execute(void* cmd_buffer, const float camera_pos[3]);

#endif /* FORWARD_TRANSPARENCY_H */
