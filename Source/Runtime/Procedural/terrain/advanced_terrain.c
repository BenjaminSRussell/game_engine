#include "advanced_terrain.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float fade_function(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

static float perlin_noise_3d(float x, float y, float z, int seed) {
    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;
    int Z = (int)floorf(z) & 255;
    
    x -= floorf(x);
    y -= floorf(y);
    z -= floorf(z);
    
    float u = fade_function(x);
    float v = fade_function(y);
    float w = fade_function(z);
    
    // Simple permutation table (would use proper one in production)
    static int perm[512] = {151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};
    
    int A = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A + 1] + Z;
    int B = perm[X + 1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B + 1] + Z;
    
    return lerp(lerp(lerp(grad(perm[AA], x, y, z),
                         grad(perm[BA], x - 1, y, z), u),
                    lerp(grad(perm[AB], x, y - 1, z),
                         grad(perm[BB], x - 1, y - 1, z), u), v),
               lerp(lerp(grad(perm[AA + 1], x, y, z - 1),
                         grad(perm[BA + 1], x - 1, y, z - 1), u),
                    lerp(grad(perm[AB + 1], x, y - 1, z - 1),
                         grad(perm[BB + 1], x - 1, y - 1, z - 1), u), v), w);
}

float terrain_perlin_noise(float x, float y, int seed) {
    return perlin_noise_3d(x, y, 0.5f, seed);
}

float terrain_ridged_noise(float x, float y, int seed) {
    float noise = terrain_perlin_noise(x, y, seed);
    return 1.0f - fabsf(noise);
}

float terrain_billow_noise(float x, float y, int seed) {
    float noise = terrain_perlin_noise(x, y, seed);
    return fabsf(noise) * 2.0f - 1.0f;
}

float terrain_domain_warping(float x, float y, float strength, int seed) {
    float warp_x = terrain_perlin_noise(x + seed, y, seed);
    float warp_y = terrain_perlin_noise(x, y + seed, seed);
    
    return terrain_perlin_noise(x + warp_x * strength, y + warp_y * strength, seed);
}

TerrainGenerator* terrain_create_generator(uint32_t width, uint32_t height, uint32_t depth) {
    TerrainGenerator* generator = malloc(sizeof(TerrainGenerator));
    if (!generator) return NULL;
    
    generator->terrain = malloc(sizeof(TerrainData));
    if (!generator->terrain) {
        free(generator);
        return NULL;
    }
    
    generator->terrain->width = width;
    generator->terrain->height = height;
    generator->terrain->depth = depth;
    
    // Allocate terrain data arrays
    generator->terrain->heightmap = malloc(width * height * sizeof(float));
    generator->terrain->moisture_map = malloc(width * height * sizeof(float));
    generator->terrain->temperature_map = malloc(width * height * sizeof(float));
    generator->terrain->biome_map = malloc(width * height * sizeof(uint8_t));
    generator->terrain->normal_map = malloc(width * height * sizeof(Vec3));
    
    if (!generator->terrain->heightmap || !generator->terrain->moisture_map || 
        !generator->terrain->temperature_map || !generator->terrain->biome_map || 
        !generator->terrain->normal_map) {
        terrain_destroy_generator(generator);
        return NULL;
    }
    
    generator->noise_params = NULL;
    generator->erosion_params = NULL;
    generator->features = NULL;
    generator->is_generated = false;
    
    return generator;
}

bool terrain_generate_heightmap(TerrainGenerator* generator, const NoiseParameters* params) {
    if (!generator || !params) return false;
    
    for (uint32_t y = 0; y < generator->terrain->height; y++) {
        for (uint32_t x = 0; x < generator->terrain->width; x++) {
            float height = 0.0f;
            float amplitude = params->amplitude;
            float frequency = params->frequency;
            
            for (int oct = 0; oct < params->octaves; oct++) {
                float sample_x = (float)x / generator->terrain->width * frequency;
                float sample_y = (float)y / generator->terrain->height * frequency;
                
                height += terrain_perlin_noise(sample_x, sample_y, params->seed + oct) * amplitude;
                
                amplitude *= params->persistence;
                frequency *= params->lacunarity;
            }
            
            generator->terrain->heightmap[y * generator->terrain->width + x] = height;
        }
    }
    
    return true;
}

bool terrain_apply_erosion(TerrainGenerator* generator, const ErosionParameters* params) {
    if (!generator || !params) return false;
    
    // Apply thermal erosion
    for (uint32_t iter = 0; iter < params->erosion_iterations; iter++) {
        terrain_thermal_erosion(generator->terrain, params->thermal_erosion_rate, 1);
        terrain_hydraulic_erosion(generator->terrain, params->hydraulic_erosion_rate, 1);
    }
    
    return true;
}

bool terrain_thermal_erosion(TerrainData* terrain, float rate, uint32_t iterations) {
    if (!terrain) return false;
    
    float* temp_heightmap = malloc(terrain->width * terrain->height * sizeof(float));
    if (!temp_heightmap) return false;
    
    memcpy(temp_heightmap, terrain->heightmap, terrain->width * terrain->height * sizeof(float));
    
    for (uint32_t iter = 0; iter < iterations; iter++) {
        for (uint32_t y = 1; y < terrain->height - 1; y++) {
            for (uint32_t x = 1; x < terrain->width - 1; x++) {
                uint32_t idx = y * terrain->width + x;
                float current_height = temp_heightmap[idx];
                
                // Check neighbors
                float max_diff = 0.0f;
                uint32_t min_neighbors = 0;
                
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        uint32_t n_idx = (y + dy) * terrain->width + (x + dx);
                        float diff = current_height - temp_heightmap[n_idx];
                        
                        if (diff > rate) {
                            max_diff += diff;
                            min_neighbors++;
                        }
                    }
                }
                
                if (min_neighbors > 0) {
                    float erosion_amount = max_diff / min_neighbors * 0.5f;
                    terrain->heightmap[idx] -= erosion_amount;
                    
                    // Distribute to lower neighbors
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            
                            uint32_t n_idx = (y + dy) * terrain->width + (x + dx);
                            float diff = current_height - temp_heightmap[n_idx];
                            
                            if (diff > rate) {
                                terrain->heightmap[n_idx] += erosion_amount / min_neighbors;
                            }
                        }
                    }
                }
            }
        }
        
        memcpy(temp_heightmap, terrain->heightmap, terrain->width * terrain->height * sizeof(float));
    }
    
    free(temp_heightmap);
    return true;
}

bool terrain_hydraulic_erosion(TerrainData* terrain, float rate, uint32_t iterations) {
    if (!terrain) return false;
    
    // Simplified hydraulic erosion
    for (uint32_t iter = 0; iter < iterations; iter++) {
        for (uint32_t i = 0; i < 1000; i++) { // Number of water droplets
            float x = (float)(rand() % terrain->width);
            float y = (float)(rand() % terrain->height);
            float water = 1.0f;
            float sediment = 0.0f;
            
            for (int step = 0; step < 30; step++) {
                uint32_t ix = (uint32_t)x;
                uint32_t iy = (uint32_t)y;
                
                if (ix >= terrain->width - 1 || iy >= terrain->height - 1) break;
                
                // Calculate gradient
                float grad_x = (terrain->heightmap[iy * terrain->width + (ix + 1)] - 
                               terrain->heightmap[iy * terrain->width + ix]) * 0.5f;
                float grad_y = (terrain->heightmap[(iy + 1) * terrain->width + ix] - 
                               terrain->heightmap[iy * terrain->width + ix]) * 0.5f;
                
                // Move water downhill
                x -= grad_x * 2.0f;
                y -= grad_y * 2.0f;
                
                // Erode and deposit
                float capacity = water * fabsf(grad_x + grad_y) * rate;
                if (sediment > capacity) {
                    // Deposit sediment
                    float deposit = (sediment - capacity) * 0.1f;
                    terrain->heightmap[iy * terrain->width + ix] += deposit;
                    sediment -= deposit;
                } else {
                    // Erode terrain
                    float erode = (capacity - sediment) * 0.1f;
                    terrain->heightmap[iy * terrain->width + ix] -= erode;
                    sediment += erode;
                }
                
                water *= 0.98f; // Evaporation
                if (water < 0.01f) break;
            }
        }
    }
    
    return true;
}

bool terrain_generate_biomes(TerrainGenerator* generator, const TerrainFeatures* features) {
    if (!generator || !features) return false;
    
    for (uint32_t y = 0; y < generator->terrain->height; y++) {
        for (uint32_t x = 0; x < generator->terrain->width; x++) {
            uint32_t idx = y * generator->terrain->width + x;
            float height = generator->terrain->heightmap[idx];
            
            // Simple biome classification based on height
            uint8_t biome;
            if (height < features->sea_level) {
                biome = 0; // Water
            } else if (height < features->sea_level + features->beach_width) {
                biome = 1; // Beach
            } else if (height < features->mountain_height_threshold) {
                biome = 2; // Plains
            } else if (height < features->mountain_height_threshold + features->plateau_height) {
                biome = 3; // Hills
            } else {
                biome = 4; // Mountains
            }
            
            generator->terrain->biome_map[idx] = biome;
            
            // Generate moisture and temperature based on height and position
            generator->terrain->moisture_map[idx] = 0.5f + 0.3f * sinf(x * 0.01f) * cosf(y * 0.01f);
            generator->terrain->temperature_map[idx] = 1.0f - height * 0.1f;
        }
    }
    
    return true;
}

float terrain_get_height_at(const TerrainData* terrain, float x, float y) {
    if (!terrain || x < 0 || y < 0 || x >= terrain->width || y >= terrain->height) {
        return 0.0f;
    }
    
    uint32_t ix = (uint32_t)x;
    uint32_t iy = (uint32_t)y;
    
    // Bilinear interpolation
    float fx = x - ix;
    float fy = y - iy;
    
    float h00 = terrain->heightmap[iy * terrain->width + ix];
    float h10 = terrain->heightmap[iy * terrain->width + (ix + 1)];
    float h01 = terrain->heightmap[(iy + 1) * terrain->width + ix];
    float h11 = terrain->heightmap[(iy + 1) * terrain->width + (ix + 1)];
    
    float h0 = h00 * (1 - fx) + h10 * fx;
    float h1 = h01 * (1 - fx) + h11 * fx;
    
    return h0 * (1 - fy) + h1 * fy;
}

Vec3 terrain_get_normal_at(const TerrainData* terrain, float x, float y) {
    if (!terrain) {
        Vec3 zero = {0, 1, 0};
        return zero;
    }
    
    float epsilon = 1.0f;
    float h_left = terrain_get_height_at(terrain, x - epsilon, y);
    float h_right = terrain_get_height_at(terrain, x + epsilon, y);
    float h_up = terrain_get_height_at(terrain, x, y - epsilon);
    float h_down = terrain_get_height_at(terrain, x, y + epsilon);
    
    Vec3 normal;
    normal.x = (h_left - h_right) / (2.0f * epsilon);
    normal.y = 2.0f; // Scale factor for Y
    normal.z = (h_up - h_down) / (2.0f * epsilon);
    
    // Normalize
    float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    normal.x /= length;
    normal.y /= length;
    normal.z /= length;
    
    return normal;
}

void terrain_destroy_generator(TerrainGenerator* generator) {
    if (!generator) return;
    
    terrain_destroy_data(generator->terrain);
    free(generator->noise_params);
    free(generator->erosion_params);
    free(generator->features);
    free(generator);
}

void terrain_destroy_data(TerrainData* terrain) {
    if (!terrain) return;
    
    free(terrain->heightmap);
    free(terrain->moisture_map);
    free(terrain->temperature_map);
    free(terrain->biome_map);
    free(terrain->normal_map);
    free(terrain);
}
