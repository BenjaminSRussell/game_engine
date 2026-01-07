/*
 * pipeline_layout.h
 * Pipeline layout management
 */

#ifndef PIPELINE_LAYOUT_H
#define PIPELINE_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct pipeline_layout pipeline_layout_t;
typedef struct descriptor_set_layout descriptor_set_layout_t; 

typedef struct push_constant_range {
    uint32_t stage_flags;
    uint32_t offset;
    uint32_t size;
} push_constant_range_t;

typedef struct pipeline_layout_desc {
    descriptor_set_layout_t** set_layouts;
    uint32_t set_layout_count;
    push_constant_range_t* push_constants;
    uint32_t push_constant_count;
} pipeline_layout_desc_t;

// Lifecycle
pipeline_layout_t* pipeline_layout_create(const pipeline_layout_desc_t* desc);
void pipeline_layout_destroy(pipeline_layout_t* layout);

void* pipeline_layout_get_handle(pipeline_layout_t* layout);

#endif // PIPELINE_LAYOUT_H
