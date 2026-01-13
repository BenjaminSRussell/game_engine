/**
 * @file rigid_body_rendering.h
 * @brief Rigid body physics rendering integration
 * @details Handles rendering of rigid bodies, dynamic lighting, shadows, and motion blur for physics objects
 *
 * This module integrates rigid body physics with the rendering pipeline, allowing
 * physics-simulated objects to be efficiently rendered with all visual effects.
 */

#ifndef RENDER_RIGID_BODY_PHYSICS_H
#define RENDER_RIGID_BODY_PHYSICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct RenderPhysicsManager RenderPhysicsManager;
typedef struct RigidBodyRenderData RigidBodyRenderData;

/**
 * @brief Rigid body render flags
 */
typedef enum {
    RIGIDBODY_RENDER_CAST_SHADOW = 0x0001,
    RIGIDBODY_RENDER_RECEIVE_SHADOW = 0x0002,
    RIGIDBODY_RENDER_DYNAMIC_LIGHT = 0x0004,
    RIGIDBODY_RENDER_MOTION_BLUR = 0x0008,
    RIGIDBODY_RENDER_CAUSTICS = 0x0010,
    RIGIDBODY_RENDER_DISTORTION = 0x0020,
    RIGIDBODY_RENDER_DECALS = 0x0040,
} RigidBodyRenderFlags;

/**
 * @brief Rigid body render data
 */
typedef struct {
    uint32_t id;
    uint32_t mesh_id;
    uint32_t material_id;
    float transform[16];
    float prev_transform[16];
    float velocity[3];
    float angular_velocity[3];
    uint32_t render_flags;
    float motion_blur_scale;
    bool is_kinematic;
    bool is_sleeping;
} RigidBodyRenderData;

/**
 * @brief Create physics render manager
 * @param[in] max_rigid_bodies Maximum rigid bodies to render
 * @return Pointer to manager, NULL on failure
 */
RenderPhysicsManager* render_physics_create(uint32_t max_rigid_bodies);

/**
 * @brief Destroy physics render manager
 * @param[in] manager Manager to destroy
 */
void render_physics_destroy(RenderPhysicsManager* manager);

/**
 * @brief Register rigid body for rendering
 * @param[in] manager Physics manager
 * @param[in] body_data Rigid body render data
 * @return Body ID, 0 on failure
 */
uint32_t render_physics_rigbody_register(RenderPhysicsManager* manager,
                                         const RigidBodyRenderData* body_data);

/**
 * @brief Update rigid body render data
 * @param[in] manager Physics manager
 * @param[in] body_id Body ID
 * @param[in] body_data Updated render data
 * @return true on success
 */
bool render_physics_rigbody_update(RenderPhysicsManager* manager,
                                   uint32_t body_id,
                                   const RigidBodyRenderData* body_data);

/**
 * @brief Update rigid body transform from physics engine
 * @param[in] manager Physics manager
 * @param[in] body_id Body ID
 * @param[in] transform 4x4 transform matrix
 * @param[in] velocity Linear velocity for motion blur
 * @return true on success
 */
bool render_physics_rigbody_set_transform(RenderPhysicsManager* manager,
                                          uint32_t body_id,
                                          const float* transform,
                                          const float* velocity);

/**
 * @brief Render all rigid bodies
 * @param[in] manager Physics manager
 * @return true on success
 */
bool render_physics_rigbody_render_all(RenderPhysicsManager* manager);

/**
 * @brief Get rigid body render data
 * @param[in] manager Physics manager
 * @param[in] body_id Body ID
 * @return Pointer to body data, NULL if not found
 */
const RigidBodyRenderData* render_physics_rigbody_get(RenderPhysicsManager* manager,
                                                      uint32_t body_id);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_RIGID_BODY_PHYSICS_H */
