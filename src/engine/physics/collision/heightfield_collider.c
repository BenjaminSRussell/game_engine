/**
 * =================================================================================================
 *                          HEIGHTFIELD COLLIDER
 * =================================================================================================
 */

#include "heightfield_collider.h"
#include <stdlib.h>
#include <math.h>

void heightfield_create(HeightfieldCollider *hf, int width, int height, float cell_size) {
    hf->width = width;
    hf->height = height;
    hf->cell_size = cell_size;
    hf->height_data = calloc(width * height, sizeof(float));
    hf->min_height = 0.0f;
    hf->max_height = 0.0f;
}

void heightfield_destroy(HeightfieldCollider *hf) {
    free(hf->height_data);
}

void heightfield_set_height(HeightfieldCollider *hf, int x, int y, float height) {
    if (x < 0 || x >= hf->width || y < 0 || y >= hf->height) return;
    
    hf->height_data[y * hf->width + x] = height;
    
    if (height < hf->min_height) hf->min_height = height;
    if (height > hf->max_height) hf->max_height = height;
}

float heightfield_get_height(HeightfieldCollider *hf, int x, int y) {
    if (x < 0 || x >= hf->width || y < 0 || y >= hf->height) return 0.0f;
    return hf->height_data[y * hf->width + x];
}

bool heightfield_raycast(HeightfieldCollider *hf, const float *origin, const float *dir, float *hit_point) {
    // DDA-style ray marching through heightfield grid
    float t = 0.0f;
    float max_t = 1000.0f;
    float dt = hf->cell_size * 0.5f;
    
    while (t < max_t) {
        float px = origin[0] + dir[0] * t;
        float py = origin[1] + dir[1] * t;
        float pz = origin[2] + dir[2] * t;
        
        // Convert to grid coords
        int gx = (int)(px / hf->cell_size);
        int gy = (int)(pz / hf->cell_size);
        
        if (gx >= 0 && gx < hf->width && gy >= 0 && gy < hf->height) {
            float terrain_height = heightfield_get_height(hf, gx, gy);
            
            if (py <= terrain_height) {
                if (hit_point) {
                    hit_point[0] = px;
                    hit_point[1] = terrain_height;
                    hit_point[2] = pz;
                }
                return true;
            }
        }
        
        t += dt;
    }
    
    return false;
}

bool heightfield_sphere_test(HeightfieldCollider *hf, const float *pos, float radius) {
    // Test sphere against heightfield
    int gx = (int)(pos[0] / hf->cell_size);
    int gy = (int)(pos[2] / hf->cell_size);
    
    // Check neighboring cells
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = gx + dx;
            int cy = gy + dy;
            
            if (cx >= 0 && cx < hf->width && cy >= 0 && cy < hf->height) {
                float terrain_height = heightfield_get_height(hf, cx, cy);
                float dist = pos[1] - terrain_height;
                
                if (dist < radius) {
                    return true; // Collision
                }
            }
        }
    }
    
    return false;
}

void heightfield_get_normal(HeightfieldCollider *hf, float world_x, float world_z, float *normal) {
    // Calculate normal from neighboring heights
    int gx = (int)(world_x / hf->cell_size);
    int gy = (int)(world_z / hf->cell_size);
    
    float h_center = heightfield_get_height(hf, gx, gy);
    float h_right = heightfield_get_height(hf, gx + 1, gy);
    float h_up = heightfield_get_height(hf, gx, gy + 1);
    
    // Finite difference gradient
    float dx = (h_right - h_center) / hf->cell_size;
    float dz = (h_up - h_center) / hf->cell_size;
    
    // Normal is perpendicular to surface gradient
    normal[0] = -dx;
    normal[1] = 1.0f;
    normal[2] = -dz;
    
    // Normalize
    float len = sqrtf(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
    normal[0] /= len;
    normal[1] /= len;
    normal[2] /= len;
}
