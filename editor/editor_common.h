#pragma once

/**
 * =================================================================================================
 *                           UNIFIED EDITOR TYPE DEFINITIONS
 * =================================================================================================
 *
 * This header consolidates all editor-related type definitions that were previously scattered
 * across editor/, src/engine/include/tools/asset_editor/, and src/engine/include/editor/.
 *
 * Purpose: Provide a single source of truth for editor data structures and establish clear
 * interfaces between editor tools and the main engine.
 */

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                                    TERRAIN EDITOR TYPES
// =================================================================================================

typedef enum {
    FALLOFF_LINEAR = 0,
    FALLOFF_SMOOTH,
    FALLOFF_SHARP,
    FALLOFF_CONSTANT,
    FALLOFF_COUNT
} FalloffType;

typedef enum {
    BRUSH_TYPE_CIRCLE = 0,
    BRUSH_TYPE_SQUARE,
    BRUSH_TYPE_CUSTOM,
    BRUSH_TYPE_COUNT
} BrushType;

typedef enum {
    SCULPT_RAISE = 0,
    SCULPT_LOWER,
    SCULPT_FLATTEN,
    SCULPT_SMOOTH,
    SCULPT_NOISE,
    SCULPT_COUNT
} SculptOperation;

typedef struct {
    u32 width;
    u32 height;
    f32* heights;
    u8* visibility_mask;
    f32* layer_weights;
    f32 scale_x;
    f32 scale_z;
    f32 height_scale;
} TerrainHeightmap;

typedef struct {
    BrushType type;
    SculptOperation operation;
    FalloffType falloff;
    f32 radius;
    f32 strength;
    f32 hardness;
    f32 target_height;
    bool is_active;
} TerrainBrush;

typedef struct {
    f32* height_data;
    u32* modified_pixels;
    u32 pixel_count;
    size_t data_size;
} SculptHistoryEntry;

typedef struct {
    TerrainHeightmap* heightmap;
    TerrainBrush current_brush;
    
    // Undo/redo system
    SculptHistoryEntry* history;
    u32 history_capacity;
    u32 history_size;
    i32 current_history_index;
    
    // Performance tracking
    u64 last_sculpt_time;
    f32 average_sculpt_time;
    u32 pixels_modified_last_frame;
} TerrainSculptingSystem;

// =================================================================================================
//                                    MATERIAL EDITOR TYPES
// =================================================================================================

typedef enum {
    CONNECTION_TYPE_FLOAT = 0,
    CONNECTION_TYPE_VEC2,
    CONNECTION_TYPE_VEC3,
    CONNECTION_TYPE_VEC4,
    CONNECTION_TYPE_TEXTURE,
    CONNECTION_TYPE_COUNT
} ConnectionType;

typedef enum {
    NODE_TYPE_CONSTANT_FLOAT = 0,
    NODE_TYPE_CONSTANT_VEC3,
    NODE_TYPE_CONSTANT_VEC4,
    NODE_TYPE_TEXTURE_SAMPLE,
    NODE_TYPE_MULTIPLY,
    NODE_TYPE_ADD,
    NODE_TYPE_SUBTRACT,
    NODE_TYPE_DIVIDE,
    NODE_TYPE_LERP,
    NODE_TYPE_DOT_PRODUCT,
    NODE_TYPE_CROSS_PRODUCT,
    NODE_TYPE_NORMALIZE,
    NODE_TYPE_FRESNEL,
    NODE_TYPE_NORMAL_MAP,
    NODE_TYPE_NOISE,
    NODE_TYPE_PBR_MASTER,
    NODE_TYPE_COUNT
} MaterialNodeType;

typedef struct MaterialNode MaterialNode;

typedef struct {
    char name[32];
    ConnectionType type;
    MaterialNode* connected_node;
    u32 connected_output_index;
    void* default_value;
} MaterialNodeInput;

typedef struct {
    char name[32];
    ConnectionType type;
    void* value;
} MaterialNodeOutput;

struct MaterialNode {
    MaterialNodeType type;
    char name[64];
    Vec2 ui_position;
    
    MaterialNodeInput inputs[8];
    MaterialNodeOutput outputs[8];
    u32 input_count;
    u32 output_count;
    
    void* parameters;
    size_t parameter_size;
    
    u32 node_id;
    bool is_selected;
};

typedef struct {
    MaterialNode* nodes;
    u32 node_count;
    u32 node_capacity;
    
    MaterialNode* master_node;
    
    char* generated_vertex_shader;
    char* generated_fragment_shader;
    bool needs_recompile;
    
    char name[64];
    char author[64];
    u32 version;
} MaterialGraph;

// =================================================================================================
//                                    COMMON EDITOR TYPES
// =================================================================================================

// Forward declarations
typedef struct EditorContext EditorContext;
typedef struct EditorViewport EditorViewport;

// Editor tool interface - all tools must implement this
typedef struct {
    const char* name;
    const char* description;
    
    // Lifecycle
    bool (*init)(EditorContext* ctx);
    void (*shutdown)(EditorContext* ctx);
    void (*update)(EditorContext* ctx, f32 delta_time);
    
    // Rendering
    void (*render_ui)(EditorContext* ctx);
    void (*render_viewport)(EditorContext* ctx);
    
    // Input handling
    void (*on_mouse_down)(EditorContext* ctx, i32 x, i32 y, i32 button);
    void (*on_mouse_up)(EditorContext* ctx, i32 x, i32 y, i32 button);
    void (*on_mouse_move)(EditorContext* ctx, i32 x, i32 y);
    void (*on_key_down)(EditorContext* ctx, i32 key);
    void (*on_key_up)(EditorContext* ctx, i32 key);
    
    // Serialization
    bool (*save)(EditorContext* ctx, const char* filepath);
    bool (*load)(EditorContext* ctx, const char* filepath);
    
    void* user_data;
} EditorTool;

// Main editor viewport
struct EditorViewport {
    u32 width;
    u32 height;
    u32 framebuffer;
    u32 color_texture;
    u32 depth_texture;
    
    Mat4 view_matrix;
    Mat4 projection_matrix;
    
    Vec3 camera_position;
    Vec3 camera_target;
    f32 camera_distance;
    f32 camera_yaw;
    f32 camera_pitch;
    
    bool is_panning;
    bool is_rotating;
    Vec2 last_mouse_pos;
};

// Main editor context - shared by all tools
struct EditorContext {
    bool is_initialized;
    
    // Active tool
    EditorTool* active_tool;
    EditorTool* available_tools[16];
    u32 tool_count;
    
    // Viewport
    EditorViewport viewport;
    
    // Shared resources
    void* renderer;
    void* ui_context;
    
    // Undo/redo stack (shared across tools)
    void* undo_stack;
    u32 undo_capacity;
    
    // Settings
    bool show_grid;
    bool show_gizmos;
    f32 grid_size;
    
    // Tool-specific data
    TerrainSculptingSystem* terrain_system;
    MaterialGraph* material_graph;
};

// =================================================================================================
//                                    FUNCTION DECLARATIONS
// =================================================================================================

// Editor context management
EditorContext* editor_context_create(void);
void editor_context_destroy(EditorContext* ctx);
bool editor_context_init(EditorContext* ctx);
void editor_context_shutdown(EditorContext* ctx);
void editor_context_update(EditorContext* ctx, f32 delta_time);

// Tool management
bool editor_register_tool(EditorContext* ctx, EditorTool* tool);
bool editor_set_active_tool(EditorContext* ctx, const char* tool_name);
EditorTool* editor_get_active_tool(EditorContext* ctx);

// Material Graph management
MaterialGraph* material_graph_create(const char* name);
void material_graph_destroy(MaterialGraph* graph);
MaterialNode* material_graph_add_node(MaterialGraph* graph, MaterialNodeType type);
bool material_graph_connect_nodes(MaterialGraph* graph, u32 source_id, u32 source_output, u32 dest_id, u32 dest_input);
bool material_graph_compile(MaterialGraph* graph);

// Terrain Sculpting management
TerrainSculptingSystem* terrain_sculpting_create(TerrainHeightmap* heightmap);
void terrain_sculpting_destroy(TerrainSculptingSystem* system);
bool terrain_sculpting_apply_brush(TerrainSculptingSystem* system, Vec3 world_position);
bool terrain_sculpting_undo(TerrainSculptingSystem* system);

// Viewport management
bool editor_viewport_init(EditorViewport* viewport, u32 width, u32 height);
void editor_viewport_shutdown(EditorViewport* viewport);
void editor_viewport_resize(EditorViewport* viewport, u32 width, u32 height);
void editor_viewport_update_camera(EditorViewport* viewport, f32 delta_time);
void editor_viewport_handle_input(EditorViewport* viewport, i32 mouse_x, i32 mouse_y, bool mouse_down);

#ifdef __cplusplus
}
#endif
