/*
 * command_encoder.h
 * High-level command encoding
 */

#ifndef COMMAND_ENCODER_H
#define COMMAND_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "command_buffer.h"

// Forward declarations
typedef struct render_pass_info render_pass_info_t;
typedef struct pipeline pipeline_t;
typedef struct buffer buffer_t;
typedef struct descriptor_set descriptor_set_t;

// Render Pass Info (Simplified)
struct render_pass_info {
    // Attachments, clear values, etc.
    void* backend_handle;
};

// Pipeline (Simplified)
struct pipeline {
    void* backend_handle;
    // Bind point (Graphics/Compute)
};

// Command Buffer specific high-level functions
// Note: cmd_begin and cmd_end are in command_buffer.h/c

// Render Pass
void cmd_begin_render_pass(command_buffer_t* cmd, const render_pass_info_t* pass_info);
void cmd_end_render_pass(command_buffer_t* cmd);

// Pipeline
void cmd_bind_pipeline(command_buffer_t* cmd, pipeline_t* pipeline);

// Drawing
void cmd_draw(command_buffer_t* cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
void cmd_draw_indexed(command_buffer_t* cmd, uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

// Compute
void cmd_dispatch(command_buffer_t* cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

#endif // COMMAND_ENCODER_H
