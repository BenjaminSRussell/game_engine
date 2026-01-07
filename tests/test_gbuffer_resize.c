/**
 * =================================================================================================
 *                          TEST: G-BUFFER RESIZE
 * =================================================================================================
 */

#include <engine/rendering/gbuffer/gbuffer.h>
#include <core/logger/logger.h>
#include <gpu_backend/framebuffer.h>

int main() {
    LOG_INFO("Starting G-Buffer Resize Test...");
    
    u32 initial_w = 1920;
    u32 initial_h = 1080;
    
    // 1. Create G-Buffer
    GBuffer *gbuffer = gbuffer_create(initial_w, initial_h);
    if (!gbuffer) {
        LOG_ERROR("Failed to create G-Buffer");
        return 1;
    }
    
    if (gbuffer->width != initial_w || gbuffer->height != initial_h) {
        LOG_ERROR("G-Buffer dimensions incorrect after creation");
        return 1;
    }
    
    // Verify Framebuffer exists
    if (!gbuffer->framebuffer) {
        LOG_ERROR("G-Buffer has no framebuffer");
        return 1;
    }
    
    if (framebuffer_get_width(gbuffer->framebuffer) != initial_w) {
        LOG_ERROR("Framebuffer width incorrect");
        return 1;
    }

    // 2. Resize G-Buffer (Downscale)
    u32 new_w = 1280;
    u32 new_h = 720;
    LOG_INFO("Resizing to %dx%d...", new_w, new_h);
    
    gbuffer_resize(gbuffer, new_w, new_h);
    
    if (gbuffer->width != new_w || gbuffer->height != new_h) {
        LOG_ERROR("G-Buffer dimensions incorrect after resize");
        return 1;
    }
    
    if (framebuffer_get_width(gbuffer->framebuffer) != new_w) {
         LOG_ERROR("Framebuffer width incorrect after resize");
         return 1;
    }
    
    // 3. Resize G-Buffer (Same size - optimization check)
    // This assumes implementation prints a log or we can check logic
    gbuffer_resize(gbuffer, new_w, new_h); 
    
    // 4. Cleanup
    gbuffer_destroy(gbuffer);
    
    LOG_INFO("SUCCESS: G-Buffer resize passed.");
    return 0;
}
