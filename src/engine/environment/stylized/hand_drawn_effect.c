#include "environment/stylized/hand_drawn_effect.h"
#include <stddef.h>

static int g_hand_drawn_ubo = 0;

void hand_drawn_init(void) {
    // Create Uniform Buffer Object for shader params
    // glGenBuffers(1, &g_hand_drawn_ubo);
}

void hand_drawn_shutdown(void) {
    // glDeleteBuffers(1, &g_hand_drawn_ubo);
}

void hand_drawn_apply(const HandDrawnConfig* config) {
    if (!config) return;
    // Update UBO data
    // glBindBuffer(GL_UNIFORM_BUFFER, g_hand_drawn_ubo);
    // glBufferSubData(..., config);
    
    // Bind shader techniques
    // use_program(HAND_DRAWN_SHADER);
}
