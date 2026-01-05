#include "asset_editor/editor_types.h"
#include "asset_editor/asset_editor.h"
#include "core/memory.h"
#include "core/file_system.h"
#include <renderer/renderer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// Import/Export System
typedef struct {
    // Supported formats
    bool supports_fbx;
    bool supports_obj;
    bool supports_gltf;
    bool supports_png;
    bool supports_jpg;
    bool supports_tga;
    bool supports_dae;
    bool supports_x3d;
    
    // Import settings
    bool generate_normals;
    bool generate_tangents;
    bool optimize_mesh;
    bool merge_vertices;
    float scale_factor;
    bool flip_uvs;
    bool center_model;
    
    // Export settings
    bool include_textures;
    bool include_materials;
    bool include_animations;
    bool compress_data;
    int export_quality; // 1-100
    bool embed_textures;
    
} ImportExportSettings;

static ImportExportSettings g_import_export_settings = {
    .supports_fbx = true,
    .supports_obj = true,
    .supports_gltf = true,
    .supports_png = true,
    .supports_jpg = true,
    .supports_tga = true,
    .supports_dae = false,
    .supports_x3d = false,
    
    .generate_normals = true,
    .generate_tangents = true,
    .optimize_mesh = true,
    .merge_vertices = true,
    .scale_factor = 1.0f,
    .flip_uvs = false,
    .center_model = true,
    
    .include_textures = true,
    .include_materials = true,
    .include_animations = true,
    .compress_data = false,
    .export_quality = 90,
    .embed_textures = false
};

// Forward declarations
static bool import_fbx_model(const char* path, Model3D* model);
static bool import_obj_model(const char* path, Model3D* model);
static bool import_gltf_model(const char* path, Model3D* model);
static bool import_png_texture(const char* path, uint32_t** texture_data, int* width, int* height);
static bool import_jpg_texture(const char* path, uint32_t** texture_data, int* width, int* height);
static bool import_tga_texture(const char* path, uint32_t** texture_data, int* width, int* height);

static bool export_fbx_model(const char* path, const Model3D* model);
static bool export_obj_model(const char* path, const Model3D* model);
static bool export_gltf_model(const char* path, const Model3D* model);
static bool export_png_texture(const char* path, const uint32_t* texture_data, int width, int height);
static bool export_jpg_texture(const char* path, const uint32_t* texture_data, int width, int height);
static bool export_tga_texture(const char* path, const uint32_t* texture_data, int width, int height);

static void optimize_model_data(Model3D* model);
static void generate_missing_normals(Model3D* model);
static void generate_missing_tangents(Model3D* model);
static void center_model(Model3D* model);
static void scale_model(Model3D* model, float scale);

bool import_export_init(void) {
    printf("Import/Export system initialized\n");
    return true;
}

void import_export_cleanup(void) {
    printf("Import/Export system cleaned up\n");
}

bool import_export_load_settings(const char* config_path) {
    // Load settings from configuration file
    // This would parse a JSON or XML configuration file
    return true;
}

bool import_export_save_settings(const char* config_path) {
    // Save current settings to configuration file
    // This would write a JSON or XML configuration file
    return true;
}

// Import Functions
bool import_3d_model(const char* path, Model3D* model) {
    if (!path || !model) return false;
    
    // Determine file format from extension
    const char* extension = strrchr(path, '.');
    if (!extension) return false;
    
    extension++; // Skip the dot
    
    if (strcasecmp(extension, "fbx") == 0) {
        return import_fbx_model(path, model);
    } else if (strcasecmp(extension, "obj") == 0) {
        return import_obj_model(path, model);
    } else if (strcasecmp(extension, "gltf") == 0 || strcasecmp(extension, "glb") == 0) {
        return import_gltf_model(path, model);
    } else {
        printf("Unsupported 3D model format: %s\n", extension);
        return false;
    }
}

bool import_2d_texture(const char* path, uint32_t** texture_data, int* width, int* height) {
    if (!path || !texture_data || !width || !height) return false;
    
    // Determine file format from extension
    const char* extension = strrchr(path, '.');
    if (!extension) return false;
    
    extension++; // Skip the dot
    
    if (strcasecmp(extension, "png") == 0) {
        return import_png_texture(path, texture_data, width, height);
    } else if (strcasecmp(extension, "jpg") == 0 || strcasecmp(extension, "jpeg") == 0) {
        return import_jpg_texture(path, texture_data, width, height);
    } else if (strcasecmp(extension, "tga") == 0) {
        return import_tga_texture(path, texture_data, width, height);
    } else {
        printf("Unsupported texture format: %s\n", extension);
        return false;
    }
}

bool import_sprite_sheet(const char* path, Sprite2D* sprite) {
    if (!path || !sprite) return false;
    
    // Load texture data
    uint32_t* texture_data = NULL;
    int width, height;
    
    if (!import_2d_texture(path, &texture_data, &width, &height)) {
        return false;
    }
    
    // Set sprite properties
    sprite->texture_id = renderer_create_texture(texture_data, width, height);
    sprite->size = (vec2){(float)width, (float)height};
    sprite->frame_count = 1;
    sprite->current_frame = 0;
    sprite->frame_time = 0.1f;
    sprite->animation_speed = 1.0f;
    sprite->looping = false;
    
    // Default sheet layout (single frame)
    sprite->sheet_size = (vec2){1.0f, 1.0f};
    sprite->frame_size = sprite->size;
    sprite->columns = 1;
    sprite->rows = 1;
    
    core_free(texture_data);
    return true;
}

// Export Functions
bool export_3d_model(const char* path, const Model3D* model, const char* format) {
    if (!path || !model || !format) return false;
    
    if (strcasecmp(format, "fbx") == 0) {
        return export_fbx_model(path, model);
    } else if (strcasecmp(format, "obj") == 0) {
        return export_obj_model(path, model);
    } else if (strcasecmp(format, "gltf") == 0) {
        return export_gltf_model(path, model);
    } else {
        printf("Unsupported export format: %s\n", format);
        return false;
    }
}

bool export_2d_texture(const char* path, const uint32_t* texture_data, int width, int height, const char* format) {
    if (!path || !texture_data || !format) return false;
    
    if (strcasecmp(format, "png") == 0) {
        return export_png_texture(path, texture_data, width, height);
    } else if (strcasecmp(format, "jpg") == 0) {
        return export_jpg_texture(path, texture_data, width, height);
    } else if (strcasecmp(format, "tga") == 0) {
        return export_tga_texture(path, texture_data, width, height);
    } else {
        printf("Unsupported export format: %s\n", format);
        return false;
    }
}

bool export_sprite_sheet(const char* path, const Sprite2D* sprite, const char* format) {
    if (!path || !sprite || !format) return false;
    
    // Get texture data from GPU
    int width = (int)sprite->size.x;
    int height = (int)sprite->size.y;
    uint32_t* texture_data = core_alloc(width * height * sizeof(uint32_t));
    
    if (!texture_data) return false;
    
    if (!renderer_get_texture_data(sprite->texture_id, texture_data, width, height)) {
        core_free(texture_data);
        return false;
    }
    
    bool success = export_2d_texture(path, texture_data, width, height, format);
    core_free(texture_data);
    
    return success;
}

// Format-specific import implementations
static bool import_fbx_model(const char* path, Model3D* model) {
    // FBX import implementation
    // This would use a library like Assimp or FBX SDK
    
    printf("Importing FBX model: %s\n", path);
    
    // Placeholder implementation
    model->mesh_count = 1;
    model->material_count = 1;
    
    // Allocate mesh
    model->meshes = core_alloc(sizeof(Mesh));
    model->materials = core_alloc(sizeof(Material));
    
    if (!model->meshes || !model->materials) {
        return false;
    }
    
    // Create a simple cube as placeholder
    Mesh* mesh = &model->meshes[0];
    mesh->vertex_count = 24; // 6 faces * 4 vertices
    mesh->index_count = 36;  // 6 faces * 6 indices
    
    mesh->vertices = core_alloc(mesh->vertex_count * sizeof(Vertex));
    mesh->indices = core_alloc(mesh->index_count * sizeof(uint32_t));
    
    if (!mesh->vertices || !mesh->indices) {
        return false;
    }
    
    // Generate cube vertices
    float size = 1.0f;
    int vertex_index = 0;
    
    // Front face
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size, size}, {0, 0, 1}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size, size}, {0, 0, 1}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size,  size, size}, {0, 0, 1}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size,  size, size}, {0, 0, 1}, {0, 0}, {1, 1, 1, 1}};
    
    // Back face
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size, -size}, {0, 0, -1}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size, -size}, {0, 0, -1}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size,  size, -size}, {0, 0, -1}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size,  size, -size}, {0, 0, -1}, {0, 0}, {1, 1, 1, 1}};
    
    // Top face
    mesh->vertices[vertex_index++] = (-style){{-size,  size, -size}, {0, 1, 0}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = ( circulation{{ style类的,  size,  size}, {0, 1, 0}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size,  size,  size}, {0, 1, 0}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size,  size,  size}, {0, 1, 0}, {0, 0}, {1, 1, 1, 1}};
    
    // Bottom face
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size, -size}, {0, -1, 0}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size, -size}, {0, -1, 0}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size,  size}, {0, -1, 0}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size,  size}, {0, -1, 0}, {0, 0}, {1, 1, 1, 1}};
    
    // Right face
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size, -size}, {1, 0, 0}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size, -size,  size}, {1, 0, 0}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size,  size,  size}, {1, 0, 0}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{ size,  size, -size}, {1, 0, 0}, {0, 0}, {1, 1, 1, 1}};
    
    // Left face
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size,  size}, {-1, 0, 0}, {0, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size, -size, -size}, {-1, 0, 0}, {1, 1}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size,  size, -size}, {-1, 0, 0}, {1, 0}, {1, 1, 1, 1}};
    mesh->vertices[vertex_index++] = (Vertex){{-size,  size,  size}, {-1, 0, 0}, {0, 0}, {1, 1, 1, 1}};
    
    // Generate indices
    uint32_t indices[] = {
        0, 1, 2, 0, 2, 3,  // Front
        4, 5, 6, 4, 6, 7,  // Back
        8, 9, 10, 8, 10, 11, // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };
    
    memcpy(mesh->indices, indices, sizeof(indices));
    
    // Set material
    mesh->material_index = 0;
    
    // Create default material
    Material* material = &model->materials[0];
    material->albedo_color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
    material->metallic = 0.0f;
    material->roughness = 0.5f;
    material->ao = 1.0f;
    material->emission = (vec3){0.0f, 0.0f, 0.0f};
    material->alpha = 1.0f;
    
    // Set metadata
    strcpy(model->name, "Imported FBX Model");
    strcpy(model->author, "Asset Editor");
    strcpy(model->description, "Model imported from FBX file");
    
    // Calculate bounds
    model->min_bounds = (vec3){-size, -size, -size};
    model->max_bounds = (vec3){size, size, size};
    model->center = (vec3){0.0f, 0.0f, 0.0f};
    model->radius = sqrtf(3.0f) * size;
    
    // Apply import settings
    if (g_import_export_settings.center_model) {
        center_model(model);
    }
    
    if (fabsf(g_import_export_settings.scale_factor - 1.0f) > 0.001f) {
        scale_model(model, g_import_export_settings.scale_factor);
    }
    
    if (g_import_export_settings.optimize_mesh) {
        optimize_model_data(model);
    }
    
    if (g_import_export_settings.generate_normals) {
        generate_missing_normals(model);
    }
    
    if (g_import_export_settings.generate_tangents) {
        generate_missing_tangents(model);
    }
    
    return true;
}

static bool import_obj_model(const char* path, Model3D* model) {
    // OBJ import implementation
    printf("Importing OBJ model: %s\n", path);
    
    // For now, delegate to FBX import with same logic
    return import_fbx_model(path, model);
}

static bool import_gltf_model(const char* path, Model3D* model) {
    // glTF import implementation
    printf("Importing glTF model: %s\n", path);
    
    // For now, delegate to FBX import with same logic
    return import_fbx_model(path, model);
}

static bool import_png_texture(const char* path, uint32_t** texture_data, int* width, int* height) {
    // PNG import implementation using stb_image or similar
    printf("Importing PNG texture: %s\n", path);
    
    // Placeholder implementation
    *width = 256;
    *height = 256;
    *texture_data = core_alloc((*width) * (*height) * sizeof(uint32_t));
    
    if (!*texture_data) return false;
    
    // Generate a simple test pattern
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            uint32_t color = ((x % 32) < 16) ^脚架 ((y % 32) < 16) ? 0xFFFFFFFF : 0xFF000000;
            (*texture_data)[y * (*width) + x] = color;
        }
    }
    
    return true;
}

static bool import_jpg_texture(const char* path, uint32_t** texture_data, int* width, int* height) {
    // JPG import implementation
    printf("Importing JPG texture: %s\n", path);
    
    // For now, delegate to PNG import
    return import_png_texture(path, texture_data, width, height);
}

static bool import_tga_texture(const char* path, uint32_t** texture_data, int* width, int* height) {
    // TGA import implementation
    printf("Importing TGA texture: %s\n", path);
    
    // For now, delegate to PNG import
    return import_png_texture(path, texture_data, width, height);
}

// Format-specific export implementations
static bool export_fbx_model(const char* path, const Model3D* model) {
    // FBX export implementation
    printf("Exporting FBX model: %s\n", path);
    
    // Placeholder implementation
    FILE* file = fopen(path, "wb");
    if (!file) return false;
    
    // Write placeholder FBX header
    const char* header = "Kaydara FBX Binary  \0\0\0\0\0\0\0\0";
    fwrite(header, 23, 1, file);
    
    fclose(file);
    return true;
}

static bool export_obj_model(const char* path, const Model3D* model) {
    // OBJ export implementation
    printf("Exporting OBJ model: %s\n", path);
    
    FILE* file = fopen(path, "w");
    if (!file) return false;
    
    // Write OBJ header
    fprintf(file, "# Exported by Minecraft v2 Asset Editor\n");
    fprintf(file, "# Model: %s\n", model->name);
    fprintf(file, "# Author: %s\n", model->author);
    fprintf(file, "\n");
    
    // Write vertices
    for (uint32_t mesh_idx = 0; mesh_idx < model->mesh_count; mesh_idx++) {
        const Mesh* mesh = &model->meshes[mesh_idx];
        
        fprintf(file, "# Mesh %d\n", mesh_idx);
        
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            const Vertex* v = &mesh->vertices[i];
            fprintf(file, "v %.6f %.6f %.6f\n", v->position.x, v->position.y, v->position.z);
        }
        
        // Write texture coordinates
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            const Vertex* v = &mesh->vertices[i];
            fprintf(file, "vt %.6f %.6f\n", v->texcoord.x, v->texcoord.y);
        }
        
        // Write normals
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            const Vertex* v = &mesh->vertices[i];
            fprintf(file, "vn %.6f %.6f %.6f\n", v->normal.x, v->normal.y, v->normal.z);
        }
        
        // Write faces
        fprintf(file, "usemtl material_%d\n", mesh->material_index);
        for (uint32_t i = 0; i < mesh->index_count; i += 3) {
            uint32_t i0 = mesh->indices[i] + 1;
            uint32_t i1 = mesh->indices[i + 1] + 1;
            uint32_t i2 = mesh->indices[i + 2] + 1;
            
            fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                    i0, i0, i0, i1, i1, i1, i2, i2, i2);
        }
        
        fprintf(file, "\n");
    }
    
    fclose(file);
    return true;
}

static bool export_gltf_model(const char* path, const Model3D* model) {
    // glTF export implementation
    printf("Exporting glTF model: %s\n", path);
    
    // Placeholder implementation
    FILE* file = fopen(path, "w");
    if (!file) return false;
    
    // Write minimal glTF JSON
    fprintf(file, "{\n");
    fprintf(file, "  \"asset\": {\n");
    fprintf(file, "    \"version\": \"2.0\",\n");
    fprintf(file, "    \"generator\": \"Minecraft v2 Asset Editor\"\n");
    fprintf(file, "  },\n");
    fprintf(file, "  \"scenes\": [{\"nodes\": [0]}],\n");
    fprintf(file, "  \"nodes\": [{\"mesh\": 0}],\n");
    fprintf(file, "  \"meshes\": [{\"primitives\": [{\"attributes\": {\"POSITION\": 0}, \"indices\": 1}]}],\n");
    fprintf(file, "  \"accessors\": [\n");
    fprintf(file, "    {\"bufferView\": 0, \"componentType\": 5123, \"count\": %d, \"type\": \"SCALAR\"},\n", model->meshes[0].vertex_count);
    fprintf(file, "    {\"bufferView\": 1, \"componentType\": 5123, \"count\": %d, \"type\": \"SCALAR\"}\n", model->meshes[0].index_count);
    fprintf(file, "  ],\n");
    fprintf(file, "  \"bufferViews\": [\n");
    fprintf(file, "    {\"buffer\": 0, \"byteOffset\": 0, \"byteLength\": %d},\n", model->meshes[0].vertex_count * sizeof(Vertex));
    fprintf(file, "    {\"buffer\": 0, \"byteOffset\": %d, \"byteLength\": %d}\n", 
            model->meshes[0].vertex_count * sizeof(Vertex), model->meshes[0].index_count * sizeof(uint32_t));
    fprintf(file, "  ],\n");
    fprintf(file, "  \"buffers\": [{\"byteLength\": %d}]\n", 
            model->meshes[0].vertex_count * sizeof(Vertex) + model->meshes[0].index_count * sizeof(uint32_t));
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

static bool export_png_texture(const char* path, const uint32_t* texture_data, int width, int height) {
    // PNG export implementation
    printf("Exporting PNG texture: %s\n", path);
    
    // Placeholder implementation
    FILE* file = fopen(path, "wb");
    if (!file) return false;
    
    // Write simple PPM header (placeholder for PNG)
    fprintf(file, "P3\n%d %d\n255\n", width, height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t pixel = texture_data[y * width + x];
            uint8_t r = (pixel >> 0) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = (pixel >> 16) & 0xFF;
            
            fprintf(file, "%d %d %d ", r, g, b);
            if (x % 5 == 4) fprintf(file, "\n");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return true;
}

static bool export_jpg_texture(const char* path, const uint32_t* texture_data, int width, int height) {
    // JPG export implementation
    printf("Exporting JPG texture: %s\n", path);
    
    // For now, delegate to PNG export
    return export_png_texture(path, texture_data, width, height);
}

static bool export_tga_texture(const char* path, const uint32_t* texture_data, int width, int height) {
    // TGA export implementation
    printf("Exporting TGA texture: %s\n", path);
    
    // For now, delegate to PNG export
    return export_png_texture(path, texture_data, width, height);
}

// Utility functions
static void optimize_model_data(Model3D* model) {
    // Mesh optimization - remove duplicate vertices, optimize index order, etc.
    printf("Optimizing model data...\n");
}

static void generate_missing_normals(Model3D* model) {
    // Generate vertex normals if they don't exist
    printf("Generating missing normals...\n");
    
    for (uint32_t mesh_idx = 0; mesh_idx < model->mesh_count; mesh_idx++) {
        Mesh* mesh = &model->meshes[mesh_idx];
        
        // Initialize normals to zero
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            mesh->vertices[i].normal = (vec3){0.0f, 0.0f, 0.0f};
        }
        
        // Calculate face normals and add to vertex normals
        for (uint32_t i = 0; i < mesh->index_count; i += 3) {
            uint32_t i0 = mesh->indices[i];
            uint32_t i1 = mesh->indices[i + 1];
            uint32_t i2 = mesh->indices[i + 2];
            
            if (i0 < mesh->vertex_count && i1 < mesh->vertex_count && i2 < mesh->vertex_count) {
                Vertex* v0 = &mesh->vertices[i0];
                Vertex* v1 = &mesh->vertices[i1];
                Vertex* v2 = &mesh->vertices[i2];
                
                // Calculate face normal
                vec3 edge1 = vec3_sub(v1->position, v0->position);
                vec3 edge2 = vec3_sub(v2->position, v0->position);
                vec3 normal = vec3_cross(edge1, edge2);
                normal = vec3_normalize(normal);
                
                // Add to vertex normals
                v0->normal = vec3_add(v0->normal, normal);
                v1->normal = vec3_add(v1->normal, normal);
                v2->normal = vec3_add(v2->normal, normal);
            }
        }
        
        // Normalize vertex normals
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            mesh->vertices[i].normal = vec3_normalize(mesh->vertices[i].normal);
        }
    }
}

static void generate_missing_tangents(Model3D* model) {
    // Generate tangent vectors if they don't exist
    printf("Generating missing tangents...\n");
    
    for (uint32_t mesh_idx = 0; mesh_idx < model->mesh_count; mesh_idx++) {
        Mesh* mesh = &model->meshes[mesh_idx];
        
        // Initialize tangents to zero
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            mesh->vertices[i].tangent = (vec3){0.0f, 0.0f, 0.0f};
            mesh->vertices[i].bitangent = (vec3){0.0f, 0.0f, 0.0f};
        }
        
        // Calculate tangents for each triangle
        for (uint32_t i = 0; i < mesh->index_count; i += 3) {
            uint32_t i0 = mesh->indices[i];
            uint32_t i1 = mesh->indices[i + 1];
            uint32_t i2 = mesh->indices[i + 2];
            
            if (i0 < mesh->vertex_count && i1 < mesh->vertex_count && i2 < mesh->vertex_count) {
                Vertex* v0 = &mesh->vertices[i0];
                Vertex* v1 = &mesh->vertices[i1];
                Vertex* v2 = &mesh->vertices[i2];
                
                // Edge vectors
                vec3 delta_pos1 = vec3_sub(v1->position, v0->position);
                vec3 delta_pos2 = vec3_sub(v2->position, v0->position);
                
                // UV delta vectors
                vec2 delta_uv1 = vec2_sub(v1->texcoord, v0->texcoord);
                vec2 delta_uv2 = vec2_sub(v2->texcoord, v0->texcoord);
                
                // Calculate tangent and bitangent
                float r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
                vec3 tangent = vec3_mul_scalar(vec3_sub(
                    vec3_mul_scalar(delta_pos1, delta_uv2.y),
                    vec3_mul_scalar(delta_pos2, delta_uv1.y)), r);
                vec3 bitangent = vec3_mul_scalar(vec3_sub(
                    vec3_mul_scalar(delta_pos2, delta_uv1.x),
                    vec3_mul_scalar(delta_pos1, delta_uv2.x)), r);
                
                // Add to vertex tangents
                v0->tangent = vec3_add(v0->tangent, tangent);
                v1->tangent = vec3_add(v1->tangent, tangent);
                v2->tangent = vec3_add(v2->tangent, tangent);
                
                v0->bitangent = vec3_add(v0->bitangent, bitangent);
                v1->bitangent = vec3_add(v1->bitangent, bitangent);
                v2->bitangent = vec3_add(v2->bitangent, bitangent);
            }
        }
        
        // Normalize and orthogonalize tangents
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            Vertex* v = &mesh->vertices[i];
            
            // Gram-Schmidt orthogonalize
            v->tangent = vec3_normalize(v->tangent);
            v->tangent = vec3_sub(v->tangent, vec3_mul_scalar(v->normal, vec3_dot(v->tangent, v->normal)));
            v->tangent = vec3_normalize(v->tangent);
            
            // Calculate handedness
            vec3 cross = vec3_cross(v->normal, v->tangent);
            float handedness = (vec3_dot(cross, v->bitangent) < 0.0f) ? -1.0f : 1.0f;
            
            v->bitangent = vec3_mul_scalar(cross, handedness);
        }
    }
}

static void center_model(Model3D* model) {
    // Center model at origin
    printf("Centering model...\n");
    
    vec3 center = model->center;
    
    for (uint32_t mesh_idx = 0; mesh_idx < model->mesh_count; mesh_idx++) {
        Mesh* mesh = &model->meshes[mesh_idx];
        
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            mesh->vertices[i].position = vec3_sub(mesh->vertices[i].position, center);
        }
    }
    
    // Update bounds
    model->min_bounds = vec3_sub(model->min_bounds, center);
    model->max_bounds = vec3_sub(model->max_bounds, center);
    model->center = (vec3){0.0f, 0.0f, 0.0f};
}

static void scale_model(Model3D* model, float scale) {
    // Scale model by factor
    printf("Scaling model by factor %.3f...\n", scale);
    
    for (uint32_t mesh_idx = 0; mesh_idx < model->mesh_count; mesh_idx++) {
        Mesh* mesh = &model->meshes[mesh_idx];
        
        for (uint32_t i = 0; i < mesh->vertex_count; i++) {
            mesh->vertices[i].position = vec3_mul_scalar(mesh->vertices[i].position, scale);
        }
    }
    
    // Update bounds
    model->min_bounds = vec3_mul_scalar(model->min_bounds, scale);
    model->max_bounds = vec3_mul_scalar(model->max_bounds, scale);
    model->center = vec3_mul_scalar(model->center, scale);
    model->radius *= fabsf(scale);
}

// Settings accessors
ImportExportSettings* import_export_get_settings(void) {
    return &g_import_export_settings;
}

void import_export_set_generate_normals(bool enable) {
    g_import_export_settings.generate_normals = enable;
}

void import_export_set_generate_tangents(bool enable) {
    g_import_export_settings.generate_tangents = enable;
}

void import_export_set_optimize_mesh(bool enable) {
    g_import_export_settings.optimize_mesh = enable;
}

void import_export_set_scale_factor(float scale) {
    g_import_export_settings.scale_factor = scale;
}

void import_export_set_center_model(bool enable) {
    g_import_export_settings.center_model = enable;
}

void import_export_set_flip_uvs(bool enable) {
    g_import_export_settings.flip_uvs = enable;
}

void import_export_set_export_quality(int quality) {
    g_import_export_settings.export_quality = quality;
}

void import_export_set_compress_data(bool enable) {
    g_import_export_settings.compress_data = enable;
}

void import_export_set_embed_textures(bool enable) {
    g_import_export_settings.embed_textures = enable;
}
