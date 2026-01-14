#ifndef REALTIME_RAYTRACING_H
#define REALTIME_RAYTRACING_H

#include "../renderer.h"
#include "../../math/vector.h"
#include "../../math/matrix.h"
#include "../../core/memory.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ray tracing acceleration structures
typedef enum {
    ACCELERATION_STRUCTURE_NONE = 0,
    ACCELERATION_STRUCTURE_BVH,
    ACCELERATION_STRUCTURE_OCTREE,
    ACCELERATION_STRUCTURE_KD_TREE,
    ACCELERATION_STRUCTURE_UNIFORM_GRID
} acceleration_structure_type_t;

// Ray tracing quality levels
typedef enum {
    RAYTRACING_QUALITY_LOW = 0,
    RAYTRACING_QUALITY_MEDIUM,
    RAYTRACING_QUALITY_HIGH,
    RAYTRACING_QUALITY_ULTRA,
    RAYTRACING_QUALITY_PATH_TRACED
} raytracing_quality_t;

// Ray types
typedef enum {
    RAY_TYPE_PRIMARY = 0,
    RAY_TYPE_REFLECTION,
    RAY_TYPE_REFRACTION,
    RAY_TYPE_SHADOW,
    RAY_TYPE_OCCLUSION,
    RAY_TYPE_DIFFUSE
} ray_type_t;

// Ray structure
typedef struct {
    vec3_t origin;
    vec3_t direction;
    float t_min;
    float t_max;
    ray_type_t type;
    uint32_t pixel_x, pixel_y;
    uint32_t depth;
    float importance;
} ray_t;

// Ray-surface intersection
typedef struct {
    bool hit;
    float t;
    vec3_t point;
    vec3_t normal;
    vec2_t uv;
    uint32_t material_id;
    uint32_t primitive_id;
    vec3_t geometric_normal;
    vec3_t shading_normal;
    float front_face;
} ray_intersection_t;

// Material properties for ray tracing
typedef struct {
    vec3_t albedo;
    vec3_t emission;
    float metallic;
    float roughness;
    float transmission;
    float ior; // Index of refraction
    vec3_t normal_map;
    vec3_t roughness_map;
    vec3_t metallic_map;
    bool has_normal_map;
    bool has_roughness_map;
    bool has_metallic_map;
} raytracing_material_t;

// Triangle for ray tracing
typedef struct {
    vec3_t v0, v1, v2;
    vec3_t n0, n1, n2;
    vec2_t uv0, uv1, uv2;
    uint32_t material_id;
    uint32_t padding;
} raytracing_triangle_t;

// BVH node
typedef struct bvh_node {
    vec3_t bounds_min;
    vec3_t bounds_max;
    struct bvh_node* left;
    struct bvh_node* right;
    raytracing_triangle_t* triangles;
    uint32_t triangle_count;
    bool is_leaf;
} bvh_node_t;

// Ray tracing scene
typedef struct {
    raytracing_triangle_t* triangles;
    uint32_t triangle_count;
    uint32_t triangle_capacity;
    
    raytracing_material_t* materials;
    uint32_t material_count;
    uint32_t material_capacity;
    
    bvh_node_t* bvh_root;
    acceleration_structure_type_t acceleration_structure;
    
    // Scene bounds
    vec3_t scene_min;
    vec3_t scene_max;
    vec3_t scene_center;
    float scene_radius;
} raytracing_scene_t;

// Ray tracing camera
typedef struct {
    vec3_t position;
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    float fov_y;
    float aspect_ratio;
    float near_plane;
    float far_plane;
    float focus_distance;
    float aperture;
} raytracing_camera_t;

// Render target
typedef struct {
    uint32_t width;
    uint32_t height;
    float* color_buffer;     // RGBA32F
    float* normal_buffer;    // RGB32F
    float* albedo_buffer;    // RGB32F
    float* depth_buffer;     // R32F
    uint32_t* sample_count;  // R32UI
} raytracing_render_target_t;

// Ray tracing renderer
typedef struct {
    raytracing_scene_t* scene;
    raytracing_camera_t camera;
    raytracing_render_target_t* render_target;
    
    // Settings
    raytracing_quality_t quality;
    uint32_t max_bounces;
    uint32_t max_samples_per_pixel;
    uint32_t current_sample;
    bool enable_denoising;
    bool enable_temporal_accumulation;
    
    // Performance
    uint32_t tile_size;
    uint32_t num_threads;
    bool use_gpu_acceleration;
    
    // Statistics
    uint64_t total_rays_cast;
    uint64_t total_intersections;
    float average_rays_per_pixel;
    float render_time_ms;
    uint32_t frames_rendered;
} raytracing_renderer_t;

// Lighting for ray tracing
typedef struct {
    vec3_t position;
    vec3_t direction;
    vec3_t color;
    float intensity;
    float radius;
    float angle; // For spotlights
    uint32_t type; // 0=point, 1=directional, 2=spot
} raytracing_light_t;

// Environment map
typedef struct {
    uint32_t width, height;
    float* data; // HDR environment map data
    float intensity;
    float rotation;
} raytracing_environment_t;

// Function declarations
raytracing_renderer_t* raytracing_renderer_create(uint32_t width, uint32_t height);
void raytracing_renderer_destroy(raytracing_renderer_t* renderer);

raytracing_scene_t* raytracing_scene_create(void);
void raytracing_scene_destroy(raytracing_scene_t* scene);

bool raytracing_scene_add_triangle(raytracing_scene_t* scene, const raytracing_triangle_t* triangle);
bool raytracing_scene_add_material(raytracing_scene_t* scene, const raytracing_material_t* material);
bool raytracing_scene_build_acceleration_structure(raytracing_scene_t* scene);

// Ray intersection tests
bool ray_triangle_intersect(const ray_t* ray, const raytracing_triangle_t* triangle, ray_intersection_t* intersection);
bool ray_aabb_intersect(const ray_t* ray, const vec3_t* min_bounds, const vec3_t* max_bounds, float* t_near, float* t_far);
bool ray_sphere_intersect(const ray_t* ray, const vec3_t* center, float radius, float* t);

// BVH operations
bvh_node_t* bvh_build(raytracing_triangle_t* triangles, uint32_t count);
bool bvh_intersect(const bvh_node_t* node, const ray_t* ray, ray_intersection_t* intersection);
void bvh_destroy(bvh_node_t* node);

// Rendering functions
void raytracing_renderer_set_camera(raytracing_renderer_t* renderer, const raytracing_camera_t* camera);
void raytracing_renderer_set_quality(raytracing_renderer_t* renderer, raytracing_quality_t quality);
void raytracing_renderer_render_frame(raytracing_renderer_t* renderer);
void raytracing_renderer_render_tile(raytracing_renderer_t* renderer, uint32_t tile_x, uint32_t tile_y);
vec3_t raytracing_trace_ray(raytracing_renderer_t* renderer, const ray_t* ray);

// Lighting calculations
vec3_t raytracing_calculate_lighting(const ray_intersection_t* intersection, const raytracing_material_t* material, 
                                     const raytracing_light_t* lights, uint32_t light_count, const vec3_t* view_dir);
vec3_t raytracing_sample_environment(const raytracing_environment_t* env, const vec3_t* direction);

// Material models
vec3_t raytracing_evaluate_material(const raytracing_material_t* material, const ray_intersection_t* intersection,
                                    const vec3_t* incident_dir, const vec3_t* outgoing_dir);
ray_t raytracing_generate_reflection_ray(const ray_intersection_t* intersection, const ray_t* incident_ray);
ray_t raytracing_generate_refraction_ray(const ray_intersection_t* intersection, const ray_t* incident_ray, 
                                         float ior_outside, float ior_inside);

// Denoising
void raytracing_denoise_frame(raytracing_renderer_t* renderer);
void raytracing_temporal_accumulation(raytracing_renderer_t* renderer, const raytracing_camera_t* previous_camera);

// Utility functions
ray_t raytracing_generate_camera_ray(const raytracing_camera_t* camera, uint32_t pixel_x, uint32_t pixel_y, 
                                     uint32_t width, uint32_t height, float sample_x, float sample_y);
vec3_t raytracing_world_to_screen(const raytracing_camera_t* camera, const vec3_t* world_pos, 
                                  uint32_t width, uint32_t height);

// Performance monitoring
void raytracing_get_stats(raytracing_renderer_t* renderer, uint64_t* rays_cast, uint64_t* intersections, 
                         float* render_time, float* rays_per_pixel);

// GPU acceleration (if available)
#ifdef ENABLE_GPU_RAYTRACING
bool raytracing_gpu_init(raytracing_renderer_t* renderer);
void raytracing_gpu_render(raytracing_renderer_t* renderer);
void raytracing_gpu_cleanup(raytracing_renderer_t* renderer);
#endif

#ifdef __cplusplus
}
#endif

#endif // REALTIME_RAYTRACING_H
