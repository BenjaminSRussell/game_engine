#ifndef NAVMESH_H
#define NAVMESH_H

#include <core/types.h>
#include "engine/include/core/logger.h"
#include <math/vec3.h>
#include <math/vec2.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Navmesh System - Navigation mesh generation and pathfinding

// Forward declarations
typedef struct Navmesh Navmesh;
typedef struct NavmeshBuilder NavmeshBuilder;
typedef struct NavmeshQuery NavmeshQuery;
typedef struct NavPath NavPath;

// Constants
#define NAVMESH_MAX_VOXELS 1000000
#define NAVMESH_MAX_POLYS 50000
#define NAVMESH_MAX_VERTS 150000
#define NAVMESH_MAX_NEIGHBORS 8
#define NAVMESH_MAX_PATH_LENGTH 1024

// Voxel representation
typedef struct {
    int x, y, z;
    unsigned char walkable;
    unsigned char region_id;
    float height;
} NavVoxel;

// Polygon representation
typedef struct {
    unsigned short verts[6];      // Max 6 verts per polygon
    unsigned short vert_count;
    unsigned short neighbors[NAVMESH_MAX_NEIGHBORS];
    unsigned short neighbor_count;
    unsigned short region_id;
    Vec3 center;
    float area;
    unsigned char flags;
} NavPoly;

// Vertex representation
typedef struct {
    Vec3 pos;
    unsigned short poly_ref;
    unsigned short edge_index;
} NavVert;

// Path representation
struct NavPath {
    Vec3 points[NAVMESH_MAX_PATH_LENGTH];
    int point_count;
    float total_length;
    unsigned short poly_refs[NAVMESH_MAX_PATH_LENGTH];
    int poly_count;
    int current_waypoint;
};

// Navigation mesh
struct Navmesh {
    // Voxel data
    unsigned char* voxels;
    int voxel_count;
    int voxel_width, voxel_height, voxel_depth;
    Vec3 voxel_origin;
    float voxel_size;
    
    // Polygon data
    NavPoly* polys;
    int poly_count;
    
    // Vertex data
    NavVert* verts;
    int vert_count;
    
    // Regions
    int region_count;
    
    // Bounding box
    Vec3 bounds_min, bounds_max;
    
    // Generation settings
    float max_slope;
    float max_step_height;
    float agent_height;
    float agent_radius;
    
    bool built;
};

// Navmesh builder
struct NavmeshBuilder {
    Navmesh* navmesh;
    
    // Temporary data for building
    unsigned char* heightfield;
    int* distance_field;
    unsigned char* region_field;
    
    // Builder settings
    float cell_size;
    float cell_height;
    float agent_height;
    float agent_radius;
    float max_slope;
    float max_step_height;
    float detail_sample_distance;
    float detail_sample_max_error;
    
    // Region settings
    int min_region_size;
    int merge_region_size;
    
    // Filtering
    bool filter_low_hanging_obstacles;
    bool filter_ledge_spans;
    bool filter_walkable_low_height_spans;
    
    bool initialized;
};

// Navmesh query system
struct NavmeshQuery {
    Navmesh* navmesh;
    
    // Query cache
    unsigned short last_poly_ref;
    Vec3 last_pos;
    
    // Raycast data
    Vec3 raycast_start;
    Vec3 raycast_end;
    float raycast_t;
    unsigned short hit_poly_ref;
    
    bool initialized;
};

// Core navmesh functions
Navmesh* navmesh_create(void);
void navmesh_destroy(Navmesh* mesh);

NavmeshBuilder* navmesh_builder_create(Navmesh* mesh);
void navmesh_builder_destroy(NavmeshBuilder* builder);

bool navmesh_builder_init(NavmeshBuilder* builder, const Vec3* bounds_min, const Vec3* bounds_max);
bool navmesh_build_from_geometry(NavmeshBuilder* builder, const Vec3* vertices, int vert_count, 
                                 const unsigned int* indices, int index_count);

// Voxelization functions (TASK_1400)
bool navmesh_voxelize_geometry(NavmeshBuilder* builder, const Vec3* vertices, int vert_count,
                               const unsigned int* indices, int index_count);
void navmesh_create_heightfield(NavmeshBuilder* builder);
bool navmesh_rasterize_triangles(NavmeshBuilder* builder, const Vec3* vertices, int vert_count,
                                 const unsigned int* indices, int index_count);

// Filtering functions (TASK_1401)
void navmesh_filter_walkable_surfaces(NavmeshBuilder* builder);
bool navmesh_is_walkable_slope(float normal_y, float max_slope);
bool navmesh_is_step_height_valid(float height_diff, float max_step_height);

// Region partitioning functions (TASK_1402)
void navmesh_build_regions(NavmeshBuilder* builder);
void navmesh_watershed_partitioning(NavmeshBuilder* builder);
void navmesh_monotone_partitioning(NavmeshBuilder* builder);
void navmesh_distance_field(NavmeshBuilder* builder);
void navmesh_build_compact_heightfield(NavmeshBuilder* builder);

// Polygon mesh generation (TASK_1403)
bool navmesh_build_polygon_mesh(NavmeshBuilder* builder);
void navmesh_trace_region_boundaries(NavmeshBuilder* builder);
void navmesh_build_contours(NavmeshBuilder* builder);
void navmesh_simplify_contours(NavmeshBuilder* builder);
void navmesh_build_poly_mesh(NavmeshBuilder* builder);

// Detail mesh generation
void navmesh_build_detail_mesh(NavmeshBuilder* builder);

// Pathfinding functions (TASK_1410)
NavmeshQuery* navmesh_query_create(Navmesh* mesh);
void navmesh_query_destroy(NavmeshQuery* query);

bool navmesh_find_path(NavmeshQuery* query, const Vec3* start, const Vec3* end, NavPath* path);
bool navmesh_find_nearest_poly(NavmeshQuery* query, const Vec3* pos, const Vec3* extents, 
                               unsigned short* poly_ref);
bool navmesh_ray_cast(NavmeshQuery* query, const Vec3* start, const Vec3* end, 
                      float* t, unsigned short* hit_poly_ref);

// Path smoothing (TASK_1411)
bool navmesh_smooth_path(NavmeshQuery* query, NavPath* path);
void navmesh_string_pulling(NavmeshQuery* query, const Vec3* start, const Vec3* end,
                            const unsigned short* poly_refs, int poly_count, NavPath* path);

// Dynamic updates (TASK_1420)
bool navmesh_update_tile(Navmesh* mesh, int tile_x, int tile_z);
bool navmesh_add_obstacle(Navmesh* mesh, const Vec3* pos, float radius, float height);
bool navmesh_remove_obstacle(Navmesh* mesh, uint32_t obstacle_id);

// Off-mesh connections
bool navmesh_add_offmesh_link(Navmesh* mesh, const Vec3* start, const Vec3* end, 
                              float radius, unsigned char bidirectional, unsigned char area);

// Crowd management
typedef struct {
    Vec3 position;
    Vec3 velocity;
    unsigned short target_poly;
    float max_speed;
    float radius;
    float height;
    uint32_t agent_id;
} NavAgent;

bool navmesh_add_crowd_agent(Navmesh* mesh, NavAgent* agent);
void navmesh_update_crowd(Navmesh* mesh, float delta_time);
void navmesh_crowd_steering(Navmesh* mesh);

// Debugging functions
void navmesh_debug_draw_mesh(Navmesh* mesh);
void navmesh_debug_draw_path(Navmesh* mesh, const NavPath* path);
void navmesh_debug_draw_voxels(Navmesh* mesh);
void navmesh_debug_draw_regions(Navmesh* mesh);

// Utility functions
Vec3 navmesh_poly_center(const Navmesh* mesh, unsigned short poly_ref);
bool navmesh_is_poly_valid(const Navmesh* mesh, unsigned short poly_ref);
float navmesh_poly_distance(const Navmesh* mesh, unsigned short poly1, unsigned short poly2);
void navmesh_get_poly_bounds(const Navmesh* mesh, unsigned short poly_ref, Vec3* min_bound, Vec3* max_bound);

// Serialization
bool navmesh_save(const Navmesh* mesh, const char* filename);
Navmesh* navmesh_load(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // NAVMESH_H
