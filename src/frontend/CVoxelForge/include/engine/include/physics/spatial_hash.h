#ifndef PHYSICS_SPATIAL_HASH_H
#define PHYSICS_SPATIAL_HASH_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec3.h"
#include "physics/physics.h"

#ifdef __cplusplus
extern "C" {
#endif

// Spatial Hash Configuration
typedef struct {
    float cell_size;
    uint32_t table_size;
    Vec3 world_min;
    Vec3 world_max;
    uint32_t max_objects;
    uint32_t max_collisions_per_cell;
    bool enable_dynamic_rehashing;
    float rehash_threshold;
} SpatialHashConfig;

// Spatial Hash Cell
typedef struct {
    uint32_t* object_indices;
    uint32_t object_count;
    uint32_t capacity;
    bool dirty;
} SpatialHashCell;

// Spatial Hash Object
typedef struct {
    EntityID entity_id;
    Vec3 position;
    Vec3 bounds_min;
    Vec3 bounds_max;
    uint32_t cell_x, cell_y, cell_z;
    uint32_t hash_value;
    bool active;
    bool static_object;
    uint32_t collision_mask;
    uint32_t collision_group;
} SpatialHashObject;

// Spatial Hash Query Result
typedef struct {
    EntityID* entity_ids;
    uint32_t count;
    uint32_t capacity;
} SpatialHashQueryResult;

// Spatial Hash Context
typedef struct {
    SpatialHashConfig config;
    
    // Hash table
    SpatialHashCell* cells;
    uint32_t* cell_indices;
    
    // Object storage
    SpatialHashObject* objects;
    uint32_t object_count;
    uint32_t object_capacity;
    
    // Free object indices
    uint32_t* free_indices;
    uint32_t free_count;
    uint32_t free_capacity;
    
    // Query buffers
    SpatialHashQueryResult query_buffer;
    
    // Performance statistics
    struct {
        uint32_t total_objects;
        uint32_t active_objects;
        uint32_t static_objects;
        uint32_t occupied_cells;
        uint32_t max_objects_per_cell;
        uint32_t total_queries;
        uint32_t broad_phase_tests;
        uint32_t narrow_phase_tests;
        uint32_t rehash_count;
        float hash_time;
        float query_time;
        float average_load_factor;
        float hash_efficiency;
    } stats;
    
    // Memory usage tracking
    struct {
        size_t hash_table_size;
        size_t objects_size;
        size_t total_size;
    } memory;
} SpatialHash;

// Spatial Hash Functions
bool spatial_hash_init(SpatialHash* hash, const SpatialHashConfig* config);
void spatial_hash_cleanup(SpatialHash* hash);
void spatial_hash_reset(SpatialHash* hash);
bool spatial_hash_rehash(SpatialHash* hash, const SpatialHashConfig* new_config);

// Object Management
uint32_t spatial_hash_add_object(SpatialHash* hash, EntityID entity_id, Vec3 position, Vec3 bounds_min, Vec3 bounds_max,
                                bool is_static, uint32_t collision_mask, uint32_t collision_group);
bool spatial_hash_remove_object(SpatialHash* hash, EntityID entity_id);
bool spatial_hash_update_object(SpatialHash* hash, EntityID entity_id, Vec3 new_position, Vec3 new_bounds_min, Vec3 new_bounds_max);
bool spatial_hash_move_object(SpatialHash* hash, EntityID entity_id, Vec3 new_position);

// Query Functions
SpatialHashQueryResult* spatial_hash_query_sphere(SpatialHash* hash, Vec3 center, float radius);
SpatialHashQueryResult* spatial_hash_query_aabb(SpatialHash* hash, Vec3 bounds_min, Vec3 bounds_max);
SpatialHashQueryResult* spatial_hash_query_ray(SpatialHash* hash, Vec3 ray_start, Vec3 ray_direction, float max_distance);
SpatialHashQueryResult* spatial_hash_query_point(SpatialHash* hash, Vec3 point);

// Collision Detection
void spatial_hash_broad_phase_collision(SpatialHash* hash);
void spatial_hash_find_potential_collisions(SpatialHash* hash, EntityID entity_id, SpatialHashQueryResult* result);
void spatial_hash_find_collisions_in_cell(SpatialHash* hash, uint32_t cell_x, uint32_t cell_y, uint32_t cell_z, SpatialHashQueryResult* result);

// Hash Functions
uint32_t spatial_hash_hash_coords(SpatialHash* hash, uint32_t x, uint32_t y, uint32_t z);
uint32_t spatial_hash_hash_position(SpatialHash* hash, Vec3 position);
void spatial_hash_position_to_cell_coords(SpatialHash* hash, Vec3 position, uint32_t* x, uint32_t* y, uint32_t* z);
void spatial_hash_cell_coords_to_position(SpatialHash* hash, uint32_t x, uint32_t y, uint32_t z, Vec3* position);

// Cell Management
uint32_t spatial_hash_get_cell_index(SpatialHash* hash, uint32_t x, uint32_t y, uint32_t z);
SpatialHashCell* spatial_hash_get_cell(SpatialHash* hash, uint32_t x, uint32_t y, uint32_t z);
void spatial_hash_clear_cell(SpatialHash* hash, uint32_t cell_index);
void spatial_hash_add_to_cell(SpatialHash* hash, uint32_t cell_index, uint32_t object_index);
void spatial_hash_remove_from_cell(SpatialHash* hash, uint32_t cell_index, uint32_t object_index);

// Object-Cell Mapping
void spatial_hash_update_object_cells(SpatialHash* hash, uint32_t object_index);
void spatial_hash_remove_object_from_cells(SpatialHash* hash, uint32_t object_index);
void spatial_hash_add_object_to_cells(SpatialHash* hash, uint32_t object_index);
bool spatial_hash_object_in_cell(SpatialHash* hash, uint32_t object_index, uint32_t cell_index);

// Optimization Functions
void spatial_hash_optimize_for_static_objects(SpatialHash* hash);
void spatial_hash_update_static_objects(SpatialHash* hash);
void spatial_hash_compact_hash_table(SpatialHash* hash);
bool spatial_hash_should_rehash(SpatialHash* hash);

// Debug and Visualization
void spatial_hash_debug_draw_cells(SpatialHash* hash);
void spatial_hash_debug_draw_objects(SpatialHash* hash);
void spatial_hash_debug_draw_cell_occupancy(SpatialHash* hash);
void spatial_hash_debug_print_statistics(SpatialHash* hash);

// Performance Analysis
void spatial_hash_get_performance_stats(const SpatialHash* hash, uint32_t* total_objects, uint32_t* occupied_cells,
                                        float* load_factor, float* hash_efficiency, float* hash_time);
void spatial_hash_get_memory_usage(const SpatialHash* hash, size_t* hash_table, size_t* objects, size_t* total);
void spatial_hash_reset_statistics(SpatialHash* hash);

// Configuration Functions
void spatial_hash_set_cell_size(SpatialHash* hash, float cell_size);
void spatial_hash_set_world_bounds(SpatialHash* hash, Vec3 world_min, Vec3 world_max);
void spatial_hash_enable_dynamic_rehashing(SpatialHash* hash, bool enable);
void spatial_hash_set_rehash_threshold(SpatialHash* hash, float threshold);

// Utility Functions
bool spatial_hash_is_valid_position(const SpatialHash* hash, Vec3 position);
bool spatial_hash_is_valid_cell_coords(const SpatialHash* hash, uint32_t x, uint32_t y, uint32_t z);
float spatial_hash_calculate_optimal_cell_size(Vec3 world_size, uint32_t expected_objects);
uint32_t spatial_hash_calculate_optimal_table_size(uint32_t expected_objects);

// Collision Mask Filtering
bool spatial_hash_can_collide(const SpatialHashObject* obj_a, const SpatialHashObject* obj_b);
void spatial_hash_set_collision_mask(SpatialHash* hash, EntityID entity_id, uint32_t mask);
void spatial_hash_set_collision_group(SpatialHash* hash, EntityID entity_id, uint32_t group);

// Iterator Functions
typedef struct {
    SpatialHash* hash;
    uint32_t current_cell_index;
    uint32_t current_object_index;
    uint32_t cell_x, cell_y, cell_z;
    bool iterating_cells;
} SpatialHashIterator;

void spatial_hash_iterator_init(SpatialHashIterator* iterator, SpatialHash* hash);
bool spatial_hash_iterator_next_object(SpatialHashIterator* iterator, SpatialHashObject** object);
bool spatial_hash_iterator_next_cell(SpatialHashIterator* iterator, uint32_t* cell_index, SpatialHashCell** cell);

// Validation and Testing
bool spatial_hash_validate(const SpatialHash* hash);
bool spatial_hash_test_collision_detection(SpatialHash* hash);
void spatial_hash_run_performance_test(SpatialHash* hash, uint32_t num_objects, uint32_t iterations);

// Helper Functions
const char* spatial_hash_get_error_string(uint32_t error_code);
bool spatial_hash_is_power_of_two(uint32_t value);
uint32_t spatial_hash_next_power_of_two(uint32_t value);
float spatial_hash_fast_inverse_sqrt(float x);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_SPATIAL_HASH_H
