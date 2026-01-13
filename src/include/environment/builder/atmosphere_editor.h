#pragma once

#include "engine/include/common.h"
#include "math/vec3.h"

typedef struct AtmosphereConfig {
    float sun_intensity;
    Vec3 sun_direction;
    
    float rayleigh_scattering;
    float mie_scattering;
    float mie_absorption;
    
    bool enable_volumetric_clouds;
    float cloud_density;
    float cloud_height;
    
    bool enable_height_fog;
    float fog_density;
    float fog_height_falloff;
} AtmosphereConfig;

void atmosphere_editor_init(void);
void atmosphere_editor_shutdown(void);
void atmosphere_apply(const AtmosphereConfig* config);
