#ifndef LIGHT_PROBES_H
#define LIGHT_PROBES_H

#include "engine/include/common.h"
#include <math/vec3.h>

// Light probe for SH coefficients
typedef struct {
    Vec3 position;
    // SH coefficients (3 bands = 9 coefficients per RGB channel)
    f32 sh_coefficients[9 * 3];  // RGB, each with 9 SH coefficients
    f32 distance_field[16];      // Distance samples for AO/Visibility
} LightProbe;

// Light probe grid
typedef struct {
    LightProbe* probes;
    u32 probes_x, probes_y, probes_z;

    Vec3 grid_min, grid_max;
    Vec3 probe_spacing;
} LightProbeGrid;

// Light probe configuration
typedef struct {
    u32 probes_per_axis;        // Probes along each axis (8-32)
    Vec3 probe_grid_min;        // Minimum corner of probe grid
    Vec3 probe_grid_max;        // Maximum corner of probe grid
    bool use_irradiance_probes;
    bool use_distance_probes;
} LightProbeConfig;

// ============================================================================
// Light Probes API
// ============================================================================

/// Initialize light probe grid
void gi_light_probe_grid_init(LightProbeGrid* grid, u32 probes_per_axis,
                             Vec3 grid_min, Vec3 grid_max);

/// Shutdown light probe grid
void gi_light_probe_grid_shutdown(LightProbeGrid* grid);

/// Bake irradiance into light probes
/// Computes and stores irradiance at each probe location
/// Args:
///   grid: Probe grid to bake
///   samples_per_probe: Number of ray samples per probe
void gi_light_probes_bake(LightProbeGrid* grid, u32 samples_per_probe);

/// Interpolate light from probes
/// Uses trilinear interpolation of nearby probes
/// Args:
///   grid: Probe grid
///   position: Position to query
///   normal: Surface normal
///   out_irradiance: Interpolated irradiance
void gi_light_probes_interpolate(const LightProbeGrid* grid,
                               Vec3 position, Vec3 normal,
                               Vec3* out_irradiance);

#endif // LIGHT_PROBES_H
