/**
 * @file 3d_rendering.h
 * @brief Main header for the 3D rendering pipeline
 * @details Comprehensive Unreal Engine-level rendering pipeline for managing
 *          3D graphics rendering with support for modern graphics techniques.
 *
 * Expected to scale to 3+ million lines of code across all modules.
 *
 * @author Benjamin Russell
 * @date 2026-01-05
 */

#ifndef ENGINE_3D_RENDERING_H
#define ENGINE_3D_RENDERING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ==================== Core Type Definitions ==================== */

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float m[4][4];
} Mat4;

typedef uint32_t RenderHandle;
typedef uint32_t ResourceHandle;
typedef uint32_t PassHandle;

#define INVALID_HANDLE 0xFFFFFFFFU

/* ==================== Rendering Pipeline Initialization ==================== */

/**
 * @brief Configuration for the 3D rendering pipeline
 */
typedef struct {
    uint32_t max_width;
    uint32_t max_height;
    uint32_t max_frame_buffering;
    uint32_t max_render_passes;
    uint32_t max_resources;
    uint32_t max_meshes;
    uint32_t max_materials;
    uint32_t max_lights;
    uint32_t max_shadows;
    uint32_t enable_ray_tracing;
    uint32_t enable_async_compute;
    uint32_t enable_dynamic_resolution;
    uint32_t enable_variable_rate_shading;
    uint32_t max_memory_mb;
} RenderPipelineConfig;

typedef struct RenderPipeline RenderPipeline;

/**
 * @brief Initialize the 3D rendering pipeline
 * @param[in] config Configuration for the pipeline
 * @return Handle to the initialized pipeline, INVALID_HANDLE on failure
 */
RenderHandle rendering_pipeline_create(const RenderPipelineConfig* config);

/**
 * @brief Destroy the rendering pipeline and release all resources
 * @param[in] pipeline_handle Handle to the pipeline to destroy
 */
void rendering_pipeline_destroy(RenderHandle pipeline_handle);

/**
 * @brief Get the rendering pipeline instance
 * @param[in] pipeline_handle Handle to the pipeline
 * @return Pointer to the pipeline structure, NULL if invalid
 */
RenderPipeline* rendering_pipeline_get(RenderHandle pipeline_handle);

/* ==================== Frame Management ==================== */

/**
 * @brief Begin a new rendering frame
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] delta_time Time since last frame in seconds
 * @return true on success, false on failure
 */
bool rendering_frame_begin(RenderHandle pipeline_handle, float delta_time);

/**
 * @brief End the current rendering frame and submit to GPU
 * @param[in] pipeline_handle Handle to the pipeline
 * @return true on success, false on failure
 */
bool rendering_frame_end(RenderHandle pipeline_handle);

/**
 * @brief Wait for GPU to finish rendering previous frame
 * @param[in] pipeline_handle Handle to the pipeline
 * @return true on success, false on failure
 */
bool rendering_frame_wait(RenderHandle pipeline_handle);

/**
 * @brief Get current frame index
 * @param[in] pipeline_handle Handle to the pipeline
 * @return Current frame index (0-based)
 */
uint32_t rendering_frame_get_index(RenderHandle pipeline_handle);

/* ==================== Resolution and Viewport Management ==================== */

/**
 * @brief Set the rendering resolution
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] width Render target width
 * @param[in] height Render target height
 * @return true on success, false on failure
 */
bool rendering_set_resolution(RenderHandle pipeline_handle, uint32_t width, uint32_t height);

/**
 * @brief Get current rendering resolution
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[out] width Pointer to store width
 * @param[out] height Pointer to store height
 */
void rendering_get_resolution(RenderHandle pipeline_handle, uint32_t* width, uint32_t* height);

/**
 * @brief Set viewport parameters
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] x Viewport X offset
 * @param[in] y Viewport Y offset
 * @param[in] width Viewport width
 * @param[in] height Viewport height
 * @param[in] min_depth Minimum depth value
 * @param[in] max_depth Maximum depth value
 * @return true on success, false on failure
 */
bool rendering_set_viewport(RenderHandle pipeline_handle,
                           uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height,
                           float min_depth, float max_depth);

/* ==================== Scene Management ==================== */

typedef struct RenderScene RenderScene;

/**
 * @brief Create a new render scene
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] name Name of the scene
 * @return Handle to the created scene, INVALID_HANDLE on failure
 */
RenderHandle rendering_scene_create(RenderHandle pipeline_handle, const char* name);

/**
 * @brief Destroy a render scene
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] scene_handle Handle to the scene
 */
void rendering_scene_destroy(RenderHandle pipeline_handle, RenderHandle scene_handle);

/**
 * @brief Get render scene instance
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] scene_handle Handle to the scene
 * @return Pointer to the scene, NULL if invalid
 */
RenderScene* rendering_scene_get(RenderHandle pipeline_handle, RenderHandle scene_handle);

/* ==================== Camera Management ==================== */

typedef struct {
    Vec3 position;
    Vec3 forward;
    Vec3 up;
    float fov_y;
    float near_plane;
    float far_plane;
    float aspect_ratio;
} CameraParameters;

/**
 * @brief Set the active camera for rendering
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] scene_handle Handle to the scene
 * @param[in] camera_params Camera parameters
 * @return Handle to the camera, INVALID_HANDLE on failure
 */
RenderHandle rendering_camera_set(RenderHandle pipeline_handle,
                                  RenderHandle scene_handle,
                                  const CameraParameters* camera_params);

/**
 * @brief Update camera parameters
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] camera_handle Handle to the camera
 * @param[in] camera_params New camera parameters
 * @return true on success, false on failure
 */
bool rendering_camera_update(RenderHandle pipeline_handle,
                             RenderHandle camera_handle,
                             const CameraParameters* camera_params);

/* ==================== Rendering Execution ==================== */

/**
 * @brief Execute the complete rendering pipeline for a frame
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] scene_handle Handle to the scene to render
 * @return true on success, false on failure
 */
bool rendering_execute(RenderHandle pipeline_handle, RenderHandle scene_handle);

/**
 * @brief Execute a specific rendering pass
 * @param[in] pipeline_handle Handle to the pipeline
 * @param[in] pass_handle Handle to the pass
 * @return true on success, false on failure
 */
bool rendering_pass_execute(RenderHandle pipeline_handle, PassHandle pass_handle);

#ifdef __cplusplus
}
#endif

#endif /* ENGINE_3D_RENDERING_H */
