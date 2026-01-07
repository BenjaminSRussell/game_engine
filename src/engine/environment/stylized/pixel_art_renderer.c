#include "environment/stylized/pixel_art_renderer.h"

void pixel_art_renderer_apply(void *scene_texture, void *output_texture, int scale_factor) {
    // 1. Render scene to low-res framebuffer (Target Width / scale_factor)
    // glBindFramebuffer(GL_FRAMEBUFFER, low_res_fbo);
    // render_screen_quad(scene_texture);
    
    // 2. Blit optional upscale to output
    // glBlitFramebuffer(... GL_NEAREST ...);
}

void pixel_art_camera_snap(float *cam_pos, float *cam_rot, int pixels_per_unit) {
    // Snaps camera position to sub-pixel grid to prevent jitter
    float snap = 1.0f / (float)pixels_per_unit;
    cam_pos[0] = (int)(cam_pos[0] / snap) * snap;
    cam_pos[1] = (int)(cam_pos[1] / snap) * snap;
    cam_pos[2] = (int)(cam_pos[2] / snap) * snap;
    // Orthographic projection setup...
}
