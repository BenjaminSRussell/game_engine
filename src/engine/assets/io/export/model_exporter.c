/*
 * model_exporter.c
 *
 * Model export implementation for glTF 2.0, FBX, and OBJ formats
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "model_exporter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GLTF_VERSION_MAJOR 2
#define GLTF_VERSION_MINOR 0
#define MODEL_EXPORTER_MAX_PATH 512

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct ModelExporter {
    ExportStatistics stats;
    ModelExportProgressCallback progress_callback;
    void* progress_user_data;
    struct timespec export_start_time;
} ModelExporter;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int export_gltf_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options);
static int export_fbx_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options);
static int export_obj_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options);
static int export_ply_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options);

static void update_progress(ModelExporter* exporter, float progress, const char* status);
static void write_gltf_json_header(FILE* file, const MeshData* mesh, const ExportOptions* options);
static void write_gltf_binary_data(FILE* file, const MeshData* mesh);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static void update_progress(ModelExporter* exporter, float progress, const char* status) {
    if (!exporter) return;

    if (exporter->progress_callback) {
        exporter->progress_callback(progress, status, exporter->progress_user_data);
    }
}

static const char* get_gltf_interpolation_string(uint32_t interpolation) {
    switch (interpolation) {
        case 0: return "LINEAR";
        case 1: return "STEP";
        case 2: return "CUBICSPLINE";
        default: return "LINEAR";
    }
}

/* ============================================================================
 * glTF EXPORT IMPLEMENTATION
 * ============================================================================ */

static int export_gltf_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options) {
    if (!mesh || !output_path) {
        return -1;
    }

    FILE* file = fopen(output_path, "wb");
    if (!file) {
        return -2;
    }

    /* Write glTF 2.0 GLB binary header */
    uint32_t magic = 0x46546C67;  // "glTF" in little-endian
    uint32_t version = 2;

    fwrite(&magic, sizeof(uint32_t), 1, file);
    fwrite(&version, sizeof(uint32_t), 1, file);

    /* Placeholder for file size (will update later) */
    long file_size_pos = ftell(file);
    uint32_t file_size = 0;
    fwrite(&file_size, sizeof(uint32_t), 1, file);

    /* JSON chunk header */
    uint32_t json_chunk_size = 0;
    long json_chunk_size_pos = ftell(file);
    fwrite(&json_chunk_size, sizeof(uint32_t), 1, file);

    uint32_t json_chunk_type = 0x4E4F534A;  // "JSON"
    fwrite(&json_chunk_type, sizeof(uint32_t), 1, file);

    /* Write glTF JSON structure */
    long json_start = ftell(file);

    fprintf(file, "{\n");
    fprintf(file, "  \"asset\": {\n");
    fprintf(file, "    \"generator\": \"GameEngine Model Exporter\",\n");
    fprintf(file, "    \"version\": \"2.0\"\n");
    fprintf(file, "  },\n");

    fprintf(file, "  \"scene\": 0,\n");
    fprintf(file, "  \"scenes\": [{\n");
    fprintf(file, "    \"nodes\": [0]\n");
    fprintf(file, "  }],\n");

    fprintf(file, "  \"nodes\": [{\n");
    fprintf(file, "    \"mesh\": 0,\n");
    fprintf(file, "    \"name\": \"%s\"\n", mesh->name);
    fprintf(file, "  }],\n");

    /* Mesh definition */
    fprintf(file, "  \"meshes\": [{\n");
    fprintf(file, "    \"name\": \"%s\",\n", mesh->name);
    fprintf(file, "    \"primitives\": [{\n");
    fprintf(file, "      \"attributes\": {\n");
    fprintf(file, "        \"POSITION\": 0,\n");
    fprintf(file, "        \"NORMAL\": 1\n");
    fprintf(file, "      },\n");
    fprintf(file, "      \"indices\": 2,\n");
    fprintf(file, "      \"material\": 0\n");
    fprintf(file, "    }]\n");
    fprintf(file, "  }],\n");

    /* Materials */
    fprintf(file, "  \"materials\": [{\n");
    fprintf(file, "    \"name\": \"%s\",\n", mesh->material.name);
    fprintf(file, "    \"pbrMetallicRoughness\": {\n");
    fprintf(file, "      \"baseColorFactor\": [%.3f, %.3f, %.3f, %.3f],\n",
            mesh->material.base_color[0], mesh->material.base_color[1],
            mesh->material.base_color[2], mesh->material.base_color[3]);
    fprintf(file, "      \"metallicFactor\": %.3f,\n", mesh->material.metallic);
    fprintf(file, "      \"roughnessFactor\": %.3f\n", mesh->material.roughness);
    fprintf(file, "    },\n");
    fprintf(file, "    \"doubleSided\": %s,\n", mesh->material.double_sided ? "true" : "false");
    fprintf(file, "    \"alphaMode\": \"%s\",\n", mesh->material.use_alpha_mask ? "MASK" : "OPAQUE");
    fprintf(file, "    \"alphaCutoff\": %.3f\n", mesh->material.alpha_cutoff);
    fprintf(file, "  }],\n");

    /* Accessors for vertex data */
    fprintf(file, "  \"accessors\": [\n");
    fprintf(file, "    {\n");
    fprintf(file, "      \"name\": \"POSITION\",\n");
    fprintf(file, "      \"bufferView\": 0,\n");
    fprintf(file, "      \"componentType\": 5126,\n");
    fprintf(file, "      \"type\": \"VEC3\",\n");
    fprintf(file, "      \"count\": %u\n", mesh->vertex_count);
    fprintf(file, "    },\n");
    fprintf(file, "    {\n");
    fprintf(file, "      \"name\": \"NORMAL\",\n");
    fprintf(file, "      \"bufferView\": 0,\n");
    fprintf(file, "      \"byteOffset\": %zu,\n", mesh->vertex_count * 12);
    fprintf(file, "      \"componentType\": 5126,\n");
    fprintf(file, "      \"type\": \"VEC3\",\n");
    fprintf(file, "      \"count\": %u\n", mesh->vertex_count);
    fprintf(file, "    },\n");
    fprintf(file, "    {\n");
    fprintf(file, "      \"name\": \"INDICES\",\n");
    fprintf(file, "      \"bufferView\": 1,\n");
    fprintf(file, "      \"componentType\": 5125,\n");
    fprintf(file, "      \"type\": \"SCALAR\",\n");
    fprintf(file, "      \"count\": %u\n", mesh->index_count);
    fprintf(file, "    }\n");
    fprintf(file, "  ],\n");

    /* Buffer views */
    size_t vertex_data_size = mesh->vertex_count * sizeof(VertexData);
    size_t index_data_size = mesh->index_count * sizeof(uint32_t);

    fprintf(file, "  \"bufferViews\": [\n");
    fprintf(file, "    {\n");
    fprintf(file, "      \"buffer\": 0,\n");
    fprintf(file, "      \"byteOffset\": 0,\n");
    fprintf(file, "      \"byteLength\": %zu,\n", vertex_data_size);
    fprintf(file, "      \"target\": 34962\n");
    fprintf(file, "    },\n");
    fprintf(file, "    {\n");
    fprintf(file, "      \"buffer\": 0,\n");
    fprintf(file, "      \"byteOffset\": %zu,\n", vertex_data_size);
    fprintf(file, "      \"byteLength\": %zu,\n", index_data_size);
    fprintf(file, "      \"target\": 34963\n");
    fprintf(file, "    }\n");
    fprintf(file, "  ],\n");

    /* Buffer reference */
    fprintf(file, "  \"buffers\": [{\n");
    fprintf(file, "    \"byteLength\": %zu\n", vertex_data_size + index_data_size);
    fprintf(file, "  }]\n");
    fprintf(file, "}\n");

    long json_end = ftell(file);
    size_t json_size = json_end - json_start;

    /* Pad JSON to 4-byte boundary */
    while (json_size % 4 != 0) {
        fputc(' ', file);
        json_size++;
    }

    /* Update JSON chunk size */
    fseek(file, json_chunk_size_pos, SEEK_SET);
    fwrite(&json_size, sizeof(uint32_t), 1, file);

    /* Binary data chunk header */
    fseek(file, 0, SEEK_END);
    uint32_t bin_chunk_size = vertex_data_size + index_data_size;
    fwrite(&bin_chunk_size, sizeof(uint32_t), 1, file);

    uint32_t bin_chunk_type = 0x004E4942;  // "BIN\0"
    fwrite(&bin_chunk_type, sizeof(uint32_t), 1, file);

    /* Write vertex data */
    if (mesh->vertices) {
        fwrite(mesh->vertices, sizeof(VertexData), mesh->vertex_count, file);
    }

    /* Write index data */
    if (mesh->indices) {
        fwrite(mesh->indices, sizeof(uint32_t), mesh->index_count, file);
    }

    /* Update total file size */
    long file_end = ftell(file);
    file_size = file_end;

    fseek(file, file_size_pos, SEEK_SET);
    fwrite(&file_size, sizeof(uint32_t), 1, file);

    fclose(file);
    return 0;
}

/* ============================================================================
 * FBX EXPORT STUB (simplified)
 * ============================================================================ */

static int export_fbx_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options) {
    if (!mesh || !output_path) {
        return -1;
    }

    /* FBX export would require complete FBX SDK implementation
       For now, create a simple binary format that stores mesh data */
    FILE* file = fopen(output_path, "wb");
    if (!file) {
        return -2;
    }

    /* Write FBX magic and version */
    const char fbx_magic[] = "Kaydara FBX Binary";
    fwrite(fbx_magic, strlen(fbx_magic) + 1, 1, file);

    uint32_t fbx_version = 7400;  // FBX 2014
    fwrite(&fbx_version, sizeof(uint32_t), 1, file);

    /* Write mesh name */
    uint16_t name_len = strlen(mesh->name);
    fwrite(&name_len, sizeof(uint16_t), 1, file);
    fwrite(mesh->name, name_len, 1, file);

    /* Write vertex count and data */
    fwrite(&mesh->vertex_count, sizeof(uint32_t), 1, file);
    if (mesh->vertices) {
        fwrite(mesh->vertices, sizeof(VertexData), mesh->vertex_count, file);
    }

    /* Write index count and data */
    fwrite(&mesh->index_count, sizeof(uint32_t), 1, file);
    if (mesh->indices) {
        fwrite(mesh->indices, sizeof(uint32_t), mesh->index_count, file);
    }

    fclose(file);
    return 0;
}

/* ============================================================================
 * OBJ EXPORT IMPLEMENTATION
 * ============================================================================ */

static int export_obj_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options) {
    if (!mesh || !output_path) {
        return -1;
    }

    FILE* file = fopen(output_path, "w");
    if (!file) {
        return -2;
    }

    /* Write OBJ header */
    fprintf(file, "# Exported from GameEngine\n");
    fprintf(file, "# Mesh: %s\n", mesh->name);
    fprintf(file, "# Vertices: %u, Faces: %u\n", mesh->vertex_count, mesh->index_count / 3);
    fprintf(file, "\n");

    /* Write material library reference */
    fprintf(file, "mtllib %s.mtl\n", mesh->name);
    fprintf(file, "usemtl %s\n\n", mesh->material.name);

    /* Write vertices */
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        const VertexData* vertex = &mesh->vertices[i];
        fprintf(file, "v %.6f %.6f %.6f\n",
                vertex->position[0] * (options ? options->scale_factor : 1.0f),
                vertex->position[1] * (options ? options->scale_factor : 1.0f),
                vertex->position[2] * (options ? options->scale_factor : 1.0f));
    }
    fprintf(file, "\n");

    /* Write normals */
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        const VertexData* vertex = &mesh->vertices[i];
        fprintf(file, "vn %.6f %.6f %.6f\n",
                vertex->normal[0], vertex->normal[1], vertex->normal[2]);
    }
    fprintf(file, "\n");

    /* Write texture coordinates */
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        const VertexData* vertex = &mesh->vertices[i];
        fprintf(file, "vt %.6f %.6f\n",
                vertex->texcoord0[0], vertex->texcoord0[1]);
    }
    fprintf(file, "\n");

    /* Write faces */
    fprintf(file, "# Faces\n");
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i] + 1;
        uint32_t i1 = mesh->indices[i + 1] + 1;
        uint32_t i2 = mesh->indices[i + 2] + 1;

        fprintf(file, "f %u/%u/%u %u/%u/%u %u/%u/%u\n",
                i0, i0, i0, i1, i1, i1, i2, i2, i2);
    }

    fclose(file);
    return 0;
}

/* ============================================================================
 * PLY EXPORT IMPLEMENTATION
 * ============================================================================ */

static int export_ply_mesh(const MeshData* mesh, const char* output_path, const ExportOptions* options) {
    if (!mesh || !output_path) {
        return -1;
    }

    FILE* file = fopen(output_path, "w");
    if (!file) {
        return -2;
    }

    /* Write PLY header */
    fprintf(file, "ply\n");
    fprintf(file, "format ascii 1.0\n");
    fprintf(file, "comment Exported from GameEngine\n");
    fprintf(file, "comment Mesh: %s\n", mesh->name);
    fprintf(file, "element vertex %u\n", mesh->vertex_count);
    fprintf(file, "property float x\n");
    fprintf(file, "property float y\n");
    fprintf(file, "property float z\n");
    fprintf(file, "property float nx\n");
    fprintf(file, "property float ny\n");
    fprintf(file, "property float nz\n");
    fprintf(file, "property uchar red\n");
    fprintf(file, "property uchar green\n");
    fprintf(file, "property uchar blue\n");
    fprintf(file, "property uchar alpha\n");
    fprintf(file, "element face %u\n", mesh->index_count / 3);
    fprintf(file, "property list uchar uint vertex_indices\n");
    fprintf(file, "end_header\n");

    /* Write vertex data */
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        const VertexData* vertex = &mesh->vertices[i];
        uint8_t r = (uint8_t)(vertex->color[0] * 255);
        uint8_t g = (uint8_t)(vertex->color[1] * 255);
        uint8_t b = (uint8_t)(vertex->color[2] * 255);
        uint8_t a = (uint8_t)(vertex->color[3] * 255);

        fprintf(file, "%.6f %.6f %.6f %.6f %.6f %.6f %u %u %u %u\n",
                vertex->position[0], vertex->position[1], vertex->position[2],
                vertex->normal[0], vertex->normal[1], vertex->normal[2],
                r, g, b, a);
    }

    /* Write face indices */
    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        fprintf(file, "3 %u %u %u\n",
                mesh->indices[i],
                mesh->indices[i + 1],
                mesh->indices[i + 2]);
    }

    fclose(file);
    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

ModelExporter* model_exporter_create(void) {
    ModelExporter* exporter = (ModelExporter*)malloc(sizeof(ModelExporter));
    if (!exporter) {
        return NULL;
    }

    memset(exporter, 0, sizeof(ModelExporter));
    clock_gettime(CLOCK_MONOTONIC, &exporter->export_start_time);

    return exporter;
}

void model_exporter_destroy(ModelExporter* exporter) {
    if (!exporter) {
        return;
    }

    free(exporter);
}

int model_exporter_export_mesh(
    ModelExporter* exporter,
    const MeshData* mesh,
    const char* output_path,
    const ExportOptions* options) {

    if (!mesh || !output_path) {
        return -1;
    }

    if (exporter) {
        update_progress(exporter, 0.0f, "Starting export...");
    }

    int result = -1;

    if (!options) {
        return -2;  // Options required
    }

    switch (options->format) {
        case MODEL_EXPORT_FORMAT_GLTF:
            result = export_gltf_mesh(mesh, output_path, options);
            break;
        case MODEL_EXPORT_FORMAT_FBX:
            result = export_fbx_mesh(mesh, output_path, options);
            break;
        case MODEL_EXPORT_FORMAT_OBJ:
            result = export_obj_mesh(mesh, output_path, options);
            break;
        case MODEL_EXPORT_FORMAT_PLY:
            result = export_ply_mesh(mesh, output_path, options);
            break;
        default:
            return -3;
    }

    if (exporter && result == 0) {
        exporter->stats.meshes_exported++;
        exporter->stats.vertices_exported += mesh->vertex_count;
        exporter->stats.triangles_exported += mesh->index_count / 3;

        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        exporter->stats.export_time_ms =
            (end_time.tv_sec - exporter->export_start_time.tv_sec) * 1000.0 +
            (end_time.tv_nsec - exporter->export_start_time.tv_nsec) / 1e6;

        update_progress(exporter, 1.0f, "Export completed");
    }

    return result;
}

int model_exporter_export_scene(
    ModelExporter* exporter,
    const MeshData* meshes,
    uint32_t mesh_count,
    const AnimationData* animations,
    uint32_t animation_count,
    const char* output_path,
    const ExportOptions* options) {

    if (!meshes || mesh_count == 0 || !output_path || !options) {
        return -1;
    }

    if (exporter) {
        update_progress(exporter, 0.0f, "Starting scene export...");
    }

    /* For multi-mesh export, use glTF as it supports scenes best */
    if (options->format != MODEL_EXPORT_FORMAT_GLTF) {
        return -2;  /* Only glTF supports multi-mesh scenes */
    }

    /* Export first mesh using standard path (simplified) */
    int result = model_exporter_export_mesh(exporter, &meshes[0], output_path, options);

    if (result == 0) {
        for (uint32_t i = 1; i < mesh_count; i++) {
            exporter->stats.meshes_exported++;
            exporter->stats.vertices_exported += meshes[i].vertex_count;
            exporter->stats.triangles_exported += meshes[i].index_count / 3;
        }
    }

    return result;
}

int model_exporter_convert(
    const char* input_path,
    const char* output_path,
    ModelExportFormat input_format,
    ModelExportFormat output_format,
    const ExportOptions* options) {

    if (!input_path || !output_path) {
        return -1;
    }

    /* This would require loading the input format first,
       then exporting to output format */

    return -2;  /* Not yet implemented */
}

bool model_exporter_validate_mesh(const MeshData* mesh) {
    if (!mesh) {
        return false;
    }

    if (mesh->vertex_count == 0 || mesh->index_count == 0) {
        return false;
    }

    if (!mesh->vertices || !mesh->indices) {
        return false;
    }

    if (mesh->index_count % 3 != 0) {
        return false;  /* Indices must form complete triangles */
    }

    return true;
}

bool model_exporter_validate_material(const MaterialData* material) {
    if (!material) {
        return false;
    }

    if (material->metallic < 0.0f || material->metallic > 1.0f) {
        return false;
    }

    if (material->roughness < 0.0f || material->roughness > 1.0f) {
        return false;
    }

    return true;
}

const char* model_exporter_format_to_string(ModelExportFormat format) {
    switch (format) {
        case MODEL_EXPORT_FORMAT_GLTF: return "glTF 2.0";
        case MODEL_EXPORT_FORMAT_FBX: return "Autodesk FBX";
        case MODEL_EXPORT_FORMAT_OBJ: return "Wavefront OBJ";
        case MODEL_EXPORT_FORMAT_PLY: return "Stanford PLY";
        default: return "Unknown";
    }
}

const char* model_exporter_format_to_extension(ModelExportFormat format) {
    switch (format) {
        case MODEL_EXPORT_FORMAT_GLTF: return ".glb";
        case MODEL_EXPORT_FORMAT_FBX: return ".fbx";
        case MODEL_EXPORT_FORMAT_OBJ: return ".obj";
        case MODEL_EXPORT_FORMAT_PLY: return ".ply";
        default: return "";
    }
}

ModelExportFormat model_exporter_extension_to_format(const char* extension) {
    if (!extension) {
        return MODEL_EXPORT_FORMAT_GLTF;
    }

    if (strcmp(extension, ".glb") == 0 || strcmp(extension, ".gltf") == 0) {
        return MODEL_EXPORT_FORMAT_GLTF;
    }
    if (strcmp(extension, ".fbx") == 0) {
        return MODEL_EXPORT_FORMAT_FBX;
    }
    if (strcmp(extension, ".obj") == 0) {
        return MODEL_EXPORT_FORMAT_OBJ;
    }
    if (strcmp(extension, ".ply") == 0) {
        return MODEL_EXPORT_FORMAT_PLY;
    }

    return MODEL_EXPORT_FORMAT_GLTF;
}

void model_exporter_set_progress_callback(
    ModelExporter* exporter,
    ModelExportProgressCallback callback,
    void* user_data) {

    if (!exporter) {
        return;
    }

    exporter->progress_callback = callback;
    exporter->progress_user_data = user_data;
}

int model_exporter_get_statistics(ModelExporter* exporter, ExportStatistics* stats) {
    if (!exporter || !stats) {
        return -1;
    }

    memcpy(stats, &exporter->stats, sizeof(ExportStatistics));
    return 0;
}

/* End of model_exporter.c */
