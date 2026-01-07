#include "environment/builder/lighting_scenario.h"
#include <include/math/math.h>
#include <string.h>

typedef struct {
    float sun_angle;
    float ambient_color[3];
    float fog_density;
} LightScenario;

static LightScenario g_scenarios[3]; // Day, Night, Custom

void lighting_scenario_set_tod(float time_01) {
    // Calculate sun position
    float theta = time_01 * 6.28318f;
    // float sun_x = cos(theta);
    // float sun_y = sin(theta);
    
    // Lerp ambient color (Day -> Sunset -> Night)
}

void lighting_scenario_generate_probes(float min[3], float max[3], float step) {
    // Grid generation for GI probes
    for (float x = min[0]; x <= max[0]; x += step) {
        for (float y = min[1]; y <= max[1]; y += step) {
            for (float z = min[2]; z <= max[2]; z += step) {
                // place_probe(x, y, z);
            }
        }
    }
}

void lighting_scenario_switch(int scenario_id, float transition_time) {
    // Blend from current settings to target settings over transition_time
    // target = g_scenarios[scenario_id];
}
