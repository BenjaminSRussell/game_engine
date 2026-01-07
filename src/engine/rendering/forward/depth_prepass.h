/*
 * depth_prepass.h
 * Depth prepass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DEPTH_PREPASS_H
#define DEPTH_PREPASS_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct static_mesh_draw_info static_mesh_draw_info_t;

/**
 * @brief Initialize the depth prepass
 * @return 0 on success
 */
int rendering_depth_prepass_init(void);

/**
 * @brief Shutdown the depth prepass
 */
void rendering_depth_prepass_shutdown(void);

/**
 * @brief Submit a mesh for the depth prepass
 * @param info Draw information
 */
void rendering_depth_prepass_submit(const static_mesh_draw_info_t* info);

/**
 * @brief Execute the depth prepass
 * @param cmd_buffer Command buffer
 * @param camera_pos Camera position (only needed if logic changes, usually depth buffer handles sorting implicitly but front-to-back is still better)
 */
void rendering_depth_prepass_execute(void* cmd_buffer, const float camera_pos[3]);

#endif /* DEPTH_PREPASS_H */
