#ifndef EDITOR_TYPES_H
#define EDITOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "math/mat4.h"

#ifdef __cplusplus
extern "C" {
#endif

// Vertex Structure for 3D Models
typedef struct {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
    vec4 color;
    vec3 tangent;
    vec3 bitangent;
    int32_t bone_indices[4];
    float bone_weights[4];
} Vertex;

// Mesh Data Structure
typedef struct {
    Vertex* vertices;
    uint32_t* indices;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t material_index;
} Mesh;

// Material Properties
typedef struct {
    vec4 albedo_color;
    float metallic;
    float roughness;
    float ao;
    vec3 emission;
    float alpha;
    
    // Texture IDs
    uint32_t albedo_texture;
    uint32_t normal_texture;
    uint32_t metallic_texture;
    uint32_t roughness_texture;
    uint32_t ao_texture;
    uint32_t emission_texture;
    
    char name[64];
    bool transparent;
    bool double_sided;
} Material;

// 3D Model Structure
typedef struct {
    Mesh* meshes;
    Material* materials;
    uint32_t mesh_count;
    uint32_t material_count;
    
    char name[64];
    char author[64];
    char description[256];
    
    // Bounding box
    vec3 min_bounds;
    vec3 max_bounds;
    vec3 center;
    float radius;
    
    // Animation data
    bool has_animation;
    uint32_t bone_count;
    uint32_t animation_count;
} Model3D;

// 2D Sprite Structure
typedef struct {
    uint32_t texture_id;
    vec2 size;
    vec2 pivot;
    vec4 color;
    
    // Animation frames
    uint32_t frame_count;
    uint32_t current_frame;
    float frame_time;
    float animation_speed;
    bool looping;
    
    // Sprite sheet properties
    vec2 sheet_size;
    vec2 frame_size;
    uint32_t columns;
    uint32_t rows;
    
    char name[64];
    char author[64];
} Sprite2D;

// Particle System Structure
typedef struct {
    struct {
        vec3 position;
        vec3 velocity;
        vec3 acceleration;
        vec4 start_color;
        vec4 end_color;
        float start_size;
        float end_size;
        float lifetime;
        float age;
        bool active;
    }* particles;
    
    uint32_t max_particles;
    uint32_t active_count;
    
    // Emitter properties
    vec3 emitter_position;
    vec3 emitter_direction;
    float emitter_angle;
    float emission_rate;
    float emission_timer;
    
    // Particle properties
    float particle_lifetime_min;
    float particle_lifetime_max;
    float start_size_min;
    float start_size_max;
    float end_size_min;
    float end_size_max;
    
    // Physics
    vec3 gravity;
    float drag;
    bool random_rotation;
    
    // Rendering
    uint32_t texture_id;
    bool additive_blending;
    bool depth_write;
    
    char name[64];
} ParticleSystem;

// UI Element Structure
typedef struct {
    vec2 position;
    vec2 size;
    vec4 color;
    uint32_t texture_id;
    
    // UI properties
    bool interactive;
    bool visible;
    float alpha;
    
    // Text properties
    char text[256];
    float font_size;
    vec4 text_color;
    bool text_centered;
    
    // Animation
    vec2 target_position;
    vec2 target_size;
    float animation_speed;
    
    char name[64];
} UIElement;

// Selection System
typedef enum {
    SELECTION_TYPE_NONE,
    SELECTION_TYPE_VERTEX,
    SELECTION_TYPE_EDGE,
    SELECTION_TYPE_FACE,
    SELECTION_TYPE_OBJECT,
    SELECTION_TYPE_COMPONENT
} SelectionType;

typedef struct {
    SelectionType type;
    uint32_t* indices;
    uint32_t count;
    uint32_t capacity;
} Selection;

// Transform Operation
typedef enum {
    TRANSFORM_MOVE,
    TRANSFORM_ROTATE,
    TRANSFORM_SCALE,
    TRANSFORM_UNIFORM_SCALE
} TransformType;

typedef struct {
    TransformType type;
    mat4 transform_matrix;
    vec3 translation;
    vec3 rotation;
    vec3 scale;
    bool local_space;
} TransformOperation;

// Undo/Redo System
typedef enum {
    ACTION_TYPE_TRANSFORM,
    ACTION_TYPE_GEOMETRY,
    ACTION_TYPE_MATERIAL,
    ACTION_TYPE_TEXTURE,
    ACTION_TYPE_CREATE,
    ACTION_TYPE_DELETE
} ActionType;

typedef struct {
    ActionType type;
    void* data;
    size_t data_size;
    char description[128];
} EditorAction;

// Asset Metadata
typedef struct {
    char name[64];
    char author[64];
    char description[256];
    char tags[8][32];
    uint32_t tag_count;
    
    // Version info
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    
    // Creation and modification times
    uint64_t created_time;
    uint64_t modified_time;
    
    // Asset statistics
    size_t file_size;
    uint32_t polygon_count;
    uint32_t vertex_count;
    uint32_t texture_count;
    
    // Preview
    uint32_t preview_texture;
} AssetMetadata;

// Editor Settings
typedef struct {
    // General
    bool auto_save;
    float auto_save_interval;
    bool show_grid;
    bool show_gizmos;
    bool snap_to_grid;
    float grid_size;
    
    // Viewport
    float camera_speed;
    float camera_sensitivity;
    bool invert_mouse_y;
    
    // Rendering
    bool wireframe_mode;
    bool show_normals;
    bool show_bounding_boxes;
    float lighting_intensity;
    
    // Editing
    float snap_distance;
    bool snap_to_vertex;
    bool snap_to_edge;
    bool snap_to_face;
    
    // UI
    float ui_scale;
    bool show_tooltips;
    bool compact_mode;
} EditorSettings;

#ifdef __cplusplus
}
#endif

#endif // EDITOR_TYPES_H
