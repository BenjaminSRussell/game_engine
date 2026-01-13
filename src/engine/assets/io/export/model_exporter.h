/*
 * model_exporter.h
 *
 * Model export functionality for glTF 2.0, FBX, and OBJ formats
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality to export 3D models in various formats with support for:
 * - Vertex/index buffer export
 * - Material and texture export
 * - Animation data export
 * - Skeletal animation baking
 * - Format conversion
 */

#ifndef MODEL_EXPORTER_H
#define MODEL_EXPORTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef enum {
    MODEL_EXPORT_FORMAT_GLTF = 0,    // glTF 2.0 (.gltf / .glb)
    MODEL_EXPORT_FORMAT_FBX,          // Autodesk FBX (.fbx)
    MODEL_EXPORT_FORMAT_OBJ,          // Wavefront OBJ (.obj)
    MODEL_EXPORT_FORMAT_PLY,          // Stanford PLY (.ply)
} ModelExportFormat;

typedef enum {
    MODEL_COMPRESSION_NONE = 0,
    MODEL_COMPRESSION_DEFLATE,        // glTF standard compression
    MODEL_COMPRESSION_DRACO,          // Google Draco mesh compression
} ModelCompressionType;

/* Vertex data structure */
typedef struct {
    float position[3];
    float normal[3];
    float tangent[4];
    float texcoord0[2];
    float texcoord1[2];
    float color[4];
    uint16_t joints[4];
    float weights[4];
} VertexData;

/* Material data structure */
typedef struct {
    char name[256];
    float base_color[4];
    float metallic;
    float roughness;
    float normal_scale;
    float occlusion_strength;
    float emissive_factor[3];
    char base_color_texture_path[512];
    char normal_texture_path[512];
    char metallic_roughness_texture_path[512];
    char occlusion_texture_path[512];
    char emissive_texture_path[512];
    bool double_sided;
    bool use_alpha_mask;
    float alpha_cutoff;
} MaterialData;

/* Mesh data structure */
typedef struct {
    char name[256];
    VertexData* vertices;
    uint32_t vertex_count;
    uint32_t* indices;
    uint32_t index_count;
    MaterialData material;
    uint32_t material_id;
} MeshData;

/* Animation keyframe */
typedef struct {
    float time;
    float value[4];           // Can be position (3), rotation (4), or scale (3)
    uint32_t interpolation;   // 0=linear, 1=step, 2=cubic spline
} KeyFrame;

/* Animation channel (tracks animation of a single property) */
typedef struct {
    char target_name[256];    // Target node name
    uint32_t target_property; // 0=translation, 1=rotation, 2=scale
    KeyFrame* keyframes;
    uint32_t keyframe_count;
} AnimationChannel;

/* Animation structure */
typedef struct {
    char name[256];
    AnimationChannel* channels;
    uint32_t channel_count;
    float duration;
} AnimationData;

/* Export options */
typedef struct {
    ModelExportFormat format;
    ModelCompressionType compression;
    bool embed_textures;
    bool export_materials;
    bool export_animations;
    bool optimize_mesh;
    bool generate_normals;
    bool generate_tangents;
    float scale_factor;
    bool flip_y_axis;
    bool flip_winding_order;
} ExportOptions;

/* Export context */
typedef struct ModelExporter ModelExporter;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy exporter context */
ModelExporter* model_exporter_create(void);
void model_exporter_destroy(ModelExporter* exporter);

/* Export a single mesh */
int model_exporter_export_mesh(
    ModelExporter* exporter,
    const MeshData* mesh,
    const char* output_path,
    const ExportOptions* options
);

/* Export multiple meshes (complete scene) */
int model_exporter_export_scene(
    ModelExporter* exporter,
    const MeshData* meshes,
    uint32_t mesh_count,
    const AnimationData* animations,
    uint32_t animation_count,
    const char* output_path,
    const ExportOptions* options
);

/* Convert between formats */
int model_exporter_convert(
    const char* input_path,
    const char* output_path,
    ModelExportFormat input_format,
    ModelExportFormat output_format,
    const ExportOptions* options
);

/* Validate export data */
bool model_exporter_validate_mesh(const MeshData* mesh);
bool model_exporter_validate_material(const MaterialData* material);

/* Get format information */
const char* model_exporter_format_to_string(ModelExportFormat format);
const char* model_exporter_format_to_extension(ModelExportFormat format);
ModelExportFormat model_exporter_extension_to_format(const char* extension);

/* Progress callback */
typedef void (*ModelExportProgressCallback)(float progress, const char* status, void* user_data);
void model_exporter_set_progress_callback(ModelExporter* exporter, ModelExportProgressCallback callback, void* user_data);

/* Get statistics */
typedef struct {
    uint32_t meshes_exported;
    uint32_t vertices_exported;
    uint32_t triangles_exported;
    size_t file_size;
    double export_time_ms;
} ExportStatistics;

int model_exporter_get_statistics(ModelExporter* exporter, ExportStatistics* stats);

#ifdef __cplusplus
}
#endif

#endif // MODEL_EXPORTER_H
