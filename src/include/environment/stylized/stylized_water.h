#pragma once

#include "common.h"

typedef struct WaterConfig {
    bool enable_foam;
    float foam_depth;
    float foam_intensity;
    
    bool enable_flow_map;
    float flow_speed;
    
    bool enable_reflection;
    float reflection_opacity;
} WaterConfig;

void stylized_water_init(void);
void stylized_water_shutdown(void);
void stylized_water_apply(const WaterConfig* config);
