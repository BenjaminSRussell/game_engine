#ifndef WAVE_FUNCTION_COLLAPSE_H
#define WAVE_FUNCTION_COLLAPSE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t id;
    char name[64];
    uint32_t* compatible_tiles;
    uint32_t compatible_count;
    float weight;
    uint32_t color;
} WFCTile;

typedef struct {
    uint32_t x, y, z;
    uint32_t width, height, depth;
    uint32_t* possible_tiles;
    uint32_t possible_count;
    bool is_collapsed;
    uint32_t collapsed_tile;
    float entropy;
} WFCCell;

typedef struct {
    uint32_t direction; // 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    uint32_t from_tile;
    uint32_t to_tile;
    bool is_valid;
} WFCAdjacencyRule;

typedef struct {
    WFCTile* tiles;
    uint32_t tile_count;
    WFCAdjacencyRule* adjacency_rules;
    uint32_t rule_count;
    uint32_t* tile_frequency;
    uint32_t total_frequency;
} WFCTileset;

typedef struct {
    WFCCell* cells;
    uint32_t width, height, depth;
    uint32_t total_cells;
    WFCTileset* tileset;
    bool is_3d;
    bool periodic_boundary;
    uint32_t seed;
    bool is_complete;
    uint32_t collapsed_count;
} WFCGenerator;

typedef struct {
    uint32_t x, y, z;
    uint32_t tile_id;
    float confidence;
} WFCObservation;

typedef struct {
    float weight_sum;
    float log_weight_sum;
    float entropy;
} WFCEntropyCache;

// WFC Generator functions
WFCGenerator* wfc_create_generator(uint32_t width, uint32_t height, uint32_t depth, bool is_3d);
bool wfc_set_tileset(WFCGenerator* generator, const WFCTileset* tileset);
bool wfc_set_periodic_boundary(WFCGenerator* generator, bool periodic);
bool wfc_add_observation(WFCGenerator* generator, const WFCObservation* observation);

// Tileset management
WFCTileset* wfc_create_tileset(uint32_t tile_count);
bool wfc_add_tile(WFCTileset* tileset, const WFCTile* tile);
bool wfc_add_adjacency_rule(WFCTileset* tileset, const WFCAdjacencyRule* rule);
bool wfc_auto_generate_adjacency_rules(WFCTileset* tileset, uint32_t sample_width, uint32_t sample_height, const uint32_t* sample_data);
bool wfc_load_tileset_from_image(WFCTileset* tileset, const char* image_path, uint32_t tile_size);

// WFC Algorithm
bool wfc_initialize(WFCGenerator* generator);
bool wfc_step(WFCGenerator* generator);
bool wfc_collapse_all(WFCGenerator* generator);
bool wfc_collapse_to_completion(WFCGenerator* generator);

// Core WFC operations
uint32_t wfc_find_min_entropy_cell(const WFCGenerator* generator);
bool wfc_collapse_cell(WFCGenerator* generator, uint32_t cell_index, uint32_t tile_id);
bool wfc_propagate_constraints(WFCGenerator* generator, uint32_t changed_cell);
bool wfc_calculate_entropy(WFCGenerator* generator, uint32_t cell_index);

// Constraint propagation
bool wfc_update_cell_constraints(WFCGenerator* generator, uint32_t cell_index);
bool wfc_check_adjacency_constraint(const WFCGenerator* generator, uint32_t cell1, uint32_t cell2, uint32_t direction);
bool wfc_propagate_wave(WFCGenerator* generator, uint32_t start_cell);

// Entropy calculation and caching
float wfc_calculate_cell_entropy(const WFCGenerator* generator, uint32_t cell_index);
bool wfc_update_entropy_cache(WFCGenerator* generator);
void wfc_invalidate_entropy_cache(WFCGenerator* generator, uint32_t cell_index);

// Advanced WFC features
typedef struct {
    uint32_t pattern_size;
    uint32_t* patterns;
    uint32_t pattern_count;
    uint32_t* pattern_weights;
} WFCPatternSet;

bool wfc_use_pattern_mode(WFCGenerator* generator, const WFCPatternSet* pattern_set);
bool wfc_extract_patterns_from_sample(const uint32_t* sample_data, uint32_t width, uint32_t height, 
                                      uint32_t pattern_size, WFCPatternSet* pattern_set);

// Multi-layer WFC
typedef struct {
    WFCGenerator* layer;
    char name[64];
    uint32_t z_offset;
    bool influences_above;
    bool influences_below;
} WFCLayer;

typedef struct {
    WFCLayer* layers;
    uint32_t layer_count;
    uint32_t width, height;
    bool propagate_between_layers;
} WFCMultiLayerGenerator;

WFCMultiLayerGenerator* wfc_create_multilayer_generator(uint32_t width, uint32_t height);
bool wfc_add_layer(WFCMultiLayerGenerator* multi_gen, const char* name, uint32_t z_offset, const WFCTileset* tileset);
bool wfc_generate_multilayer(WFCMultiLayerGenerator* multi_gen);

// WFC with constraints and goals
typedef struct {
    uint32_t x, y, z;
    uint32_t required_tile;
    float strength; // 0.0 = suggestion, 1.0 = requirement
} WFCConstraint;

bool wfc_add_constraint(WFCGenerator* generator, const WFCConstraint* constraint);
bool wfc_remove_constraint(WFCGenerator* generator, uint32_t constraint_id);
bool wfc_apply_constraints(WFCGenerator* generator);

// WFC analysis and debugging
typedef struct {
    uint32_t total_cells;
    uint32_t collapsed_cells;
    uint32_t remaining_possibilities;
    float average_entropy;
    uint32_t contradictions;
    uint32_t propagation_steps;
} WFCStatistics;

WFCStatistics wfc_get_statistics(const WFCGenerator* generator);
bool wfc_detect_contradictions(const WFCGenerator* generator);
bool wfc_export_state(const WFCGenerator* generator, const char* filename);
bool wfc_import_state(WFCGenerator* generator, const char* filename);

// WFC utilities
uint32_t wfc_get_cell_index(const WFCGenerator* generator, uint32_t x, uint32_t y, uint32_t z);
bool wfc_get_cell_coords(const WFCGenerator* generator, uint32_t index, uint32_t* x, uint32_t* y, uint32_t* z);
bool wfc_is_valid_position(const WFCGenerator* generator, uint32_t x, uint32_t y, uint32_t z);
uint32_t wfc_get_neighbor_cell(const WFCGenerator* generator, uint32_t cell_index, uint32_t direction);

// Output and rendering
bool wfc_export_to_image(const WFCGenerator* generator, const char* filename);
bool wfc_export_to_3d_model(const WFCGenerator* generator, const char* filename);
bool wfc_export_to_tilemap(const WFCGenerator* generator, const char* filename);
uint32_t* wfc_get_output_data(const WFCGenerator* generator);

// Performance optimization
bool wfc_enable_parallel_propagation(WFCGenerator* generator, bool enabled);
bool wfc_optimize_tileset(WFCTileset* tileset);
bool wfc_precompute_constraints(WFCGenerator* generator);

// Cleanup
void wfc_destroy_generator(WFCGenerator* generator);
void wfc_destroy_tileset(WFCTileset* tileset);
void wfc_destroy_multilayer_generator(WFCMultiLayerGenerator* multi_gen);
void wfc_destroy_pattern_set(WFCPatternSet* pattern_set);

#ifdef __cplusplus
}
#endif

#endif // WAVE_FUNCTION_COLLAPSE_H
