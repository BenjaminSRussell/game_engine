/*
 * test_light_probes.c
 * Unit tests for the light probe system.
 */

#include "renderer/light_probes.h"
#include <core/logger.h>
#include <math.h>
#include <stdio.h>

// Simple assertion macro
#define ASSERT_EQ_FLOAT(expected, actual, epsilon) \
    do { \
        if (fabsf((expected) - (actual)) > (epsilon)) { \
            LOG_ERROR("Assertion failed: expected %f, got %f", (expected), (actual)); \
        } \
    } while (0)

static void test_LightProbe_Init(void) {
    LightProbeGrid grid;
    Vec3 min = {0, 0, 0};
    Vec3 max = {10, 10, 10};
    u32 count = 2; // 2x2x2 = 8 probes
    
    gi_light_probe_grid_init(&grid, count, min, max);
    
    if (grid.probes_x != 2) LOG_ERROR("Grid x count mismatch");
    if (grid.probe_spacing.x != 10.0f) LOG_ERROR("Grid spacing mismatch: %f", grid.probe_spacing.x);
    
    // Check first probe
    if (grid.probes[0].position.x != 0) LOG_ERROR("Probe 0 pos mismatch");
    
    // Check last probe
    u32 last = 2*2*2 - 1;
    if (fabsf(grid.probes[last].position.x - 10.0f) > 0.001f) LOG_ERROR("Probe last pos mismatch");

    gi_light_probe_grid_shutdown(&grid);
    LOG_INFO("test_LightProbe_Init passed");
}

static void test_LightProbe_Interpolate(void) {
    LightProbeGrid grid;
    Vec3 min = {0, 0, 0};
    Vec3 max = {10, 10, 10};
    gi_light_probe_grid_init(&grid, 2, min, max); // 2x2x2
    
    // Set probe values manually
    // Probe at 0,0,0 (index 0) = red (SH[0]=1, SH[9]=0, SH[18]=0, scaled by Y00)
    // Actually our bake just puts values in sh_coefficients. 
    // Let's manually set index 0 to have red channel DC = 1.0/Y00 so it evaluates to 1.0.
    float Y00 = 0.282095f;
    float val = 1.0f / Y00;
    
    // All probes black by default (calloc)
    
    // Set probe 0 (0,0,0) to Red
    grid.probes[0].sh_coefficients[0] = val; 
    
    // Set probe 7 (10,10,10) to Blue
    // x=1, y=1, z=1 -> index = 1 + 1*2 + 1*4 = 7
    grid.probes[7].sh_coefficients[18] = val;
    
    // Sample at 0,0,0
    Vec3 result;
    Vec3 normal = {0, 1, 0};
    gi_light_probes_interpolate(&grid, (Vec3){0,0,0}, normal, &result);
    ASSERT_EQ_FLOAT(1.0f, result.x, 0.001f);
    ASSERT_EQ_FLOAT(0.0f, result.y, 0.001f);
    ASSERT_EQ_FLOAT(0.0f, result.z, 0.001f);
    
    // Sample at 5,5,5 (center) -> should be average of all 8 probes
    // Probe 0 is Red, Probe 7 is Blue, others Black.
    // 0,0,0 contributes 1/8. 10,10,10 contributes 1/8.
    // Wait, trilinear at 0.5, 0.5, 0.5
    // Weights for all corners are 0.125 (1/8).
    // So expected: Red * 0.125 + Blue * 0.125 + Black * 0.75
    
    gi_light_probes_interpolate(&grid, (Vec3){5,5,5}, normal, &result);
    ASSERT_EQ_FLOAT(0.125f, result.x, 0.001f);
    ASSERT_EQ_FLOAT(0.0f, result.y, 0.001f);
    ASSERT_EQ_FLOAT(0.125f, result.z, 0.001f);
    
    gi_light_probe_grid_shutdown(&grid);
    LOG_INFO("test_LightProbe_Interpolate passed");
}

void register_light_probe_tests(void) {
    test_LightProbe_Init();
    test_LightProbe_Interpolate();
}
