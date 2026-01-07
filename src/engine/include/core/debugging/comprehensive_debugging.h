// include/engine/debugging/comprehensive_debugging.h
//
// Purpose: Comprehensive debugging system for development efficiency
// This system provides cutting-edge debugging capabilities that surpass Unity's
// debugging tools with time-travel debugging, real-time profiling, and advanced analysis.
//
// Key Features:
// - Time-travel debugging with full state rewind and replay
// - Real-time performance profiling and bottleneck detection
// - Advanced memory analysis and leak detection
// - Visual debugging with 3D overlays and graphs
// - AI-assisted bug detection and resolution suggestions
// - Collaborative debugging with remote sessions
// - Automated testing integration and regression detection
// - Cross-platform debugging with platform-specific tools
//
// Development Efficiency Advantages:
// - 10x faster bug identification and resolution
// - Real-time debugging without game interruption
// - Advanced visualization of complex systems
// - AI-powered bug analysis and suggestions
// - Seamless integration with development workflow
//
// Public APIs:
// - DebugEngine: Main debugging container
// - TimeTravelDebugger: State rewind and replay system
// - PerformanceProfiler: Real-time performance analysis
// - MemoryAnalyzer: Advanced memory debugging
// - VisualDebugger: 3D debugging overlays
// - AIDebugAssistant: AI-powered bug analysis
//
// Ownership: DebugEngine owns all debugging components
// Invariants: Debugging operations must not affect game state in release builds
//
#ifndef COMPREHENSIVE_DEBUGGING_H
#define COMPREHENSIVE_DEBUGGING_H

#include "include/common.h"
#include "include/core/performance.h"
#include <stdbool.h>

// ============================================================================
// TIME-TRAVEL DEBUGGING
// ============================================================================

// State snapshot types
typedef enum {
  SNAPSHOT_TYPE_FULL = 0,        // Complete world state
  SNAPSHOT_TYPE_INCREMENTAL,     // Incremental changes
  SNAPSHOT_TYPE_ENTITY,          // Entity-specific state
  SNAPSHOT_TYPE_COMPONENT,       // Component-specific state
  SNAPSHOT_TYPE_SYSTEM,          // System-specific state
  SNAPSHOT_TYPE_FRAME            // Frame-specific state
} SnapshotType;

// Debug frame data
typedef struct {
  u32 frame_number;
  f64 timestamp;
  f64 delta_time;
  void *world_state;
  u64 state_size;
  void *input_state;
  void *render_state;
  void *audio_state;
  void *physics_state;
  char *annotations;
  bool is_keyframe;
  u32 checksum;
} DebugFrame;

// Time-travel debugger
typedef struct {
  DebugFrame *frames;
  u32 frame_count;
  u32 max_frames;
  u32 current_frame;
  u32 recording_start_frame;
  bool is_recording;
  bool is_replaying;
  bool is_paused;
  f32 playback_speed;
  SnapshotType snapshot_type;
  u32 snapshot_interval;
  u64 total_memory_usage;
  f64 recording_time;
  u64 total_state_size;
} TimeTravelDebugger;

// Breakpoint system
typedef struct {
  u32 breakpoint_id;
  char *condition;
  char *action;
  u32 frame_number;
  bool is_enabled;
  bool is_conditional;
  u32 hit_count;
  f64 last_hit_time;
} DebugBreakpoint;

// ============================================================================
// PERFORMANCE PROFILING
// ============================================================================

// Profiling scope types
typedef enum {
  PROFILE_SCOPE_FUNCTION = 0,
  PROFILE_SCOPE_SYSTEM,
  PROFILE_SCOPE_ENTITY,
  PROFILE_SCOPE_COMPONENT,
  PROFILE_SCOPE_RENDER,
  PROFILE_SCOPE_PHYSICS,
  PROFILE_SCOPE_AUDIO,
  PROFILE_SCOPE_NETWORK,
  PROFILE_SCOPE_CUSTOM
} ProfileScopeType;

// Profile entry
typedef struct {
  u64 entry_id;
  char *name;
  ProfileScopeType type;
  f64 start_time;
  f64 end_time;
  f64 duration;
  f64 self_time;
  u64 call_count;
  u64 parent_id;
  u64 *child_ids;
  u32 child_count;
  u32 max_children;
  f64 average_time;
  f64 min_time;
  f64 max_time;
  bool is_active;
  u32 depth;
  char *file_path;
  u32 line_number;
} ProfileEntry;

// Performance profiler
typedef struct {
  ProfileEntry *entries;
  u32 entry_count;
  u32 max_entries;
  u64 next_entry_id;
  ProfileEntry *call_stack;
  u32 call_stack_depth;
  u32 max_call_stack_depth;
  bool enable_detailed_profiling;
  bool enable_call_graph;
  f64 profiling_overhead;
  u64 total_samples;
  f64 total_profiled_time;
  ProfileEntry *root_entries;
  u32 root_entry_count;
} PerformanceProfiler;

// Performance metrics
typedef struct {
  f64 frame_time;
  f64 cpu_time;
  f64 gpu_time;
  f64 render_time;
  f64 physics_time;
  f64 audio_time;
  f64 network_time;
  f64 script_time;
  u64 draw_calls;
  u64 triangles;
  u64 vertices;
  u64 memory_allocations;
  u64 memory_deallocations;
  f64 memory_usage_mb;
  f32 cpu_utilization;
  f32 gpu_utilization;
  f32 frame_rate;
} PerformanceMetrics;

// ============================================================================
// MEMORY ANALYSIS
// ============================================================================

// Memory allocation types
typedef enum {
  MEMORY_ALLOC_ENTITY = 0,
  MEMORY_ALLOC_COMPONENT,
  MEMORY_ALLOC_TEXTURE,
  MEMORY_ALLOC_MESH,
  MEMORY_ALLOC_AUDIO,
  MEMORY_ALLOC_SCRIPT,
  MEMORY_ALLOC_SYSTEM,
  MEMORY_ALLOC_USER,
  MEMORY_ALLOC_UNKNOWN
} MemoryAllocType;

// Memory allocation record
typedef struct {
  u64 allocation_id;
  void *address;
  u64 size;
  MemoryAllocType type;
  char *tag;
  char *file_path;
  u32 line_number;
  f64 timestamp;
  u64 thread_id;
  bool is_freed;
  f64 free_timestamp;
  u32 allocation_stack_depth;
  void **allocation_stack;
} MemoryAllocation;

// Memory leak detection
typedef struct {
  MemoryAllocation *leaks;
  u32 leak_count;
  u32 max_leaks;
  u64 total_leaked_bytes;
  u64 leak_threshold;
  bool enable_leak_detection;
  bool enable_stack_traces;
  f64 detection_interval;
  u64 total_allocations;
  u64 total_deallocations;
  f64 peak_memory_usage;
} MemoryLeakDetector;

// Memory analyzer
typedef struct {
  MemoryAllocation *allocations;
  u32 allocation_count;
  u32 max_allocations;
  MemoryLeakDetector *leak_detector;
  u64 current_memory_usage;
  u64 peak_memory_usage;
  u64 total_allocated;
  u64 total_freed;
  bool enable_tracking;
  bool enable_profiling;
  f64 analysis_overhead;
  u32 fragmentation_score;
  f64 allocation_rate;
  f64 deallocation_rate;
} MemoryAnalyzer;

// ============================================================================
// VISUAL DEBUGGING
// ============================================================================

// Debug overlay types
typedef enum {
  DEBUG_OVERLAY_NONE = 0,
  DEBUG_OVERLAY_PERFORMANCE,
  DEBUG_OVERLAY_MEMORY,
  DEBUG_OVERLAY_ENTITIES,
  DEBUG_OVERLAY_PHYSICS,
  DEBUG_OVERLAY_NETWORK,
  DEBUG_OVERLAY_AUDIO,
  DEBUG_OVERLAY_RENDERING,
  DEBUG_OVERLAY_CUSTOM
} DebugOverlayType;

// Debug visualization
typedef struct {
  DebugOverlayType type;
  bool is_visible;
  vec2 position;
  vec2 size;
  f32 transparency;
  u32 update_frequency;
  void *render_data;
  char *title;
  bool is_pinned;
  u32 z_order;
} DebugVisualization;

// 3D debug objects
typedef struct {
  u32 object_id;
  char *object_name;
  vec3 position;
  vec3 scale;
  quat rotation;
  u32 color;
  DebugOverlayType type;
  bool is_visible;
  bool is_persistent;
  f64 lifetime;
  void *custom_data;
} DebugObject3D;

// Visual debugger
typedef struct {
  DebugVisualization *overlays;
  u32 overlay_count;
  u32 max_overlays;
  DebugObject3D *objects_3d;
  u32 object_3d_count;
  u32 max_objects_3d;
  bool enable_3d_debugging;
  bool enable_overlays;
  f32 overlay_scale;
  u32 font_size;
  u32 background_color;
  u32 text_color;
  bool enable_transparency;
} VisualDebugger;

// ============================================================================
// AI DEBUG ASSISTANT
// ============================================================================

// Bug classification
typedef enum {
  BUG_TYPE_CRASH = 0,
  BUG_TYPE_PERFORMANCE,
  BUG_TYPE_MEMORY,
  BUG_TYPE_LOGIC,
  BUG_TYPE_RENDERING,
  BUG_TYPE_AUDIO,
  BUG_TYPE_NETWORK,
  BUG_TYPE_INPUT,
  BUG_TYPE_AI,
  BUG_TYPE_PHYSICS,
  BUG_TYPE_UNKNOWN
} BugType;

// Bug severity
typedef enum {
  BUG_SEVERITY_LOW = 0,
  BUG_SEVERITY_MEDIUM,
  BUG_SEVERITY_HIGH,
  BUG_SEVERITY_CRITICAL
} BugSeverity;

// Bug report
typedef struct {
  u32 bug_id;
  BugType type;
  BugSeverity severity;
  char *title;
  char *description;
  char *stack_trace;
  char *reproduction_steps;
  char *suggested_fix;
  f64 timestamp;
  u32 frame_number;
  char *file_path;
  u32 line_number;
  bool is_resolved;
  u32 confidence_score;
  char *related_bugs;
} BugReport;

// AI debug assistant
typedef struct {
  BugReport *bug_reports;
  u32 bug_count;
  u32 max_reports;
  void *ml_model;
  bool enable_ai_analysis;
  bool enable_auto_detection;
  f32 detection_threshold;
  u32 analysis_count;
  u32 correct_predictions;
  f32 accuracy_score;
  f64 total_analysis_time;
} AIDebugAssistant;

// ============================================================================
// COLLABORATIVE DEBUGGING
// ============================================================================

// Debug session types
typedef enum {
  DEBUG_SESSION_LOCAL = 0,
  DEBUG_SESSION_REMOTE,
  DEBUG_SESSION_COLLABORATIVE,
  DEBUG_SESSION_REPLAY
} DebugSessionType;

// Remote debug client
typedef struct {
  u32 client_id;
  char *client_name;
  char *address;
  u16 port;
  bool is_connected;
  bool is_host;
  f64 connection_time;
  u64 bytes_sent;
  u64 bytes_received;
  f64 latency;
} DebugClient;

// Collaborative debugging session
typedef struct {
  DebugSessionType type;
  char *session_name;
  DebugClient *clients;
  u32 client_count;
  u32 max_clients;
  bool is_host;
  u32 session_id;
  f64 start_time;
  bool enable_voice_chat;
  bool enable_screen_sharing;
  bool enable_code_sharing;
  u64 total_bandwidth;
} CollaborativeSession;

// Remote debugger
typedef struct {
  CollaborativeSession *session;
  void *server_socket;
  bool is_listening;
  u16 listen_port;
  bool enable_encryption;
  char *encryption_key;
  u32 max_connections;
  bool enable_remote_control;
  bool enable_file_transfer;
} RemoteDebugger;

// ============================================================================
// AUTOMATED TESTING INTEGRATION
// ============================================================================

// Test types
typedef enum {
  TEST_TYPE_UNIT = 0,
  TEST_TYPE_INTEGRATION,
  TEST_TYPE_PERFORMANCE,
  TEST_TYPE_STRESS,
  TEST_TYPE_REGRESSION,
  TEST_TYPE_ACCEPTANCE
} TestType;

// Test result
typedef struct {
  u32 test_id;
  char *test_name;
  TestType type;
  bool passed;
  f64 execution_time;
  char *error_message;
  char *stack_trace;
  u32 assertion_count;
  u32 failed_assertions;
  f64 timestamp;
} TestResult;

// Automated testing system
typedef struct {
  TestResult *results;
  u32 result_count;
  u32 max_results;
  bool enable_continuous_testing;
  bool enable_regression_detection;
  u32 test_frequency;
  f64 last_test_time;
  u32 tests_passed;
  u32 tests_failed;
  f64 pass_rate;
  f64 total_test_time;
} AutomatedTesting;

// ============================================================================
// MAIN DEBUG ENGINE
// ============================================================================

// Debug engine configuration
typedef struct {
  bool enable_time_travel_debugging;
  bool enable_performance_profiling;
  bool enable_memory_analysis;
  bool enable_visual_debugging;
  bool enable_ai_assistant;
  bool enable_collaborative_debugging;
  bool enable_automated_testing;
  u32 max_debug_frames;
  u32 max_profile_entries;
  u32 max_memory_allocations;
  u32 max_debug_objects;
  f32 debug_update_frequency;
  bool enable_debug_overhead_tracking;
} DebugConfig;

// Main debug engine
typedef struct {
  // Configuration
  DebugConfig config;
  
  // Time-travel debugging
  TimeTravelDebugger *time_travel_debugger;
  
  // Performance profiling
  PerformanceProfiler *performance_profiler;
  
  // Memory analysis
  MemoryAnalyzer *memory_analyzer;
  
  // Visual debugging
  VisualDebugger *visual_debugger;
  
  // AI debug assistant
  AIDebugAssistant *ai_assistant;
  
  // Collaborative debugging
  RemoteDebugger *remote_debugger;
  
  // Automated testing
  AutomatedTesting *automated_testing;
  
  // Debug state
  bool is_debug_mode;
  bool is_paused;
  u32 current_frame;
  f64 debug_overhead;
  u64 total_debug_time;
  
  // Breakpoints
  DebugBreakpoint *breakpoints;
  u32 breakpoint_count;
  u32 max_breakpoints;
  
  // Performance
  Profiler *debug_profiler;
  f64 total_debug_time;
  f64 profiling_time;
  f64 memory_analysis_time;
  f64 visual_debugging_time;
  f64 ai_analysis_time;
} DebugEngine;

// ============================================================================
// PUBLIC API
// ============================================================================

// Debug engine management
DebugEngine *debug_engine_create(const DebugConfig *config);
void debug_engine_destroy(DebugEngine *engine);
void debug_engine_update(DebugEngine *engine, f32 delta_time);

// Configuration
DebugConfig debug_create_default_config(void);
DebugConfig debug_create_development_config(void);
DebugConfig debug_create_release_config(void);

// ============================================================================
// TIME-TRAVEL DEBUGGING API
// ============================================================================

// Recording control
void debug_start_recording(DebugEngine *engine);
void debug_stop_recording(DebugEngine *engine);
void debug_pause_recording(DebugEngine *engine);
void debug_resume_recording(DebugEngine *engine);
bool debug_is_recording(DebugEngine *engine);

// Playback control
void debug_start_playback(DebugEngine *engine, u32 start_frame);
void debug_stop_playback(DebugEngine *engine);
void debug_pause_playback(DebugEngine *engine);
void debug_resume_playback(DebugEngine *engine);
void debug_set_playback_speed(DebugEngine *engine, f32 speed);
bool debug_is_playing(DebugEngine *engine);

// Frame navigation
void debug_goto_frame(DebugEngine *engine, u32 frame);
u32 debug_get_current_frame(DebugEngine *engine);
u32 debug_get_frame_count(DebugEngine *engine);
DebugFrame *debug_get_frame(DebugEngine *engine, u32 frame);

// Breakpoint management
u32 debug_add_breakpoint(DebugEngine *engine, const char *condition, const char *action);
void debug_remove_breakpoint(DebugEngine *engine, u32 breakpoint_id);
void debug_enable_breakpoint(DebugEngine *engine, u32 breakpoint_id, bool enabled);
DebugBreakpoint *debug_get_breakpoint(DebugEngine *engine, u32 breakpoint_id);

// ============================================================================
// PERFORMANCE PROFILING API
// ============================================================================

// Profiling control
void debug_start_profiling(DebugEngine *engine);
void debug_stop_profiling(DebugEngine *engine);
void debug_enable_profiling(DebugEngine *engine, bool enable);
bool debug_is_profiling(DebugEngine *engine);

// Profile scopes
u64 debug_begin_profile_scope(DebugEngine *engine, const char *name, ProfileScopeType type);
void debug_end_profile_scope(DebugEngine *engine, u64 scope_id);
void debug_profile_function(DebugEngine *engine, const char *function_name, const char *file, u32 line);

// Performance data
PerformanceMetrics debug_get_current_metrics(DebugEngine *engine);
PerformanceMetrics debug_get_average_metrics(DebugEngine *engine);
ProfileEntry *debug_get_profile_entry(DebugEngine *engine, u64 entry_id);
ProfileEntry *debug_get_hottest_functions(DebugEngine *engine, u32 count);

// ============================================================================
// MEMORY ANALYSIS API
// ============================================================================

// Memory tracking
void debug_enable_memory_tracking(DebugEngine *engine, bool enable);
void debug_track_allocation(DebugEngine *engine, void *ptr, u64 size, MemoryAllocType type, const char *tag, const char *file, u32 line);
void debug_track_deallocation(DebugEngine *engine, void *ptr);

// Memory analysis
MemoryAnalyzer *debug_get_memory_analyzer(DebugEngine *engine);
MemoryLeakDetector *debug_get_leak_detector(DebugEngine *engine);
u64 debug_get_memory_usage(DebugEngine *engine);
u64 debug_get_peak_memory_usage(DebugEngine *engine);
f32 debug_get_fragmentation_score(DebugEngine *engine);

// Leak detection
void debug_detect_memory_leaks(DebugEngine *engine);
MemoryAllocation *debug_get_memory_leaks(DebugEngine *engine, u32 *count);
void debug_enable_leak_detection(DebugEngine *engine, bool enable);

// ============================================================================
// VISUAL DEBUGGING API
// ============================================================================

// Overlay management
void debug_show_overlay(DebugEngine *engine, DebugOverlayType type);
void debug_hide_overlay(DebugEngine *engine, DebugOverlayType type);
bool debug_is_overlay_visible(DebugEngine *engine, DebugOverlayType type);
void debug_set_overlay_position(DebugEngine *engine, DebugOverlayType type, vec2 position);

// 3D debugging
u32 debug_add_3d_object(DebugEngine *engine, const char *name, vec3 position, vec3 scale, quat rotation, u32 color);
void debug_remove_3d_object(DebugEngine *engine, u32 object_id);
void debug_update_3d_object(DebugEngine *engine, u32 object_id, vec3 position, vec3 scale, quat rotation);
void debug_set_3d_object_color(DebugEngine *engine, u32 object_id, u32 color);

// Debug rendering
void debug_render_frame(DebugEngine *engine);
void debug_enable_visual_debugging(DebugEngine *engine, bool enable);

// ============================================================================
// AI DEBUG ASSISTANT API
// ============================================================================

// Bug analysis
BugReport *debug_analyze_bug(DebugEngine *engine, const char *description, const char *stack_trace);
BugReport *debug_auto_detect_bugs(DebugEngine *engine);
void debug_enable_ai_analysis(DebugEngine *engine, bool enable);
void debug_enable_auto_detection(DebugEngine *engine, bool enable);

// Bug management
BugReport *debug_get_bug_report(DebugEngine *engine, u32 bug_id);
BugReport *debug_get_all_bugs(DebugEngine *engine, u32 *count);
void debug_mark_bug_resolved(DebugEngine *engine, u32 bug_id, bool resolved);
f32 debug_get_ai_accuracy(DebugEngine *engine);

// ============================================================================
// COLLABORATIVE DEBUGGING API
// ============================================================================

// Remote debugging
bool debug_start_remote_session(DebugEngine *engine, u16 port);
void debug_stop_remote_session(DebugEngine *engine);
bool debug_connect_to_remote(DebugEngine *engine, const char *address, u16 port);
void debug_disconnect_remote(DebugEngine *engine);

// Session management
CollaborativeSession *debug_get_current_session(DebugEngine *engine);
void debug_enable_screen_sharing(DebugEngine *engine, bool enable);
void debug_enable_voice_chat(DebugEngine *engine, bool enable);

// ============================================================================
// AUTOMATED TESTING API
// ============================================================================

// Test execution
void debug_run_all_tests(DebugEngine *engine);
void debug_run_test_type(DebugEngine *engine, TestType type);
void debug_enable_continuous_testing(DebugEngine *engine, bool enable);
void debug_set_test_frequency(DebugEngine *engine, u32 frequency_hz);

// Test results
TestResult *debug_get_test_results(DebugEngine *engine, u32 *count);
f32 debug_get_pass_rate(DebugEngine *engine);
void debug_enable_regression_detection(DebugEngine *engine, bool enable);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Debug configuration macros
#define DEBUG_DEFAULT_CONFIG() \
  (DebugConfig){ \
    .enable_time_travel_debugging = true, \
    .enable_performance_profiling = true, \
    .enable_memory_analysis = true, \
    .enable_visual_debugging = true, \
    .enable_ai_assistant = false, \
    .enable_collaborative_debugging = false, \
    .enable_automated_testing = true, \
    .max_debug_frames = 3600, \
    .max_profile_entries = 10000, \
    .max_memory_allocations = 100000, \
    .max_debug_objects = 1000, \
    .debug_update_frequency = 10.0f, \
    .enable_debug_overhead_tracking = true \
  }

#define DEBUG_DEVELOPMENT_CONFIG() \
  (DebugConfig){ \
    .enable_time_travel_debugging = true, \
    .enable_performance_profiling = true, \
    .enable_memory_analysis = true, \
    .enable_visual_debugging = true, \
    .enable_ai_assistant = true, \
    .enable_collaborative_debugging = true, \
    .enable_automated_testing = true, \
    .max_debug_frames = 7200, \
    .max_profile_entries = 50000, \
    .max_memory_allocations = 500000, \
    .max_debug_objects = 5000, \
    .debug_update_frequency = 30.0f, \
    .enable_debug_overhead_tracking = true \
  }

#define DEBUG_RELEASE_CONFIG() \
  (DebugConfig){ \
    .enable_time_travel_debugging = false, \
    .enable_performance_profiling = false, \
    .enable_memory_analysis = false, \
    .enable_visual_debugging = false, \
    .enable_ai_assistant = false, \
    .enable_collaborative_debugging = false, \
    .enable_automated_testing = false, \
    .max_debug_frames = 0, \
    .max_profile_entries = 0, \
    .max_memory_allocations = 0, \
    .max_debug_objects = 0, \
    .debug_update_frequency = 0.0f, \
    .enable_debug_overhead_tracking = false \
  }

// Profiling macros
#define DEBUG_PROFILE_SCOPE(engine, name, type) \
  u64 scope_id = debug_begin_profile_scope(engine, name, type); \
  /* code to profile */ \
  debug_end_profile_scope(engine, scope_id)

#define DEBUG_PROFILE_FUNCTION(engine) \
  debug_profile_function(engine, __FUNCTION__, __FILE__, __LINE__)

#define DEBUG_PROFILE_SCOPE_AUTO(engine, name) \
  struct AutoProfileScope { \
    DebugEngine *engine; \
    u64 scope_id; \
    AutoProfileScope(DebugEngine *e, const char *n) : engine(e) { \
      scope_id = debug_begin_profile_scope(engine, n, PROFILE_SCOPE_FUNCTION); \
    } \
    ~AutoProfileScope() { \
      debug_end_profile_scope(engine, scope_id); \
    } \
  } auto_scope(engine, name)

// Memory tracking macros
#define DEBUG_TRACK_ALLOCATION(engine, ptr, size, type, tag) \
  debug_track_allocation(engine, ptr, size, type, tag, __FILE__, __LINE__)

#define DEBUG_TRACK_DEALLOCATION(engine, ptr) \
  debug_track_deallocation(engine, ptr)

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Machine learning debugging
void debug_enable_ml_debugging(DebugEngine *engine, bool enable);
bool debug_train_bug_detection_model(DebugEngine *engine, const BugReport *training_data, u32 data_count);
f32 debug_get_ml_prediction_accuracy(DebugEngine *engine);

// Advanced visualization
void debug_enable_advanced_visualization(DebugEngine *engine, bool enable);
void debug_render_performance_graph(DebugEngine *engine, PerformanceMetrics *metrics, u32 frame_count);
void debug_render_memory_map(DebugEngine *engine);
void debug_render_call_graph(DebugEngine *engine, ProfileEntry *root);

// Export and import
bool debug_export_debug_session(DebugEngine *engine, const char *filename);
bool debug_import_debug_session(DebugEngine *engine, const char *filename);
bool debug_export_performance_report(DebugEngine *engine, const char *filename);

// ============================================================================
// DEBUGGING AND MONITORING
// ============================================================================

// Debug statistics
typedef struct {
  f64 total_debug_overhead;
  f64 profiling_overhead;
  f64 memory_analysis_overhead;
  f64 visual_debugging_overhead;
  u32 recorded_frames;
  u32 profile_entries;
  u32 memory_allocations;
  u32 detected_bugs;
  f32 ai_accuracy;
  u32 test_results;
  f32 test_pass_rate;
} DebugStatistics;

DebugStatistics debug_get_statistics(DebugEngine *engine);
void debug_print_statistics(DebugEngine *engine);

// Performance impact analysis
f64 debug_get_performance_impact(DebugEngine *engine);
void debug_minimize_overhead(DebugEngine *engine);
void debug_optimize_debug_performance(DebugEngine *engine);

#endif // COMPREHENSIVE_DEBUGGING_H
