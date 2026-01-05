/*
 * gbuffer_pass.h
 * G-buffer geometry pass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GBUFFER_PASS_H
#define GBUFFER_PASS_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct static_mesh_draw_info static_mesh_draw_info_t;

/**
 * @brief Initialize the G-buffer pass
 * @return 0 on success
 */
int rendering_gbuffer_pass_init(void);

/**
 * @brief Shutdown the G-buffer pass
 */
void rendering_gbuffer_pass_shutdown(void);

/**
 * @brief Submit a mesh to be drawn in the G-buffer pass
 * @param info Draw information
 */
void rendering_gbuffer_pass_submit(const static_mesh_draw_info_t* info);

/**
 * @brief Execute the G-buffer pass
 * @param cmd_buffer Command buffer to record commands to
 * @param camera_pos Camera position for sorting (front-to-back)
 */
void rendering_gbuffer_pass_execute(void* cmd_buffer, const float camera_pos[3]);

#endif /* GBUFFER_PASS_H */
