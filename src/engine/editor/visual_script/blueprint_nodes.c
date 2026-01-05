#include "editor/visual_script/blueprint_nodes.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_NODES 1024
#define MAX_PINS_PER_NODE 16

static u32 global_pin_id_counter = 1;
static u32 global_node_id_counter = 1;

void blueprint_editor_init(BlueprintEditor* editor) {
    if (!editor) return;
    memset(editor, 0, sizeof(BlueprintEditor));
    editor->zoom = 1.0f;
    LOG_INFO("Blueprint editor initialized");
}

void blueprint_editor_shutdown(BlueprintEditor* editor) {
    if (!editor) return;
    // Graph cleanup handled by caller usually, but we clear ref
    editor->current_graph = NULL;
}

BlueprintGraph* blueprint_create_graph(const char* name) {
    BlueprintGraph* graph = (BlueprintGraph*)calloc(1, sizeof(BlueprintGraph));
    strncpy(graph->name, name, sizeof(graph->name) - 1);
    
    graph->node_capacity = 64;
    graph->nodes = (BlueprintNode*)calloc(graph->node_capacity, sizeof(BlueprintNode));
    
    return graph;
}

void blueprint_destroy_graph(BlueprintGraph* graph) {
    if (!graph) return;
    
    for (u32 i = 0; i < graph->node_count; i++) {
        BlueprintNode* node = &graph->nodes[i];
        if (node->inputs) free(node->inputs);
        if (node->outputs) free(node->outputs);
    }
    free(graph->nodes);
    free(graph);
}

void blueprint_set_active_graph(BlueprintEditor* editor, BlueprintGraph* graph) {
    if (editor) editor->current_graph = graph;
}

BlueprintNode* blueprint_add_node(BlueprintGraph* graph, NodeType type, const char* title, Vec2 pos) {
    if (!graph || graph->node_count >= graph->node_capacity) return NULL;
    
    BlueprintNode* node = &graph->nodes[graph->node_count++];
    memset(node, 0, sizeof(BlueprintNode));
    
    node->id = global_node_id_counter++;
    node->type = type;
    strncpy(node->title, title, sizeof(node->title) - 1);
    node->position = pos;
    node->size = (Vec2){200.0f, 100.0f}; // Default size
    
    return node;
}

void blueprint_remove_node(BlueprintGraph* graph, u32 node_id) {
    if (!graph) return;
    
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].id == node_id) {
            BlueprintNode* node = &graph->nodes[i];
            
            // Disconnect all pins
            // Iterate all other nodes to find links to this node's pins
            for (u32 j = 0; j < graph->node_count; j++) {
                BlueprintNode* other = &graph->nodes[j];
                if (other->id == node_id) continue;
                
                // Check inputs of other node (that might be connected to our outputs)
                for (u32 p = 0; p < other->input_count; p++) {
                   if (other->inputs[p].is_connected) {
                       // Find if it connects to one of our outputs
                       for (u32 k = 0; k < node->output_count; k++) {
                           if (other->inputs[p].connected_pin_id == node->outputs[k].id) {
                               other->inputs[p].is_connected = false;
                               other->inputs[p].connected_pin_id = 0;
                           }
                       }
                   }
                }
                
                // Check outputs of other node (that might be connected to our inputs)
                for (u32 p = 0; p < other->output_count; p++) {
                   if (other->outputs[p].is_connected) {
                       // Find if it connects to one of our inputs
                       for (u32 k = 0; k < node->input_count; k++) {
                           if (other->outputs[p].connected_pin_id == node->inputs[k].id) {
                               other->outputs[p].is_connected = false;
                               other->outputs[p].connected_pin_id = 0;
                           }
                       }
                   }
                }
            }
            
            if (node->inputs) free(node->inputs);
            if (node->outputs) free(node->outputs);
            
            // Swap remove
            graph->nodes[i] = graph->nodes[graph->node_count - 1];
            graph->node_count--;
            return;
        }
    }
}

BlueprintNode* blueprint_get_node(BlueprintGraph* graph, u32 node_id) {
    if (!graph) return NULL;
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].id == node_id) return &graph->nodes[i];
    }
    return NULL;
}

void blueprint_add_pin(BlueprintNode* node, const char* name, PinType type, PinDirection dir) {
    if (!node) return;
    
    u32 count = (dir == PIN_DIR_INPUT) ? node->input_count : node->output_count;
    BlueprintPin** pins_ptr = (dir == PIN_DIR_INPUT) ? &node->inputs : &node->outputs;
    
    *pins_ptr = (BlueprintPin*)realloc(*pins_ptr, (count + 1) * sizeof(BlueprintPin));
    BlueprintPin* pin = &(*pins_ptr)[count];
    
    memset(pin, 0, sizeof(BlueprintPin));
    pin->id = global_pin_id_counter++;
    pin->node_id = node->id;
    strncpy(pin->name, name, sizeof(pin->name) - 1);
    pin->type = type;
    pin->direction = dir;
    
    if (dir == PIN_DIR_INPUT) node->input_count++;
    else node->output_count++;
    
    // Adjust node height based on pins
    float pin_height = 20.0f;
    float header_height = 30.0f;
    u32 max_pins = (node->input_count > node->output_count) ? node->input_count : node->output_count;
    node->size.y = header_height + max_pins * pin_height + 10.0f;
}

static BlueprintPin* find_pin_global(BlueprintGraph* graph, u32 pin_id) {
    for (u32 i = 0; i < graph->node_count; i++) {
        BlueprintNode* node = &graph->nodes[i];
        for (u32 j = 0; j < node->input_count; j++) {
            if (node->inputs[j].id == pin_id) return &node->inputs[j];
        }
        for (u32 j = 0; j < node->output_count; j++) {
            if (node->outputs[j].id == pin_id) return &node->outputs[j];
        }
    }
    return NULL;
}

bool blueprint_connect_pins(BlueprintGraph* graph, u32 output_pin_id, u32 input_pin_id) {
    if (!graph) return false;
    
    BlueprintPin* out = find_pin_global(graph, output_pin_id);
    BlueprintPin* in = find_pin_global(graph, input_pin_id);
    
    if (!out || !in) return false;
    if (out->direction != PIN_DIR_OUTPUT || in->direction != PIN_DIR_INPUT) return false;
    if (out->type != in->type && out->type != PIN_TYPE_EXEC) return false; // Type check (exec is special)
    // Strict typing: inputs match output type unless wildcards (not impl yet)
    
    // For exec pins, we allow 1 output -> 1 input (usually)
    // But data pins: 1 output -> N inputs
    // Input pin can only have 1 source
    
    in->connected_pin_id = output_pin_id;
    in->is_connected = true;
    
    // For exec pins, the output also tracks connection (flow)
    if (out->type == PIN_TYPE_EXEC) {
        out->connected_pin_id = input_pin_id;
        out->is_connected = true;
    }
    
    return true;
}

void blueprint_disconnect_pin(BlueprintGraph* graph, u32 pin_id) {
    if (!graph) return;
    
    BlueprintPin* pin = find_pin_global(graph, pin_id);
    if (!pin || !pin->is_connected) return;
    
    u32 other_id = pin->connected_pin_id;
    BlueprintPin* other = find_pin_global(graph, other_id);
    
    pin->is_connected = false;
    pin->connected_pin_id = 0;
    
    if (other) {
        // Need to check if logic is 1:1 or 1:N.
        // If 'other' is an output feeding multiple inputs, we shouldn't necessarily clear its connection status
        // unless this was the only connection.
        // For simplified exec flow (1:1), clear it.
        if (pin->type == PIN_TYPE_EXEC || other->type == PIN_TYPE_EXEC) {
            other->is_connected = false;
            other->connected_pin_id = 0;
        }
    }
}

BlueprintNode* blueprint_create_event_node(BlueprintGraph* graph, const char* event_name, Vec2 pos) {
    BlueprintNode* node = blueprint_add_node(graph, NODE_TYPE_EVENT, event_name, pos);
    blueprint_add_pin(node, "Out", PIN_TYPE_EXEC, PIN_DIR_OUTPUT);
    return node;
}

BlueprintNode* blueprint_create_math_node(BlueprintGraph* graph, const char* op, Vec2 pos) {
    BlueprintNode* node = blueprint_add_node(graph, NODE_TYPE_MATH, op, pos);
    // Generic math node template (float add)
    blueprint_add_pin(node, "A", PIN_TYPE_FLOAT, PIN_DIR_INPUT);
    blueprint_add_pin(node, "B", PIN_TYPE_FLOAT, PIN_DIR_INPUT);
    blueprint_add_pin(node, "Result", PIN_TYPE_FLOAT, PIN_DIR_OUTPUT);
    return node;
}

BlueprintNode* blueprint_create_flow_node(BlueprintGraph* graph, const char* type, Vec2 pos) {
    BlueprintNode* node = blueprint_add_node(graph, NODE_TYPE_FLOW, type, pos);
    
    if (strcmp(type, "Branch") == 0) {
        blueprint_add_pin(node, "In", PIN_TYPE_EXEC, PIN_DIR_INPUT);
        blueprint_add_pin(node, "Condition", PIN_TYPE_BOOL, PIN_DIR_INPUT);
        blueprint_add_pin(node, "True", PIN_TYPE_EXEC, PIN_DIR_OUTPUT);
        blueprint_add_pin(node, "False", PIN_TYPE_EXEC, PIN_DIR_OUTPUT);
    }
    return node;
}

void blueprint_save(BlueprintGraph* graph, const char* path) {
    // Stub: Serialize to JSON
    LOG_INFO("Saving blueprint to %s", path);
}

BlueprintGraph* blueprint_load(const char* path) {
    // Stub: Load from JSON
    LOG_INFO("Loading blueprint from %s", path);
    return blueprint_create_graph("Loaded Graph");
}

void blueprint_register_standard_nodes(void) {
    // Registry initialization (stub)
}
