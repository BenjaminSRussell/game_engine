#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float pos_offset_min[3];
    float pos_offset_max[3];
    float rot_offset_min[3];
    float rot_offset_max[3];
    float scale_offset_min[3];
    float scale_offset_max[3];
} RandomizationParams;

typedef struct {
    int count;
    bool use_instances; // vs deep copy
    RandomizationParams randomization;
} DuplicationSettings;

void duplication_init(void);
void* duplication_duplicate_entity(void *entity, DuplicationSettings *settings);
void duplication_scatter_on_surface(void *prefab, const float *surface_point, const float *surface_normal, float radius, int density);

// Randomization utilities
void randomize_transform(float *pos, float *rot, float *scale, const RandomizationParams *params);
