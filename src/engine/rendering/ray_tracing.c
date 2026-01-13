/*
 * ray_tracing.c
 * Ray tracing integration implementation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "rendering/ray_tracing.h"
#include "rendering/vulkan_raytracing.h"
#include "rendering/metal_raytracing.h"
#include "rendering/raytracing_acceleration_structures.h"
#include "rendering/raytracing_pipeline.h"
#include "rendering/raytracing_denoiser.h"
#include "rendering/raytracing_global_illumination.h"
#include "rendering/raytracing_reflections.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>

/* ============================================================================
 * RAY TRACING CONFIGURATION
 * ============================================================================ */

#define MAX_RAY_TRACING_INSTANCES 1024
#define MAX_ACCELERATION_STRUCTURES 256
#define RAY_TRACING_MAGIC 0x52545258  // "RTRX"

#define MAX_BOUNCES 8
#define RAY_CONE_ANGLE 0.001f
#define RAY_T_MAX 1000.0f

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct ray_tracing_instance {
    uint32_t magic;
    uint32_t id;
    uint32_t flags;
    bool initialized;
    bool dirty;
    
    // Transform
    mat4 transform;
    vec3 position;
    vec3 scale;
    quat rotation;
    
    // Material properties
    uint32_t material_id;
    float roughness;
    float metallic;
    float emissive;
    
    // Geometry
    uint32_t mesh_id;
    uint32_t vertex_count;
    uint32_t index_count;
    
    // Ray tracing specific
    void* blas_handle;  // Bottom Level Acceleration Structure
    uint32_t instance_mask;
    uint32_t sbt_offset;  // Shader Binding Table offset
} ray_tracing_instance_t;

typedef struct ray_tracing_camera {
    vec3 position;
    vec3 direction;
    vec3 up;
    vec3 right;
    float fov;
    float aspect;
    float near_plane;
    float far_plane;
    mat4 view_matrix;
    mat4 proj_matrix;
    mat4 view_proj_matrix;
} ray_tracing_camera_t;

typedef struct ray_tracing_light {
    vec3 position;
    vec3 color;
    float intensity;
    uint32_t type;  // 0=directional, 1=point, 2=spot
    vec3 direction;
    float radius;
    float inner_cone;
    float outer_cone;
} ray_tracing_light_t;

typedef struct ray_tracing_scene {
    ray_tracing_instance_t instances[MAX_RAY_TRACING_INSTANCES];
    uint32_t instance_count;
    
    ray_tracing_light_t lights[64];
    uint32_t light_count;
    
    void* tlas_handle;  // Top Level Acceleration Structure
    void* sbt_handle;   // Shader Binding Table
    
    vec3 ambient_light;
    float environment_intensity;
} ray_tracing_scene_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static struct {
    bool initialized;
    bool available;
    pthread_mutex_t mutex;
    
    // Backend support
    bool vulkan_supported;
    bool metal_supported;
    void* vulkan_context;
    void* metal_context;
    
    // Ray tracing scene
    ray_tracing_scene_t scene;
    ray_tracing_camera_t camera;
    
    // Performance statistics
    uint64_t total_rays_traced;
    uint64_t total_trace_time;
    float avg_trace_time;
    uint32_t rays_per_frame;
    float trace_time_ms;
    
    // Configuration
    uint32_t max_bounces;
    float ray_cone_angle;
    float ray_t_max;
    bool enable_denoising;
    bool enable_gi;
    bool enable_reflections;
} ray_tracing_state = {0};

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static uint32_t find_free_instance_slot(void) {
    for (uint32_t i = 0; i < MAX_RAY_TRACING_INSTANCES; i++) {
        if (ray_tracing_state.scene.instances[i].magic != RAY_TRACING_MAGIC) {
            return i;
        }
    }
    return UINT32_MAX;
}

static ray_tracing_instance_t* get_instance(uint32_t instance_id) {
    if (instance_id >= MAX_RAY_TRACING_INSTANCES) {
        return NULL;
    }
    
    ray_tracing_instance_t* instance = &ray_tracing_state.scene.instances[instance_id];
    if (instance->magic != RAY_TRACING_MAGIC) {
        return NULL;
    }
    
    return instance;
}

static int initialize_vulkan_ray_tracing(void* renderer) {
    // Initialize Vulkan ray tracing backend
    ray_tracing_state.vulkan_supported = vulkan_raytracing_init(renderer);
    if (ray_tracing_state.vulkan_supported) {
        ray_tracing_state.vulkan_context = renderer;
        printf("Vulkan ray tracing initialized\n");
        return 0;
    }
    return -1;
}

static int initialize_metal_ray_tracing(void* renderer) {
    // Initialize Metal ray tracing backend
    ray_tracing_state.metal_supported = metal_raytracing_init(renderer);
    if (ray_tracing_state.metal_supported) {
        ray_tracing_state.metal_context = renderer;
        printf("Metal ray tracing initialized\n");
        return 0;
    }
    return -1;
}

static void update_camera_matrices(void) {
    ray_tracing_camera_t* cam = &ray_tracing_state.camera;
    
    // Calculate right vector
    cam->right = vec3_cross(cam->direction, cam->up);
    cam->right = vec3_normalize(cam->right);
    
    // Recalculate up vector to ensure orthogonality
    cam->up = vec3_cross(cam->right, cam->direction);
    cam->up = vec3_normalize(cam->up);
    
    // Create view matrix
    cam->view_matrix = mat4_look_at(cam->position, 
                                   vec3_add(cam->position, cam->direction), 
                                   cam->up);
    
    // Create projection matrix
    cam->proj_matrix = mat4_perspective(cam->fov, cam->aspect, 
                                       cam->near_plane, cam->far_plane);
    
    // Combined view-projection matrix
    cam->view_proj_matrix = mat4_multiply(cam->proj_matrix, cam->view_matrix);
}

static void trace_rays_cpu(uint32_t width, uint32_t height, void* output_buffer) {
    // CPU fallback ray tracing implementation
    float* pixels = (float*)output_buffer;
    ray_tracing_camera_t* cam = &ray_tracing_state.camera;
    
    clock_t start = clock();
    uint64_t rays_traced = 0;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            // Calculate ray direction
            float u = (2.0f * x) / width - 1.0f;
            float v = (2.0f * y) / height - 1.0f;
            
            vec3 ray_dir = vec3_normalize(
                vec3_add(
                    vec3_add(
                        vec3_scale(cam->right, u * cam->aspect),
                        vec3_scale(cam->up, -v)
                    ),
                    cam->direction
                )
            );
            
            // Simple ray tracing - trace primary ray
            vec3 color = {0.1f, 0.1f, 0.2f};  // Sky color
            float closest_t = ray_tracing_state.ray_t_max;
            
            // Test against all instances
            for (uint32_t i = 0; i < ray_tracing_state.scene.instance_count; i++) {
                ray_tracing_instance_t* instance = get_instance(i);
                if (!instance) continue;
                
                // Simple sphere intersection test (placeholder)
                vec3 to_center = vec3_subtract(instance->position, cam->position);
                float projection = vec3_dot(to_center, ray_dir);
                
                if (projection > 0.0f) {
                    vec3 closest_point = vec3_add(cam->position, vec3_scale(ray_dir, projection));
                    float distance = vec3_length(vec3_subtract(closest_point, instance->position));
                    
                    if (distance < instance->scale.x && projection < closest_t) {
                        closest_t = projection;
                        // Simple shading based on material
                        color.x = instance->roughness;
                        color.y = instance->metallic;
                        color.z = instance->emissive;
                    }
                }
                rays_traced++;
            }
            
            // Write pixel color
            uint32_t pixel_index = (y * width + x) * 4;
            pixels[pixel_index + 0] = color.x;
            pixels[pixel_index + 1] = color.y;
            pixels[pixel_index + 2] = color.z;
            pixels[pixel_index + 3] = 1.0f;
        }
    }
    
    clock_t end = clock();
    float trace_time = ((float)(end - start)) / CLOCKS_PER_SEC * 1000.0f;
    
    // Update statistics
    ray_tracing_state.total_rays_traced += rays_traced;
    ray_tracing_state.total_trace_time += (uint64_t)(trace_time * 1000);
    ray_tracing_state.avg_trace_time = 
        (float)ray_tracing_state.total_trace_time / ray_tracing_state.total_rays_traced / 1000.0f;
    ray_tracing_state.rays_per_frame = rays_traced;
    ray_tracing_state.trace_time_ms = trace_time;
}

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

bool ray_tracing_init(void* renderer) {
    if (ray_tracing_state.initialized) {
        return true;  // Already initialized
    }
    
    if (pthread_mutex_init(&ray_tracing_state.mutex, NULL) != 0) {
        return false;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    // Initialize scene
    memset(&ray_tracing_state.scene, 0, sizeof(ray_tracing_state.scene));
    
    // Initialize camera
    ray_tracing_state.camera.position = (vec3){0, 0, 5};
    ray_tracing_state.camera.direction = (vec3){0, 0, -1};
    ray_tracing_state.camera.up = (vec3){0, 1, 0};
    ray_tracing_state.camera.fov = 60.0f * M_PI / 180.0f;
    ray_tracing_state.camera.aspect = 16.0f / 9.0f;
    ray_tracing_state.camera.near_plane = 0.1f;
    ray_tracing_state.camera.far_plane = 1000.0f;
    update_camera_matrices();
    
    // Initialize configuration
    ray_tracing_state.max_bounces = MAX_BOUNCES;
    ray_tracing_state.ray_cone_angle = RAY_CONE_ANGLE;
    ray_tracing_state.ray_t_max = RAY_T_MAX;
    ray_tracing_state.enable_denoising = true;
    ray_tracing_state.enable_gi = true;
    ray_tracing_state.enable_reflections = true;
    
    // Try to initialize backends
    bool backend_initialized = false;
    
    if (initialize_vulkan_ray_tracing(renderer) == 0) {
        backend_initialized = true;
    } else if (initialize_metal_ray_tracing(renderer) == 0) {
        backend_initialized = true;
    }
    
    ray_tracing_state.available = backend_initialized;
    ray_tracing_state.initialized = true;
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
    
    printf("Ray tracing system initialized: %s\n", 
           ray_tracing_state.available ? "Hardware accelerated" : "CPU fallback");
    
    return true;
}

void ray_tracing_cleanup(void) {
    if (!ray_tracing_state.initialized) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    // Cleanup backends
    if (ray_tracing_state.vulkan_supported && ray_tracing_state.vulkan_context) {
        vulkan_raytracing_cleanup();
    }
    
    if (ray_tracing_state.metal_supported && ray_tracing_state.metal_context) {
        metal_raytracing_cleanup();
    }
    
    // Cleanup scene
    for (uint32_t i = 0; i < MAX_RAY_TRACING_INSTANCES; i++) {
        ray_tracing_instance_t* instance = &ray_tracing_state.scene.instances[i];
        if (instance->magic == RAY_TRACING_MAGIC) {
            instance->magic = 0;
        }
    }
    
    ray_tracing_state.initialized = false;
    ray_tracing_state.available = false;
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
    pthread_mutex_destroy(&ray_tracing_state.mutex);
}

void ray_tracing_update_camera(vec3 position, vec3 direction, vec3 up, float fov, float aspect) {
    if (!ray_tracing_state.initialized) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    ray_tracing_state.camera.position = position;
    ray_tracing_state.camera.direction = vec3_normalize(direction);
    ray_tracing_state.camera.up = vec3_normalize(up);
    ray_tracing_state.camera.fov = fov;
    ray_tracing_state.camera.aspect = aspect;
    
    update_camera_matrices();
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
}

void ray_tracing_trace(uint32_t width, uint32_t height, void* output_buffer) {
    if (!ray_tracing_state.initialized || !output_buffer) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    if (ray_tracing_state.vulkan_supported) {
        // Use Vulkan ray tracing
        vulkan_raytracing_trace(width, height, output_buffer, &ray_tracing_state.scene, &ray_tracing_state.camera);
    } else if (ray_tracing_state.metal_supported) {
        // Use Metal ray tracing
        metal_raytracing_trace(width, height, output_buffer, &ray_tracing_state.scene, &ray_tracing_state.camera);
    } else {
        // Use CPU fallback
        trace_rays_cpu(width, height, output_buffer);
    }
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
}

bool ray_tracing_is_available(void) {
    return ray_tracing_state.initialized && ray_tracing_state.available;
}

/* ============================================================================
 * EXTENDED API FOR INSTANCE MANAGEMENT
 * ============================================================================ */

uint32_t ray_tracing_add_instance(const vec3* position, const vec3* scale, const quat* rotation,
                                 uint32_t mesh_id, uint32_t material_id) {
    if (!ray_tracing_state.initialized) {
        return UINT32_MAX;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    uint32_t slot = find_free_instance_slot();
    if (slot == UINT32_MAX) {
        pthread_mutex_unlock(&ray_tracing_state.mutex);
        return UINT32_MAX;
    }
    
    ray_tracing_instance_t* instance = &ray_tracing_state.scene.instances[slot];
    memset(instance, 0, sizeof(ray_tracing_instance_t));
    
    instance->magic = RAY_TRACING_MAGIC;
    instance->id = slot;
    instance->position = position ? *position : (vec3){0, 0, 0};
    instance->scale = scale ? *scale : (vec3){1, 1, 1};
    instance->rotation = rotation ? *rotation : (quat){0, 0, 0, 1};
    instance->mesh_id = mesh_id;
    instance->material_id = material_id;
    instance->roughness = 0.5f;
    instance->metallic = 0.0f;
    instance->emissive = 0.0f;
    instance->instance_mask = 0xFF;
    instance->sbt_offset = 0;
    instance->dirty = true;
    
    // Update transform matrix
    instance->transform = mat4_transform(&instance->position, &instance->scale, &instance->rotation);
    
    ray_tracing_state.scene.instance_count++;
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
    return slot;
}

void ray_tracing_remove_instance(uint32_t instance_id) {
    if (!ray_tracing_state.initialized) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    ray_tracing_instance_t* instance = get_instance(instance_id);
    if (instance) {
        instance->magic = 0;
        ray_tracing_state.scene.instance_count--;
    }
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
}

void ray_tracing_set_instance_material(uint32_t instance_id, float roughness, float metallic, float emissive) {
    if (!ray_tracing_state.initialized) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    ray_tracing_instance_t* instance = get_instance(instance_id);
    if (instance) {
        instance->roughness = roughness;
        instance->metallic = metallic;
        instance->emissive = emissive;
        instance->dirty = true;
    }
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
}

/* ============================================================================
 * STATISTICS AND DEBUGGING
 * ============================================================================ */

void ray_tracing_get_stats(uint64_t* total_rays, float* avg_time, uint32_t* rays_per_frame, float* frame_time) {
    if (!ray_tracing_state.initialized) {
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    if (total_rays) *total_rays = ray_tracing_state.total_rays_traced;
    if (avg_time) *avg_time = ray_tracing_state.avg_trace_time;
    if (rays_per_frame) *rays_per_frame = ray_tracing_state.rays_per_frame;
    if (frame_time) *frame_time = ray_tracing_state.trace_time_ms;
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
}

void ray_tracing_debug_print(void) {
    if (!ray_tracing_state.initialized) {
        printf("Ray tracing: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&ray_tracing_state.mutex);
    
    printf("=== Ray Tracing Debug Info ===\n");
    printf("Initialized: %s\n", ray_tracing_state.initialized ? "Yes" : "No");
    printf("Available: %s\n", ray_tracing_state.available ? "Yes" : "No");
    printf("Vulkan support: %s\n", ray_tracing_state.vulkan_supported ? "Yes" : "No");
    printf("Metal support: %s\n", ray_tracing_state.metal_supported ? "Yes" : "No");
    printf("Active instances: %u\n", ray_tracing_state.scene.instance_count);
    printf("Active lights: %u\n", ray_tracing_state.scene.light_count);
    printf("Max bounces: %u\n", ray_tracing_state.max_bounces);
    printf("Denoising: %s\n", ray_tracing_state.enable_denoising ? "Enabled" : "Disabled");
    printf("Global illumination: %s\n", ray_tracing_state.enable_gi ? "Enabled" : "Disabled");
    printf("Reflections: %s\n", ray_tracing_state.enable_reflections ? "Enabled" : "Disabled");
    
    printf("\nPerformance Statistics:\n");
    printf("Total rays traced: %llu\n", (unsigned long long)ray_tracing_state.total_rays_traced);
    printf("Average trace time: %.3f ms\n", ray_tracing_state.avg_trace_time);
    printf("Rays per frame: %u\n", ray_tracing_state.rays_per_frame);
    printf("Frame trace time: %.3f ms\n", ray_tracing_state.trace_time_ms);
    
    printf("\nCamera Information:\n");
    printf("Position: (%.2f, %.2f, %.2f)\n", 
           ray_tracing_state.camera.position.x, ray_tracing_state.camera.position.y, ray_tracing_state.camera.position.z);
    printf("Direction: (%.2f, %.2f, %.2f)\n", 
           ray_tracing_state.camera.direction.x, ray_tracing_state.camera.direction.y, ray_tracing_state.camera.direction.z);
    printf("FOV: %.1f degrees\n", ray_tracing_state.camera.fov * 180.0f / M_PI);
    printf("Aspect: %.2f\n", ray_tracing_state.camera.aspect);
    
    pthread_mutex_unlock(&ray_tracing_state.mutex);
    printf("=============================\n");
}
