#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <core/logger/unified_logger.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ERROR CODES - Standardized across all systems
 * ============================================================================
 */

typedef enum {
  // Success codes (0-99)
  ERROR_NONE = 0,
  ERROR_SUCCESS = 0,
  ERROR_OK = 0,

  // General errors (100-199)
  ERROR_INVALID_PARAMETER = 100,
  ERROR_NULL_POINTER = 101,
  ERROR_OUT_OF_MEMORY = 102,
  ERROR_BUFFER_OVERFLOW = 103,
  ERROR_BUFFER_UNDERFLOW = 104,
  ERROR_INVALID_OPERATION = 105,
  ERROR_OPERATION_FAILED = 106,
  ERROR_TIMEOUT = 107,
  ERROR_NOT_FOUND = 108,
  ERROR_ALREADY_EXISTS = 109,
  ERROR_ACCESS_DENIED = 110,
  ERROR_PERMISSION_DENIED = 111,
  ERROR_RESOURCE_BUSY = 112,
  ERROR_RESOURCE_EXHAUSTED = 113,

  // System errors (200-299)
  ERROR_FILE_NOT_FOUND = 200,
  ERROR_FILE_ACCESS_DENIED = 201,
  ERROR_FILE_ALREADY_EXISTS = 202,
  ERROR_DIRECTORY_NOT_FOUND = 203,
  ERROR_PATH_TOO_LONG = 204,
  ERROR_DISK_FULL = 205,
  ERROR_IO_ERROR = 206,
  ERROR_NETWORK_ERROR = 207,
  ERROR_NETWORK_UNREACHABLE = 208,
  ERROR_CONNECTION_FAILED = 209,
  ERROR_CONNECTION_TIMEOUT = 210,
  ERROR_CONNECTION_LOST = 211,

  // Graphics/Rendering errors (300-399)
  ERROR_OPENGL_INIT_FAILED = 300,
  ERROR_VULKAN_INIT_FAILED = 301,
  ERROR_SHADER_COMPILATION_FAILED = 302,
  ERROR_SHADER_LINK_FAILED = 303,
  ERROR_TEXTURE_LOAD_FAILED = 304,
  ERROR_MESH_LOAD_FAILED = 305,
  ERROR_BUFFER_CREATION_FAILED = 306,
  ERROR_PIPELINE_CREATION_FAILED = 307,
  ERROR_SWAPCHAIN_CREATION_FAILED = 308,
  ERROR_DEVICE_LOST = 309,
  ERROR_OUT_OF_VIDEO_MEMORY = 310,

  // Audio errors (400-499)
  ERROR_AUDIO_INIT_FAILED = 400,
  ERROR_AUDIO_DEVICE_NOT_FOUND = 401,
  ERROR_AUDIO_FORMAT_NOT_SUPPORTED = 402,
  ERROR_AUDIO_BUFFER_UNDERFLOW = 403,
  ERROR_AUDIO_BUFFER_OVERFLOW = 404,
  ERROR_AUDIO_STREAM_FAILED = 405,

  // Physics errors (500-599)
  ERROR_PHYSICS_INIT_FAILED = 500,
  ERROR_COLLISION_SHAPE_INVALID = 501,
  ERROR_RIGID_BODY_CREATION_FAILED = 502,
  ERROR_CONSTRAINT_CREATION_FAILED = 503,
  ERROR_SIMULATION_FAILED = 504,

  // AI errors (600-699)
  ERROR_AI_INIT_FAILED = 600,
  ERROR_BEHAVIOR_TREE_INVALID = 601,
  ERROR_GOAP_PLANNING_FAILED = 602,
  ERROR_PATHFINDING_FAILED = 603,
  ERROR_NAVMESH_LOAD_FAILED = 604,

  // Animation errors (700-799)
  ERROR_ANIMATION_LOAD_FAILED = 700,
  ERROR_SKELETON_INVALID = 701,
  ERROR_ANIMATION_BLEND_FAILED = 702,
  ERROR_IK_SOLVER_FAILED = 703,

  // Asset/Resource errors (800-899)
  ERROR_ASSET_LOAD_FAILED = 800,
  ERROR_ASSET_NOT_FOUND = 801,
  ERROR_ASSET_CORRUPTED = 802,
  ERROR_ASSET_VERSION_MISMATCH = 803,
  ERROR_RESOURCE_NOT_FOUND = 804,
  ERROR_RESOURCE_LOAD_FAILED = 805,

  // Threading errors (900-999)
  ERROR_THREAD_CREATION_FAILED = 900,
  ERROR_MUTEX_INIT_FAILED = 901,
  ERROR_CONDITION_VARIABLE_FAILED = 902,
  ERROR_THREAD_JOIN_FAILED = 903,
  ERROR_DEADLOCK_DETECTED = 904,

  // Custom errors (1000+)
  ERROR_CUSTOM_BASE = 1000
} error_code_t;

/* ============================================================================
 * ERROR CONTEXT STRUCTURE
 * ============================================================================
 */

typedef struct {
  error_code_t code;
  const char *message;
  const char *file;
  const char *function;
  int line;
  uint64_t timestamp;
  uint32_t thread_id;
  void *user_data;
  struct error_context_t *cause; // For error chaining
} error_context_t;

/* ============================================================================
 * ERROR HANDLING FUNCTIONS
 * ============================================================================
 */

// Core error handling
void error_init(void);
void error_shutdown(void);
void error_set_context(error_context_t *context);
error_context_t *error_get_context(void);
void error_clear_context(void);

// Error creation and reporting
error_context_t *error_create(error_code_t code, const char *message,
                              const char *file, const char *function, int line);
void error_report(error_context_t *error);
void error_report_with_cause(error_code_t code, const char *message,
                             const char *file, const char *function, int line,
                             error_context_t *cause);

// Error utilities
const char *error_code_string(error_code_t code);
const char *error_category_string(error_code_t code);
bool error_is_fatal(error_code_t code);
bool error_is_recoverable(error_code_t code);
bool error_is_network_related(error_code_t code);
bool error_is_file_related(error_code_t code);

// Error handling strategies
typedef enum {
  ERROR_STRATEGY_IGNORE,
  ERROR_STRATEGY_LOG_ONLY,
  ERROR_STRATEGY_LOG_AND_CONTINUE,
  ERROR_STRATEGY_LOG_AND_RETURN,
  ERROR_STRATEGY_LOG_AND_ABORT,
  ERROR_STRATEGY_CUSTOM
} error_strategy_t;

void error_set_strategy(error_code_t code, error_strategy_t strategy);
error_strategy_t error_get_strategy(error_code_t code);
void error_execute_strategy(error_context_t *error);

/* ============================================================================
 * CONVENIENCE MACROS - Standardized error reporting
 * ============================================================================
 */

// Basic error reporting
#define REPORT_ERROR(code, message)                                            \
  error_report(error_create(code, message, __FILE__, __FUNCTION__, __LINE__))

#define REPORT_ERROR_WITH_CAUSE(code, message, cause)                          \
  error_report_with_cause(code, message, __FILE__, __FUNCTION__, __LINE__,     \
                          cause)

// Conditional error reporting
#define REPORT_ERROR_IF(condition, code, message)                              \
  do {                                                                         \
    if (condition)                                                             \
      REPORT_ERROR(code, message);                                             \
  } while (0)

#define REPORT_ERROR_IF_NULL(ptr, message)                                     \
  REPORT_ERROR_IF((ptr) == NULL, ERROR_NULL_POINTER, message)

#define REPORT_ERROR_IF_INVALID(condition, message)                            \
  REPORT_ERROR_IF(condition, ERROR_INVALID_PARAMETER, message)

// Return on error pattern
#define RETURN_ERROR_IF(condition, code, message)                              \
  do {                                                                         \
    if (condition) {                                                           \
      REPORT_ERROR(code, message);                                             \
      return code;                                                             \
    }                                                                          \
  } while (0)

#define RETURN_ERROR_IF_NULL(ptr, message)                                     \
  RETURN_ERROR_IF((ptr) == NULL, ERROR_NULL_POINTER, message)

#define RETURN_ERROR_IF_INVALID(condition, message)                            \
  RETURN_ERROR_IF(condition, ERROR_INVALID_PARAMETER, message)

// Success/error return helpers
#define RETURN_SUCCESS() return ERROR_NONE
#define RETURN_ON_ERROR(expr)                                                  \
  do {                                                                         \
    error_code_t _err = (expr);                                                \
    if (_err != ERROR_NONE)                                                    \
      return _err;                                                             \
  } while (0)

// Validation macros
#define VALIDATE_POINTER(ptr)                                                  \
  RETURN_ERROR_IF_NULL(ptr, "Pointer cannot be null")

#define VALIDATE_PARAMETER(condition)                                          \
  RETURN_ERROR_IF(!(condition), ERROR_INVALID_PARAMETER, "Invalid parameter")

#define VALIDATE_RANGE(value, min, max)                                        \
  RETURN_ERROR_IF((value) < (min) || (value) > (max), ERROR_INVALID_PARAMETER, \
                  "Value out of range")

/* ============================================================================
 * SYSTEM-SPECIFIC ERROR MACROS
 * ============================================================================
 */

// Graphics errors
#define REPORT_GRAPHICS_ERROR(code, message) REPORT_ERROR(code, message)

#define RETURN_GRAPHICS_ERROR_IF(condition, code, message)                     \
  RETURN_ERROR_IF(condition, code, message)

// Audio errors
#define REPORT_AUDIO_ERROR(code, message) REPORT_ERROR(code, message)

#define RETURN_AUDIO_ERROR_IF(condition, code, message)                        \
  RETURN_ERROR_IF(condition, code, message)

// Physics errors
#define REPORT_PHYSICS_ERROR(code, message) REPORT_ERROR(code, message)

#define RETURN_PHYSICS_ERROR_IF(condition, code, message)                      \
  RETURN_ERROR_IF(condition, code, message)

// AI errors
#define REPORT_AI_ERROR(code, message) REPORT_ERROR(code, message)

#define RETURN_AI_ERROR_IF(condition, code, message)                           \
  RETURN_ERROR_IF(condition, code, message)

/* ============================================================================
 * ERROR RECOVERY SYSTEM
 * ============================================================================
 */

typedef struct {
  error_code_t code;
  error_code_t (*recover_func)(error_context_t *error);
  const char *description;
} error_recovery_t;

void error_register_recovery(error_code_t code,
                             error_code_t (*recover_func)(error_context_t *),
                             const char *description);
error_code_t error_attempt_recovery(error_context_t *error);
void error_unregister_recovery(error_code_t code);

/* ============================================================================
 * ERROR STATISTICS AND MONITORING
 * ============================================================================
 */

typedef struct {
  uint64_t total_errors;
  uint64_t errors_per_category[10]; // Grouped by error type ranges
  uint64_t fatal_errors;
  uint64_t recovered_errors;
  uint64_t unrecovered_errors;
  error_code_t most_common_error;
  uint64_t most_common_count;
  float average_recovery_time_ms;
} error_stats_t;

void error_get_stats(error_stats_t *stats);
void error_reset_stats(void);
void error_dump_stats(void);

/* ============================================================================
 * COMPATIBILITY LAYER - For existing code migration
 * ============================================================================
 */

// Legacy compatibility for common patterns
#ifdef log_error
#undef log_error
#endif
#define log_error(message) REPORT_ERROR(ERROR_OPERATION_FAILED, message)
#define log_error_code(code, message) REPORT_ERROR(code, message)

// For systems that still use old error handling
#define HANDLE_ERROR(code)                                                     \
  do {                                                                         \
    if (code != ERROR_NONE) {                                                  \
      REPORT_ERROR(code, "Operation failed");                                  \
    }                                                                          \
  } while (0)

/* ============================================================================
 * DEBUG ERROR HANDLING
 * ============================================================================
 */

#ifdef DEBUG_BUILD
#define DEBUG_REPORT_ERROR(code, message) REPORT_ERROR(code, message)
#define DEBUG_RETURN_ERROR_IF(condition, code, message)                        \
  RETURN_ERROR_IF(condition, code, message)
#else
#define DEBUG_REPORT_ERROR(code, message) ((void)0)
#define DEBUG_RETURN_ERROR_IF(condition, code, message) ((void)0)
#endif

/* ============================================================================
 * ERROR BOUNDARY SUPPORT
 * ============================================================================
 */

typedef struct error_boundary {
  error_context_t *last_error;
  uint32_t error_count;
  bool enabled;
  void (*error_handler)(error_context_t *error, void *user_data);
  void *user_data;
} error_boundary_t;

error_boundary_t *error_boundary_create(void);
void error_boundary_destroy(error_boundary_t *boundary);
void error_boundary_enter(error_boundary_t *boundary);
void error_boundary_exit(error_boundary_t *boundary);
error_context_t *error_boundary_get_last_error(error_boundary_t *boundary);
void error_boundary_set_handler(error_boundary_t *boundary,
                                void (*handler)(error_context_t *, void *),
                                void *user_data);

#define ERROR_BOUNDARY(boundary)                                               \
  error_boundary_enter(boundary);                                              \
  if (error_boundary_get_last_error(boundary) == NULL)

#ifdef __cplusplus
}
#endif

#endif /* ERROR_HANDLING_H */
