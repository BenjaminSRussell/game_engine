#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t *selected_entities;
    int entity_count;
    
    // Pivot modes
    bool use_shared_pivot;
    float shared_pivot[3];
    
    // Transform
    bool relative_mode; // vs absolute
} MultiEditContext;

void multi_edit_init(MultiEditContext *ctx);
void multi_edit_set_selection(MultiEditContext *ctx, uint64_t *entities, int count);

// Pivot calculation
void multi_edit_calculate_pivot(MultiEditContext *ctx);

// Transform operations
void multi_edit_translate(MultiEditContext *ctx, const float *delta, bool relative);
void multi_edit_rotate(MultiEditContext *ctx, const float *axis, float angle, bool relative);
void multi_edit_scale(MultiEditContext *ctx, const float *scale, bool relative);

// Property distribution
void multi_edit_distribute_property(MultiEditContext *ctx, const char *property_name, float start, float end);

// Component operations
void multi_edit_add_component(MultiEditContext *ctx, const char *component_type);
void multi_edit_remove_component(MultiEditContext *ctx, const char *component_type);
