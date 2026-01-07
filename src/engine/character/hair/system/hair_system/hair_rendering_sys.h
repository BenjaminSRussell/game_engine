/*
 * hair_rendering_sys.h
 * Hair Rendering System
 */

#ifndef HAIR_RENDERING_SYS_H
#define HAIR_RENDERING_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

int hair_rendering_sys_init(void);
void hair_rendering_sys_shutdown(void);
void hair_rendering_sys_draw(void* cmd_buffer, void* view_data);

#ifdef __cplusplus
}
#endif

#endif /* HAIR_RENDERING_SYS_H */
