/**
 * =================================================================================================
 *                              TERRAIN SCULPTING TOOLS
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Complete terrain editing tools with brushes, stamps, and procedural
 * operators.
 *
 * =================================================================================================
 */

#ifndef TERRAIN_TOOLS_H
#define TERRAIN_TOOLS_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    BRUSH TYPES
 * =================================================================================================
 */

typedef enum BrushType {
  BRUSH_TYPE_RAISE,
  BRUSH_TYPE_LOWER,
  BRUSH_TYPE_FLATTEN,
  BRUSH_TYPE_SMOOTH,
  BRUSH_TYPE_SHARPEN,
  BRUSH_TYPE_NOISE,
  BRUSH_TYPE_STAMP,
  BRUSH_TYPE_CLONE,
  BRUSH_TYPE_EROSION,
  BRUSH_TYPE_HYDRO,
  BRUSH_TYPE_THERMAL,
  BRUSH_TYPE_PAINT_TEXTURE,
  BRUSH_TYPE_PAINT_FOLIAGE,
  BRUSH_TYPE_PAINT_MASK,
} BrushType;

typedef struct BrushFalloff {
  float inner_radius;
  float outer_radius;
  float curve_exponent;
  bool use_texture;
  uint32_t falloff_texture;
} BrushFalloff;

typedef struct TerrainBrush {
  BrushType type;
  char name[32];

  // Size
  float radius;
  BrushFalloff falloff;

  // Strength
  float strength;
  float opacity;

  // Behavior
  float spacing;
  float jitter_position;
  float jitter_rotation;
  float jitter_scale;
  bool use_pen_pressure;
  bool use_pen_tilt;

  // Stamp settings
  uint32_t stamp_texture;
  float stamp_rotation;
  float stamp_height;
  bool stamp_additive;

  // Clone settings
  float clone_source[3];
  bool clone_source_set;

  // Paint settings
  uint32_t paint_layer;
  float paint_value;

  // Noise settings
  float noise_scale;
  float noise_octaves;
  float noise_persistence;
} TerrainBrush;

TerrainBrush *brush_create(const char *name, BrushType type);
void brush_apply(TerrainBrush *brush, float x, float y, float dt,
                 void *terrain);
void brush_raise(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor);
void brush_lower(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor);
void brush_flatten(TerrainBrush *brush, void *terrain, float x, float y,
                   float height);
void brush_smooth(TerrainBrush *brush, void *terrain, float x, float y,
                  float factor);
void brush_sharpen(TerrainBrush *brush, void *terrain, float x, float y,
                   float factor);
void brush_noise(TerrainBrush *brush, void *terrain, float x, float y,
                 float factor);
void brush_stamp(TerrainBrush *brush, void *terrain, float x, float y);
void brush_clone(TerrainBrush *brush, void *terrain, float x, float y);
float brush_calculate_falloff(TerrainBrush *brush, float dist);
void brush_preview_render(TerrainBrush *brush, float x, float y, float z);
void brush_stroke_start(TerrainBrush *brush, float x, float y);
void brush_stroke_update(TerrainBrush *brush, float x, float y, float pressure);
void brush_stroke_end(TerrainBrush *brush);
void brush_undo_record(TerrainBrush *brush, void *terrain);

/* =================================================================================================
 *                                    TEXTURE PAINTING
 * =================================================================================================
 */

typedef struct TerrainLayer {
  uint32_t id;
  char name[32];
  uint32_t albedo_texture;
  uint32_t normal_texture;
  uint32_t mask_texture;
  float tiling[2];
  float offset[2];
  float metallic;
  float smoothness;
  float height_blend;
} TerrainLayer;

typedef struct TerrainPainter {
  TerrainLayer *layers;
  uint32_t layer_count;
  uint32_t max_layers;

  uint32_t active_layer;
  uint32_t splatmap_resolution;
  uint32_t *splatmaps;
  uint32_t splatmap_count;

  // Auto-painting rules
  bool auto_paint_enabled;
  float slope_to_layer[8];
  float altitude_to_layer[8][2]; // min/max altitude per layer
} TerrainPainter;

void painter_add_layer(TerrainPainter *painter, TerrainLayer *layer);
void painter_remove_layer(TerrainPainter *painter, uint32_t layer_id);
void painter_paint(TerrainPainter *painter, float x, float y, uint32_t layer,
                   float opacity);
void painter_blend(TerrainPainter *painter, float x, float y, float radius);
void painter_auto_paint(TerrainPainter *painter, void *terrain);
void painter_generate_splatmaps(TerrainPainter *painter, void *terrain);
void painter_triplanar_sample(TerrainPainter *painter, float *pos,
                              float *normal, float *color);
void painter_height_blend(TerrainPainter *painter, float h1, float h2,
                          float blend);

/* =================================================================================================
 *                                    EROSION SIMULATION
 * =================================================================================================
 */

typedef struct ErosionSettings {
  // Hydraulic erosion
  uint32_t hydraulic_iterations;
  float rain_amount;
  float evaporation_rate;
  float sediment_capacity;
  float erosion_strength;
  float deposition_strength;
  float min_slope;

  // Thermal erosion
  uint32_t thermal_iterations;
  float talus_angle;
  float thermal_strength;

  // Wind erosion
  bool wind_enabled;
  float wind_direction[2];
  float wind_strength;
  float sand_threshold;
} ErosionSettings;

typedef struct ErosionSimulation {
  ErosionSettings settings;

  float *heightmap;
  float *water_map;
  float *sediment_map;
  float *velocity_map_x;
  float *velocity_map_y;
  float *flux_map;

  uint32_t resolution;
  bool is_simulating;
  uint32_t current_iteration;
} ErosionSimulation;

void erosion_init(ErosionSimulation *sim, uint32_t resolution);
void erosion_shutdown(ErosionSimulation *sim);
void erosion_simulate_hydraulic(ErosionSimulation *sim, int iterations);
void erosion_simulate_thermal(ErosionSimulation *sim, int iterations);
void erosion_simulate_wind(ErosionSimulation *sim, int iterations);
void erosion_add_water(ErosionSimulation *sim, float amount);
void erosion_calculate_outflow(ErosionSimulation *sim);
void erosion_transport_sediment(ErosionSimulation *sim, float dt);
void erosion_evaporate(ErosionSimulation *sim, float amount);
void erosion_deposit(ErosionSimulation *sim);
void erosion_gpu_accelerate(ErosionSimulation *sim);
void erosion_preview(ErosionSimulation *sim);

/* =================================================================================================
 *                                    ROAD/PATH TOOLS
 * =================================================================================================
 */

typedef struct RoadSpline {
  float *control_points;
  uint32_t point_count;
  float width;
  float falloff_width;
  float surface_offset;
  bool carve_terrain;
  bool smooth_edges;
  uint32_t texture_layer;
} RoadSpline;

RoadSpline *road_create(void);
void road_add_point(RoadSpline *road, float x, float y, float z);
void road_remove_point(RoadSpline *road, int index);
void road_carve_terrain(RoadSpline *road, void *terrain);
void road_generate_mesh(RoadSpline *road);
void road_paint_texture(RoadSpline *road, void *terrain);
bool road_intersect(RoadSpline *r1, RoadSpline *r2, float *point);

/* =================================================================================================
 *                                    RIVER/WATER TOOLS
 * =================================================================================================
 */

typedef struct RiverSpline {
  float *control_points;
  uint32_t point_count;
  float *width_at_point;
  float *depth_at_point;
  float flow_speed;
  float foam_amount;
  bool carve_terrain;
  float carve_depth;
  bool add_vegetation;
} RiverSpline;

RiverSpline *river_create(void);
void river_carve(RiverSpline *river, void *terrain);
void river_fill_water(RiverSpline *river, float height);
void river_flow_simulation(RiverSpline *river);
void lake_create(float x, float y, float radius);
void lake_fill(void *lake, float height);
void waterfall_create(RiverSpline *river, int point_index);

/* =================================================================================================
 *                                    FOLIAGE PAINTING
 * =================================================================================================
 */

typedef struct FoliageType {
  uint32_t id;
  char name[32];
  char mesh_path[128];
  float scale_min;
  float scale_max;
  float random_rotation;
  bool align_to_surface;
  float slope_max;
  float altitude_min;
  float altitude_max;
  float density;
  bool cast_shadows;
  bool receive_shadows;
  float cull_distance;
} FoliageType;

typedef struct FoliagePainter {
  FoliageType *types;
  uint32_t type_count;

  uint32_t active_type;
  float brush_radius;
  float brush_density;
  float brush_spacing;

  bool erase_mode;
  bool random_rotation;
  bool align_to_terrain;
} FoliagePainter;

void foliage_add_type(FoliagePainter *fp, FoliageType *type);
void foliage_remove_type(FoliagePainter *fp, uint32_t type_id);
void foliage_paint(FoliagePainter *fp, float x, float y, float density);
void foliage_erase(FoliagePainter *fp, float x, float y, float radius);
void foliage_scatter(FoliagePainter *fp, void *terrain, int count);
void foliage_instance_buffer(FoliagePainter *fp);
void foliage_culling(FoliagePainter *fp, void *camera);
void foliage_lod(FoliagePainter *fp, float dist);

/* =================================================================================================
 *                                    TERRAIN TOOL MANAGER
 * =================================================================================================
 */

typedef struct TerrainToolManager {
  TerrainBrush *brushes;
  uint32_t brush_count;
  uint32_t active_brush;

  TerrainPainter painter;
  ErosionSimulation erosion;
  FoliagePainter foliage;

  RoadSpline *roads;
  uint32_t road_count;

  RiverSpline *rivers;
  uint32_t river_count;

  // Undo system
  void **undo_stack;
  uint32_t undo_count;
  uint32_t undo_index;
  uint32_t max_undo_levels;

  // Preview
  bool show_preview;
  uint32_t preview_mesh;
} TerrainToolManager;

void tool_manager_init(TerrainToolManager *mgr);
void tool_manager_shutdown(TerrainToolManager *mgr);
void tool_manager_update(TerrainToolManager *mgr, float dt);
void tool_manager_apply(TerrainToolManager *mgr);
void tool_manager_undo(TerrainToolManager *mgr);
void tool_manager_redo(TerrainToolManager *mgr);
void tool_manager_serialize(TerrainToolManager *mgr, const char *path);
void tool_manager_deserialize(TerrainToolManager *mgr, const char *path);

#endif // TERRAIN_TOOLS_H
