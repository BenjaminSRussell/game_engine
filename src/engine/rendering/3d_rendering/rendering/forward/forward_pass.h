/*
 * forward_pass.h
 * Forward rendering pass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef FORWARD_PASS_H
#define FORWARD_PASS_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct static_mesh_draw_info static_mesh_draw_info_t;

/**
 * @brief Initialize the forward rendering pass
 * @return 0 on success
 */
int rendering_forward_pass_init(void);

/**
 * @brief Shutdown the forward rendering pass
 */
void rendering_forward_pass_shutdown(void);

/**
 * @brief Submit a mesh to be drawn in the forward pass
 * @param info Draw information
 */
void rendering_forward_pass_submit_mesh(const static_mesh_draw_info_t* info);

/**
 * @brief Execute the forward render pass
 * @param cmd_buffer Command buffer to record commands to
 * @param camera_pos Camera position for sorting
 */
void rendering_forward_pass_execute(void* cmd_buffer, const float camera_pos[3]);

#endif /* FORWARD_PASS_H */
