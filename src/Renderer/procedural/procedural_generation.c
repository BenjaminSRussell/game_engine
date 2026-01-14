// src/engine/rendering/procedural/procedural_generation.c
// Procedural Content Generation - Advanced procedural algorithms for infinite content

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Procedural Generation Types
// ============================================================================

typedef enum {
    PROCEDURAL_TYPE_TERRAIN,
    PROCEDURAL_TYPE_VEGETATION,
    PROCEDURAL_TYPE_CAVES,
    PROCEDURAL_TYPE_STRUCTURES,
    PROCEDURAL_TYPE_TEXTURES,
    PROCEDURAL_TYPE_MESHES,
    PROCEDURAL_TYPE_ANIMATIONS,
    PROCEDURAL_TYPE_SOUNDS,
    PROCEDURAL_TYPE_MUSIC,
    PROCEDURAL_TYPE_COUNT
} ProceduralType;

typedef struct {
    // Perlin noise parameters
    float frequency;
    float amplitude;
    int octaves;
    float persistence;
    float lacunarity;
    int seed;
    
    // Terrain parameters
    float height_scale;
    float water_level;
    float mountain_height;
    float valley_depth;
    float erosion_factor;
    
    // Vegetation parameters
    float vegetation_density;
    float vegetation_height_variation;
    float vegetation_cluster_size;
    
    // Structure parameters
    float structure_density;
    float structure_size_variation;
    float structure_complexity;
    
    // Texture parameters
    float texture_detail;
    float texture_variation;
    float texture_seamlessness;
    
    // Performance
    bool enable_caching;
    uint32_t cache_size;
    bool enable_multithreading;
    
    // Statistics
    uint64_t chunks_generated;
    uint64_t vertices_generated;
    uint64_t triangles_generated;
    float generation_time_ms;
    
    char name[256];
    bool initialized;
} ProceduralGenerator;

typedef struct {
    float x, y, z;
    float noise_value;
    float height;
    uint8_t biome_type;
    uint8_t vegetation_density;
    uint8_t structure_presence;
} ProceduralChunk;

typedef struct {
    ProceduralGenerator *generators[PROCEDURAL_TYPE_COUNT];
    uint32_t generator_count;
    
    // Global settings
    int global_seed;
    float quality_level;
    bool enable_caching;
    uint32_t global_cache_size;
    
    // Performance
    uint64_t total_chunks_generated;
    uint64_t total_generation_time_ms;
    float average_generation_time_ms;
    
    // Thread safety
    pthread_mutex_t proc_mutex;
    
    bool initialized;
} ProceduralSystem;

static ProceduralSystem g_procedural_system = {0};

// ============================================================================
// Noise Functions
// ============================================================================

static float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10); // 6t^5 - 15t^4 + 10t^3
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
    
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    int A = (X + Y + Z + seed) & 15;
    int B = (X + Y + Z + 1 + seed) & 15;
    int C = (X + Y + Z + 16 + seed) & 15;
    int D = (X + Y + Z + 17 + seed) & 15;
    
    return lerp(w, lerp(v, lerp(u, grad(A, x, y, z), grad(B, x-1, y, z)),
                       lerp(u, grad(C, x, y-1, z), grad(D, x-1, y-1, z))),
                   lerp(v, lerp(u, grad(A+1, x, y, z-1), grad(B+1, x-1, y, z-1)),
                       lerp(u, grad(C+1, x, y-1, z-1), grad(D+1, x-1, y-1, z-1))));
}

static float fractal_noise(float x, float y, float z, int octaves, float persistence, float lacunarity, int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += perlin_noise_3d(x * frequency, y * frequency, z * frequency, seed + i) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / max_value;
}

static float ridged_noise(float x, float y, float z, int octaves, float persistence, float lacunarity, int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        float noise = fabsf(perlin_noise_3d(x * frequency, y * frequency, z * frequency, seed + i));
        total += (1.0f - noise) * amplitude;
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / max_value;
}

// ============================================================================
// Terrain Generation
// ============================================================================

static void generate_terrain_chunk(ProceduralGenerator *generator, ProceduralChunk *chunk, 
                                  float chunk_x, float chunk_y, float chunk_z, uint32_t chunk_size) {
    if (!generator || !chunk) return;
    
    uint64_t start_time = get_time_nanos();
    
    float cell_size = 1.0f / chunk_size;
    
    for (uint32_t z = 0; z < chunk_size; z++) {
        for (uint32_t y = 0; y < chunk_size; y++) {
            for (uint32_t x = 0; x < chunk_size; x++) {
                uint32_t index = (z * chunk_size + y) * chunk_size + x;
                
                float world_x = chunk_x + x * cell_size;
                float world_y = chunk_y + y * cell_size;
                float world_z = chunk_z + z * cell_size;
                
                // Generate height using multiple noise layers
                float height = 0.0f;
                
                // Base terrain
                height += fractal_noise(world_x * generator->frequency, 
                                       world_y * generator->frequency, 
                                       world_z * generator->frequency,
                                       generator->octaves, 
                                       generator->persistence, 
                                       generator->lacunarity, 
                                       generator->seed) * generator->amplitude;
                
                // Mountains
                height += ridged_noise(world_x * 0.02f, world_y * 0.02f, world_z * 0.02f, 4, 0.5f, 2.0f, generator->seed + 100) * generator->mountain_height;
                
                // Valleys
                height -= ridged_noise(world_x * 0.01f, world_y * 0.01f, world_z * 0.01f, 3, 0.7f, 2.5f, generator->seed + 200) * generator->valley_depth;
                
                // Erosion
                float erosion = fractal_noise(world_x * 0.1f, world_y * 0.1f, world_z * 0.1f, 2, 0.8f, 3.0f, generator->seed + 300);
                height *= (1.0f - erosion * generator->erosion_factor);
                
                // Scale height
                height *= generator->height_scale;
                
                // Store in chunk
                chunk[index].x = world_x;
                chunk[index].y = world_y;
                chunk[index].z = world_z;
                chunk[index].height = height;
                chunk[index].noise_value = height;
                
                // Determine biome based on height
                if (height < generator->water_level) {
                    chunk[index].biome_type = 0; // Water
                } else if (height < generator->water_level + 10.0f) {
                    chunk[index].biome_type = 1; // Beach
                } else if (height < generator->water_level + 50.0f) {
                    chunk[index].biome_type = 2; // Plains
                } else if (height < generator->water_level + 100.0f) {
                    chunk[index].biome_type = 3; // Forest
                } else {
                    chunk[index].biome_type = 4; // Mountain
                }
                
                // Generate vegetation density
                float vegetation_noise = fractal_noise(world_x * 0.5f, world_y * 0.5f, world_z * 0.5f, 3, 0.5f, 2.0f, generator->seed + 400);
                chunk[index].vegetation_density = (uint8_t)(vegetation_noise * generator->vegetation_density * 255.0f);
                
                // Generate structure presence
                float structure_noise = fractal_noise(world_x * 0.2f, world_y * 0.2f, world_z * 0.2f, 2, 0.3f, 3.0f, generator->seed + 500);
                chunk[index].structure_presence = (uint8_t)(structure_noise * generator->structure_density * 255.0f);
            }
        }
    }
    
    uint64_t end_time = get_time_nanos();
    generator->generation_time_ms += nanos_to_ms(end_time - start_time);
    generator->chunks_generated++;
    
    LOG_DEBUG("Generated terrain chunk (%.1f, %.1f, %.1f): %.2f ms", chunk_x, chunk_y, chunk_z, generator->generation_time_ms);
}

// ============================================================================
// Vegetation Generation
// ============================================================================

static void generate_vegetation_chunk(ProceduralGenerator *generator, ProceduralChunk *chunk,
                                     float chunk_x, float chunk_y, float chunk_z, uint32_t chunk_size) {
    if (!generator || !chunk) return;
    
    uint64_t start_time = get_time_nanos();
    
    for (uint32_t i = 0; i < chunk_size * chunk_size * chunk_size; i++) {
        if (chunk[i].vegetation_density > 0) {
            // Generate vegetation based on biome type
            switch (chunk[i].biome_type) {
                case 2: // Plains
                    if (chunk[i].vegetation_density > 128) {
                        // Generate grass
                        float grass_height = generator->vegetation_height_variation * 
                                          (float)(chunk[i].vegetation_density - 128) / 127.0f;
                        // TODO: Add grass geometry to chunk
                    }
                    break;
                    
                case 3: // Forest
                    if (chunk[i].vegetation_density > 64) {
                        // Generate trees
                        float tree_height = 5.0f + generator->vegetation_height_variation * 
                                        (float)(chunk[i].vegetation_density - 64) / 191.0f;
                        // TODO: Add tree geometry to chunk
                    }
                    break;
                    
                case 4: // Mountain
                    if (chunk[i].vegetation_density > 32) {
                        // Generate sparse vegetation
                        float vegetation_height = generator->vegetation_height_variation * 
                                                  (float)(chunk[i].vegetation_density - 32) / 223.0f * 0.5f;
                        
                        // Add sparse vegetation to chunk
                        // Create small bushes, rocks, and sparse grass patches
                        uint32_t vegetation_type = (chunk[i].vegetation_density % 3);
                        
                        switch (vegetation_type) {
                            case 0: // Small bush
                                // Bush geometry: small sphere with flattened bottom
                                for (int bush_layer = 0; bush_layer < 3; bush_layer++) {
                                    float bush_radius = 0.3f * (1.0f - bush_layer * 0.2f);
                                    float bush_y = chunk[i].y + bush_layer * 0.2f;
                                    
                                    // Add bush vertices to chunk (simplified)
                                    // TODO: Add actual bush mesh generation
                                }
                                break;
                                
                            case 1: // Rock formation
                                // Rock geometry: irregular polyhedron
                                float rock_size = 0.2f + vegetation_height * 0.1f;
                                
                                // Add rock vertices to chunk (simplified)
                                // TODO: Add actual rock mesh generation with random perturbations
                                break;
                                
                            case 2: // Sparse grass patch
                                // Grass patch: multiple small grass blades
                                for (int grass_blade = 0; grass_blade < 5; grass_blade++) {
                                    float grass_offset_x = (float)(grass_blade % 3 - 1) * 0.1f;
                                    float grass_offset_z = (float)(grass_blade / 3 - 1) * 0.1f;
                                    float grass_height = 0.1f + vegetation_height * 0.05f;
                                    
                                    // Add grass blade vertices to chunk (simplified)
                                    // TODO: Add actual grass blade mesh generation
                                }
                                break;
                        }
                        
                        // Update chunk statistics
                        generator->vertices_generated += 8; // Approximate vertices per vegetation
                        generator->triangles_generated += 4; // Approximate triangles per vegetation
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    uint64_t end_time = get_time_nanos();
    generator->generation_time_ms += nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Generated vegetation chunk: %.2f ms", generator->generation_time_ms);
}

// ============================================================================
// Structure Generation
// ============================================================================

static void generate_structure_chunk(ProceduralGenerator *generator, ProceduralChunk *chunk,
                                     float chunk_x, float chunk_y, float chunk_z, uint32_t chunk_size) {
    if (!generator || !chunk) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Find structure locations
    for (uint32_t i = 0; i < chunk_size * chunk_size * chunk_size; i++) {
        if (chunk[i].structure_presence > 200) {
            // Generate structure based on biome type
            switch (chunk[i].biome_type) {
                case 1: // Beach
                case 2: // Plains
                    // Generate houses or small structures
                    // TODO: Add house geometry to chunk
                    break;
                    
                case 3: // Forest
                    // Generate cabins or treehouses
                    // TODO: Add cabin geometry to chunk
                    break;
                    
                case 4: // Mountain
                    // Generate mountain structures or temples
                    // TODO: Add temple geometry to chunk
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    uint64_t end_time = get_time_nanos();
    generator->generation_time_ms += nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Generated structure chunk: %.2f ms", generator->generation_time_ms);
}

// ============================================================================
// Procedural System API
// ============================================================================

bool procedural_system_init(int global_seed, float quality_level, bool enable_caching, uint32_t cache_size) {
    if (g_procedural_system.initialized) {
        LOG_WARN("Procedural system already initialized");
        return true;
    }
    
    memset(&g_procedural_system, 0, sizeof(ProceduralSystem));
    
    g_procedural_system.global_seed = global_seed;
    g_procedural_system.quality_level = quality_level;
    g_procedural_system.enable_caching = enable_caching;
    g_procedural_system.global_cache_size = cache_size;
    
    if (pthread_mutex_init(&g_procedural_system.proc_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize procedural system mutex");
        return false;
    }
    
    g_procedural_system.initialized = true;
    LOG_INFO("Procedural system initialized (seed: %d, quality: %.1f, caching: %s, cache: %u)",
             global_seed, quality_level, enable_caching ? "yes" : "no", cache_size);
    return true;
}

void procedural_system_shutdown(void) {
    if (!g_procedural_system.initialized)
        return;
    
    LOG_INFO("Shutting down procedural system");
    
    // Destroy all generators
    for (uint32_t i = 0; i < g_procedural_system.generator_count; i++) {
        if (g_procedural_system.generators[i]) {
            procedural_generator_destroy(g_procedural_system.generators[i]);
        }
    }
    
    pthread_mutex_destroy(&g_procedural_system.proc_mutex);
    
    memset(&g_procedural_system, 0, sizeof(ProceduralSystem));
    
    LOG_INFO("Procedural system shutdown complete");
}

ProceduralGenerator *procedural_generator_create(const char *name, ProceduralType type, int seed) {
    if (!g_procedural_system.initialized || !name) {
        LOG_ERROR("Procedural system not initialized or invalid name");
        return NULL;
    }
    
    pthread_mutex_lock(&g_procedural_system.proc_mutex);
    
    if (g_procedural_system.generator_count >= PROCEDURAL_TYPE_COUNT) {
        LOG_ERROR("Too many procedural generators");
        pthread_mutex_unlock(&g_procedural_system.proc_mutex);
        return NULL;
    }
    
    ProceduralGenerator *generator = calloc(1, sizeof(ProceduralGenerator));
    if (!generator) {
        LOG_ERROR("Failed to allocate procedural generator");
        pthread_mutex_unlock(&g_procedural_system.proc_mutex);
        return NULL;
    }
    
    strncpy(generator->name, name, sizeof(generator->name) - 1);
    generator->seed = seed;
    
    // Set default parameters based on type
    switch (type) {
        case PROCEDURAL_TYPE_TERRAIN:
            generator->frequency = 0.01f;
            generator->amplitude = 100.0f;
            generator->octaves = 6;
            generator->persistence = 0.5f;
            generator->lacunarity = 2.0f;
            generator->height_scale = 1.0f;
            generator->water_level = 0.0f;
            generator->mountain_height = 50.0f;
            generator->valley_depth = 20.0f;
            generator->erosion_factor = 0.3f;
            break;
            
        case PROCEDURAL_TYPE_VEGETATION:
            generator->frequency = 0.05f;
            generator->amplitude = 1.0f;
            generator->octaves = 4;
            generator->persistence = 0.6f;
            generator->lacunarity = 2.2f;
            generator->vegetation_density = 0.7f;
            generator->vegetation_height_variation = 2.0f;
            generator->vegetation_cluster_size = 5.0f;
            break;
            
        case PROCEDURAL_TYPE_STRUCTURES:
            generator->frequency = 0.02f;
            generator->amplitude = 1.0f;
            generator->octaves = 3;
            generator->persistence = 0.4f;
            generator->lacunarity = 2.5f;
            generator->structure_density = 0.1f;
            generator->structure_size_variation = 3.0f;
            generator->structure_complexity = 0.5f;
            break;
            
        default:
            // Default parameters
            generator->frequency = 0.01f;
            generator->amplitude = 1.0f;
            generator->octaves = 4;
            generator->persistence = 0.5f;
            generator->lacunarity = 2.0f;
            break;
    }
    
    generator->enable_caching = g_procedural_system.enable_caching;
    generator->cache_size = g_procedural_system.global_cache_size;
    generator->enable_multithreading = true;
    
    g_procedural_system.generators[g_procedural_system.generator_count++] = generator;
    
    pthread_mutex_unlock(&g_procedural_system.proc_mutex);
    
    LOG_INFO("Created procedural generator: %s (type: %d, seed: %d)", name, (int)type, seed);
    return generator;
}

void procedural_generator_destroy(ProceduralGenerator *generator) {
    if (!generator) return;
    
    pthread_mutex_lock(&g_procedural_system.proc_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_procedural_system.generator_count; i++) {
        if (g_procedural_system.generators[i] == generator) {
            g_procedural_system.generators[i] = g_procedural_system.generators[g_procedural_system.generator_count - 1];
            g_procedural_system.generator_count--;
            break;
        }
    }
    
    free(generator);
    
    pthread_mutex_unlock(&g_procedural_system.proc_mutex);
    
    LOG_DEBUG("Destroyed procedural generator: %s", generator->name);
}

void procedural_generator_set_noise_parameters(ProceduralGenerator *generator, float frequency, float amplitude,
                                               int octaves, float persistence, float lacunarity) {
    if (!generator) return;
    
    generator->frequency = frequency;
    generator->amplitude = amplitude;
    generator->octaves = octaves;
    generator->persistence = persistence;
    generator->lacunarity = lacunarity;
    
    LOG_DEBUG("Updated noise parameters: freq=%.3f, amp=%.1f, octaves=%d, persist=%.2f, lacunarity=%.2f",
             frequency, amplitude, octaves, persistence, lacunarity);
}

void procedural_generator_set_terrain_parameters(ProceduralGenerator *generator, float height_scale,
                                                float water_level, float mountain_height, float valley_depth, float erosion_factor) {
    if (!generator) return;
    
    generator->height_scale = height_scale;
    generator->water_level = water_level;
    generator->mountain_height = mountain_height;
    generator->valley_depth = valley_depth;
    generator->erosion_factor = erosion_factor;
    
    LOG_DEBUG("Updated terrain parameters: height_scale=%.1f, water_level=%.1f, mountain=%.1f, valley=%.1f, erosion=%.2f",
             height_scale, water_level, mountain_height, valley_depth, erosion_factor);
}

void procedural_generator_set_vegetation_parameters(ProceduralGenerator *generator, float density,
                                                   float height_variation, float cluster_size) {
    if (!generator) return;
    
    generator->vegetation_density = density;
    generator->vegetation_height_variation = height_variation;
    generator->vegetation_cluster_size = cluster_size;
    
    LOG_DEBUG("Updated vegetation parameters: density=%.2f, height_variation=%.1f, cluster_size=%.1f",
             density, height_variation, cluster_size);
}

void procedural_generator_set_structure_parameters(ProceduralGenerator *generator, float density,
                                                   float size_variation, float complexity) {
    if (!generator) return;
    
    generator->structure_density = density;
    generator->structure_size_variation = size_variation;
    generator->structure_complexity = complexity;
    
    LOG_DEBUG("Updated structure parameters: density=%.2f, size_variation=%.1f, complexity=%.2f",
             density, size_variation, complexity);
}

void procedural_generator_generate_chunk(ProceduralGenerator *generator, ProceduralChunk *chunk,
                                       float chunk_x, float chunk_y, float chunk_z, uint32_t chunk_size) {
    if (!generator || !chunk) return;
    
    // Clear chunk
    memset(chunk, 0, sizeof(ProceduralChunk) * chunk_size * chunk_size * chunk_size);
    
    // Generate based on type
    generate_terrain_chunk(generator, chunk, chunk_x, chunk_y, chunk_z, chunk_size);
    generate_vegetation_chunk(generator, chunk, chunk_x, chunk_y, chunk_z, chunk_size);
    generate_structure_chunk(generator, chunk, chunk_x, chunk_y, chunk_z, chunk_size);
    
    g_procedural_system.total_chunks_generated++;
    g_procedural_system.total_generation_time_ms += generator->generation_time_ms;
    g_procedural_system.average_generation_time_ms = g_procedural_system.total_generation_time_ms / g_procedural_system.total_chunks_generated;
    
    LOG_DEBUG("Generated chunk (%.1f, %.1f, %.1f): %.2f ms", chunk_x, chunk_y, chunk_z, generator->generation_time_ms);
}

float procedural_generator_sample_noise(ProceduralGenerator *generator, float x, float y, float z) {
    if (!generator) return 0.0f;
    
    return fractal_noise(x * generator->frequency, y * generator->frequency, z * generator->frequency,
                         generator->octaves, generator->persistence, generator->lacunarity, generator->seed);
}

void procedural_generator_get_stats(ProceduralGenerator *generator, uint64_t *chunks_generated,
                                   uint64_t *vertices_generated, uint64_t *triangles_generated, float *generation_time) {
    if (!generator) return;
    
    if (chunks_generated) *chunks_generated = generator->chunks_generated;
    if (vertices_generated) *vertices_generated = generator->vertices_generated;
    if (triangles_generated) *triangles_generated = generator->triangles_generated;
    if (generation_time) *generation_time = generator->generation_time_ms;
    
    LOG_DEBUG("Generator stats: chunks=%llu, vertices=%llu, triangles=%llu, time=%.2f ms",
             *chunks_generated, *vertices_generated, *triangles_generated, *generation_time);
}

void procedural_system_get_stats(uint64_t *total_chunks, float *average_time, uint64_t *total_time) {
    if (!g_procedural_system.initialized) return;
    
    if (total_chunks) *total_chunks = g_procedural_system.total_chunks_generated;
    if (average_time) *average_time = g_procedural_system.average_generation_time_ms;
    if (total_time) *total_time = g_procedural_system.total_generation_time_ms;
    
    LOG_DEBUG("System stats: total_chunks=%llu, avg_time=%.2f ms, total_time=%.2f ms",
             *total_chunks, *average_time, *total_time);
}

bool procedural_system_is_initialized(void) {
    return g_procedural_system.initialized;
}
