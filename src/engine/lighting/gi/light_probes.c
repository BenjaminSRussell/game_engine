#include "rendering/light_probes.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// ============================================================================
// Light Probes
// ============================================================================

void gi_light_probe_grid_init(LightProbeGrid* grid, u32 probes_per_axis,
                             Vec3 grid_min, Vec3 grid_max) {
    if (!grid) return;

    grid->probes_x = probes_per_axis;
    grid->probes_y = probes_per_axis;
    grid->probes_z = probes_per_axis;

    u32 total_probes = probes_per_axis * probes_per_axis * probes_per_axis;
    grid->probes = (LightProbe*)calloc(total_probes, sizeof(LightProbe));

    grid->grid_min = grid_min;
    grid->grid_max = grid_max;

    // Avoid division by zero
    if (probes_per_axis > 1) {
        grid->probe_spacing = (Vec3){
            (grid_max.x - grid_min.x) / (f32)(probes_per_axis - 1),
            (grid_max.y - grid_min.y) / (f32)(probes_per_axis - 1),
            (grid_max.z - grid_min.z) / (f32)(probes_per_axis - 1)
        };
    } else {
        grid->probe_spacing = (Vec3){0, 0, 0};
    }

    // Initialize probe positions
    for (u32 x = 0; x < probes_per_axis; x++) {
        for (u32 y = 0; y < probes_per_axis; y++) {
            for (u32 z = 0; z < probes_per_axis; z++) {
                u32 idx = x + y * probes_per_axis + z * probes_per_axis * probes_per_axis;
                grid->probes[idx].position = (Vec3){
                    grid_min.x + x * grid->probe_spacing.x,
                    grid_min.y + y * grid->probe_spacing.y,
                    grid_min.z + z * grid->probe_spacing.z
                };
            }
        }
    }
}

void gi_light_probe_grid_shutdown(LightProbeGrid* grid) {
    if (!grid || !grid->probes) return;
    free(grid->probes);
    grid->probes = NULL;
}


#include "rendering/global_illumination.h"

void gi_light_probes_bake(LightProbeGrid* grid, u32 samples_per_probe) {
    if (!grid || !grid->probes) return;

    u32 total_probes = grid->probes_x * grid->probes_y * grid->probes_z;
    PathTracingConfig config = {0};
    config.max_bounces = 2;
    config.samples_per_pixel = 1; // 1 sample per direction
    // ... other config init

    for (u32 p = 0; p < total_probes; p++) {
        LightProbe* probe = &grid->probes[p];
        
        // Random seed for this probe
        u32 seed = p * 12345 + 1;

        // Reset coefficients
        memset(probe->sh_coefficients, 0, sizeof(probe->sh_coefficients));

        // Cast rays in spherical directions
        // Fibonacci sphere or random Directions
        for (u32 s = 0; s < samples_per_probe; s++) {
            // Generate direction on sphere
            // (Simplified random for now)
            f32 z = 1.0f - 2.0f * (f32)s / (f32)(samples_per_probe - 1); // -1 to 1? Or just random
            // Lets use consistent sampling
             // .. math ..
            
            // For now, just call path trace in random directions
            // We need a direction.
            Vec3 dir = {0,0,1}; // TODO: Better direction generation
            
            Vec3 radiance;
            gi_path_trace(probe->position, dir, &config, &seed, &radiance, NULL);
            
            // Project to SH (accumulate)
            // L_lm += radiance * Y_lm(dir)
            // Weighted average at the end
            
            // Simple accumulation for DC term (Y00 approx 0.282)
            f32 Y00 = 0.282095f;
            probe->sh_coefficients[0] += radiance.x * Y00;
            probe->sh_coefficients[9] += radiance.y * Y00;
            probe->sh_coefficients[18] += radiance.z * Y00;
        }
        
        // Normalize
        if (samples_per_probe > 0) {
            f32 scale = 4.0f * 3.14159f / (f32)samples_per_probe; // Integral over sphere/samples
            probe->sh_coefficients[0] *= scale;
            probe->sh_coefficients[9] *= scale;
            probe->sh_coefficients[18] *= scale;
        }
    }
}

void gi_light_probes_interpolate(const LightProbeGrid* grid,
                               Vec3 position, Vec3 normal,
                               Vec3* out_irradiance) {
    if (!grid || !grid->probes || !out_irradiance) return;
    
    // Bounds check to avoid invalid access
    if (position.x < grid->grid_min.x || position.x > grid->grid_max.x ||
        position.y < grid->grid_min.y || position.y > grid->grid_max.y ||
        position.z < grid->grid_min.z || position.z > grid->grid_max.z) {
        // Fallback or clamp? For now, simple fallback.
        *out_irradiance = (Vec3){0.1f, 0.1f, 0.1f};
        return;
    }

    // Find surrounding probes and interpolate
    if (grid->probe_spacing.x <= 0.0001f) {
        // Single probe case
        if (grid->probes_x > 0) {
             // simplified SH eval for DC
             *out_irradiance = (Vec3){
                grid->probes[0].sh_coefficients[0] * 0.282095f,
                grid->probes[0].sh_coefficients[9] * 0.282095f,
                grid->probes[0].sh_coefficients[18] * 0.282095f
             };
        }
        return;
    }

    f32 tx = (position.x - grid->grid_min.x) / grid->probe_spacing.x;
    f32 ty = (position.y - grid->grid_min.y) / grid->probe_spacing.y;
    f32 tz = (position.z - grid->grid_min.z) / grid->probe_spacing.z;

    u32 x0 = (u32)tx;
    u32 y0 = (u32)ty;
    u32 z0 = (u32)tz;
    
    // Clamp indices to be safe
    if (x0 >= grid->probes_x - 1) x0 = grid->probes_x - 2;
    if (y0 >= grid->probes_y - 1) y0 = grid->probes_y - 2;
    if (z0 >= grid->probes_z - 1) z0 = grid->probes_z - 2;

    f32 fx = tx - (f32)x0;
    f32 fy = ty - (f32)y0;
    f32 fz = tz - (f32)z0;
    
    // Clamp fractions just in case
    if (fx < 0) fx = 0; if (fx > 1) fx = 1;
    if (fy < 0) fy = 0; if (fy > 1) fy = 1;
    if (fz < 0) fz = 0; if (fz > 1) fz = 1;

    // Trilinear interpolation of SH coeffs
    // We only interpolate DC term (0th coeff) for simplicity in this step, 
    // but full implementation would interpolate all relevant bands or result of evaluation.
    Vec3 interpolated = {0.0f, 0.0f, 0.0f};
    f32 total_weight = 0.0f;

    for (u32 dx = 0; dx < 2; dx++) {
        for (u32 dy = 0; dy < 2; dy++) {
            for (u32 dz = 0; dz < 2; dz++) {
                u32 px = x0 + dx;
                u32 py = y0 + dy;
                u32 pz = z0 + dz;

                f32 weight = (dx ? fx : (1.0f - fx)) *
                            (dy ? fy : (1.0f - fy)) *
                            (dz ? fz : (1.0f - fz));

                u32 idx = px + py * grid->probes_x + pz * grid->probes_x * grid->probes_y;
                LightProbe* probe = &grid->probes[idx];
                
                // SH DC term index is 0 for R, 9 for G, 18 for B
                interpolated.x += probe->sh_coefficients[0] * weight;
                interpolated.y += probe->sh_coefficients[9] * weight;
                interpolated.z += probe->sh_coefficients[18] * weight;
                
                total_weight += weight;
            }
        }
    }
    
    // Evaluate SH (DC term only: Y00 = 1/2 * sqrt(1/pi) approx 0.282095)
    float Y00 = 0.282095f;
    *out_irradiance = (Vec3){
        interpolated.x * Y00,
        interpolated.y * Y00,
        interpolated.z * Y00
    };
}
