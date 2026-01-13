#pragma once

#include "engine/include/common.h"

typedef struct HandDrawnConfig {
    bool enable_wobble;
    float wobble_speed;
    float wobble_intensity;
    
    bool enable_paper_texture;
    float paper_intensity;
} HandDrawnConfig;

void hand_drawn_init(void);
void hand_drawn_shutdown(void);
void hand_drawn_apply(const HandDrawnConfig* config);
