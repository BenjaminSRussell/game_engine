#ifndef ADVANCED_TERRAIN_H
#define ADVANCED_TERRAIN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    uint32_t width, height, depth;
    float* heightmap;
    float* moisture_map;
    float* temperature_map;
    uint8_t* biome_map;
    Vec3* normal_map;
} TerrainData;

typedef struct {
    float frequency;
    float amplitude;
    int octaves;
    float persistence;
    float lacunarity;
    int seed;
} NoiseParameters;

typedef struct {
    NoiseParameters base_noise;
    NoiseParameters detail_noise;
    NoiseParameters erosion_noise;
    float erosion_strength;
    float thermal_erosion_rate;
    float hydraulic_erosion_rate;
    uint32_t erosion_iterations;
} ErosionParameters;

typedef struct {
    float sea_level;
    float beach_width;
    float mountain_height_threshold;
    float plateau_height;
    float valley_depth;
    float river_width;
    float river_meander_strength;
} TerrainFeatures;

typedef struct {
    TerrainData* terrain;
    NoiseParameters* noise_params;
    ErosionParameters* erosion_params;
    TerrainFeatures* features;
    bool is_generated;
} TerrainGenerator;

// Terrain generation algorithms
TerrainGenerator* terrain_create_generator(uint32_t width, uint32_t height, uint32_t depth);
bool terrain_generate_heightmap(TerrainGenerator* generator, const NoiseParameters* params);
bool terrain_apply_erosion(TerrainGenerator* generator, const ErosionParameters* params);
bool terrain_generate_biomes(TerrainGenerator* generator, const TerrainFeatures* features);

// Advanced noise functions
float terrain_perlin_noise(float x, float y, int seed);
float terrain_ridged_noise(float x, float y, int seed);
float terrain_billow_noise(float x, float y, int seed);
float terrain_domain_warping(float x, float y, float strength, int seed);

// Erosion simulation
bool terrain_thermal_erosion(TerrainData* terrain, float rate, uint32_t iterations);
bool terrain_hydraulic_erosion(TerrainData* terrain, float rate, uint32_t iterations);
bool terrain_wind_erosion(TerrainData* terrain, float strength, uint32_t iterations);

// River and road generation
bool terrain_generate_rivers(TerrainGenerator* generator, uint32_t num_rivers);
bool terrain_generate_roads(TerrainGenerator* generator, uint32_t num_roads);
bool terrain_carve_valleys(TerrainGenerator* generator, float depth, float width);

// Cave and tunnel generation
bool terrain_generate_caves(TerrainGenerator* generator, float cave_density, uint32_t seed);
bool terrain_generate_tunnels(TerrainGenerator* generator, const Vec3* start, const Vec3* end, float radius);

// Vegetation placement
bool terrain_place_vegetation(TerrainGenerator* generator, float vegetation_density);
bool terrain_place_trees(TerrainGenerator* generator, const Vec3* positions, uint32_t count);
bool terrain_place_grass(TerrainGenerator* generator, float grass_density);

// Terrain optimization
bool terrain_lod_generation(TerrainData* terrain, uint32_t levels);
bool terrain_chunk_optimization(TerrainData* terrain, uint32_t chunk_size);
bool terrain_normal_map_generation(TerrainData* terrain);

// Export and import
bool terrain_export_to_obj(const TerrainData* terrain, const char* filename);
bool terrain_export_to_heightmap(const TerrainData* terrain, const char* filename);
bool terrain_import_heightmap(TerrainData* terrain, const char* filename);

// Utility functions
float terrain_get_height_at(const TerrainData* terrain, float x, float y);
Vec3 terrain_get_normal_at(const TerrainData* terrain, float x, float y);
uint8_t terrain_get_biome_at(const TerrainData* terrain, float x, float y);
bool terrain_is_point_in_bounds(const TerrainData* terrain, float x, float y);

// Cleanup
void terrain_destroy_generator(TerrainGenerator* generator);
void terrain_destroy_data(TerrainData* terrain);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_TERRAIN_H
