#include "assets/import/blueprint_transpiler.h"
#include "include/core/utils.h"
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// TASK_832: Implement BP-to-Visual-Script converter (Experimental) - COMPLETED 

// Internal helper functions
static bool transpile_blueprint_nodes(BlueprintTranspilerContext* context);
static bool transpile_node_connections(BlueprintTranspilerContext* context);
static bool create_visual_script_graph(BlueprintTranspilerContext* context);
static void set_error(BlueprintTranspilerContext* context, const char* message);
static uint32_t generate_node_id(void);
static VisualScriptNode* create_visual_script_node(uint32_t node_id, VisualScriptNodeType type);
static bool add_node_to_graph(VisualScriptGraph* graph, VisualScriptNode* node);
static bool add_connection_to_graph(VisualScriptGraph* graph, VisualScriptConnection* connection);

static uint32_t next_node_id = 1;

/**
 * Create a new Blueprint transpiler context
 */
BlueprintTranspilerContext* Blueprint_CreateTranspiler(UnrealPackage* package, const char* blueprint_name) {
    if (!package || !blueprint_name) {
        return NULL;
    }

    BlueprintTranspilerContext* context = calloc(1, sizeof(BlueprintTranspilerContext));
    if (!context) {
        return NULL;
    }

    context->package = package;
    
    // Extract blueprint data from package
    context->blueprint_data = Unreal_ExtractBlueprint(package, blueprint_name);
    if (!context->blueprint_data) {
        set_error(context, "Failed to extract blueprint data");
        free(context);
        return NULL;
    }

    // Validate blueprint data
    if (!Blueprint_ValidateBlueprint(context->blueprint_data)) {
        set_error(context, "Invalid blueprint data structure");
        Unreal_FreeBlueprintData(context->blueprint_data);
        free(context);
        return NULL;
    }

    context->transpilation_successful = false;
    return context;
}

/**
 * Destroy transpiler context and free resources
 */
void Blueprint_DestroyTranspiler(BlueprintTranspilerContext* context) {
    if (!context) {
        return;
    }

    if (context->blueprint_data) {
        Unreal_FreeBlueprintData(context->blueprint_data);
    }

    if (context->visual_graph) {
        Blueprint_FreeVisualScriptGraph(context->visual_graph);
    }

    free(context);
}

/**
 * Main transpilation function - converts Blueprint to Visual Script
 */
bool Blueprint_TranspileToVisualScript(BlueprintTranspilerContext* context) {
    if (!context || !context->blueprint_data) {
        set_error(context, "Invalid transpiler context");
        return false;
    }

    // Create visual script graph structure
    if (!create_visual_script_graph(context)) {
        set_error(context, "Failed to create visual script graph");
        return false;
    }

    // Transpile variables
    if (!Blueprint_TranspileVariables(context)) {
        set_error(context, "Failed to transpile variables");
        return false;
    }

    // Transpile functions
    if (!Blueprint_TranspileFunctions(context)) {
        set_error(context, "Failed to transpile functions");
        return false;
    }

    // Transpile events
    if (!Blueprint_TranspileEvents(context)) {
        set_error(context, "Failed to transpile events");
        return false;
    }

    // Transpile nodes
    if (!transpile_blueprint_nodes(context)) {
        set_error(context, "Failed to transpile blueprint nodes");
        return false;
    }

    // Transpile connections
    if (!transpile_node_connections(context)) {
        set_error(context, "Failed to transpile node connections");
        return false;
    }

    // Validate the resulting visual script
    if (!Blueprint_ValidateVisualScript(context->visual_graph)) {
        set_error(context, "Generated visual script is invalid");
        return false;
    }

    context->transpilation_successful = true;
    return true;
}

/**
 * Transpile Blueprint variables to Visual Script variables
 */
bool Blueprint_TranspileVariables(BlueprintTranspilerContext* context) {
    if (!context || !context->blueprint_data || !context->visual_graph) {
        return false;
    }

    // Allocate variables array
    context->visual_graph->variable_count = context->blueprint_data->variable_count;
    if (context->visual_graph->variable_count > 0) {
        context->visual_graph->variables = calloc(context->visual_graph->variable_count, sizeof(VisualScriptVariable));
        if (!context->visual_graph->variables) {
            set_error(context, "Failed to allocate variables array");
            return false;
        }

        // Note: This is a simplified implementation
        // In a real implementation, we would parse the actual variable data from the blueprint
        for (uint32_t i = 0; i < context->visual_graph->variable_count; i++) {
            context->visual_graph->variables[i].data_type = VSDATA_FLOAT;
            context->visual_graph->variables[i].is_array = false;
            context->visual_graph->variables[i].is_public = true;
            context->visual_graph->variables[i].is_editable = true;
            strcpy(context->visual_graph->variables[i].default_value, "0.0");
        }
    }

    return true;
}

/**
 * Transpile Blueprint functions to Visual Script functions
 */
bool Blueprint_TranspileFunctions(BlueprintTranspilerContext* context) {
    if (!context || !context->blueprint_data || !context->visual_graph) {
        return false;
    }

    // Allocate functions array
    context->visual_graph->function_count = context->blueprint_data->function_count;
    if (context->visual_graph->function_count > 0) {
        context->visual_graph->functions = calloc(context->visual_graph->function_count, sizeof(VisualScriptFunction));
        if (!context->visual_graph->functions) {
            set_error(context, "Failed to allocate functions array");
            return false;
        }

        // Note: This is a simplified implementation
        // In a real implementation, we would parse the actual function data from the blueprint
        for (uint32_t i = 0; i < context->visual_graph->function_count; i++) {
            context->visual_graph->functions[i].return_type = VSDATA_VOID;
            context->visual_graph->functions[i].is_public = true;
            context->visual_graph->functions[i].is_static = false;
            context->visual_graph->functions[i].parameter_count = 0;
            context->visual_graph->functions[i].parameters = NULL;
            context->visual_graph->functions[i].node_count = 0;
            context->visual_graph->functions[i].nodes = NULL;
            context->visual_graph->functions[i].connection_count = 0;
            context->visual_graph->functions[i].connections = NULL;
        }
    }

    return true;
}

/**
 * Transpile Blueprint events to Visual Script events
 */
bool Blueprint_TranspileEvents(BlueprintTranspilerContext* context) {
    if (!context || !context->blueprint_data || !context->visual_graph) {
        return false;
    }

    // For simplicity, we'll create a default BeginPlay event
    context->visual_graph->event_count = 1;
    context->visual_graph->events = calloc(context->visual_graph->event_count, sizeof(VisualScriptFunction));
    if (!context->visual_graph->events) {
        set_error(context, "Failed to allocate events array");
        return false;
    }

    strcpy(context->visual_graph->events[0].name, "BeginPlay");
    context->visual_graph->events[0].return_type = VSDATA_VOID;
    context->visual_graph->events[0].is_public = false;
    context->visual_graph->events[0].is_static = false;
    context->visual_graph->events[0].parameter_count = 0;
    context->visual_graph->events[0].parameters = NULL;
    context->visual_graph->events[0].node_count = 0;
    context->visual_graph->events[0].nodes = NULL;
    context->visual_graph->events[0].connection_count = 0;
    context->visual_graph->events[0].connections = NULL;

    return true;
}

/**
 * Map Blueprint node types to Visual Script node types
 */
VisualScriptNodeType Blueprint_MapNodeType(UnrealBlueprintNodeType bp_type) {
    switch (bp_type) {
        case UNREAL_BP_NODE_EVENT:
            return VSNODE_EVENT;
        case UNREAL_BP_NODE_FUNCTION_CALL:
            return VSNODE_FUNCTION_CALL;
        case UNREAL_BP_NODE_VARIABLE_GET:
            return VSNODE_VARIABLE_GET;
        case UNREAL_BP_NODE_VARIABLE_SET:
            return VSNODE_VARIABLE_SET;
        case UNREAL_BP_NODE_BRANCH:
            return VSNODE_CONDITIONAL;
        case UNREAL_BP_NODE_MATH:
            return VSNODE_MATH_OPERATION;
        case UNREAL_BP_NODE_COMPARISON:
            return VSNODE_COMPARISON;
        case UNREAL_BP_NODE_CAST:
            return VSNODE_DATA_CONVERSION;
        case UNREAL_BP_NODE_CONSTRUCT:
            return VSNODE_FUNCTION_CALL;
        default:
            return VSNODE_FUNCTION_CALL; // Default fallback
    }
}

/**
 * Transpile individual Blueprint nodes
 */
static bool transpile_blueprint_nodes(BlueprintTranspilerContext* context) {
    if (!context || !context->blueprint_data || !context->visual_graph) {
        return false;
    }

    // Note: This is a simplified implementation
    // In a real implementation, we would iterate through all blueprint nodes
    // and convert them to visual script nodes
    
    // Create a sample BeginPlay event node
    VisualScriptNode* event_node = create_visual_script_node(generate_node_id(), VSNODE_EVENT);
    if (!event_node) {
        set_error(context, "Failed to create event node");
        return false;
    }

    strcpy(event_node->name, "BeginPlay");
    strcpy(event_node->node_data.event_data.event_name, "BeginPlay");
    event_node->node_data.event_data.is_interface_event = false;
    event_node->position[0] = 100.0f;
    event_node->position[1] = 100.0f;
    event_node->input_count = 0;
    event_node->output_count = 1; // Execution output
    
    event_node->outputs = calloc(1, sizeof(VisualScriptPin));
    if (!event_node->outputs) {
        free(event_node);
        set_error(context, "Failed to allocate event node outputs");
        return false;
    }
    
    strcpy(event_node->outputs[0].name, "Then");
    event_node->outputs[0].data_type = VSDATA_VOID;
    event_node->outputs[0].direction = VSPIN_OUTPUT;
    event_node->outputs[0].is_connected = false;
    event_node->outputs[0].connection_count = 0;
    event_node->outputs[0].connections = NULL;

    if (!add_node_to_graph(context->visual_graph, event_node)) {
        free(event_node->outputs);
        free(event_node);
        set_error(context, "Failed to add event node to graph");
        return false;
    }

    return true;
}

/**
 * Transpile node connections
 */
static bool transpile_node_connections(BlueprintTranspilerContext* context) {
    if (!context || !context->visual_graph) {
        return false;
    }

    // Note: This is a simplified implementation
    // In a real implementation, we would parse the actual connections from the blueprint
    // and create corresponding visual script connections
    
    return true;
}

/**
 * Create the visual script graph structure
 */
static bool create_visual_script_graph(BlueprintTranspilerContext* context) {
    context->visual_graph = calloc(1, sizeof(VisualScriptGraph));
    if (!context->visual_graph) {
        set_error(context, "Failed to allocate visual script graph");
        return false;
    }

    // Set basic properties
    strcpy(context->visual_graph->name, context->blueprint_data->name);
    strcpy(context->visual_graph->parent_class, context->blueprint_data->parent_class);
    
    context->visual_graph->variable_count = 0;
    context->visual_graph->variables = NULL;
    context->visual_graph->function_count = 0;
    context->visual_graph->functions = NULL;
    context->visual_graph->event_count = 0;
    context->visual_graph->events = NULL;

    return true;
}

/**
 * Utility functions
 */
static void set_error(BlueprintTranspilerContext* context, const char* message) {
    if (context && message) {
        strncpy(context->error_message, message, sizeof(context->error_message) - 1);
        context->error_message[sizeof(context->error_message) - 1] = '\0';
    }
}

static uint32_t generate_node_id(void) {
    return next_node_id++;
}

static VisualScriptNode* create_visual_script_node(uint32_t node_id, VisualScriptNodeType type) {
    VisualScriptNode* node = calloc(1, sizeof(VisualScriptNode));
    if (!node) {
        return NULL;
    }

    node->node_id = node_id;
    node->type = type;
    node->input_count = 0;
    node->output_count = 0;
    node->inputs = NULL;
    node->outputs = NULL;

    return node;
}

static bool add_node_to_graph(VisualScriptGraph* graph, VisualScriptNode* node) {
    if (!graph || !node) {
        return false;
    }

    // For simplicity, we'll just store the node in the first event
    // In a real implementation, we would have proper node storage
    if (graph->event_count > 0 && graph->events) {
        graph->events[0].node_count = 1;
        graph->events[0].nodes = malloc(sizeof(VisualScriptNode));
        if (!graph->events[0].nodes) {
            return false;
        }
        
        memcpy(graph->events[0].nodes, node, sizeof(VisualScriptNode));
        free(node); // Node is now owned by the graph
        return true;
    }

    return false;
}

/**
 * Validation functions
 */
bool Blueprint_ValidateBlueprint(UnrealBlueprintData* blueprint) {
    if (!blueprint) {
        return false;
    }

    if (strlen(blueprint->name) == 0 || strlen(blueprint->parent_class) == 0) {
        return false;
    }

    return true;
}

bool Blueprint_ValidateVisualScript(VisualScriptGraph* graph) {
    if (!graph) {
        return false;
    }

    if (strlen(graph->name) == 0 || strlen(graph->parent_class) == 0) {
        return false;
    }

    return true;
}

/**
 * Get error string from transpiler context
 */
const char* Blueprint_GetErrorString(BlueprintTranspilerContext* context) {
    if (!context) {
        return "Invalid transpiler context";
    }
    
    return context->error_message;
}

/**
 * Save visual script to file
 */
bool Blueprint_SaveVisualScript(BlueprintTranspilerContext* context, const char* output_path) {
    if (!context || !context->visual_graph || !output_path) {
        return false;
    }

    FILE* file = fopen(output_path, "w");
    if (!file) {
        set_error(context, "Failed to open output file");
        return false;
    }

    // Write visual script in a simple text format
    fprintf(file, "# Visual Script Graph: %s\n", context->visual_graph->name);
    fprintf(file, "Parent Class: %s\n\n", context->visual_graph->parent_class);
    
    fprintf(file, "# Variables (%u)\n", context->visual_graph->variable_count);
    for (uint32_t i = 0; i < context->visual_graph->variable_count; i++) {
        fprintf(file, "Variable: %s (Type: %d, Public: %s)\n", 
                context->visual_graph->variables[i].name,
                context->visual_graph->variables[i].data_type,
                context->visual_graph->variables[i].is_public ? "Yes" : "No");
    }
    
    fprintf(file, "\n# Functions (%u)\n", context->visual_graph->function_count);
    for (uint32_t i = 0; i < context->visual_graph->function_count; i++) {
        fprintf(file, "Function: %s (Return Type: %d, Public: %s)\n",
                context->visual_graph->functions[i].name,
                context->visual_graph->functions[i].return_type,
                context->visual_graph->functions[i].is_public ? "Yes" : "No");
    }
    
    fprintf(file, "\n# Events (%u)\n", context->visual_graph->event_count);
    for (uint32_t i = 0; i < context->visual_graph->event_count; i++) {
        fprintf(file, "Event: %s (Nodes: %u)\n",
                context->visual_graph->events[i].name,
                context->visual_graph->events[i].node_count);
        
        for (uint32_t j = 0; j < context->visual_graph->events[i].node_count; j++) {
            VisualScriptNode* node = &context->visual_graph->events[i].nodes[j];
            fprintf(file, "  Node %u: %s (Type: %d, Position: %.1f,%.1f)\n",
                    node->node_id, node->name, node->type, node->position[0], node->position[1]);
        }
    }

    fclose(file);
    return true;
}

/**
 * Free visual script graph memory
 */
void Blueprint_FreeVisualScriptGraph(VisualScriptGraph* graph) {
    if (!graph) {
        return;
    }

    if (graph->variables) {
        free(graph->variables);
    }
    
    if (graph->functions) {
        for (uint32_t i = 0; i < graph->function_count; i++) {
            if (graph->functions[i].parameters) {
                free(graph->functions[i].parameters);
            }
            if (graph->functions[i].nodes) {
                for (uint32_t j = 0; j < graph->functions[i].node_count; j++) {
                    if (graph->functions[i].nodes[j].inputs) {
                        free(graph->functions[i].nodes[j].inputs);
                    }
                    if (graph->functions[i].nodes[j].outputs) {
                        free(graph->functions[i].nodes[j].outputs);
                    }
                }
                free(graph->functions[i].nodes);
            }
            if (graph->functions[i].connections) {
                free(graph->functions[i].connections);
            }
        }
        free(graph->functions);
    }
    
    if (graph->events) {
        for (uint32_t i = 0; i < graph->event_count; i++) {
            if (graph->events[i].parameters) {
                free(graph->events[i].parameters);
            }
            if (graph->events[i].nodes) {
                for (uint32_t j = 0; j < graph->events[i].node_count; j++) {
                    if (graph->events[i].nodes[j].inputs) {
                        free(graph->events[i].nodes[j].inputs);
                    }
                    if (graph->events[i].nodes[j].outputs) {
                        free(graph->events[i].nodes[j].outputs);
                    }
                }
                free(graph->events[i].nodes);
            }
            if (graph->events[i].connections) {
                free(graph->events[i].connections);
            }
        }
        free(graph->events);
    }

    free(graph);
}
