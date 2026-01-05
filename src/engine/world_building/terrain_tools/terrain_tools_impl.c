/**
 * =================================================================================================
 *                              TERRAIN TOOLS IMPLEMENTATION
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 */

#include "terrain_tools.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    BRUSHES
 * =================================================================================================
 */

TerrainBrush *brush_create(const char *name, BrushType type) {
  TerrainBrush *brush = calloc(1, sizeof(TerrainBrush));
  strncpy(brush->name, name, 31);
  brush->type = type;
  brush->radius = 10.0f;
  brush->strength = 1.0f;
  return brush;
}

void brush_apply(TerrainBrush *brush, float x, float y, float dt,
                 void *terrain) {}

// Brush Actions
void brush_raise(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor) {}
void brush_lower(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor) {}
void brush_flatten(TerrainBrush *brush, void *terrain, float x, float y,
                   float height) {}
void brush_smooth(TerrainBrush *brush, void *terrain, float x, float y,
                  float factor) {}
void brush_sharpen(TerrainBrush *brush, void *terrain, float x, float y,
                   float factor) {}
void brush_noise(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor) {}
void brush_stamp(TerrainBrush *brush, void *terrain, float x, float y) {}
void brush_clone(TerrainBrush *brush, void *terrain, float x, float y) {}

// Brush Utils
float brush_calculate_falloff(TerrainBrush *brush, float dist) { return 1.0f; }
void brush_preview_render(TerrainBrush *brush, float x, float y, float z) {}
void brush_stroke_start(TerrainBrush *brush, float x, float y) {}
void brush_stroke_update(TerrainBrush *brush, float x, float y,
                         float pressure) {}
void brush_stroke_end(TerrainBrush *brush) {}
void brush_undo_record(TerrainBrush *brush, void *terrain) {}

/* =================================================================================================
 *                                    TEXTURE PAINTING
 * =================================================================================================
 */

void painter_add_layer(TerrainPainter *painter, TerrainLayer *layer) {}
void painter_remove_layer(TerrainPainter *painter, uint32_t layer_id) {}
void painter_paint(TerrainPainter *painter, float x, float y, uint32_t layer,
                   float opacity) {}
void painter_blend(TerrainPainter *painter, float x, float y, float radius) {}
void painter_auto_paint(TerrainPainter *painter, void *terrain) {}
void painter_generate_splatmaps(TerrainPainter *painter, void *terrain) {}
void painter_triplanar_sample(TerrainPainter *painter, float *pos,
                              float *normal, float *color) {}
void painter_height_blend(TerrainPainter *painter, float h1, float h2,
                          float blend) {}

/* =================================================================================================
 *                                    EROSION SIMULATION
 * =================================================================================================
 */

void erosion_init(ErosionSimulation *sim, uint32_t resolution) {
  sim->resolution = resolution;
}
void erosion_shutdown(ErosionSimulation *sim) {}
void erosion_simulate_hydraulic(ErosionSimulation *sim, int iterations) {}
void erosion_simulate_thermal(ErosionSimulation *sim, int iterations) {}
void erosion_simulate_wind(ErosionSimulation *sim, int iterations) {}
void erosion_add_water(ErosionSimulation *sim, float amount) {}
void erosion_calculate_outflow(ErosionSimulation *sim) {}
void erosion_transport_sediment(ErosionSimulation *sim, float dt) {}
void erosion_evaporate(ErosionSimulation *sim, float amount) {}
void erosion_deposit(ErosionSimulation *sim) {}
void erosion_gpu_accelerate(ErosionSimulation *sim) {}
void erosion_preview(ErosionSimulation *sim) {}

/* =================================================================================================
 *                                    ROAD/PATH TOOLS
 * =================================================================================================
 */

RoadSpline *road_create(void) { return calloc(1, sizeof(RoadSpline)); }
void road_add_point(RoadSpline *road, float x, float y, float z) {}
void road_remove_point(RoadSpline *road, int index) {}
void road_carve_terrain(RoadSpline *road, void *terrain) {}
void road_generate_mesh(RoadSpline *road) {}
void road_paint_texture(RoadSpline *road, void *terrain) {}
bool road_intersect(RoadSpline *r1, RoadSpline *r2, float *point) {
  return false;
}

/* =================================================================================================
 *                                    RIVER/WATER TOOLS
 * =================================================================================================
 */

RiverSpline *river_create(void) { return calloc(1, sizeof(RiverSpline)); }
void river_carve(RiverSpline *river, void *terrain) {}
void river_fill_water(RiverSpline *river, float height) {}
void river_flow_simulation(RiverSpline *river) {}
void lake_create(float x, float y, float radius) {}
void lake_fill(void *lake, float height) {}
void waterfall_create(RiverSpline *river, int point_index) {}

/* =================================================================================================
 *                                    FOLIAGE PAINTING
 * =================================================================================================
 */

void foliage_add_type(FoliagePainter *fp, FoliageType *type) {}
void foliage_remove_type(FoliagePainter *fp, uint32_t type_id) {}
void foliage_paint(FoliagePainter *fp, float x, float y, float density) {}
void foliage_erase(FoliagePainter *fp, float x, float y, float radius) {}
void foliage_scatter(FoliagePainter *fp, void *terrain, int count) {}
void foliage_instance_buffer(FoliagePainter *fp) {}
void foliage_culling(FoliagePainter *fp, void *camera) {}
void foliage_lod(FoliagePainter *fp, float dist) {}

/* =================================================================================================
 *                                    TERRAIN TOOL MANAGER
 * =================================================================================================
 */

void tool_manager_init(TerrainToolManager *mgr) {}
void tool_manager_shutdown(TerrainToolManager *mgr) {}
void tool_manager_update(TerrainToolManager *mgr, float dt) {}
void tool_manager_apply(TerrainToolManager *mgr) {}
void tool_manager_undo(TerrainToolManager *mgr) {}
void tool_manager_redo(TerrainToolManager *mgr) {}
void tool_manager_serialize(TerrainToolManager *mgr, const char *path) {}
void tool_manager_deserialize(TerrainToolManager *mgr, const char *path) {}
