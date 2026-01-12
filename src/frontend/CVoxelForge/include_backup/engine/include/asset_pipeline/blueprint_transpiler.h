#ifndef BLUEPRINT_TRANSPILER_H
#define BLUEPRINT_TRANSPILER_H

#include "unreal_asset_importer.h"
#include <stdint.h>
#include <stdbool.h>

// Blueprint Transpiler Configuration
#define MAX_BP_NODES 1000
#define MAX_BP_VARIABLES 100
#define MAX_BP_FUNCTIONS 50
#define MAX_BP_CONNECTIONS 2000

// Visual Script Node Types (Engine-side)
typedef enum {
    VSNODE_EVENT = 1,
    VSNODE_FUNCTION_CALL = 2,
    VSNODE_VARIABLE_GET = 3,
    VSNODE_VARIABLE_SET = 4,
    VSNODE_CONDITIONAL = 5,
    VSNODE_MATH_OPERATION = 6,
    VSNODE_COMPARISON = 7,
    VSNODE_LOOP = 8,
    VSNODE_LOGIC_GATE = 9,
    VSNODE_DATA_CONVERSION = 10,
    VSNODE_CUSTOM_EVENT = 11
} VisualScriptNodeType;

// Visual Script Data Types
typedef enum {
    VSDATA_VOID = 0,
    VSDATA_BOOL = 1,
    VSDATA_INT = 2,
    VSDATA_FLOAT = 3,
    VSDATA_STRING = 4,
    VSDATA_VECTOR = 5,
    VSDATA_OBJECT = 6,
    VSDATA_COLOR = 7
} VisualScriptDataType;

// Visual Script Pin Direction
typedef enum {
    VSPIN_INPUT = 0,
    VSPIN_OUTPUT = 1
} VisualScriptPinDirection;

// Visual Script Connection
typedef struct {
    uint32_t source_node_id;
    uint32_t source_pin_index;
    uint32_t target_node_id;
    uint32_t target_pin_index;
} VisualScriptConnection;

// Visual Script Pin
typedef struct {
    char name[64];
    VisualScriptDataType data_type;
    VisualScriptPinDirection direction;
    bool is_connected;
    uint32_t connection_count;
    VisualScriptConnection* connections;
} VisualScriptPin;

// Visual Script Node
typedef struct {
    uint32_t node_id;
    VisualScriptNodeType type;
    char name[128];
    char class_name[128];
    float position[2];
    uint32_t input_count;
    uint32_t output_count;
    VisualScriptPin* inputs;
    VisualScriptPin* outputs;
    
    // Node-specific data
    union {
        struct {
            char event_name[64];
            bool is_interface_event;
        } event_data;
        struct {
            char function_name[128];
            char target_class[128];
            bool is_static;
        } function_call_data;
        struct {
            char variable_name[64];
            VisualScriptDataType variable_type;
        } variable_data;
        struct {
            char operation[16]; // "+", "-", "*", "/", "%", etc.
        } math_data;
        struct {
            char comparison[8]; // "==", "!=", "<", ">", "<=", ">="
        } comparison_data;
        struct {
            char loop_type[16]; // "for", "while", "foreach"
            char condition[128];
        } loop_data;
    } node_data;
} VisualScriptNode;

// Visual Script Variable
typedef struct {
    char name[64];
    VisualScriptDataType data_type;
    bool is_array;
    uint32_t array_size;
    bool is_public;
    bool is_editable;
    char default_value[128];
} VisualScriptVariable;

// Visual Script Function
typedef struct {
    char name[128];
    VisualScriptDataType return_type;
    bool is_public;
    bool is_static;
    uint32_t parameter_count;
    VisualScriptVariable* parameters;
    uint32_t node_count;
    VisualScriptNode* nodes;
    uint32_t connection_count;
    VisualScriptConnection* connections;
} VisualScriptFunction;

// Visual Script Graph
typedef struct {
    char name[128];
    char parent_class[128];
    uint32_t variable_count;
    VisualScriptVariable* variables;
    uint32_t function_count;
    VisualScriptFunction* functions;
    uint32_t event_count;
    VisualScriptFunction* events;
} VisualScriptGraph;

// Blueprint Transpiler Context
typedef struct {
    UnrealPackage* package;
    UnrealBlueprintData* blueprint_data;
    VisualScriptGraph* visual_graph;
    char error_message[512];
    bool transpilation_successful;
} BlueprintTranspilerContext;

// Core Transpiler API
BlueprintTranspilerContext* Blueprint_CreateTranspiler(UnrealPackage* package, const char* blueprint_name);
void Blueprint_DestroyTranspiler(BlueprintTranspilerContext* context);
bool Blueprint_TranspileToVisualScript(BlueprintTranspilerContext* context);
bool Blueprint_SaveVisualScript(BlueprintTranspilerContext* context, const char* output_path);

// Node Mapping Functions
VisualScriptNodeType Blueprint_MapNodeType(UnrealBlueprintNodeType bp_type);
VisualScriptDataType Blueprint_MapDataType(const char* unreal_type);
bool Blueprint_TranspileNode(BlueprintTranspilerContext* context, UnrealBlueprintNode* bp_node, VisualScriptNode* vs_node);

// Connection and Flow Functions
bool Blueprint_TranspileConnections(BlueprintTranspilerContext* context);
bool Blueprint_TranspileExecutionFlow(BlueprintTranspilerContext* context);
bool Blueprint_TranspileDataFlow(BlueprintTranspilerContext* context);

// Variable and Function Mapping
bool Blueprint_TranspileVariables(BlueprintTranspilerContext* context);
bool Blueprint_TranspileFunctions(BlueprintTranspilerContext* context);
bool Blueprint_TranspileEvents(BlueprintTranspilerContext* context);

// Utility Functions
const char* Blueprint_GetErrorString(BlueprintTranspilerContext* context);
bool Blueprint_ValidateBlueprint(UnrealBlueprintData* blueprint);
bool Blueprint_ValidateVisualScript(VisualScriptGraph* graph);

// Code Generation Functions
bool Blueprint_GenerateScriptCode(BlueprintTranspilerContext* context, char** output_code, size_t* code_size);
bool Blueprint_GenerateNodeDocumentation(BlueprintTranspilerContext* context, char** documentation, size_t* doc_size);

// Debug and Analysis Functions
void Blueprint_PrintGraphStructure(BlueprintTranspilerContext* context);
bool Blueprint_AnalyzeGraphComplexity(BlueprintTranspilerContext* context, uint32_t* node_count, uint32_t* connection_count, uint32_t* cyclomatic_complexity);

// Memory Management Functions
void Blueprint_FreeVisualScriptGraph(VisualScriptGraph* graph);
void Unreal_FreeBlueprintData(UnrealBlueprintData* blueprint_data);

#endif // BLUEPRINT_TRANSPILER_H
