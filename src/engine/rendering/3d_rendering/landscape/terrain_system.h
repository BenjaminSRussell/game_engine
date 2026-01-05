/**
 * @file terrain_system.h
 * @brief Terrain rendering system for large-scale landscapes
 * @details High-performance terrain rendering with LOD, streaming, dynamic lighting, and vegetation
 *
 * Supports Valley of the Ancients-style open world terrain with:
 * - Seamless LOD transitions
 * - Streaming for infinite worlds
 * - Dynamic lighting and shadows
 * - Vegetation placement
 * - Deformation and editing
 */

#ifndef RENDER_TERRAIN_SYSTEM_H
#define RENDER_TERRAIN_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct RenderTerrainManager RenderTerrainManager;
typedef struct TerrainChunk TerrainChunk;

/**
 * @brief Terrain format
 */
typedef enum {
    TERRAIN_FORMAT_HEIGHTMAP = 0,
    TERRAIN_FORMAT_VOXEL = 1,
    TERRAIN_FORMAT_MESH = 2,
} TerrainFormat;

/**
 * @brief Terrain material layer
 */
typedef struct {
    uint32_t material_id;
    uint32_t detail_texture_id;
    float blend_strength;
    float uv_scale;
} TerrainMaterialLayer;

/**
 * @brief Terrain chunk configuration
 */
typedef struct {
    uint32_t chunk_size;
    uint32_t lod_levels;
    float chunk_extent;
    uint32_t max_material_layers;
} TerrainConfig;

/**
 * @brief Create terrain manager
 * @param[in] config Terrain configuration
 * @param[in] max_chunks Maximum terrain chunks
 * @return Pointer to manager, NULL on failure
 */
RenderTerrainManager* render_terrain_create(const TerrainConfig* config, uint32_t max_chunks);

/**
 * @brief Destroy terrain manager
 * @param[in] manager Manager to destroy
 */
void render_terrain_destroy(RenderTerrainManager* manager);

/**
 * @brief Create terrain chunk
 * @param[in] manager Terrain manager
 * @param[in] chunk_x Grid X coordinate
 * @param[in] chunk_y Grid Y coordinate
 * @param[in] height_data Height data for chunk
 * @return Chunk ID, 0 on failure
 */
uint32_t render_terrain_create_chunk(RenderTerrainManager* manager,
                                     int32_t chunk_x, int32_t chunk_y,
                                     const float* height_data);

/**
 * @brief Set terrain material layer
 * @param[in] manager Terrain manager
 * @param[in] chunk_id Chunk ID
 * @param[in] layer_index Material layer index
 * @param[in] layer Material layer data
 * @return true on success
 */
bool render_terrain_set_material_layer(RenderTerrainManager* manager,
                                       uint32_t chunk_id,
                                       uint32_t layer_index,
                                       const TerrainMaterialLayer* layer);

/**
 * @brief Update terrain chunk heights
 * @param[in] manager Terrain manager
 * @param[in] chunk_id Chunk ID
 * @param[in] height_data New height data
 * @return true on success
 */
bool render_terrain_update_heights(RenderTerrainManager* manager,
                                   uint32_t chunk_id,
                                   const float* height_data);

/**
 * @brief Get height at position
 * @param[in] manager Terrain manager
 * @param[in] x World X position
 * @param[in] y World Y position
 * @param[out] height Interpolated height
 * @return true if position is within terrain
 */
bool render_terrain_sample_height(RenderTerrainManager* manager,
                                  float x, float y,
                                  float* height);

/**
 * @brief Render all visible terrain chunks
 * @param[in] manager Terrain manager
 * @param[in] camera_pos Camera position for LOD calculation
 * @return true on success
 */
bool render_terrain_render(RenderTerrainManager* manager, const float* camera_pos);

/**
 * @brief Stream terrain chunks in/out
 * @param[in] manager Terrain manager
 * @param[in] center_x Center X position for streaming
 * @param[in] center_y Center Y position for streaming
 * @param[in] radius Streaming radius
 * @return true on success
 */
bool render_terrain_stream(RenderTerrainManager* manager,
                          float center_x, float center_y, float radius);

/**
 * @brief Get terrain statistics
 * @param[in] manager Terrain manager
 * @param[out] loaded_chunks Number of loaded chunks
 * @param[out] total_triangles Total triangles rendered
 * @param[out] memory_used Memory used in MB
 */
void render_terrain_get_stats(RenderTerrainManager* manager,
                             uint32_t* loaded_chunks,
                             uint32_t* total_triangles,
                             float* memory_used);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_TERRAIN_SYSTEM_H */
