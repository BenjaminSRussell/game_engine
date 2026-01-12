#include "../editor_common.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                           MATERIAL NODE GRAPH EDITOR - CORE IMPLEMENTATION
 * =================================================================================================
 *
 * STATUS: Core graph management implemented, UI and shader generation still needed
 *
 *  IMPLEMENTED:
 *    - Graph creation/destruction
 *    - Node add/remove
 *    - Node connection management
 *    - Basic validation
 *
 *  STILL MISSING:
 *    - UI rendering with bezier connections
 *    - Shader code generation/compilation
 *    - Preview sphere rendering
 *    - File serialization
 *    - Graph optimization
 */

// =================================================================================================
//                                    GRAPH MANAGEMENT
// =================================================================================================

MaterialGraph* material_graph_create(const char* name) {
    MaterialGraph* graph = (MaterialGraph*)malloc(sizeof(MaterialGraph));
    if (!graph) {
        LOG_ERROR("Failed to allocate MaterialGraph");
        return NULL;
    }
    
    memset(graph, 0, sizeof(MaterialGraph));
    
    // Copy name
    if (name) {
        strncpy(graph->name, name, sizeof(graph->name) - 1);
        graph->name[sizeof(graph->name) - 1] = '\0';
    }
    
    // Initialize node array
    graph->node_capacity = 32;
    graph->nodes = (MaterialNode*)malloc(sizeof(MaterialNode) * graph->node_capacity);
    if (!graph->nodes) {
        free(graph);
        LOG_ERROR("Failed to allocate node array");
        return NULL;
    }
    
    graph->node_count = 0;
    graph->master_node = NULL;
    graph->needs_recompile = true;
    graph->version = 1;
    
    LOG_INFO("Created material graph: %s", name ? name : "unnamed");
    return graph;
}

void material_graph_destroy(MaterialGraph* graph) {
    if (!graph) return;
    
    // Free all nodes
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* node = &graph->nodes[i];
        
        // Free node parameters
        if (node->parameters) {
            free(node->parameters);
        }
        
        // Free input default values
        for (u32 j = 0; j < node->input_count; j++) {
            if (node->inputs[j].default_value) {
                free(node->inputs[j].default_value);
            }
        }
    }
    
    free(graph->nodes);
    
    // Free generated shader code
    if (graph->generated_vertex_shader) free(graph->generated_vertex_shader);
    if (graph->generated_fragment_shader) free(graph->generated_fragment_shader);
    
    free(graph);
    LOG_INFO("Destroyed material graph");
}

// =================================================================================================
//                                    NODE MANAGEMENT
// =================================================================================================

static MaterialNode* material_graph_get_node_by_id(MaterialGraph* graph, u32 node_id) {
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].node_id == node_id) {
            return &graph->nodes[i];
        }
    }
    return NULL;
}

MaterialNode* material_graph_add_node(MaterialGraph* graph, MaterialNodeType type) {
    if (!graph) return NULL;
    
    // Resize if needed
    if (graph->node_count >= graph->node_capacity) {
        u32 new_capacity = graph->node_capacity * 2;
        MaterialNode* new_nodes = (MaterialNode*)realloc(graph->nodes, 
                                                        sizeof(MaterialNode) * new_capacity);
        if (!new_nodes) {
            LOG_ERROR("Failed to resize node array");
            return NULL;
        }
        graph->nodes = new_nodes;
        graph->node_capacity = new_capacity;
    }
    
    // Initialize new node
    MaterialNode* node = &graph->nodes[graph->node_count];
    memset(node, 0, sizeof(MaterialNode));
    
    node->type = type;
    node->node_id = graph->node_count;  // Simple ID assignment
    node->is_selected = false;
    node->ui_position = (Vec2){0.0f, 0.0f};
    
    // Set default name based on type
    switch (type) {
        case NODE_TYPE_CONSTANT_FLOAT:
            strncpy(node->name, "Constant (Float)", sizeof(node->name) - 1);
            node->output_count = 1;
            strncpy(node->outputs[0].name, "Value", sizeof(node->outputs[0].name) - 1);
            node->outputs[0].type = CONNECTION_TYPE_FLOAT;
            break;
            
        case NODE_TYPE_CONSTANT_VEC3:
            strncpy(node->name, "Constant (Vector3)", sizeof(node->name) - 1);
            node->output_count = 1;
            strncpy(node->outputs[0].name, "RGB", sizeof(node->outputs[0].name) - 1);
            node->outputs[0].type = CONNECTION_TYPE_VEC3;
            break;
            
        case NODE_TYPE_TEXTURE_SAMPLE:
            strncpy(node->name, "Texture Sample", sizeof(node->name) - 1);
            node->input_count = 1;
            strncpy(node->inputs[0].name, "UV", sizeof(node->inputs[0].name) - 1);
            node->inputs[0].type = CONNECTION_TYPE_VEC2;
            node->output_count = 1;
            strncpy(node->outputs[0].name, "Color", sizeof(node->outputs[0].name) - 1);
            node->outputs[0].type = CONNECTION_TYPE_VEC4;
            break;
            
        case NODE_TYPE_MULTIPLY:
            strncpy(node->name, "Multiply", sizeof(node->name) - 1);
            node->input_count = 2;
            strncpy(node->inputs[0].name, "A", sizeof(node->inputs[0].name) - 1);
            node->inputs[0].type = CONNECTION_TYPE_VEC3;
            strncpy(node->inputs[1].name, "B", sizeof(node->inputs[1].name) - 1);
            node->inputs[1].type = CONNECTION_TYPE_VEC3;
            node->output_count = 1;
            strncpy(node->outputs[0].name, "Result", sizeof(node->outputs[0].name) - 1);
            node->outputs[0].type = CONNECTION_TYPE_VEC3;
            break;
            
        case NODE_TYPE_PBR_MASTER:
            strncpy(node->name, "PBR Master", sizeof(node->name) - 1);
            node->input_count = 5;
            strncpy(node->inputs[0].name, "Albedo", sizeof(node->inputs[0].name) - 1);
            node->inputs[0].type = CONNECTION_TYPE_VEC3;
            strncpy(node->inputs[1].name, "Normal", sizeof(node->inputs[1].name) - 1);
            node->inputs[1].type = CONNECTION_TYPE_VEC3;
            strncpy(node->inputs[2].name, "Metallic", sizeof(node->inputs[2].name) - 1);
            node->inputs[2].type = CONNECTION_TYPE_FLOAT;
            strncpy(node->inputs[3].name, "Roughness", sizeof(node->inputs[3].name) - 1);
            node->inputs[3].type = CONNECTION_TYPE_FLOAT;
            strncpy(node->inputs[4].name, "AO", sizeof(node->inputs[4].name) - 1);
            node->inputs[4].type = CONNECTION_TYPE_FLOAT;
            node->output_count = 0;  // Master node has no outputs
            graph->master_node = node;  // Set as master
            break;
            
        default:
            snprintf(node->name, sizeof(node->name), "Node %u", node->node_id);
            break;
    }
    
    graph->node_count++;
    graph->needs_recompile = true;
    
    LOG_INFO("Added node: %s (ID: %u)", node->name, node->node_id);
    return node;
}

void material_graph_remove_node(MaterialGraph* graph, u32 node_id) {
    if (!graph) return;
    
    // Find node index
    i32 remove_index = -1;
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].node_id == node_id) {
            remove_index = (i32)i;
            break;
        }
    }
    
    if (remove_index < 0) {
        LOG_WARN("Node %u not found", node_id);
        return;
    }
    
    MaterialNode* node = &graph->nodes[remove_index];
    
    // Free node parameters
    if (node->parameters) free(node->parameters);
    
    // Disconnect all connections to/from this node
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* other = &graph->nodes[i];
        for (u32 j = 0; j < other->input_count; j++) {
            if (other->inputs[j].connected_node == node) {
                other->inputs[j].connected_node = NULL;
                other->inputs[j].connected_output_index = 0;
            }
        }
    }
    
    // Shift remaining nodes down
    for (u32 i = remove_index; i < graph->node_count - 1; i++) {
        graph->nodes[i] = graph->nodes[i + 1];
    }
    
    graph->node_count--;
    graph->needs_recompile = true;
    
    LOG_INFO("Removed node %u", node_id);
}

// =================================================================================================
//                                    CONNECTION MANAGEMENT
// =================================================================================================

bool material_graph_connect_nodes(MaterialGraph* graph, u32 source_node_id, u32 source_output,
                                  u32 dest_node_id, u32 dest_input) {
    if (!graph) return false;
    
    MaterialNode* source = material_graph_get_node_by_id(graph, source_node_id);
    MaterialNode* dest = material_graph_get_node_by_id(graph, dest_node_id);
    
    if (!source || !dest) {
        LOG_ERROR("Connection failed: node not found");
        return false;
    }
    
    if (source_output >= source->output_count) {
        LOG_ERROR("Connection failed: invalid source output %u", source_output);
        return false;
    }
    
    if (dest_input >= dest->input_count) {
        LOG_ERROR("Connection failed: invalid dest input %u", dest_input);
        return false;
    }
    
    // Type compatibility check
    ConnectionType source_type = source->outputs[source_output].type;
    ConnectionType dest_type = dest->inputs[dest_input].type;
    
    if (source_type != dest_type) {
        LOG_WARN("Type mismatch: connecting %d to %d (may need conversion)", 
                source_type, dest_type);
    }
    
    // Make connection
    dest->inputs[dest_input].connected_node = source;
    dest->inputs[dest_input].connected_output_index = source_output;
    
    graph->needs_recompile = true;
    
    LOG_INFO("Connected %s.%s -> %s.%s", 
            source->name, source->outputs[source_output].name,
            dest->name, dest->inputs[dest_input].name);
    
    return true;
}

void material_graph_disconnect_input(MaterialGraph* graph, u32 node_id, u32 input_index) {
    if (!graph) return;
    
    MaterialNode* node = material_graph_get_node_by_id(graph, node_id);
    if (!node) {
        LOG_ERROR("Disconnect failed: node %u not found", node_id);
        return;
    }
    
    if (input_index >= node->input_count) {
        LOG_ERROR("Disconnect failed: invalid input %u", input_index);
        return;
    }
    
    node->inputs[input_index].connected_node = NULL;
    node->inputs[input_index].connected_output_index = 0;
    
    graph->needs_recompile = true;
    
    LOG_INFO("Disconnected input %s.%s", node->name, node->inputs[input_index].name);
}

// =================================================================================================
//                                    VALIDATION
// =================================================================================================

bool material_graph_validate(MaterialGraph* graph) {
    if (!graph) return false;
    
    // Check for master node
    if (!graph->master_node) {
        LOG_ERROR("Validation failed: no PBR master node");
        return false;
    }
    
    // Basic cycle detection (simplified - just check for self-connections)
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* node = &graph->nodes[i];
        for (u32 j = 0; j < node->input_count; j++) {
            if (node->inputs[j].connected_node == node) {
                LOG_ERROR("Validation failed: node %s connects to itself", node->name);
                return false;
            }
        }
    }
    
    LOG_INFO("Graph validation passed");
    return true;
}

// =================================================================================================
//                                    SHADER COMPILATION (STUB)
// =================================================================================================

// Include the compiler implementation directly for now to avoid header complexity in this phase
// In a full build system this would be separate
#include "material_compiler.c"

bool material_graph_compile(MaterialGraph* graph) {
    if (!graph) return false;
    
    if (!material_graph_validate(graph)) {
        return false;
    }
    
    return true;
}

// =================================================================================================
//                                    RENDERING STUBS
// =================================================================================================

#include "material_editor_ui.c"

void material_graph_render_ui(MaterialGraph* graph) {
    // In strict architecture, we might pass ctx, but for stub compatibility we use global or assume ctx
    // For now, let's just log or call the implementation if we had the context
    // Since the stub signature matches, we can't easily pass ctx without changing header
    // But we can forward to our new implementation if we had access.
    
    // For this refactor, we've implemented the logic in material_editor_ui.c
    // We would need to expose the context to call material_editor_ui_render(ctx, graph);
    // Since we don't have ctx here, we'll leave this as a shim or placeholder for the build system.
    
    // Attempt to call the internal render if possible, or just note it's handled by that file
}

#include "material_editor_preview.c"

void material_graph_render_preview(MaterialGraph* graph) {
    // Similar to UI, we forward to our implementation
    // Ideally we'd pass ctx, but for now we assume this is called within a valid context or use globals
    // material_editor_preview_render(NULL, graph);
}

// Serialization
#include "material_serialization.c"
