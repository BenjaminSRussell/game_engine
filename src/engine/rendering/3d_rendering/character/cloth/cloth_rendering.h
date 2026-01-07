/*
 * cloth_rendering.h
 * Cloth Rendering System
 *
 * Part of the Physics/Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_RENDERING_H
#define CLOTH_RENDERING_H

#ifdef __cplusplus
extern "C" {
#endif

int cloth_rendering_init(void);
void cloth_rendering_shutdown(void);
void cloth_rendering_draw(void* cmd_buffer, void* camera);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_RENDERING_H */
