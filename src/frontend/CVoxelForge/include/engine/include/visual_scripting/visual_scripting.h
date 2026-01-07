// include/engine/visual_scripting/visual_scripting.h
//
// Purpose: Advanced visual scripting system superior to Unity's Bolt/Visual Scripting
// This system provides node-based visual programming with real-time compilation,
// JIT optimization, and seamless C/C++ integration for maximum performance.
//
// Key Features:
// - Node-based visual editor with drag-and-drop interface
// - Real-time compilation with LLVM JIT backend
// - Hot-reload support during gameplay
// - Custom node creation with C/C++ integration
// - Advanced debugging with breakpoints and profiling
// - Performance optimization with automatic node merging
// - Template library for common game logic patterns
// - Visual debugging with execution flow visualization
//
// Performance Advantages over Unity Bolt:
// - Native JIT compilation vs interpreted execution
// - SIMD optimization for data processing nodes
// - Automatic node batching and parallel execution
// - Zero-allocation execution paths
// - GPU acceleration for compute-intensive nodes
//
// Public APIs:
// - VisualScript: Main container for visual scripts
// - Node: Individual operation nodes with inputs/outputs
// - Graph: Network of connected nodes
// - Compiler: Real-time compilation and optimization
// - Debugger: Advanced debugging and profiling tools
//
// Ownership: VisualScript owns all nodes and graphs
// Invariants: Node connections must match type signatures
//
#ifndef VISUAL_SCRIPTING_H
#define VISUAL_SCRIPTING_H

#include "../../common.h"
#include "../core/performance.h"
#include "../ecs/ecs.h"
#include <stdbool.h>

// ============================================================================
// NODE SYSTEM
// ============================================================================

// Node types
typedef enum {
  NODE_TYPE_INPUT = 0,
  NODE_TYPE_OUTPUT,
  NODE_TYPE_VARIABLE,
  NODE_TYPE_OPERATION,
  NODE_TYPE_CONDITIONAL,
  NODE_TYPE_LOOP,
  NODE_TYPE_FUNCTION_CALL,
  NODE_TYPE_EVENT,
  NODE_TYPE_CUSTOM,
  NODE_TYPE_COMMENT
} NodeType;

// Data types for node pins
typedef enum {
  PIN_TYPE_VOID = 0,
  PIN_TYPE_BOOL,
  PIN_TYPE_INT,
  PIN_TYPE_FLOAT,
  PIN_TYPE_STRING,
  PIN_TYPE_VECTOR2,
  PIN_TYPE_VECTOR3,
  PIN_TYPE_VECTOR4,
  PIN_TYPE_ENTITY,
  PIN_TYPE_COMPONENT,
  PIN_TYPE_ASSET,
  PIN_TYPE_ANY
} PinType;

// Pin direction
typedef enum {
  PIN_DIRECTION_INPUT = 0,
  PIN_DIRECTION_OUTPUT,
  PIN_DIRECTION_INPUT_OUTPUT
} PinDirection;

// Node pin (connection point)
typedef struct {
  u32 id;
  const char *name;
  PinType type;
  PinDirection direction;
  bool is_optional;
  bool has_default_value;
  union {
    bool bool_default;
    i32 int_default;
    f32 float_default;
    char *string_default;
    struct { f32 x, y; } vector2_default;
    struct { f32 x, y, z; } vector3_default;
    struct { f32 x, y, z, w; } vector4_default;
    Entity entity_default;
  } default_value;
} Pin;

// Node execution context
typedef struct {
  World *ecs_world;
  f32 delta_time;
  Entity calling_entity;
  void *user_data;
  void *local_variables;
  u32 execution_depth;
  bool debug_mode;
} ExecutionContext;

// Node function signature
typedef void (*NodeFunc)(ExecutionContext *context, void **inputs, void **outputs);

// Custom node implementation
typedef struct {
  const char *name;
  const char *category;
  const char *description;
  Pin *input_pins;
  u32 input_pin_count;
  Pin *output_pins;
  u32 output_pin_count;
  NodeFunc execute;
  bool is_pure;  // No side effects
  bool is_thread_safe;
  u32 color;  // UI color (RGBA)
} CustomNode;

// Node structure
typedef struct {
  u32 id;
  NodeType type;
  const char *name;
  const char *category;
  vec2 position;
  vec2 size;
  Pin *input_pins;
  u32 input_pin_count;
  Pin *output_pins;
  u32 output_pin_count;
  NodeFunc execute;
  CustomNode *custom_data;
  char *comment_text;
  bool is_enabled;
  bool is_breakpoint;
  u32 color;
  f64 execution_time;
  u32 execution_count;
} Node;

// ============================================================================
// CONNECTION SYSTEM
// ============================================================================

// Connection between node pins
typedef struct {
  u32 id;
  u32 output_node_id;
  u32 output_pin_id;
  u32 input_node_id;
  u32 input_pin_id;
  bool is_enabled;
} Connection;

// ============================================================================
// GRAPH SYSTEM
// ============================================================================

// Visual graph containing nodes and connections
typedef struct {
  Node *nodes;
  u32 node_count;
  u32 max_nodes;
  Connection *connections;
  u32 connection_count;
  u32 max_connections;
  char *name;
  char *description;
  vec2 scroll_offset;
  f32 zoom_level;
  bool is_dirty;
  u32 version;
} Graph;

// Graph execution state
typedef struct {
  Graph *graph;
  Node *current_node;
  u32 execution_id;
  bool is_running;
  bool is_paused;
  bool step_mode;
  u32 break_node_id;
  void *execution_stack;
  u32 stack_depth;
} GraphExecution;

// ============================================================================
// VARIABLE SYSTEM
// ============================================================================

// Variable types
typedef enum {
  VAR_TYPE_LOCAL = 0,
  VAR_TYPE_GLOBAL,
  VAR_TYPE_COMPONENT,
  VAR_TYPE_SYSTEM
} VariableType;

// Variable definition
typedef struct {
  u32 id;
  const char *name;
  PinType type;
  VariableType var_type;
  bool is_readonly;
  bool is_persistent;
  void *value;
  void *default_value;
} Variable;

// Variable scope
typedef struct {
  Variable *variables;
  u32 variable_count;
  u32 max_variables;
  char *scope_name;
  struct VariableScope *parent_scope;
} VariableScope;

// ============================================================================
// COMPILATION SYSTEM
// ============================================================================

// Compilation target
typedef enum {
  COMPILE_TARGET_INTERPRETED = 0,
  COMPILE_TARGET_JIT,
  COMPILE_TARGET_NATIVE,
  COMPILE_TARGET_GPU
} CompileTarget;

// Compilation result
typedef struct {
  bool success;
  char *error_message;
  void *compiled_code;
  u32 code_size;
  f64 compilation_time;
  u32 optimization_level;
} CompilationResult;

// Compiler configuration
typedef struct {
  CompileTarget target;
  u32 optimization_level;
  bool enable_debug_symbols;
  bool enable_profiling;
  bool enable_parallel_execution;
  bool enable_gpu_acceleration;
} CompilerConfig;

// ============================================================================
// DEBUGGING SYSTEM
// ============================================================================

// Debug information
typedef struct {
  u32 node_id;
  u32 execution_count;
  f64 total_execution_time;
  f64 average_execution_time;
  f64 min_execution_time;
  f64 max_execution_time;
  bool has_error;
  char *error_message;
} NodeDebugInfo;

// Breakpoint
typedef struct {
  u32 node_id;
  bool enabled;
  u32 hit_count;
  char *condition;  // Conditional breakpoint
} Breakpoint;

// Debug session
typedef struct {
  Graph *graph;
  GraphExecution *execution;
  Breakpoint *breakpoints;
  u32 breakpoint_count;
  NodeDebugInfo *node_debug_info;
  u32 node_debug_count;
  bool is_paused;
  u32 current_node_id;
  void *call_stack;
  u32 call_stack_depth;
} DebugSession;

// ============================================================================
// VISUAL SCRIPT
// ============================================================================

// Main visual script container
typedef struct {
  Graph *main_graph;
  Graph *function_graphs;
  u32 function_graph_count;
  u32 max_function_graphs;
  VariableScope *global_scope;
  char *script_name;
  char *script_path;
  bool is_dirty;
  bool auto_compile;
  CompilerConfig compiler_config;
  CompilationResult last_compilation;
  DebugSession *debug_session;
  u32 version;
} VisualScript;

// ============================================================================
// REGISTRY SYSTEM
// ============================================================================

// Node registry for built-in and custom nodes
typedef struct {
  CustomNode *nodes;
  u32 node_count;
  u32 max_nodes;
  CustomNode **category_nodes;
  u32 category_count;
  char **category_names;
} NodeRegistry;

// ============================================================================
// PUBLIC API
// ============================================================================

// Visual script management
VisualScript *visual_script_create(const char *name);
void visual_script_destroy(VisualScript *script);
bool visual_script_load(VisualScript *script, const char *filename);
bool visual_script_save(VisualScript *script, const char *filename);

// Graph operations
Graph *visual_script_create_graph(VisualScript *script, const char *name);
Node *graph_add_node(Graph *graph, NodeType type, vec2 position);
Connection *graph_add_connection(Graph *graph, u32 output_node, u32 output_pin, 
                                 u32 input_node, u32 input_pin);
void graph_remove_node(Graph *graph, u32 node_id);
void graph_remove_connection(Graph *graph, u32 connection_id);

// Node operations
Node *graph_find_node(Graph *graph, u32 node_id);
Pin *node_find_pin(Node *node, u32 pin_id);
bool node_can_connect(Node *output_node, u32 output_pin, Node *input_node, u32 input_pin);

// Variable management
Variable *visual_script_create_variable(VisualScript *script, const char *name, PinType type);
Variable *visual_script_get_variable(VisualScript *script, const char *name);
bool visual_script_set_variable(VisualScript *script, const char *name, void *value);

// Compilation
CompilationResult visual_script_compile(VisualScript *script, const CompilerConfig *config);
bool visual_script_execute(VisualScript *script, ExecutionContext *context);

// Custom node registration
bool visual_script_register_node(VisualScript *script, const CustomNode *node);
CustomNode *visual_script_get_registered_node(VisualScript *script, const char *name);

// ============================================================================
// EXECUTION API
// ============================================================================

// Script execution
bool visual_script_start(VisualScript *script, ExecutionContext *context);
bool visual_script_stop(VisualScript *script);
bool visual_script_pause(VisualScript *script);
bool visual_script_resume(VisualScript *script);
bool visual_script_step(VisualScript *script);

// Event system
typedef struct {
  const char *event_name;
  void *event_data;
  Entity target_entity;
} ScriptEvent;

void visual_script_emit_event(VisualScript *script, const ScriptEvent *event);
void visual_script_set_event_handler(VisualScript *script, const char *event_name, u32 node_id);

// ============================================================================
// DEBUGGING API
// ============================================================================

// Debug session management
DebugSession *visual_script_start_debug(VisualScript *script);
void visual_script_stop_debug(VisualScript *script);
bool visual_script_set_breakpoint(VisualScript *script, u32 node_id, bool enabled);
bool visual_script_set_conditional_breakpoint(VisualScript *script, u32 node_id, const char *condition);

// Debug information
NodeDebugInfo *visual_script_get_node_debug_info(VisualScript *script, u32 node_id);
void visual_script_print_debug_report(VisualScript *script);

// ============================================================================
// PERFORMANCE OPTIMIZATION
// ============================================================================

// JIT compilation
void *visual_script_compile_to_native(VisualScript *script);
bool visual_script_enable_gpu_acceleration(VisualScript *script);

// Node optimization
void visual_script_optimize_graph(VisualScript *script);
bool visual_script_batch_similar_nodes(VisualScript *script);
bool visual_script_parallelize_execution(VisualScript *script);

// Performance profiling
void visual_script_enable_profiling(VisualScript *script, bool enable);
void visual_script_print_performance_report(VisualScript *script);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Node creation macros
#define VSC_CREATE_NODE(graph, type, x, y) \
  graph_add_node(graph, type, (vec2){x, y})

#define VSC_CONNECT_NODES(graph, out_node, out_pin, in_node, in_pin) \
  graph_add_connection(graph, out_node, out_pin, in_node, in_pin)

// Custom node creation macro
#define VSC_REGISTER_CUSTOM_NODE(script, name, category, desc, func, ...) \
  do { \
    static Pin input_pins[] = {__VA_ARGS__}; \
    CustomNode node = { \
      .name = name, \
      .category = category, \
      .description = desc, \
      .input_pins = input_pins, \
      .input_pin_count = sizeof(input_pins)/sizeof(input_pins[0]), \
      .execute = func \
    }; \
    visual_script_register_node(script, &node); \
  } while(0)

// ============================================================================
// BUILT-IN NODES
// ============================================================================

// Math nodes
void node_add_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_subtract_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_multiply_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_divide_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_sin_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_cos_execute(ExecutionContext *context, void **inputs, void **outputs);

// Logic nodes
void node_and_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_or_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_not_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_compare_execute(ExecutionContext *context, void **inputs, void **outputs);

// Control flow nodes
void node_branch_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_for_loop_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_while_loop_execute(ExecutionContext *context, void **inputs, void **outputs);

// ECS integration nodes
void node_get_component_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_set_component_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_has_component_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_create_entity_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_destroy_entity_execute(ExecutionContext *context, void **inputs, void **outputs);

// Time nodes
void node_get_delta_time_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_get_time_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_wait_execute(ExecutionContext *context, void **inputs, void **outputs);

// Input nodes
void node_get_key_pressed_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_get_mouse_position_execute(ExecutionContext *context, void **inputs, void **outputs);
void node_get_button_pressed_execute(ExecutionContext *context, void **inputs, void **outputs);

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Template system
typedef struct {
  const char *name;
  const char *description;
  Graph *template_graph;
  Variable *template_variables;
  u32 variable_count;
} ScriptTemplate;

ScriptTemplate *visual_script_create_template(VisualScript *script, const char *name);
bool visual_script_apply_template(VisualScript *script, ScriptTemplate *template);

// Hot-reload system
bool visual_script_enable_hot_reload(VisualScript *script, bool enable);
void visual_script_on_file_changed(VisualScript *script, const char *filename);

// Version control integration
bool visual_script_save_version(VisualScript *script, const char *comment);
bool visual_script_load_version(VisualScript *script, u32 version_id);

// Collaboration features
bool visual_script_enable_collaboration(VisualScript *script, const char *server_url);
void visual_script_sync_changes(VisualScript *script);

// ============================================================================
// INTEGRATION API
// ============================================================================

// C/C++ integration
typedef void (*NativeFunction)(ExecutionContext *context, void **params, u32 param_count);

void visual_script_register_native_function(VisualScript *script, const char *name, NativeFunction func);
bool visual_script_call_native_function(VisualScript *script, const char *name, void **params, u32 param_count);

// Engine integration
void visual_script_set_ecs_world(VisualScript *script, World *world);
void visual_script_register_engine_apis(VisualScript *script);

// Asset integration
void visual_script_register_asset_types(VisualScript *script);
bool visual_script_load_asset(VisualScript *script, const char *asset_path);

#endif // VISUAL_SCRIPTING_H
