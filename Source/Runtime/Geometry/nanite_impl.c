#include "geometry/geometry_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Nanite virtualized geometry system
// Based on Epic's Nanite technology for rendering massive scenes

#define NANITE_MAX_CLUSTERS 65536
#define NANITE_CLUSTER_SIZE 128  // Vertices per cluster
#define NANITE_MAX_LOD_LEVELS 8
#define NANITE_MIN_SCREEN_SIZE 0.001f

// Cluster data structure
typedef struct nanite_cluster {
    u32 id;
    u32 parent_id;
    u32 children[4];  // Up to 4 children for subdivision
    u32 child_count;
    u32 lod_level;
    
    // Bounding data
    mesh_bounds_t bounds;
    f32 error_metric;
    
    // Geometry data
    u32 vertex_count;
    u32 index_count;
    vertex_t* vertices;
    u32* indices;
    
    // Streaming data
    bool is_loaded;
    bool is_visible;
    u32 last_frame_used;
    f32 screen_size;
} nanite_cluster_t;

// Nanite scene structure
typedef struct nanite_scene {
    nanite_cluster_t* clusters;
    u32 cluster_count;
    u32 max_clusters;
    
    // LOD configuration
    f32 error_thresholds[NANITE_MAX_LOD_LEVELS];
    u32 cluster_target_count;
    
    // Statistics
    u32 clusters_rendered;
    u32 clusters_streamed_in;
    u32 clusters_streamed_out;
    f64 total_processing_time;
} nanite_scene_t;

// Global nanite instance
static nanite_scene_t* g_nanite_scene = NULL;

// Initialize nanite system
bool nanite_init() {
    if (g_nanite_scene) {
        printf("Warning: Nanite already initialized\n");
        return true;
    }
    
    g_nanite_scene = (nanite_scene_t*)calloc(1, sizeof(nanite_scene_t));
    if (!g_nanite_scene) {
        printf("Error: Failed to allocate nanite scene\n");
        return false;
    }
    
    // Allocate cluster array
    g_nanite_scene->clusters = (nanite_cluster_t*)calloc(NANITE_MAX_CLUSTERS, sizeof(nanite_cluster_t));
    if (!g_nanite_scene->clusters) {
        free(g_nanite_scene);
        g_nanite_scene = NULL;
        printf("Error: Failed to allocate cluster array\n");
        return false;
    }
    
    g_nanite_scene->max_clusters = NANITE_MAX_CLUSTERS;
    g_nanite_scene->cluster_target_count = 1000;  // Target clusters per frame
    
    // Initialize error thresholds for LOD levels
    for (u32 i = 0; i < NANITE_MAX_LOD_LEVELS; i++) {
        g_nanite_scene->error_thresholds[i] = powf(2.0f, (f32)i) * 0.001f;
    }
    
    printf("Nanite system initialized\n");
    return true;
}

// Shutdown nanite system
void nanite_shutdown() {
    if (!g_nanite_scene) {
        return;
    }
    
    // Free all cluster data
    for (u32 i = 0; i < g_nanite_scene->cluster_count; i++) {
        nanite_cluster_t* cluster = &g_nanite_scene->clusters[i];
        free(cluster->vertices);
        free(cluster->indices);
    }
    
    free(g_nanite_scene->clusters);
    free(g_nanite_scene);
    g_nanite_scene = NULL;
    
    printf("Nanite system shutdown\n");
}

// Create a nanite cluster from mesh data
u32 nanite_create_cluster(const mesh_t* mesh, u32 start_vertex, u32 vertex_count, 
                        u32 start_index, u32 index_count, u32 lod_level) {
    if (!g_nanite_scene || !mesh || vertex_count == 0 || index_count == 0) {
        return 0;
    }
    
    if (g_nanite_scene->cluster_count >= g_nanite_scene->max_clusters) {
        printf("Error: Maximum cluster count reached\n");
        return 0;
    }
    
    u32 cluster_id = g_nanite_scene->cluster_count++;
    nanite_cluster_t* cluster = &g_nanite_scene->clusters[cluster_id];
    
    // Initialize cluster
    memset(cluster, 0, sizeof(nanite_cluster_t));
    cluster->id = cluster_id;
    cluster->lod_level = lod_level;
    cluster->vertex_count = vertex_count;
    cluster->index_count = index_count;
    
    // Allocate and copy vertex data
    cluster->vertices = (vertex_t*)malloc(vertex_count * sizeof(vertex_t));
    if (cluster->vertices && start_vertex + vertex_count <= mesh->vertex_count) {
        memcpy(cluster->vertices, &mesh->vertices[start_vertex], vertex_count * sizeof(vertex_t));
    }
    
    // Allocate and copy index data
    cluster->indices = (u32*)malloc(index_count * sizeof(u32));
    if (cluster->indices && start_index + index_count <= mesh->index_count) {
        memcpy(cluster->indices, &mesh->indices[start_index], index_count * sizeof(u32));
        
        // Adjust indices to be relative to cluster start
        for (u32 i = 0; i < index_count; i++) {
            cluster->indices[i] -= start_vertex;
        }
    }
    
    // Calculate cluster bounds
    if (cluster->vertices && vertex_count > 0) {
        cluster->bounds.min = cluster->vertices[0].position;
        cluster->bounds.max = cluster->vertices[0].position;
        
        for (u32 i = 1; i < vertex_count; i++) {
            Vec3 pos = cluster->vertices[i].position;
            
            if (pos.x < cluster->bounds.min.x) cluster->bounds.min.x = pos.x;
            if (pos.y < cluster->bounds.min.y) cluster->bounds.min.y = pos.y;
            if (pos.z < cluster->bounds.min.z) cluster->bounds.min.z = pos.z;
            
            if (pos.x > cluster->bounds.max.x) cluster->bounds.max.x = pos.x;
            if (pos.y > cluster->bounds.max.y) cluster->bounds.max.y = pos.y;
            if (pos.z > cluster->bounds.max.z) cluster->bounds.max.z = pos.z;
        }
        
        // Calculate sphere bounds
        cluster->bounds.sphere_center.x = (cluster->bounds.min.x + cluster->bounds.max.x) * 0.5f;
        cluster->bounds.sphere_center.y = (cluster->bounds.min.y + cluster->bounds.max.y) * 0.5f;
        cluster->bounds.sphere_center.z = (cluster->bounds.min.z + cluster->bounds.max.z) * 0.5f;
        
        Vec3 to_corner = {
            cluster->bounds.max.x - cluster->bounds.sphere_center.x,
            cluster->bounds.max.y - cluster->bounds.sphere_center.y,
            cluster->bounds.max.z - cluster->bounds.sphere_center.z
        };
        cluster->bounds.sphere_radius = sqrtf(to_corner.x * to_corner.x + 
                                           to_corner.y * to_corner.y + 
                                           to_corner.z * to_corner.z);
    }
    
    // Calculate error metric (simplified)
    cluster->error_metric = g_nanite_scene->error_thresholds[lod_level];
    
    printf("Created nanite cluster %u (LOD %u, %u vertices, %u indices)\n", 
           cluster_id, lod_level, vertex_count, index_count);
    
    return cluster_id;
}

// Build cluster hierarchy (simplified)
void nanite_build_hierarchy(u32 parent_cluster_id, u32* child_cluster_ids, u32 child_count) {
    if (!g_nanite_scene || parent_cluster_id >= g_nanite_scene->cluster_count) {
        return;
    }
    
    nanite_cluster_t* parent = &g_nanite_scene->clusters[parent_cluster_id];
    parent->child_count = MIN(child_count, 4);
    
    for (u32 i = 0; i < parent->child_count; i++) {
        if (child_cluster_ids[i] < g_nanite_scene->cluster_count) {
            parent->children[i] = child_cluster_ids[i];
            g_nanite_scene->clusters[child_cluster_ids[i]].parent_id = parent_cluster_id;
        }
    }
    
    printf("Built hierarchy for cluster %u (%u children)\n", parent_cluster_id, parent->child_count);
}

// Calculate screen size of a cluster
static f32 calculate_screen_size(const nanite_cluster_t* cluster, const float* view_matrix, 
                               const float* projection_matrix, u32 viewport_width, u32 viewport_height) {
    if (!cluster) {
        return 0.0f;
    }
    
    // Simplified screen size calculation
    // In a real implementation, this would project the cluster bounds to screen space
    
    // Transform cluster center to view space
    Vec3 center = cluster->bounds.sphere_center;
    Vec3 view_pos = {
        view_matrix[0] * center.x + view_matrix[4] * center.y + view_matrix[8] * center.z + view_matrix[12],
        view_matrix[1] * center.x + view_matrix[5] * center.y + view_matrix[9] * center.z + view_matrix[13],
        view_matrix[2] * center.x + view_matrix[6] * center.y + view_matrix[10] * center.z + view_matrix[14]
    };
    
    // Project to screen space (simplified)
    f32 fov = 60.0f * PI / 180.0f;  // Assume 60 degree FOV
    f32 screen_size = (2.0f * cluster->bounds.sphere_radius) / (view_pos.z * tanf(fov * 0.5f));
    
    return screen_size * MIN(viewport_width, viewport_height);
}

// Select LOD level for a cluster
static u32 select_lod_level(const nanite_cluster_t* cluster, f32 screen_size, f32 error_threshold) {
    if (!cluster || screen_size < NANITE_MIN_SCREEN_SIZE) {
        return NANITE_MAX_LOD_LEVELS;  // Don't render
    }
    
    // Find the highest LOD that meets the error threshold
    for (u32 lod = 0; lod < NANITE_MAX_LOD_LEVELS; lod++) {
        if (g_nanite_scene->error_thresholds[lod] <= error_threshold) {
            return lod;
        }
    }
    
    return 0;  // Use highest LOD
}

// Update nanite scene for current frame
void nanite_update(const float* view_matrix, const float* projection_matrix, 
                   u32 viewport_width, u32 viewport_height, u64 current_frame) {
    if (!g_nanite_scene) {
        return;
    }
    
    // Reset statistics
    g_nanite_scene->clusters_rendered = 0;
    g_nanite_scene->clusters_streamed_in = 0;
    g_nanite_scene->clusters_streamed_out = 0;
    
    // Update cluster visibility and LOD selection
    for (u32 i = 0; i < g_nanite_scene->cluster_count; i++) {
        nanite_cluster_t* cluster = &g_nanite_scene->clusters[i];
        
        // Calculate screen size
        cluster->screen_size = calculate_screen_size(cluster, view_matrix, projection_matrix, 
                                                   viewport_width, viewport_height);
        
        // Determine visibility
        cluster->is_visible = (cluster->screen_size >= NANITE_MIN_SCREEN_SIZE);
        
        // Update frame usage
        if (cluster->is_visible) {
            cluster->last_frame_used = current_frame;
            g_nanite_scene->clusters_rendered++;
        }
    }
    
    printf("Nanite update: %u clusters visible, %u rendered\n", 
           g_nanite_scene->cluster_count, g_nanite_scene->clusters_rendered);
}

// Get clusters to render for current frame
u32 nanite_get_render_clusters(u32* out_cluster_ids, u32 max_clusters) {
    if (!g_nanite_scene || !out_cluster_ids || max_clusters == 0) {
        return 0;
    }
    
    u32 count = 0;
    
    // Collect visible clusters, sorted by LOD (high to low)
    for (u32 lod = 0; lod < NANITE_MAX_LOD_LEVELS && count < max_clusters; lod++) {
        for (u32 i = 0; i < g_nanite_scene->cluster_count && count < max_clusters; i++) {
            nanite_cluster_t* cluster = &g_nanite_scene->clusters[i];
            
            if (cluster->is_visible && cluster->lod_level == lod) {
                out_cluster_ids[count++] = cluster->id;
            }
        }
    }
    
    return count;
}

// Get cluster data
bool nanite_get_cluster_data(u32 cluster_id, vertex_t** out_vertices, u32* out_vertex_count,
                             u32** out_indices, u32* out_index_count) {
    if (!g_nanite_scene || cluster_id >= g_nanite_scene->cluster_count) {
        return false;
    }
    
    nanite_cluster_t* cluster = &g_nanite_scene->clusters[cluster_id];
    
    if (out_vertices) *out_vertices = cluster->vertices;
    if (out_vertex_count) *out_vertex_count = cluster->vertex_count;
    if (out_indices) *out_indices = cluster->indices;
    if (out_index_count) *out_index_count = cluster->index_count;
    
    return cluster->is_loaded;
}

// Get cluster bounds
bool nanite_get_cluster_bounds(u32 cluster_id, mesh_bounds_t* out_bounds) {
    if (!g_nanite_scene || cluster_id >= g_nanite_scene->cluster_count || !out_bounds) {
        return false;
    }
    
    *out_bounds = g_nanite_scene->clusters[cluster_id].bounds;
    return true;
}

// Stream in cluster data
bool nanite_stream_in_cluster(u32 cluster_id) {
    if (!g_nanite_scene || cluster_id >= g_nanite_scene->cluster_count) {
        return false;
    }
    
    nanite_cluster_t* cluster = &g_nanite_scene->clusters[cluster_id];
    
    if (cluster->is_loaded) {
        return true;  // Already loaded
    }
    
    // In a real implementation, this would load data from disk
    // For now, we assume data is already loaded
    cluster->is_loaded = true;
    g_nanite_scene->clusters_streamed_in++;
    
    printf("Streamed in cluster %u\n", cluster_id);
    return true;
}

// Stream out cluster data
void nanite_stream_out_cluster(u32 cluster_id) {
    if (!g_nanite_scene || cluster_id >= g_nanite_scene->cluster_count) {
        return;
    }
    
    nanite_cluster_t* cluster = &g_nanite_scene->clusters[cluster_id];
    
    if (!cluster->is_loaded) {
        return;  // Already unloaded
    }
    
    // In a real implementation, this would free GPU memory
    cluster->is_loaded = false;
    g_nanite_scene->clusters_streamed_out++;
    
    printf("Streamed out cluster %u\n", cluster_id);
}

// Get nanite statistics
void nanite_get_statistics(u32* total_clusters, u32* rendered_clusters, 
                          u32* streamed_in, u32* streamed_out) {
    if (!g_nanite_scene) {
        if (total_clusters) *total_clusters = 0;
        if (rendered_clusters) *rendered_clusters = 0;
        if (streamed_in) *streamed_in = 0;
        if (streamed_out) *streamed_out = 0;
        return;
    }
    
    if (total_clusters) *total_clusters = g_nanite_scene->cluster_count;
    if (rendered_clusters) *rendered_clusters = g_nanite_scene->clusters_rendered;
    if (streamed_in) *streamed_in = g_nanite_scene->clusters_streamed_in;
    if (streamed_out) *streamed_out = g_nanite_scene->clusters_streamed_out;
}

// Convert a mesh to nanite clusters
u32 nanite_convert_mesh(const mesh_t* mesh) {
    if (!g_nanite_scene || !mesh || mesh->vertex_count == 0) {
        return 0;
    }
    
    printf("Converting mesh '%s' to nanite clusters (%u vertices, %u indices)\n", 
           mesh->name, mesh->vertex_count, mesh->index_count);
    
    u32 clusters_created = 0;
    u32 vertices_per_cluster = NANITE_CLUSTER_SIZE;
    
    // Create clusters from mesh data
    for (u32 vertex_start = 0; vertex_start < mesh->vertex_count; vertex_start += vertices_per_cluster) {
        u32 vertex_count = MIN(vertices_per_cluster, mesh->vertex_count - vertex_start);
        
        // Find corresponding indices (simplified)
        u32 index_start = (vertex_start / vertices_per_cluster) * vertices_per_cluster * 3;
        u32 index_count = MIN(vertex_count * 3, mesh->index_count - index_start);
        
        if (vertex_count > 0 && index_count > 0) {
            u32 cluster_id = nanite_create_cluster(mesh, vertex_start, vertex_count, 
                                                  index_start, index_count, 0);
            if (cluster_id > 0) {
                clusters_created++;
            }
        }
    }
    
    printf("Created %u nanite clusters from mesh '%s'\n", clusters_created, mesh->name);
    return clusters_created;
}
