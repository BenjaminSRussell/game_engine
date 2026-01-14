#pragma once

typedef struct {
    float *height_data;  // 2D height array
    int width;
    int height;
    float cell_size;
    float min_height;
    float max_height;
} HeightfieldCollider;

void heightfield_create(HeightfieldCollider *hf, int width, int height, float cell_size);
void heightfield_destroy(HeightfieldCollider *hf);

void heightfield_set_height(HeightfieldCollider *hf, int x, int y, float height);
float heightfield_get_height(HeightfieldCollider *hf, int x, int y);

// Collision
bool heightfield_raycast(HeightfieldCollider *hf, const float *origin, const float *dir, float *hit_point);
bool heightfield_sphere_test(HeightfieldCollider *hf, const float *pos, float radius);
void heightfield_get_normal(HeightfieldCollider *hf, float world_x, float world_z, float *normal);
