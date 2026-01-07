/*
 * static_mesh_draw.h
 * Batched static mesh drawing API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef STATIC_MESH_DRAW_H
#define STATIC_MESH_DRAW_H

#include <stdint.h>
#include <stdbool.h>

/* Using forward declarations to avoid circular dependencies */
typedef struct rendering_context rendering_context_t;
typedef struct mesh_data mesh_data_t;
typedef struct material_instance material_instance_t;
typedef struct command_buffer command_buffer_t;

/* Handle type for static mesh draw batches */
typedef struct static_mesh_batch_handle {
    uint32_t id;
} static_mesh_batch_handle_t;

typedef struct static_mesh_draw_desc {
    uint32_t max_batches;
    uint32_t max_instances_per_batch;
    bool enable_culling;
} static_mesh_draw_desc_t;

typedef struct static_mesh_draw_info {
    mesh_data_t* mesh;
    material_instance_t* material;
    uint32_t instance_count;
    uint32_t start_instance;
    // Transform matrix would typically be in an instance buffer, 
    // but for simple non-instanced draw we can pass a pointer to matrix array
    const float* transforms; 
} static_mesh_draw_info_t;

/**
 * @brief Initialize the static mesh drawing system
 * @param desc Configuration descriptor
 * @return 0 on success, non-zero on error
 */
int static_mesh_draw_init(const static_mesh_draw_desc_t* desc);

/**
 * @brief Shutdown the static mesh drawing system
 */
void static_mesh_draw_shutdown(void);

/**
 * @brief Begin a new static mesh drawing frame/pass
 * @param cmd_buffer Command buffer to record draw calls to
 */
void static_mesh_draw_begin(command_buffer_t* cmd_buffer);

/**
 * @brief End the static mesh drawing pass
 */
void static_mesh_draw_end(void);

/**
 * @brief Draw a static mesh (batched if possible)
 * @param info Draw information
 */
void static_mesh_draw_submit(const static_mesh_draw_info_t* info);

/**
 * @brief Flush pending batches to the command buffer
 * This is called automatically by end(), but can be called manually
 */
void static_mesh_draw_flush(void);

/**
 * @brief Get statistics for the current frame
 * @param out_draw_calls Pointer to store number of draw calls
 * @param out_triangles Pointer to store number of triangles
 */
void static_mesh_draw_get_stats(uint32_t* out_draw_calls, uint32_t* out_triangles);

#endif /* STATIC_MESH_DRAW_H */
