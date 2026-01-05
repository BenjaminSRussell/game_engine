/**
 * @file water_system.h
 * @brief Water and ocean rendering system
 * @details Complete water simulation and rendering for Valley of the Ancients
 *
 * TODO: Implement full water simulation pipeline
 * TODO: Add ocean wave generation (Gerstner, FFT-based)
 * TODO: Implement water surface rendering with proper refraction
 * TODO: Add foam simulation and rendering
 * TODO: Implement underwater caustics
 * TODO: Add water physics interactions with rigid bodies
 * TODO: Implement water streaming for large ocean areas
 * TODO: Add dynamic water level changes
 * TODO: Implement water decals and wakes
 * TODO: Add underwater fog and volumetric effects
 */

#ifndef RENDER_WATER_SYSTEM_H
#define RENDER_WATER_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct RenderWaterManager RenderWaterManager;
typedef struct WaterVolume WaterVolume;

/**
 * @brief Water simulation method
 */
typedef enum {
    WATER_SIM_GERSTNER_WAVES = 0,
    WATER_SIM_FFT_OCEAN = 1,
    WATER_SIM_GRID_BASED = 2,
    WATER_SIM_PARTICLE_BASED = 3,
} WaterSimulationMethod;

/**
 * @brief Water volume configuration
 *
 * TODO: Add wind direction and strength configuration
 * TODO: Add wave spectrum parameters
 * TODO: Add foam generation parameters
 * TODO: Add underwater light absorption parameters
 * TODO: Add water color gradient configuration
 */
typedef struct {
    float position[3];
    float width;
    float height;
    float water_level;
    WaterSimulationMethod simulation_method;
    uint32_t resolution;
    float damping;
    float flow_speed;
} WaterVolumeConfig;

/**
 * @brief Create water manager
 * @param[in] max_volumes Maximum water volumes
 * @return Pointer to manager, NULL on failure
 *
 * TODO: Initialize wave spectrum data
 * TODO: Allocate simulation buffers
 * TODO: Create water render targets
 */
RenderWaterManager* render_water_create(uint32_t max_volumes);

/**
 * @brief Destroy water manager
 * @param[in] manager Manager to destroy
 *
 * TODO: Release all water volumes
 * TODO: Free simulation buffers
 * TODO: Release render targets
 */
void render_water_destroy(RenderWaterManager* manager);

/**
 * @brief Create water volume
 * @param[in] manager Water manager
 * @param[in] config Water volume configuration
 * @return Volume ID, 0 on failure
 *
 * TODO: Generate initial mesh from heightfield
 * TODO: Create simulation data structures
 * TODO: Initialize material for water rendering
 */
uint32_t render_water_create_volume(RenderWaterManager* manager,
                                    const WaterVolumeConfig* config);

/**
 * @brief Update water simulation
 * @param[in] manager Water manager
 * @param[in] delta_time Frame time
 * @return true on success
 *
 * TODO: Update wave simulation
 * TODO: Calculate foam generation
 * TODO: Update particle systems
 * TODO: Handle GPU-CPU synchronization
 */
bool render_water_update(RenderWaterManager* manager, float delta_time);

/**
 * @brief Render water
 * @param[in] manager Water manager
 * @return true on success
 *
 * TODO: Render water surface with refraction
 * TODO: Render underwater caustics
 * TODO: Render foam layer
 * TODO: Apply underwater fog
 */
bool render_water_render(RenderWaterManager* manager);

/**
 * @brief Get water height at position
 * @param[in] manager Water manager
 * @param[in] volume_id Volume ID
 * @param[in] x X position
 * @param[in] z Z position
 * @param[out] height Water surface height
 * @return true if position is over water
 *
 * TODO: Implement wave sampling at position
 * TODO: Add interpolation for smooth height queries
 */
bool render_water_sample_height(RenderWaterManager* manager,
                               uint32_t volume_id,
                               float x, float z,
                               float* height);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_WATER_SYSTEM_H */
