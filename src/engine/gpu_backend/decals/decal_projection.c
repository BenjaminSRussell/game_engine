#include "renderer/decals/decal_projection.h"

void decal_projection_init() {}

void decal_project_box(float position[3], float size[3], void *texture, void *output) {
    // Project decal using oriented bounding box
}

void decal_project_sphere(float position[3], float radius, void *texture, void *output) {}

void decal_clip_to_geometry(void *decal, void *mesh) {}
