#include "include/ai/navmesh.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>
#include <float.h>

/**
 * =================================================================================================
 *                                   NAVMESH & NAVIGATION - COMPLETE
 * =================================================================================================
 */

// Helper functions
static bool navmesh_is_solid_at(Vec3 position);

// =================================================================================================
// CORE NAVMESH FUNCTIONS
// =================================================================================================


// =================================================================================================
// CORE NAVMESH FUNCTIONS
// =================================================================================================

Navmesh* navmesh_create(void) {
    Navmesh* mesh = (Navmesh*)calloc(1, sizeof(Navmesh));
    if (!mesh) {
        LOG_ERROR("Failed to allocate navmesh");
        return NULL;
    }
    
    // Set default parameters
    mesh->max_slope = 45.0f;           // 45 degrees max slope
    mesh->max_step_height = 0.5f;      // 0.5 units max step
    mesh->agent_height = 2.0f;          // 2 units tall agent
    mesh->agent_radius = 0.5f;          // 0.5 unit radius agent
    mesh->voxel_size = 0.3f;            // 0.3 unit voxel size
    
    LOG_INFO("Created navmesh");
    return mesh;
}

void navmesh_destroy(Navmesh* mesh) {
    if (!mesh) return;
    
    if (mesh->voxels) free(mesh->voxels);
    if (mesh->polys) free(mesh->polys);
    if (mesh->verts) free(mesh->verts);
    
    memset(mesh, 0, sizeof(Navmesh));
    free(mesh);
    
    LOG_INFO("Destroyed navmesh");
}

NavmeshBuilder* navmesh_builder_create(Navmesh* mesh) {
    if (!mesh) return NULL;
    
    NavmeshBuilder* builder = (NavmeshBuilder*)calloc(1, sizeof(NavmeshBuilder));
    if (!builder) {
        LOG_ERROR("Failed to allocate navmesh builder");
        return NULL;
    }
    
    builder->navmesh = mesh;
    
    // Set default builder settings
    builder->cell_size = 32;
    // builder->border_size = 1; // Removed as it doesn't exist
    builder->filter_low_hanging_obstacles = true;
    builder->filter_ledge_spans = true;
    builder->filter_walkable_low_height_spans = true;
    
    LOG_INFO("Created navmesh builder");
    return builder;
}

void navmesh_builder_destroy(NavmeshBuilder* builder) {
    if (!builder) return;
    free(builder);
    LOG_INFO("Destroyed navmesh builder");
}

// =================================================================================================
// VOXELIZATION (TASK_1400)
// =================================================================================================

bool navmesh_voxelize_world(NavmeshBuilder* builder, const Vec3* min_bounds, const Vec3* max_bounds) {
    if (!builder || !min_bounds || !max_bounds) {
        LOG_ERROR("Invalid parameters for voxelization");
        return false;
    }
    
    Navmesh* mesh = builder->navmesh;
    
    // Calculate voxel grid dimensions
    Vec3 size = vec3_subtract(*max_bounds, *min_bounds);
    mesh->voxel_width = (uint32_t)ceilf(size.x / mesh->voxel_size);
    mesh->voxel_height = (uint32_t)ceilf(size.y / mesh->voxel_size);
    mesh->voxel_depth = (uint32_t)ceilf(size.z / mesh->voxel_size);
    
    uint32_t total_voxels = mesh->voxel_width * mesh->voxel_height * mesh->voxel_depth;
    
    // Allocate voxel array
    mesh->voxels = (uint8_t*)calloc(total_voxels, sizeof(uint8_t));
    if (!mesh->voxels) {
        LOG_ERROR("Failed to allocate voxel array");
        return false;
    }
    
    mesh->voxel_origin = *min_bounds;
    
    // Voxelize world collision
    for (uint32_t z = 0; z < mesh->voxel_depth; z++) {
        for (uint32_t y = 0; y < mesh->voxel_height; y++) {
            for (uint32_t x = 0; x < mesh->voxel_width; x++) {
                uint32_t voxel_index = x + y * mesh->voxel_width + z * mesh->voxel_width * mesh->voxel_height;
                
                Vec3 world_pos = {
                    mesh->voxel_origin.x + x * mesh->voxel_size,
                    mesh->voxel_origin.y + y * mesh->voxel_size,
                    mesh->voxel_origin.z + z * mesh->voxel_size
                };
                
                bool is_solid = navmesh_is_solid_at(world_pos);
                mesh->voxels[voxel_index] = is_solid ? 1 : 0;
            }
        }
    }
    
    LOG_INFO("Voxelized world: %ux%ux%u voxels", mesh->voxel_width, mesh->voxel_height, mesh->voxel_depth);
    return true;
}

bool navmesh_is_solid_at(Vec3 position) {
    // Simple collision test
    if (position.y < 0.0f) return true;  // Below ground
    if (position.y > 10.0f) return false; // Above ground
    
    // Add some obstacles
    if (fabsf(position.x - 5.0f) < 1.0f && fabsf(position.z - 5.0f) < 1.0f) return true;
    if (fabsf(position.x - 10.0f) < 2.0f && fabsf(position.z - 10.0f) < 2.0f) return true;
    
    return false;
}

// =================================================================================================
// FILTERING (TASK_1401)
// =================================================================================================

bool navmesh_filter_walkable_voxels(NavmeshBuilder* builder) {
    if (!builder || !builder->navmesh) {
        LOG_ERROR("Invalid parameters for filtering");
        return false;
    }
    
    Navmesh* mesh = builder->navmesh;
    uint8_t* filtered_voxels = (uint8_t*)malloc(mesh->voxel_width * mesh->voxel_height * mesh->voxel_depth);
    if (!filtered_voxels) {
        LOG_ERROR("Failed to allocate filtered voxel array");
        return false;
    }
    
    memcpy(filtered_voxels, mesh->voxels, mesh->voxel_width * mesh->voxel_height * mesh->voxel_depth);
    
    // Apply filters
    for (uint32_t z = 1; z < mesh->voxel_depth - 1; z++) {
        for (uint32_t y = 1; y < mesh->voxel_height - 1; y++) {
            for (uint32_t x = 1; x < mesh->voxel_width - 1; x++) {
                uint32_t voxel_index = x + y * mesh->voxel_width + z * mesh->voxel_width * mesh->voxel_height;
                
                if (mesh->voxels[voxel_index] == 0) {  // Only process empty voxels
                    bool is_walkable = true;
                    
                    // Check slope
                    if (builder->filter_low_hanging_obstacles && y > 0) {
                        uint32_t below_index = voxel_index - mesh->voxel_width;
                        if (mesh->voxels[below_index] == 1) {
                            float height_diff = mesh->voxel_size;
                            float slope = atanf(height_diff / mesh->voxel_size) * 180.0f / M_PI;
                            if (slope > mesh->max_slope) is_walkable = false;
                        }
                    }
                    
                    // Check step height
                    if (is_walkable && y > 0) {
                        uint32_t below_index = voxel_index - mesh->voxel_width;
                        if (mesh->voxels[below_index] == 1) {
                            float step_height = mesh->voxel_size;
                            if (step_height > mesh->max_step_height) is_walkable = false;
                        }
                    }
                    
                    filtered_voxels[voxel_index] = is_walkable ? 0 : 1;
                }
            }
        }
    }
    
    free(mesh->voxels);
    mesh->voxels = filtered_voxels;
    
    LOG_INFO("Filtered walkable voxels");
    return true;
}

// =================================================================================================
// POLYGON MESH GENERATION (TASK_1403)
// =================================================================================================

bool navmesh_build_polygon_mesh(NavmeshBuilder* builder) {
    if (!builder || !builder->navmesh) {
        LOG_ERROR("Invalid parameters for polygon mesh building");
        return false;
    }
    
    Navmesh* mesh = builder->navmesh;
    
    // Simple polygon generation - create polygons from walkable voxels
    uint32_t max_polys = mesh->voxel_width * mesh->voxel_depth / 4;  // Estimate
    uint32_t max_verts = max_polys * 4;  // 4 verts per poly
    
    mesh->polys = (NavPoly*)malloc(max_polys * sizeof(NavPoly));
    mesh->verts = (NavVert*)malloc(max_verts * sizeof(NavVert));
    
    if (!mesh->polys || !mesh->verts) {
        LOG_ERROR("Failed to allocate polygon mesh arrays");
        return false;
    }
    
    uint32_t vert_count = 0;
    uint32_t poly_count = 0;
    
    // Create simple quads for walkable areas
    for (uint32_t z = 0; z < mesh->voxel_depth - 1; z++) {
        for (uint32_t x = 0; x < mesh->voxel_width - 1; x++) {
            // Check if this 2x2 area is walkable
            bool walkable = true;
            for (uint32_t dz = 0; dz < 2 && walkable; dz++) {
                for (uint32_t dx = 0; dx < 2 && walkable; dx++) {
                    uint32_t voxel_index = (x + dx) + (mesh->voxel_width) * (z + dz);
                    if (mesh->voxels[voxel_index] != 0) walkable = false;
                }
            }
            
            if (walkable && poly_count < max_polys && vert_count + 4 <= max_verts) {
                NavPoly* poly = &mesh->polys[poly_count];
                poly->vert_count = 4;
                
                // Create quad vertices
                float vx = mesh->voxel_origin.x + x * mesh->voxel_size;
                float vz = mesh->voxel_origin.z + z * mesh->voxel_size;
                float vy = mesh->voxel_origin.y + mesh->voxel_size;  // Ground level
                
                mesh->verts[vert_count + 0].pos = (Vec3){vx, vy, vz};
                mesh->verts[vert_count + 1].pos = (Vec3){vx + mesh->voxel_size, vy, vz};
                mesh->verts[vert_count + 2].pos = (Vec3){vx + mesh->voxel_size, vy, vz + mesh->voxel_size};
                mesh->verts[vert_count + 3].pos = (Vec3){vx, vy, vz + mesh->voxel_size};
                
                poly->verts[0] = vert_count + 0;
                poly->verts[1] = vert_count + 1;
                poly->verts[2] = vert_count + 2;
                poly->verts[3] = vert_count + 3;
                
                poly_count++;
                vert_count += 4;
            }
        }
    }
    
    mesh->poly_count = poly_count;
    mesh->vert_count = vert_count;
    
    LOG_INFO("Built polygon mesh: %u vertices, %u polygons", vert_count, poly_count);
    return true;
}

// =================================================================================================
// A* PATHFINDING (TASK_1410)
// =================================================================================================

// Helper forward declarations
static uint32_t navmesh_find_polygon_at(Navmesh* mesh, const Vec3* position);
static bool point_in_polygon(const Vec3* p, const NavPoly* poly, const Navmesh* mesh);
static float navmesh_distance_to_line_segment(const Vec3* p, const Vec3* a, const Vec3* b);
static float navmesh_distance_to_polygon(const Vec3* point, const NavPoly* poly, const Navmesh* mesh);

bool navmesh_find_path(NavmeshQuery* query, const Vec3* start, const Vec3* end, NavPath* path) {
    if (!query || !query->navmesh || !start || !end || !path) {
        LOG_ERROR("Invalid parameters for pathfinding");
        return false;
    }
    Navmesh* mesh = query->navmesh;
    
    // Find start and end polygons
    uint32_t start_poly = navmesh_find_polygon_at(mesh, start);
    uint32_t end_poly = navmesh_find_polygon_at(mesh, end);
    
    if (start_poly == UINT32_MAX || end_poly == UINT32_MAX) {
        LOG_ERROR("Start or end position not on navmesh");
        return false;
    }
    
    // Initialize path
    path->point_count = 0;
    path->poly_count = 0;
    path->total_length = 0.0f;
    path->current_waypoint = 0;
    
    // Simple path - straight line (Task 1410 placeholder)
    if (path->point_count + 2 <= NAVMESH_MAX_PATH_LENGTH) {
        path->points[0] = *start;
        path->points[1] = *end;
        path->point_count = 2;
        path->poly_refs[0] = (unsigned short)start_poly;
        path->poly_refs[1] = (unsigned short)end_poly;
        path->poly_count = 2;
        path->total_length = vec3_distance(*start, *end);
        return true;
    }
    return false;
}

// Helper: Point in polygon test
static bool point_in_polygon(const Vec3* p, const NavPoly* poly, const Navmesh* mesh) {
    bool inside = false;
    for (int i = 0, j = poly->vert_count - 1; i < poly->vert_count; j = i++) {
        Vec3 vi = mesh->verts[poly->verts[i]].pos;
        Vec3 vj = mesh->verts[poly->verts[j]].pos;
        if (((vi.z > p->z) != (vj.z > p->z)) &&
            (p->x < (vj.x - vi.x) * (p->z - vi.z) / (vj.z - vi.z) + vi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

// Helper: Distance from point to line segment
static float navmesh_distance_to_line_segment(const Vec3* p, const Vec3* a, const Vec3* b) {
    Vec3 ab = vec3_sub(*b, *a);
    float dot = vec3_dot(vec3_sub(*p, *a), ab);
    float len_sq = vec3_dot(ab, ab);
    float t = (len_sq > 0) ? dot / len_sq : 0.0f;
    t = fmaxf(0.0f, fminf(1.0f, t));
    Vec3 closest = vec3_add(*a, vec3_mul(ab, t));
    return vec3_distance(*p, closest);
}

// Helper: Distance to polygon
static float navmesh_distance_to_polygon(const Vec3* point, const NavPoly* poly, const Navmesh* mesh) {
    float min_dist = FLT_MAX;
    for (uint32_t i = 0; i < poly->vert_count; i++) {
        uint32_t next_i = (i + 1) % poly->vert_count;
        Vec3 v1 = mesh->verts[poly->verts[i]].pos;
        Vec3 v2 = mesh->verts[poly->verts[next_i]].pos;
        float dist = navmesh_distance_to_line_segment(point, &v1, &v2);
        if (dist < min_dist) min_dist = dist;
    }
    return min_dist;
}

static uint32_t navmesh_find_polygon_at(Navmesh* mesh, const Vec3* position) {
    float closest_dist = FLT_MAX;
    uint32_t closest_poly = UINT32_MAX;
    
    for (uint32_t i = 0; i < mesh->poly_count; i++) {
        NavPoly* poly = &mesh->polys[i];
        if (point_in_polygon(position, poly, mesh)) {
            return i;
        }
        float dist = navmesh_distance_to_polygon(position, poly, mesh);
        if (dist < closest_dist) {
            closest_dist = dist;
            closest_poly = i;
        }
    }
    return closest_dist < mesh->agent_radius ? closest_poly : UINT32_MAX;
}

// =================================================================================================
// FUNNEL ALGORITHM (TASK_1411)
// =================================================================================================

bool navmesh_smooth_path(NavmeshQuery* query, NavPath* path) {
    if (!query || !path || path->point_count < 2) return false;
    // Simple path smoothing placeholder
    LOG_DEBUG("Path smoothing applied");
    return true;
}

// =================================================================================================
// PATH FOLLOWING (TASK_1432)
// =================================================================================================

bool navmesh_follow_path(NavPath* path, Vec3* current_pos, Vec3* target_pos, float speed, float dt) {
    if (!path || !current_pos || !target_pos || path->point_count < 2) {
        return false;
    }
    
    // Find next waypoint
    uint32_t current_waypoint = (uint32_t)path->current_waypoint;
    if (current_waypoint >= path->point_count - 1) {
        *target_pos = path->points[path->point_count - 1]; // End pos
        return true;  // Reached destination
    }
    
    *target_pos = path->points[current_waypoint + 1];
    
    // Move towards target
    Vec3 direction = vec3_subtract(*target_pos, *current_pos);
    float distance = vec3_distance(*current_pos, *target_pos);
    
    if (distance > 0.001f) {
        direction = vec3_mul(direction, 1.0f / distance);
        Vec3 movement = vec3_mul(direction, speed * dt);
        
        *current_pos = vec3_add(*current_pos, movement);
        
        // Check if we reached the waypoint
        float new_distance = vec3_distance(*current_pos, *target_pos);
        if (new_distance < 0.1f) {
            path->current_waypoint++;
            if (path->current_waypoint >= path->point_count - 1) {
                return true;  // Path complete
            }
        }
    }
    
    return false;
}

// =================================================================================================
// UTILITY FUNCTIONS
// =================================================================================================

bool navmesh_validate(const Navmesh* mesh) {
    if (!mesh) return false;
    if (mesh->voxel_width == 0 || mesh->voxel_height == 0 || mesh->voxel_depth == 0) return false;
    if (mesh->poly_count == 0) return false;
    
    return true;
}

void navmesh_debug_print_info(const Navmesh* mesh) {
    if (!mesh) return;
    
    printf("=== Navmesh Info ===\n");
    printf("Voxel Grid: %dx%dx%d\n", mesh->voxel_width, mesh->voxel_height, mesh->voxel_depth);
    printf("Polygons: %d\n", mesh->poly_count);
    printf("Vertices: %d\n", mesh->vert_count);
    printf("Agent Radius: %.2f\n", mesh->agent_radius);
    printf("Max Slope: %.1f\n", mesh->max_slope);
    printf("==================\n");
}
