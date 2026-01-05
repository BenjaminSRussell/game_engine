/*
 * deferred_lighting.h
 * Deferred lighting pass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DEFERRED_LIGHTING_H
#define DEFERRED_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the deferred lighting pass
 * @return 0 on success
 */
int rendering_deferred_lighting_init(void);

/**
 * @brief Shutdown the deferred lighting pass
 */
void rendering_deferred_lighting_shutdown(void);

/**
 * @brief Execute the deferred lighting pass
 * @param cmd_buffer Command buffer to record commands to
 * @param light_data Pointer to light data (UBO/SSBO handle)
 */
void rendering_deferred_lighting_execute(void* cmd_buffer, void* light_data);

#endif /* DEFERRED_LIGHTING_H */
