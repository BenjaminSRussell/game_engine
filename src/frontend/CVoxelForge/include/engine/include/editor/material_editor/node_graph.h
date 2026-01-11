#ifndef EDITOR_MATERIAL_EDITOR_NODE_GRAPH_H
#define EDITOR_MATERIAL_EDITOR_NODE_GRAPH_H

#include "core/core.h"
#include "renderer/core/material.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                      MATERIAL NODE GRAPH SYSTEM - AGENT_EDITOR_2
// =================================================================================================

// Node types for material editing
typedef enum {
    NODE_TYPE_INPUT,          // Input nodes (UVs, Position, Normal, etc.)
    NODE_TYPE_OUTPUT,         // Output nodes (Base Color, Normal, Roughness, etc.)
    NODE_TYPE_TEXTURE,        // Texture sampling nodes
    NODE_TYPE_MATH,           // Mathematical operations
    NODE_TYPE_PBR,            // Physically-based rendering nodes
    NODE_TYPE_UTILITY,        // Utility nodes (lerp, clamp, etc.)
    NODE_TYPE_CONSTANT,       // Constant values
    NODE_TYPE_COMMENT,        // Comment nodes
    NODE_TYPE_REROUTE         // Reroute nodes
} NodeType;

// Data types for node pins
typedef enum {
    PIN_TYPE_FLOAT,
    PIN_TYPE_FLOAT2,
    PIN_TYPE_FLOAT3,
    PIN_TYPE_FLOAT4,
    PIN_TYPE_TEXTURE2D,
    PIN_TYPE_TEXTURE3D,
    PIN_TYPE_BOOL,
    PIN_TYPE_INT
} PinType;

// Pin direction
typedef enum {
    PIN_DIRECTION_INPUT,
    PIN_DIRECTION_OUTPUT
} PinDirection;

// Node pin structure
typedef struct NodePin {
    u32 id;
    char name[64];
    PinType type;
    PinDirection direction;
    struct MaterialNode* node;
    struct NodePin* connected_to;  // Single connection for input pins
    struct NodePin* connected_from; // For output pins, track connected inputs
    
    // Pin value (for unconnected inputs)
    union {
        f32 float_val;
        vec2 float2_val;
        vec3 float3_val;
        vec4 float4_val;
        bool bool_val;
        int int_val;
    } default_value;
    
    bool is_connected;
} NodePin;

// Material node structure
typedef struct MaterialNode {
    u32 id;
    char name[64];
    NodeType type;
    vec2 position;
    vec2 size;
    
    // Node pins
    NodePin* inputs;
    NodePin* outputs;
    u32 input_count;
    u32 output_count;
    
    // Node-specific data
    union {
        struct {
            char texture_path[256];
            u32 texture_id;
            bool sRGB;
        } texture;
        
        struct {
            char operation[32];  // add, subtract, multiply, divide, etc.
        } math;
        
        struct {
            f32 metallic;
            f32 roughness;
            f32 ao;
        } pbr;
        
        struct {
            char function[64];  // lerp, clamp, smoothstep, etc.
        } utility;
        
        struct {
            char comment[512];
        } comment;
    } data;
    
    bool is_selected;
    bool is_enabled;
} MaterialNode;

// Node connection
typedef struct NodeConnection {
    NodePin* output_pin;
    NodePin* input_pin;
} NodeConnection;

// Material node graph
typedef struct MaterialNodeGraph {
    MaterialNode* nodes;
    u32 node_count;
    u32 node_capacity;
    
    NodeConnection* connections;
    u32 connection_count;
    u32 connection_capacity;
    
    u32 next_node_id;
    u32 next_pin_id;
    
    // Graph properties
    char name[256];
    char description[512];
    bool is_dirty;
    
    // Preview material
    Material* preview_material;
    bool preview_needs_update;
} MaterialNodeGraph;

// =================================================================================================
//                                    CORE API FUNCTIONS
// =================================================================================================

// Graph management
MaterialNodeGraph* node_graph_create(const char* name);
void node_graph_destroy(MaterialNodeGraph* graph);
void node_graph_clear(MaterialNodeGraph* graph);
void node_graph_mark_dirty(MaterialNodeGraph* graph);

// Node management
MaterialNode* node_graph_add_node(MaterialNodeGraph* graph, NodeType type, vec2 position);
void node_graph_remove_node(MaterialNodeGraph* graph, u32 node_id);
MaterialNode* node_graph_get_node(MaterialNodeGraph* graph, u32 node_id);
void node_graph_set_node_position(MaterialNodeGraph* graph, u32 node_id, vec2 position);

// Connection management
bool node_graph_connect(MaterialNodeGraph* graph, u32 output_node_id, u32 output_pin_id,
                       u32 input_node_id, u32 input_pin_id);
void node_graph_disconnect(MaterialNodeGraph* graph, u32 input_node_id, u32 input_pin_id);
bool node_graph_can_connect(MaterialNodeGraph* graph, u32 output_node_id, u32 output_pin_id,
                           u32 input_node_id, u32 input_pin_id);

// Node type utilities
const char* node_type_get_name(NodeType type);
const char* pin_type_get_name(PinType type);
bool node_type_is_valid_for_connection(PinType output_type, PinType input_type);

// =================================================================================================
//                                NODE CREATION FUNCTIONS
// =================================================================================================

// Input nodes
MaterialNode* create_node_uv_input(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_position_input(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_normal_input(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_tangent_input(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_color_input(MaterialNodeGraph* graph, vec2 position);

// Output nodes
MaterialNode* create_node_base_color_output(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_normal_output(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_metallic_output(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_roughness_output(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_ao_output(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_emission_output(MaterialNodeGraph* graph, vec2 position);

// Texture nodes
MaterialNode* create_node_texture_sample(MaterialNodeGraph* graph, vec2 position, const char* texture_path);
MaterialNode* create_node_normal_map(MaterialNodeGraph* graph, vec2 position);

// Math nodes
MaterialNode* create_node_add(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_subtract(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_multiply(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_divide(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_dot_product(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_cross_product(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_normalize(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_length(MaterialNodeGraph* graph, vec2 position);

// PBR nodes
MaterialNode* create_node_pbr_setup(MaterialNodeGraph* graph, vec2 position);

// Utility nodes
MaterialNode* create_node_lerp(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_clamp(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_smoothstep(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_abs(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_sin(MaterialNodeGraph* graph, vec2 position);
MaterialNode* create_node_cos(MaterialNodeGraph* graph, vec2 position);

// Constant nodes
MaterialNode* create_node_constant_float(MaterialNodeGraph* graph, vec2 position, f32 value);
MaterialNode* create_node_constant_float3(MaterialNodeGraph* graph, vec2 position, vec3 value);
MaterialNode* create_node_constant_float4(MaterialNodeGraph* graph, vec2 position, vec4 value);

// Organization nodes
MaterialNode* create_node_comment(MaterialNodeGraph* graph, vec2 position, const char* text);
MaterialNode* create_node_reroute(MaterialNodeGraph* graph, vec2 position);

// =================================================================================================
//                              SHADER CODE GENERATION
// =================================================================================================

typedef struct {
    char vertex_shader[4096];
    char fragment_shader[8192];
    bool generation_success;
    char error_message[512];
} ShaderCodeResult;

ShaderCodeResult node_graph_generate_shader_code(MaterialNodeGraph* graph);
bool node_graph_validate_graph(MaterialNodeGraph* graph, char* error_buffer, size_t error_buffer_size);

// =================================================================================================
//                                PREVIEW SYSTEM
// =================================================================================================

void node_graph_update_preview(MaterialNodeGraph* graph);
Material* node_graph_get_preview_material(MaterialNodeGraph* graph);
void node_graph_set_preview_mesh(MaterialNodeGraph* graph, u32 mesh_id);

// =================================================================================================
//                                SERIALIZATION
// =================================================================================================

bool node_graph_save_to_file(MaterialNodeGraph* graph, const char* file_path);
MaterialNodeGraph* node_graph_load_from_file(const char* file_path);

// =================================================================================================
//                                DEBUGGING UTILITIES
// =================================================================================================

void node_graph_print_graph(MaterialNodeGraph* graph);
void node_graph_print_node(MaterialNode* node);
void node_graph_validate_connections(MaterialNodeGraph* graph);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_MATERIAL_EDITOR_NODE_GRAPH_H
