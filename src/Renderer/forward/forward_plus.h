/*
 * forward_plus.h
 * Forward+ rendering API
 * 
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef FORWARD_PLUS_H
#define FORWARD_PLUS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize Forward+ lighting system
 * @param width Screen width
 * @param height Screen height
 * @return 0 on success
 */
int rendering_forward_plus_init(uint32_t width, uint32_t height);

/**
 * @brief Shutdown Forward+ lighting system
 */
void rendering_forward_plus_shutdown(void);

/**
 * @brief Perform light culling compute pass
 * @param cmd_buffer Command buffer
 * @param camera_data Camera constant buffer/descriptor
 * @param light_data Light buffer/descriptor
 */
void rendering_forward_plus_cull_lights(void* cmd_buffer, void* camera_data, void* light_data);

/**
 * @brief Get computed grid dimensions
 */
void rendering_forward_plus_get_grid_dims(uint32_t* width, uint32_t* height);

#endif /* FORWARD_PLUS_H */
