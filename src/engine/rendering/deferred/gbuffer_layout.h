/*
 * gbuffer_layout.h
 * G-buffer layout API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GBUFFER_LAYOUT_H
#define GBUFFER_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize/resize the G-buffer
 * @param width Screen width
 * @param height Screen height
 * @return 0 on success
 */
int rendering_gbuffer_init(uint32_t width, uint32_t height);

/**
 * @brief Shutdown G-buffer system
 */
void rendering_gbuffer_shutdown(void);

/**
 * @brief Get G-buffer render targets
 * @param albedo Output albedo texture handle (can be NULL)
 * @param normal Output normal texture handle (can be NULL)
 * @param material Output material texture handle (can be NULL)
 * @param depth Output depth texture handle (can be NULL)
 */
void rendering_gbuffer_get_targets(void** albedo, void** normal, void** material, void** depth);

/**
 * @brief Get the G-buffer framebuffer handle
 * @return Framebuffer handle (void*)
 */
void* rendering_gbuffer_get_framebuffer(void);

#endif /* GBUFFER_LAYOUT_H */
