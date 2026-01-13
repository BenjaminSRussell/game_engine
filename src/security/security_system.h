/*
 * security_system.h
 * Comprehensive Input Validation and Security System Header
 *
 * Advanced 3D Rendering Engine - Security Module
 */

#ifndef SECURITY_SYSTEM_H
#define SECURITY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SECURITY_VERSION_MAJOR 1
#define SECURITY_VERSION_MINOR 0
#define SECURITY_VERSION_PATCH 0

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum {
    SECURITY_THREAT_NONE = 0,
    SECURITY_THREAT_BUFFER_OVERFLOW = 1 << 0,
    SECURITY_THREAT_SQL_INJECTION = 1 << 1,
    SECURITY_THREAT_COMMAND_INJECTION = 1 << 2,
    SECURITY_THREAT_XSS = 1 << 3,
    SECURITY_THREAT_PATH_TRAVERSAL = 1 << 4,
    SECURITY_THREAT_FORMAT_STRING = 1 << 5,
    SECURITY_THREAT_INTEGER_OVERFLOW = 1 << 6,
    SECURITY_THREAT_INVALID_INPUT = 1 << 7,
    SECURITY_THREAT_RACE_CONDITION = 1 << 8,
    SECURITY_THREAT_MEMORY_CORRUPTION = 1 << 9
} security_threat_type_t;

typedef enum {
    SECURITY_SEVERITY_LOW = 1,
    SECURITY_SEVERITY_MEDIUM = 2,
    SECURITY_SEVERITY_HIGH = 3,
    SECURITY_SEVERITY_CRITICAL = 4
} security_severity_t;

typedef enum {
    THREAT_LEVEL_NONE = 0,
    THREAT_LEVEL_LOW = 1,
    THREAT_LEVEL_MEDIUM = 2,
    THREAT_LEVEL_HIGH = 3,
    THREAT_LEVEL_CRITICAL = 4
} threat_level_t;

/* ============================================================================
 * STRUCTS
 * ============================================================================ */

typedef struct {
    uint32_t rules;
    size_t max_length;
    security_severity_t severity;
} security_validation_config_t;

typedef struct {
    char input[4096];
    char sanitized[4096];
    uint32_t applied_rules;
    security_threat_type_t detected_threats;
    bool passed;
    uint64_t timestamp;
} security_validation_result_t;

typedef struct {
    uint64_t total_validations;
    uint64_t blocked_attempts;
    uint32_t log_entries;
    uint32_t blocked_patterns;
} security_stats_t;

typedef struct {
    uint32_t player_id;
    char username[64];
    threat_level_t current_threat_level;
    uint32_t violation_count;
    uint64_t last_violation_time;
    bool is_banned;
    uint64_t ban_expiry;
} player_security_profile_t;

typedef struct {
    uint32_t violation_id;
    uint32_t player_id;
    security_threat_type_t threat_type;
    uint64_t timestamp;
    char description[256];
    uint32_t severity;
} security_violation_t;

/* Validation rule flags (bitmask) */
#define SECURITY_RULE_NONE 0
#define SECURITY_RULE_NO_SQL_INJECTION (1 << 0)
#define SECURITY_RULE_NO_COMMAND_INJECTION (1 << 1)
#define SECURITY_RULE_NO_XSS (1 << 2)
#define SECURITY_RULE_NO_PATH_TRAVERSAL (1 << 3)
#define SECURITY_RULE_NO_FORMAT_STRING (1 << 4)
#define SECURITY_RULE_LENGTH_LIMIT (1 << 5)
#define SECURITY_RULE_ALPHANUMERIC_ONLY (1 << 6)
#define SECURITY_RULE_FILENAME_SAFE (1 << 7)
#define SECURITY_RULE_NO_INTEGER_OVERFLOW (1 << 8)

/* ============================================================================
 * PUBLIC API - System Management
 * ============================================================================ */

/**
 * Initialize the security system
 * @return 0 on success, negative error code on failure
 */
int security_system_init(void);

/**
 * Shutdown the security system and cleanup resources
 */
void security_system_shutdown(void);

/* ============================================================================
 * PUBLIC API - Input Validation
 * ============================================================================ */

/**
 * Validate input string against security rules
 * @param input Input string to validate
 * @param config Validation configuration
 * @param result Validation result output
 * @return true if validation passed, false otherwise
 */
bool security_validate_input(const char* input, 
                           const security_validation_config_t* config,
                           security_validation_result_t* result);

/**
 * Simple string validation with default security rules
 * @param input Input string to validate
 * @param max_length Maximum allowed length
 * @return true if validation passed, false otherwise
 */
bool security_validate_string_safe(const char* input, size_t max_length);

/**
 * Validate filename for safety
 * @param filename Filename to validate
 * @return true if filename is safe, false otherwise
 */
bool security_validate_filename(const char* filename);

/**
 * Validate network data packet
 * @param data Pointer to network data
 * @param size Size of data
 * @param max_size Maximum allowed size
 * @return true if data is valid, false otherwise
 */
bool security_validate_network_data(const void* data, size_t size, size_t max_size);

/* ============================================================================
 * PUBLIC API - Safe String Operations
 * ============================================================================ */

/**
 * Safe string copy with buffer overflow protection
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return true if copy succeeded, false otherwise
 */
bool security_safe_string_copy(char* dest, const char* src, size_t dest_size);

/**
 * Safe string concatenation with buffer overflow protection
 * @param dest Destination buffer
 * @param src Source string to append
 * @param dest_size Size of destination buffer
 * @return true if concatenation succeeded, false otherwise
 */
bool security_safe_string_concat(char* dest, const char* src, size_t dest_size);

/**
 * Safe sprintf with buffer overflow protection
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param format Format string
 * @param ... Format arguments
 * @return Number of characters written on success, -1 on error
 */
int security_sprintf_safe(char* dest, size_t dest_size, const char* format, ...);

/* ============================================================================
 * PUBLIC API - Numeric Validation
 * ============================================================================ */

/**
 * Validate integer value is within range
 * @param value Integer value to validate
 * @param min_val Minimum allowed value
 * @param max_val Maximum allowed value
 * @return true if value is in range, false otherwise
 */
bool security_validate_integer_range(int value, int min_val, int max_val);

/**
 * Validate size_t value is within maximum
 * @param value Size_t value to validate
 * @param max_val Maximum allowed value
 * @return true if value is valid, false otherwise
 */
bool security_validate_size_t_range(size_t value, size_t max_val);

/* ============================================================================
 * PUBLIC API - Player Security Management
 * ============================================================================ */

/**
 * Initialize player security profile
 * @param player_id Player ID
 * @param username Player username
 * @return Player security profile handle, or NULL on failure
 */
player_security_profile_t* security_player_profile_init(uint32_t player_id, const char* username);

/**
 * Get player security profile
 * @param player_id Player ID
 * @return Player security profile, or NULL if not found
 */
player_security_profile_t* security_player_profile_get(uint32_t player_id);

/**
 * Log security violation for player
 * @param player_id Player ID
 * @param threat_type Type of threat
 * @param description Violation description
 * @param severity Violation severity
 * @return Violation ID, or 0 on failure
 */
uint32_t security_log_violation(uint32_t player_id, 
                               security_threat_type_t threat_type,
                               const char* description, 
                               uint32_t severity);

/**
 * Ban player for security violations
 * @param player_id Player ID
 * @param duration_seconds Ban duration in seconds (0 for permanent)
 * @return true if ban succeeded, false otherwise
 */
bool security_ban_player(uint32_t player_id, uint64_t duration_seconds);

/**
 * Unban player
 * @param player_id Player ID
 * @return true if unban succeeded, false otherwise
 */
bool security_unban_player(uint32_t player_id);

/**
 * Check if player is banned
 * @param player_id Player ID
 * @return true if player is banned, false otherwise
 */
bool security_is_player_banned(uint32_t player_id);

/* ============================================================================
 * PUBLIC API - Statistics and Logging
 * ============================================================================ */

/**
 * Get security system statistics
 * @param stats Output statistics structure
 */
void security_get_statistics(security_stats_t* stats);

/**
 * Clear security logs
 */
void security_clear_logs(void);

/**
 * Get recent security violations
 * @param violations Output array for violations
 * @param max_count Maximum number of violations to retrieve
 * @return Number of violations retrieved
 */
uint32_t security_get_recent_violations(security_violation_t* violations, uint32_t max_count);

/* ============================================================================
 * PUBLIC API - Configuration
 * ============================================================================ */

/**
 * Set global security level
 * @param severity Minimum severity level for blocking
 */
void security_set_global_level(security_severity_t severity);

/**
 * Enable/disable specific validation rules globally
 * @param rules Validation rules bitmask
 * @param enabled Whether to enable the rules
 */
void security_set_global_rules(uint32_t rules, bool enabled);

/**
 * Add custom threat pattern
 * @param pattern Pattern string to block
 * @param threat_type Associated threat type
 * @param severity Pattern severity
 * @return true if pattern added successfully, false otherwise
 */
bool security_add_threat_pattern(const char* pattern, 
                                security_threat_type_t threat_type, 
                                uint32_t severity);

/**
 * Remove threat pattern
 * @param pattern Pattern string to remove
 * @return true if pattern removed successfully, false otherwise
 */
bool security_remove_threat_pattern(const char* pattern);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_SYSTEM_H */
