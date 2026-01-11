#include "editor/debug/debug_visualization.h"
#include <stdlib.h>
#include <include/math/math.h>

void debug_viz_init_config(debug_viz_config_t* config) {
    if (!config) return;
    
    config->show_lights = false;
    config->show_shadow_frustums = false;
    config->show_probe_grid = false;
    config->show_cluster_grid = false;
    config->show_bvh = false;
    config->show_gbuffer_channels = false;
    
    config->light_sphere_radius = 0.2f;
    config->light_color = (simd_float4){1.0f, 1.0f, 0.0f, 0.8f}; // Yellow
    config->shadow_frustum_color = (simd_float4){1.0f, 0.0f, 1.0f, 0.5f}; // Magenta
    config->probe_color = (simd_float4){0.0f, 1.0f, 1.0f, 0.6f}; // Cyan
    config->bvh_color = (simd_float4){0.0f, 1.0f, 0.0f, 0.4f}; // Green
}

void debug_viz_draw_lights(debug_renderer_t* dbg, const light_system_t* lights, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_lights) return;
    
    // Placeholder light system structure - in real implementation this would come from the actual light system
    typedef struct {
        simd_float3 position;
        simd_float3 direction;
        simd_float4 color;
        float intensity;
        float radius;
        int type; // 0 = directional, 1 = point, 2 = spot
        bool enabled;
    } debug_light_t;
    
    // Simulate some lights for demonstration
    debug_light_t demo_lights[8] = {
        // Directional light (sun)
        {{10.0f, 20.0f, 10.0f}, {-0.5f, -0.7f, -0.5f}, {1.0f, 0.95f, 0.8f, 1.0f}, 1.0f, 0.0f, 0, true},
        
        // Point lights
        {{5.0f, 2.0f, 5.0f}, {0, -1, 0}, {1.0f, 0.2f, 0.2f, 1.0f}, 10.0f, 3.0f, 1, true},
        {{-5.0f, 2.0f, 5.0f}, {0, -1, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, 10.0f, 3.0f, 1, true},
        {{5.0f, 2.0f, -5.0f}, {0, -1, 0}, {0.2f, 0.2f, 1.0f, 1.0f}, 10.0f, 3.0f, 1, true},
        {{-5.0f, 2.0f, -5.0f}, {0, -1, 0}, {1.0f, 1.0f, 0.2f, 1.0f}, 10.0f, 3.0f, 1, true},
        
        // Spot lights
        {{0.0f, 5.0f, 0.0f}, {0, -1, 0}, {1.0f, 0.5f, 0.0f, 1.0f}, 15.0f, 5.0f, 2, true},
        {{8.0f, 3.0f, 0.0f}, {-0.7f, -0.3f, 0}, {0.0f, 0.5f, 1.0f, 1.0f}, 12.0f, 4.0f, 2, true},
        {{-8.0f, 3.0f, 0.0f}, {0.7f, -0.3f, 0}, {1.0f, 0.0f, 0.5f, 1.0f}, 12.0f, 4.0f, 2, true}
    };
    
    for (int i = 0; i < 8; i++) {
        debug_light_t* light = &demo_lights[i];
        if (!light->enabled) continue;
        
        // Scale color by intensity for visualization
        simd_float4 viz_color = light->color * light->intensity;
        viz_color.w = 1.0f; // Ensure alpha is 1.0
        
        switch (light->type) {
            case 0: // Directional light
                debug_viz_draw_directional_light(dbg, light, viz_color);
                break;
            case 1: // Point light
                debug_viz_draw_point_light(dbg, light, viz_color);
                break;
            case 2: // Spot light
                debug_viz_draw_spot_light(dbg, light, viz_color);
                break;
        }
        
        // Draw light label
        char light_label[64];
        switch (light->type) {
            case 0: snprintf(light_label, sizeof(light_label), "DIR"); break;
            case 1: snprintf(light_label, sizeof(light_label), "POINT"); break;
            case 2: snprintf(light_label, sizeof(light_label), "SPOT"); break;
            default: snprintf(light_label, sizeof(light_label), "LIGHT"); break;
        }
        
        simd_float3 label_pos = light->position + simd_make_float3(0, 0.5f, 0);
        debug_draw_text_3d(dbg, label_pos, light_label, viz_color);
    }
}

static void debug_viz_draw_directional_light(debug_renderer_t* dbg, const debug_light_t* light, simd_float4 color) {
    // Draw sun symbol and direction arrows
    simd_float3 sun_pos = light->position;
    float sun_radius = 0.5f;
    
    // Draw sun as a sphere
    debug_draw_sphere(dbg, sun_pos, sun_radius, color);
    
    // Draw light rays pointing in direction
    simd_float3 normalized_dir = simd_normalize(light->direction);
    float ray_length = 3.0f;
    
    for (int i = 0; i < 8; i++) {
        float angle = (float)i * M_PI * 2.0f / 8.0f;
        simd_float3 offset = simd_make_float3(
            cosf(angle) * sun_radius * 1.5f,
            sinf(angle) * sun_radius * 0.3f,
            sinf(angle) * sun_radius * 1.5f
        );
        
        simd_float3 ray_start = sun_pos + offset;
        simd_float3 ray_end = ray_start + normalized_dir * ray_length;
        
        // Draw arrow shaft
        debug_draw_line(dbg, ray_start, ray_end, color);
        
        // Draw arrowhead
        simd_float3 arrowhead_base = ray_end - normalized_dir * 0.2f;
        simd_float3 arrowhead_left = arrowhead_base + simd_make_float3(0.1f, 0, 0);
        simd_float3 arrowhead_right = arrowhead_base - simd_make_float3(0.1f, 0, 0);
        
        debug_draw_line(dbg, ray_end, arrowhead_left, color);
        debug_draw_line(dbg, ray_end, arrowhead_right, color);
    }
}

static void debug_viz_draw_point_light(debug_renderer_t* dbg, const debug_light_t* light, simd_float4 color) {
    // Draw light sphere
    debug_draw_sphere(dbg, light->position, 0.2f, color);
    
    // Draw influence radius as wireframe sphere
    simd_float4 faded_color = color;
    faded_color.w *= 0.3f; // Make radius more transparent
    debug_draw_sphere_wireframe(dbg, light->position, light->radius, faded_color);
    
    // Draw radiating lines to show light emission
    for (int i = 0; i < 12; i++) {
        float theta = (float)i * M_PI * 2.0f / 12.0f;
        float phi = M_PI * 0.25f; // 45 degree elevation
        
        simd_float3 direction = simd_make_float3(
            sinf(phi) * cosf(theta),
            cosf(phi),
            sinf(phi) * sinf(theta)
        );
        
        simd_float3 line_start = light->position + direction * 0.3f;
        simd_float3 line_end = light->position + direction * (light->radius * 0.8f);
        
        simd_float4 line_color = color;
        line_color.w *= 0.5f; // Make lines semi-transparent
        
        debug_draw_line(dbg, line_start, line_end, line_color);
    }
}

static void debug_viz_draw_spot_light(debug_renderer_t* dbg, const debug_light_t* light, simd_float4 color) {
    // Draw light source
    debug_draw_sphere(dbg, light->position, 0.2f, color);
    
    // Draw spot cone
    debug_viz_draw_spot_cone(dbg, light->position, light->direction, light->radius, 45.0f, color);
    
    // Draw center line
    simd_float3 cone_end = light->position + light->direction * light->radius;
    debug_draw_line(dbg, light->position, cone_end, color);
}

static void debug_viz_draw_spot_cone(debug_renderer_t* dbg, simd_float3 position, simd_float3 direction, float length, float angle_degrees, simd_float4 color) {
    float angle_rad = angle_degrees * M_PI / 180.0f;
    float cone_radius = length * tanf(angle_rad);
    
    // Create orthonormal basis for cone
    simd_float3 dir = simd_normalize(direction);
    simd_float3 up = simd_make_float3(0, 1, 0);
    if (fabsf(simd_dot(dir, up)) > 0.9f) {
        up = simd_make_float3(1, 0, 0);
    }
    simd_float3 right = simd_normalize(simd_cross(dir, up));
    up = simd_cross(right, dir);
    
    // Draw cone base circle
    simd_float3 cone_tip = position;
    simd_float3 cone_base_center = position + dir * length;
    
    const int circle_segments = 16;
    for (int i = 0; i < circle_segments; i++) {
        float angle1 = (float)i * 2.0f * M_PI / circle_segments;
        float angle2 = (float)(i + 1) * 2.0f * M_PI / circle_segments;
        
        simd_float3 p1 = cone_base_center + 
            right * (cosf(angle1) * cone_radius) + 
            up * (sinf(angle1) * cone_radius);
        simd_float3 p2 = cone_base_center + 
            right * (cosf(angle2) * cone_radius) + 
            up * (sinf(angle2) * cone_radius);
        
        simd_float4 faded_color = color;
        faded_color.w *= 0.4f;
        
        debug_draw_line(dbg, p1, p2, faded_color);
        debug_draw_line(dbg, cone_tip, p1, faded_color);
    }
}

static void debug_draw_sphere_wireframe(debug_renderer_t* dbg, simd_float3 center, float radius, simd_float4 color) {
    const int segments = 16;
    const int rings = 8;
    
    // Draw latitude lines
    for (int ring = 1; ring < rings; ring++) {
        float phi = (float)ring * M_PI / rings;
        float y = cosf(phi) * radius;
        float ring_radius = sinf(phi) * radius;
        
        for (int seg = 0; seg < segments; seg++) {
            float theta1 = (float)seg * 2.0f * M_PI / segments;
            float theta2 = (float)(seg + 1) * 2.0f * M_PI / segments;
            
            simd_float3 p1 = center + simd_make_float3(
                cosf(theta1) * ring_radius,
                y,
                sinf(theta1) * ring_radius
            );
            simd_float3 p2 = center + simd_make_float3(
                cosf(theta2) * ring_radius,
                y,
                sinf(theta2) * ring_radius
            );
            
            debug_draw_line(dbg, p1, p2, color);
        }
    }
    
    // Draw longitude lines
    for (int seg = 0; seg < segments; seg++) {
        float theta = (float)seg * 2.0f * M_PI / segments;
        
        for (int ring = 0; ring < rings; ring++) {
            float phi1 = (float)ring * M_PI / rings;
            float phi2 = (float)(ring + 1) * M_PI / rings;
            
            simd_float3 p1 = center + simd_make_float3(
                cosf(theta) * sinf(phi1) * radius,
                cosf(phi1) * radius,
                sinf(theta) * sinf(phi1) * radius
            );
            simd_float3 p2 = center + simd_make_float3(
                cosf(theta) * sinf(phi2) * radius,
                cosf(phi2) * radius,
                sinf(theta) * sinf(phi2) * radius
            );
            
            debug_draw_line(dbg, p1, p2, color);
        }
    }
}

void debug_viz_draw_shadow_frustums(debug_renderer_t* dbg, const shadow_system_t* shadows, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_shadow_frustums) return;
    
    // Placeholder shadow system structure - in real implementation this would come from the actual shadow system
    typedef struct {
        simd_float4x4 view_proj_matrix;
        float cascade_split;
        int cascade_index;
        bool enabled;
    } shadow_cascade_t;
    
    // Simulate shadow cascades for demonstration (typical 4 cascades)
    shadow_cascade_t cascades[4] = {
        // Cascade 0 - Near
        {simd_matrix_identity(), 0.05f, 0, true},
        // Cascade 1 - Mid-Near  
        {simd_matrix_identity(), 0.15f, 1, true},
        // Cascade 2 - Mid-Far
        {simd_matrix_identity(), 0.50f, 2, true},
        // Cascade 3 - Far
        {simd_matrix_identity(), 1.00f, 3, true}
    };
    
    // Create perspective matrices for each cascade
    for (int i = 0; i < 4; i++) {
        shadow_cascade_t* cascade = &cascades[i];
        if (!cascade->enabled) continue;
        
        // Calculate cascade frustum parameters
        float near = 0.1f;
        float far = cascade->cascade_split * 100.0f; // Scale up for visibility
        float fov = 45.0f * M_PI / 180.0f;
        float aspect = 16.0f / 9.0f;
        
        // Create perspective matrix for this cascade
        float tan_half_fov = tanf(fov * 0.5f);
        cascade->view_proj_matrix.columns[0][0] = 1.0f / (aspect * tan_half_fov);
        cascade->view_proj_matrix.columns[1][1] = 1.0f / tan_half_fov;
        cascade->view_proj_matrix.columns[2][2] = -(far + near) / (far - near);
        cascade->view_proj_matrix.columns[2][3] = -1.0f;
        cascade->view_proj_matrix.columns[3][2] = -(2.0f * far * near) / (far - near);
        
        // Position the cascade frustum in world space for visualization
        simd_float4x4 translation = simd_matrix_identity();
        translation.columns[3][0] = i * 15.0f - 22.5f; // Spread them out horizontally
        translation.columns[3][1] = 5.0f; // Raise them up
        translation.columns[3][2] = -10.0f; // Move them forward
        
        cascade->view_proj_matrix = simd_mul(translation, cascade->view_proj_matrix);
        
        // Draw the frustum
        debug_viz_draw_shadow_cascade(dbg, cascade, config);
    }
}

static void debug_viz_draw_shadow_cascade(debug_renderer_t* dbg, const shadow_cascade_t* cascade, const debug_viz_config_t* config) {
    // Extract frustum corners from the view-projection matrix
    simd_float4x4 inv_view_proj = simd_inverse(cascade->view_proj_matrix);
    
    // Define the 8 corners of clip space
    simd_float4 clip_corners[8] = {
        {-1, -1, -1, 1}, // 0: near-bottom-left
        { 1, -1, -1, 1}, // 1: near-bottom-right
        { 1,  1, -1, 1}, // 2: near-top-right
        {-1,  1, -1, 1}, // 3: near-top-left
        {-1, -1,  1, 1}, // 4: far-bottom-left
        { 1, -1,  1, 1}, // 5: far-bottom-right
        { 1,  1,  1, 1}, // 6: far-top-right
        {-1,  1,  1, 1}  // 7: far-top-left
    };
    
    // Transform corners to world space
    simd_float3 world_corners[8];
    for (int i = 0; i < 8; i++) {
        simd_float4 world_corner = simd_mul(inv_view_proj, clip_corners[i]);
        // Perspective divide
        if (world_corner.w > 0.001f) {
            world_corner = world_corner / world_corner.w;
        }
        world_corners[i] = simd_make_float3(world_corner.x, world_corner.y, world_corner.z);
    }
    
    // Color based on cascade index
    simd_float4 cascade_colors[4] = {
        {1.0f, 0.0f, 0.0f, 0.6f}, // Red - Cascade 0
        {0.0f, 1.0f, 0.0f, 0.6f}, // Green - Cascade 1
        {0.0f, 0.0f, 1.0f, 0.6f}, // Blue - Cascade 2
        {1.0f, 1.0f, 0.0f, 0.6f}  // Yellow - Cascade 3
    };
    
    simd_float4 color = cascade_colors[cascade->cascade_index % 4];
    
    // Draw frustum edges
    const int edges[12][2] = {
        // Near face
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        // Far face
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        // Connecting edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    for (int i = 0; i < 12; i++) {
        simd_float3 start = world_corners[edges[i][0]];
        simd_float3 end = world_corners[edges[i][1]];
        debug_draw_line(dbg, start, end, color);
    }
    
    // Draw filled faces with transparency
    debug_viz_draw_frustum_faces(dbg, world_corners, color);
    
    // Draw cascade label
    char cascade_label[64];
    snprintf(cascade_label, sizeof(cascade_label), "CASCADE %d", cascade->cascade_index);
    
    // Calculate center of frustum for label placement
    simd_float3 center = simd_make_float3(0, 0, 0);
    for (int i = 0; i < 8; i++) {
        center = center + world_corners[i];
    }
    center = center / 8.0f;
    
    simd_float3 label_pos = center + simd_make_float3(0, 2.0f, 0);
    debug_draw_text_3d(dbg, label_pos, cascade_label, color);
    
    // Draw split plane indicator
    debug_viz_draw_split_plane(dbg, cascade, config);
}

static void debug_viz_draw_frustum_faces(debug_renderer_t* dbg, const simd_float3 corners[8], simd_float4 color) {
    // Draw semi-transparent faces for better visualization
    simd_float4 face_color = color;
    face_color.w *= 0.2f; // Make faces more transparent
    
    // Near face
    debug_draw_quad(dbg, corners[0], corners[1], corners[2], corners[3], face_color);
    
    // Far face
    debug_draw_quad(dbg, corners[4], corners[7], corners[6], corners[5], face_color);
    
    // Left face
    debug_draw_quad(dbg, corners[0], corners[4], corners[7], corners[3], face_color);
    
    // Right face
    debug_draw_quad(dbg, corners[1], corners[2], corners[6], corners[5], face_color);
    
    // Top face
    debug_draw_quad(dbg, corners[3], corners[2], corners[6], corners[7], face_color);
    
    // Bottom face
    debug_draw_quad(dbg, corners[0], corners[1], corners[5], corners[4], face_color);
}

static void debug_viz_draw_split_plane(debug_renderer_t* dbg, const shadow_cascade_t* cascade, const debug_viz_config_t* config) {
    // Draw a plane representing the cascade split distance
    float split_distance = cascade->cascade_split * 50.0f; // Scale for visibility
    
    // Create a large plane at the split distance
    float plane_size = 20.0f;
    simd_float3 plane_center = simd_make_float3(
        cascade->view_proj_matrix.columns[3][0],
        cascade->view_proj_matrix.columns[3][1],
        -split_distance
    );
    
    // Calculate plane corners
    simd_float3 plane_corners[4] = {
        plane_center + simd_make_float3(-plane_size, 0, -plane_size),
        plane_center + simd_make_float3(plane_size, 0, -plane_size),
        plane_center + simd_make_float3(plane_size, 0, plane_size),
        plane_center + simd_make_float3(-plane_size, 0, plane_size)
    };
    
    // Draw plane outline
    simd_float4 split_color = config->shadow_frustum_color;
    split_color.w *= 0.3f; // Make split plane more transparent
    
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        debug_draw_line(dbg, plane_corners[i], plane_corners[next], split_color);
    }
    
    // Draw cross pattern on plane
    debug_draw_line(dbg, plane_corners[0], plane_corners[2], split_color);
    debug_draw_line(dbg, plane_corners[1], plane_corners[3], split_color);
    
    // Draw split distance text
    char split_label[64];
    snprintf(split_label, sizeof(split_label), "SPLIT: %.2f", cascade->cascade_split);
    simd_float3 label_pos = plane_center + simd_make_float3(0, 1.0f, 0);
    debug_draw_text_3d(dbg, label_pos, split_label, split_color);
}

static void debug_draw_quad(debug_renderer_t* dbg, simd_float3 bl, simd_float3 br, simd_float3 tr, simd_float3 tl, simd_float4 color) {
    // Draw quad outline using debug lines
    debug_draw_line(dbg, bl, br, color);
    debug_draw_line(dbg, br, tr, color);
    debug_draw_line(dbg, tr, tl, color);
    debug_draw_line(dbg, tl, bl, color);
}

void debug_viz_draw_probe_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int grid_size_x, int grid_size_y, int grid_size_z, float spacing, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_probe_grid) return;
    
    // Draw spheres at each probe position
    for (int x = 0; x < grid_size_x; x++) {
        for (int y = 0; y < grid_size_y; y++) {
            for (int z = 0; z < grid_size_z; z++) {
                simd_float3 probe_pos = grid_origin + simd_make_float3(
                    x * spacing,
                    y * spacing,
                    z * spacing
                );
                debug_draw_sphere(dbg, probe_pos, 0.1f, config->probe_color);
            }
        }
    }
}

void debug_viz_draw_cluster_grid(debug_renderer_t* dbg, simd_float3 grid_origin, int clusters_x, int clusters_y, int clusters_z, float cluster_size, const debug_viz_config_t* config) {
    if (!dbg || !config || !config->show_cluster_grid) return;
    
    // Draw AABBs for each cluster
    for (int x = 0; x < clusters_x; x++) {
        for (int y = 0; y < clusters_y; y++) {
            for (int z = 0; z < clusters_z; z++) {
                simd_float3 cluster_min = grid_origin + simd_make_float3(
                    x * cluster_size,
                    y * cluster_size,
                    z * cluster_size
                );
                simd_float3 cluster_max = cluster_min + simd_make_float3(
                    cluster_size,
                    cluster_size,
                    cluster_size
                );
                
                simd_float4 cluster_color = config->probe_color;
                cluster_color.w = 0.2f; // Very transparent
                debug_draw_aabb(dbg, cluster_min, cluster_max, cluster_color);
            }
        }
    }
}

// Recursive BVH drawing helper
static void debug_viz_draw_bvh_node(debug_renderer_t* dbg, const bvh_node_t* node, const debug_viz_config_t* config, int current_depth, int max_depth) {
    if (!node || current_depth > max_depth) return;
    
    // Draw the AABB bounds of this node
    debug_draw_aabb(dbg, node->bounds_min, node->bounds_max, config->bvh_color);
    
    // Calculate node depth color intensity
    float depth_factor = (float)current_depth / (float)max_depth;
    simd_float4 depth_color = config->bvh_color;
    depth_color.w *= (1.0f - depth_factor * 0.5f); // Fade out deeper nodes
    
    // Draw node information
    if (current_depth < 3) { // Only show labels for top 3 levels
        char node_info[128];
        if (node->is_leaf) {
            snprintf(node_info, sizeof(node_info), "L%d:%d", current_depth, node->object_count);
        } else {
            snprintf(node_info, sizeof(node_info), "N%d:%d", current_depth, node->object_count);
        }
        
        simd_float3 center = {
            (node->bounds_min.x + node->bounds_max.x) * 0.5f,
            (node->bounds_min.y + node->bounds_max.y) * 0.5f,
            (node->bounds_min.z + node->bounds_max.z) * 0.5f
        };
        
        debug_draw_text_3d(dbg, center, node_info, depth_color);
    }
    
    // Draw split plane for internal nodes
    if (!node->is_leaf && current_depth < max_depth) {
        debug_viz_draw_bvh_split_plane(dbg, node, depth_color);
    }
    
    // Recursive calls for children
    if (node->left) {
        debug_viz_draw_bvh_node(dbg, node->left, config, current_depth + 1, max_depth);
    }
    if (node->right) {
        debug_viz_draw_bvh_node(dbg, node->right, config, current_depth + 1, max_depth);
    }
}

void debug_viz_draw_bvh(debug_renderer_t* dbg, const bvh_node_t* root, const debug_viz_config_t* config, int max_depth) {
    if (!dbg || !config || !config->show_bvh) return;
    
    // If no root provided, create a demo BVH for visualization
    if (!root) {
        bvh_node_t* demo_bvh = debug_viz_create_demo_bvh();
        debug_viz_draw_bvh_node(dbg, demo_bvh, config, 0, max_depth);
        debug_viz_cleanup_demo_bvh(demo_bvh);
    } else {
        debug_viz_draw_bvh_node(dbg, root, config, 0, max_depth);
    }
}

// Demo BVH creation for testing
static bvh_node_t* debug_viz_create_demo_bvh(void) {
    // Create a sample BVH structure for demonstration
    bvh_node_t* root = malloc(sizeof(bvh_node_t));
    memset(root, 0, sizeof(bvh_node_t));
    
    // Root node bounds (large volume)
    root->bounds_min = simd_make_float3(-20.0f, -5.0f, -20.0f);
    root->bounds_max = simd_make_float3(20.0f, 15.0f, 20.0f);
    root->is_leaf = false;
    root->object_count = 16;
    
    // Create left child
    root->left = malloc(sizeof(bvh_node_t));
    memset(root->left, 0, sizeof(bvh_node_t));
    root->left->bounds_min = simd_make_float3(-20.0f, -5.0f, -20.0f);
    root->left->bounds_max = simd_make_float3(0.0f, 15.0f, 0.0f);
    root->left->is_leaf = false;
    root->left->object_count = 8;
    root->left->parent = root;
    
    // Create right child
    root->right = malloc(sizeof(bvh_node_t));
    memset(root->right, 0, sizeof(bvh_node_t));
    root->right->bounds_min = simd_make_float3(0.0f, -5.0f, 0.0f);
    root->right->bounds_max = simd_make_float3(20.0f, 15.0f, 20.0f);
    root->right->is_leaf = false;
    root->right->object_count = 8;
    root->right->parent = root;
    
    // Create left-left leaf
    root->left->left = malloc(sizeof(bvh_node_t));
    memset(root->left->left, 0, sizeof(bvh_node_t));
    root->left->left->bounds_min = simd_make_float3(-20.0f, -5.0f, -20.0f);
    root->left->left->bounds_max = simd_make_float3(-10.0f, 5.0f, -10.0f);
    root->left->left->is_leaf = true;
    root->left->left->object_count = 4;
    root->left->left->parent = root->left;
    
    // Create left-right leaf
    root->left->right = malloc(sizeof(bvh_node_t));
    memset(root->left->right, 0, sizeof(bvh_node_t));
    root->left->right->bounds_min = simd_make_float3(-10.0f, -5.0f, -20.0f);
    root->left->right->bounds_max = simd_make_float3(0.0f, 5.0f, 0.0f);
    root->left->right->is_leaf = true;
    root->left->right->object_count = 4;
    root->left->right->parent = root->left;
    
    // Create right-left leaf
    root->right->left = malloc(sizeof(bvh_node_t));
    memset(root->right->left, 0, sizeof(bvh_node_t));
    root->right->left->bounds_min = simd_make_float3(0.0f, -5.0f, 0.0f);
    root->right->left->bounds_max = simd_make_float3(10.0f, 5.0f, 10.0f);
    root->right->left->is_leaf = true;
    root->right->left->object_count = 4;
    root->right->left->parent = root->right;
    
    // Create right-right leaf
    root->right->right = malloc(sizeof(bvh_node_t));
    memset(root->right->right, 0, sizeof(bvh_node_t));
    root->right->right->bounds_min = simd_make_float3(10.0f, -5.0f, 0.0f);
    root->right->right->bounds_max = simd_make_float3(20.0f, 5.0f, 20.0f);
    root->right->right->is_leaf = true;
    root->right->right->object_count = 4;
    root->right->right->parent = root->right;
    
    return root;
}

static void debug_viz_cleanup_demo_bvh(bvh_node_t* node) {
    if (!node) return;
    
    debug_viz_cleanup_demo_bvh(node->left);
    debug_viz_cleanup_demo_bvh(node->right);
    free(node);
}

static void debug_viz_draw_bvh_split_plane(debug_renderer_t* dbg, const bvh_node_t* node, simd_float4 color) {
    // Determine split axis based on bounds
    simd_float3 size = node->bounds_max - node->bounds_min;
    
    // Find the largest axis (this is typically the split axis)
    int split_axis = 0;
    if (size.y > size.x && size.y > size.z) split_axis = 1;
    else if (size.z > size.x && size.z > size.y) split_axis = 2;
    
    // Calculate split plane position
    float split_pos = node->bounds_min.x;
    if (split_axis == 0) split_pos = (node->bounds_min.x + node->bounds_max.x) * 0.5f;
    else if (split_axis == 1) split_pos = (node->bounds_min.y + node->bounds_max.y) * 0.5f;
    else split_pos = (node->bounds_min.z + node->bounds_max.z) * 0.5f;
    
    // Create split plane vertices
    simd_float3 plane_corners[4];
    if (split_axis == 0) { // YZ plane
        plane_corners[0] = simd_make_float3(split_pos, node->bounds_min.y, node->bounds_min.z);
        plane_corners[1] = simd_make_float3(split_pos, node->bounds_max.y, node->bounds_min.z);
        plane_corners[2] = simd_make_float3(split_pos, node->bounds_max.y, node->bounds_max.z);
        plane_corners[3] = simd_make_float3(split_pos, node->bounds_min.y, node->bounds_max.z);
    } else if (split_axis == 1) { // XZ plane
        plane_corners[0] = simd_make_float3(node->bounds_min.x, split_pos, node->bounds_min.z);
        plane_corners[1] = simd_make_float3(node->bounds_max.x, split_pos, node->bounds_min.z);
        plane_corners[2] = simd_make_float3(node->bounds_max.x, split_pos, node->bounds_max.z);
        plane_corners[3] = simd_make_float3(node->bounds_min.x, split_pos, node->bounds_max.z);
    } else { // XY plane
        plane_corners[0] = simd_make_float3(node->bounds_min.x, node->bounds_min.y, split_pos);
        plane_corners[1] = simd_make_float3(node->bounds_max.x, node->bounds_min.y, split_pos);
        plane_corners[2] = simd_make_float3(node->bounds_max.x, node->bounds_max.y, split_pos);
        plane_corners[3] = simd_make_float3(node->bounds_min.x, node->bounds_max.y, split_pos);
    }
    
    // Draw split plane outline
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        debug_draw_line(dbg, plane_corners[i], plane_corners[next], color);
    }
    
    // Draw cross pattern on split plane
    debug_draw_line(dbg, plane_corners[0], plane_corners[2], color);
    debug_draw_line(dbg, plane_corners[1], plane_corners[3], color);
}
