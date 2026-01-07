/*
 * skeletal_mesh_draw.h
 * Skeletal (skinned) mesh drawing API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SKELETAL_MESH_DRAW_H
#define SKELETAL_MESH_DRAW_H

#include <stdint.h>
#include <stdbool.h>

/* Using forward declarations */
typedef struct rendering_context rendering_context_t;
typedef struct skeletal_mesh_t skeletal_mesh_t; 
typedef struct material_instance material_instance_t;
typedef struct command_buffer command_buffer_t;

/* Matrix type (assumed 4x4 float) */
typedef struct mat4_s {
    float m[16];
} mat4_t;

typedef struct skeletal_mesh_draw_desc {
    uint32_t max_bones_per_mesh;
    uint32_t bone_buffer_size;
} skeletal_mesh_draw_desc_t;

typedef struct skeletal_mesh_draw_info {
    skeletal_mesh_t* mesh;
    material_instance_t* material;
    mat4_t* bone_transforms;
    uint32_t bone_count;
    mat4_t model_matrix;
} skeletal_mesh_draw_info_t;

/**
 * @brief Initialize the skeletal mesh drawing system
 * @param desc Configuration descriptor
 * @return 0 on success, non-zero on error
 */
int skeletal_mesh_draw_init(const skeletal_mesh_draw_desc_t* desc);

/**
 * @brief Shutdown the skeletal mesh drawing system
 */
void skeletal_mesh_draw_shutdown(void);

/**
 * @brief Begin a new skeletal mesh drawing frame/pass
 * @param cmd_buffer Command buffer to record draw calls to
 */
void skeletal_mesh_draw_begin(command_buffer_t* cmd_buffer);

/**
 * @brief End the skeletal mesh drawing pass
 */
void skeletal_mesh_draw_end(void);

/**
 * @brief Upload bone matrices for a mesh
 * @param bones Array of bone matrices
 * @param count Number of bones
 * @param out_offset Output offset in the global bone buffer
 * @return 0 on success
 */
int skeletal_mesh_upload_bones(const mat4_t* bones, uint32_t count, uint32_t* out_offset);

/**
 * @brief Draw a skeletal mesh
 * @param info Draw information
 */
void skeletal_mesh_draw(const skeletal_mesh_draw_info_t* info);

#endif /* SKELETAL_MESH_DRAW_H */
