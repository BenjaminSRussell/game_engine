// include/engine/ai/terrain_enhancement.h
//
// Purpose: ML-based procedural terrain enhancement system
// Uses neural networks to enhance terrain generation with realistic features
//

#ifndef TERRAIN_ENHANCEMENT_H
#define TERRAIN_ENHANCEMENT_H

#include "../../common.h"
#include "../ml/ml_core.h"
#include <stdbool.h>

typedef enum {
    TERRAIN_MODEL_EROSION = 0,
    TERRAIN_MODEL_VEGETATION,
    TERRAIN_MODEL_GEOMORPHOLOGY,
    TERRAIN_MODEL_WATER_FLOW,
    TERRAIN_MODEL_CUSTOM
} TerrainModel;

typedef struct {
    u32 width, height, depth;
    f32 *heightmap;
    f32 *vegetation_density;
    f32 *erosion_map;
    f32 *water_flow;
    f32 enhancement_strength;
    bool apply_erosion;
    bool add_vegetation;
    bool simulate_water;
} TerrainData;

typedef struct {
    MLSystem *ml_system;
    void *terrain_models[5];
    TerrainModel active_model;
    TerrainData terrain_data;
    void *enhanced_output;
    f32 processing_time;
    bool is_enhancing;
    bool initialized;
} TerrainEnhancementSystem;

TerrainEnhancementSystem *terrain_enhancement_create(MLSystem *ml_system);
void terrain_enhancement_destroy(TerrainEnhancementSystem *system);
bool terrain_enhancement_initialize(TerrainEnhancementSystem *system, TerrainModel model);
bool terrain_enhance_process(TerrainEnhancementSystem *system, TerrainData *input_data);
void terrain_enhancement_set_strength(TerrainEnhancementSystem *system, f32 strength);

#endif // TERRAIN_ENHANCEMENT_H
