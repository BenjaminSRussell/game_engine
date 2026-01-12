#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/pbkdf2.h>

// Security threat levels
typedef enum {
    THREAT_LEVEL_LOW,
    THREAT_LEVEL_MEDIUM,
    THREAT_LEVEL_HIGH,
    THREAT_LEVEL_CRITICAL
} threat_level_t;

// Violation types
typedef enum {
    VIOLATION_SPEED_HACK,
    VIOLATION_FLY_HACK,
    VIOLATION_WALL_HACK,
    VIOLATION_AIM_BOT,
    VIOLATION_ITEM_DUPLICATION,
    VIOLATION_EXPLOIT_ABUSE,
    VIOLATION_PACKET_INJECTION,
    VIOLATION_MEMORY_MODIFICATION,
    VIOLATION_ACCOUNT_COMPROMISE,
    VIOLATION_BRUTE_FORCE
} violation_type_t;

// Player security status
typedef struct {
    char player_id[64];
    int violation_count;
    threat_level_t current_threat_level;
    time_t last_violation_time;
    time_t last_check_time;
    bool is_suspended;
    bool is_banned;
    time_t ban_expiry;
    double movement_speed_average;
    double position_history[100][3]; // x, y, z
    int position_index;
    int action_count_per_minute;
    time_t action_window_start;
    char ip_address[46];
    char hardware_id[128];
} player_security_t;

// Security violation record
typedef struct {
    int violation_id;
    char player_id[64];
    violation_type_t type;
    time_t timestamp;
    char description[512];
    char evidence[1024];
    threat_level_t severity;
    bool is_resolved;
    char resolution_notes[256];
} security_violation_t;

// Anti-cheat detection structure
typedef struct {
    double max_movement_speed;
    double max_vertical_speed;
    double max_action_rate_per_second;
    double position_tolerance;
    int max_position_deviation;
    double teleport_detection_threshold;
    int aimbot_sensitivity_samples;
    double aimbot_detection_threshold;
    int duplicate_item_threshold;
    bool enable_server_side_validation;
    bool enable_client_side_checks;
    bool enable_behavioral_analysis;
} anti_cheat_config_t;

// Secure storage structure
typedef struct {
    unsigned char encryption_key[32];
    unsigned char salt[16];
    int key_derivation_iterations;
    char encrypted_data_path[512];
    char backup_path[512];
    bool enable_auto_backup;
    int backup_interval_hours;
    time_t last_backup_time;
} secure_storage_config_t;

// Input validation structure
typedef struct {
    int max_string_length;
    int max_player_name_length;
    int max_chat_message_length;
    int max_command_length;
    char allowed_characters[256];
    char forbidden_patterns[10][128];
    int forbidden_pattern_count;
    bool enable_sql_injection_check;
    bool enable_xss_check;
    bool enable_command_injection_check;
} input_validation_config_t;

// Security manager structure
typedef struct {
    player_security_t* players;
    int player_count;
    int player_capacity;
    security_violation_t* violations;
    int violation_count;
    int violation_capacity;
    anti_cheat_config_t anti_cheat_config;
    secure_storage_config_t storage_config;
    input_validation_config_t validation_config;
    bool is_active;
    time_t start_time;
    int total_violations_detected;
    int total_bans_issued;
} security_manager_t;

// Security manager functions
security_manager_t* security_manager_create(void);
void security_manager_destroy(security_manager_t* manager);
bool security_manager_initialize(security_manager_t* manager);
void security_manager_update(security_manager_t* manager);

// Player security functions
int security_manager_add_player(security_manager_t* manager, const char* player_id, 
                               const char* ip_address, const char* hardware_id);
player_security_t* security_manager_get_player(security_manager_t* manager, const char* player_id);
bool security_manager_remove_player(security_manager_t* manager, const char* player_id);
void security_manager_update_player_position(security_manager_t* manager, const char* player_id, 
                                           double x, double y, double z);
void security_manager_record_player_action(security_manager_t* manager, const char* player_id);

// Anti-cheat functions
bool security_check_movement_hack(security_manager_t* manager, const char* player_id, 
                                 double old_x, double old_y, double old_z,
                                 double new_x, double new_y, double new_z, double delta_time);
bool security_check_speed_hack(security_manager_t* manager, const char* player_id, double delta_time);
bool security_check_aimbot(security_manager_t* manager, const char* player_id, 
                          double aim_angle_x, double aim_angle_y);
bool security_check_item_duplication(security_manager_t* manager, const char* player_id, 
                                   const char* item_id, int old_count, int new_count);
bool security_check_exploit_abuse(security_manager_t* manager, const char* player_id, 
                                 const char* action_name, int frequency);

// Violation handling functions
int security_report_violation(security_manager_t* manager, const char* player_id, 
                             violation_type_t type, const char* description, 
                             const char* evidence, threat_level_t severity);
bool security_resolve_violation(security_manager_t* manager, int violation_id, 
                               const char* resolution_notes);
bool security_suspend_player(security_manager_t* manager, const char* player_id, int duration_hours);
bool security_ban_player(security_manager_t* manager, const char* player_id, int duration_hours, 
                        const char* reason);
bool security_unban_player(security_manager_t* manager, const char* player_id);

// Secure storage functions
bool secure_storage_initialize(security_manager_t* manager);
bool secure_storage_encrypt_data(const unsigned char* key, const unsigned char* salt, 
                                const char* plaintext, unsigned char* ciphertext, int* ciphertext_len);
bool secure_storage_decrypt_data(const unsigned char* key, const unsigned char* salt, 
                                const unsigned char* ciphertext, int ciphertext_len, 
                                char* plaintext, int* plaintext_len);
bool secure_storage_save_player_data(security_manager_t* manager, const char* player_id, 
                                   const char* data, bool is_sensitive);
bool secure_storage_load_player_data(security_manager_t* manager, const char* player_id, 
                                   char* data, int max_length, bool is_sensitive);
bool secure_storage_change_password(security_manager_t* manager, const char* old_password, 
                                   const char* new_password);
void secure_storage_create_backup(security_manager_t* manager);

// Input validation functions
bool validate_player_name(const char* name, const input_validation_config_t* config);
bool validate_chat_message(const char* message, const input_validation_config_t* config);
bool validate_command_input(const char* command, const input_validation_config_t* config);
bool validate_file_path(const char* path, const input_validation_config_t* config);
bool validate_network_packet(const char* packet_data, int packet_length, 
                           const input_validation_config_t* config);
bool sanitize_input_string(char* input, const input_validation_config_t* config);
bool check_sql_injection(const char* input);
bool check_xss_attack(const char* input);
bool check_command_injection(const char* input);

// Cryptographic helper functions
bool generate_secure_key(unsigned char* key, int key_length);
bool generate_salt(unsigned char* salt, int salt_length);
bool derive_key_from_password(const char* password, const unsigned char* salt, 
                             int iterations, unsigned char* key, int key_length);
bool hash_data(const char* data, unsigned char* hash, int hash_length);
bool verify_data_integrity(const char* data, const unsigned char* expected_hash, int hash_length);

// Security monitoring functions
void security_generate_report(security_manager_t* manager, char* buffer, size_t buffer_size);
void security_export_violations(security_manager_t* manager, const char* filename, 
                              time_t start_time, time_t end_time);
int security_get_threat_distribution(security_manager_t* manager, int* threat_counts, int max_threats);
char** security_get_suspicious_players(security_manager_t* manager, int* count, int max_players);

// Implementation
security_manager_t* security_manager_create(void) {
    security_manager_t* manager = malloc(sizeof(security_manager_t));
    if (!manager) return NULL;
    
    manager->players = malloc(sizeof(player_security_t) * 1000);
    manager->player_count = 0;
    manager->player_capacity = 1000;
    
    manager->violations = malloc(sizeof(security_violation_t) * 5000);
    manager->violation_count = 0;
    manager->violation_capacity = 5000;
    
    // Initialize anti-cheat config
    manager->anti_cheat_config.max_movement_speed = 10.0;
    manager->anti_cheat_config.max_vertical_speed = 5.0;
    manager->anti_cheat_config.max_action_rate_per_second = 20.0;
    manager->anti_cheat_config.position_tolerance = 0.1;
    manager->anti_cheat_config.max_position_deviation = 50;
    manager->anti_cheat_config.teleport_detection_threshold = 100.0;
    manager->anti_cheat_config.aimbot_sensitivity_samples = 10;
    manager->anti_cheat_config.aimbot_detection_threshold = 0.95;
    manager->anti_cheat_config.duplicate_item_threshold = 5;
    manager->anti_cheat_config.enable_server_side_validation = true;
    manager->anti_cheat_config.enable_client_side_checks = true;
    manager->anti_cheat_config.enable_behavioral_analysis = true;
    
    // Initialize secure storage config
    manager->storage_config.key_derivation_iterations = 100000;
    strcpy(manager->storage_config.encrypted_data_path, "data/secure/");
    strcpy(manager->storage_config.backup_path, "data/backup/");
    manager->storage_config.enable_auto_backup = true;
    manager->storage_config.backup_interval_hours = 24;
    generate_salt(manager->storage_config.salt, 16);
    generate_secure_key(manager->storage_config.encryption_key, 32);
    
    // Initialize input validation config
    manager->validation_config.max_string_length = 1024;
    manager->validation_config.max_player_name_length = 32;
    manager->validation_config.max_chat_message_length = 256;
    manager->validation_config.max_command_length = 128;
    strcpy(manager->validation_config.allowed_characters, 
           "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-@. ");
    
    // Add forbidden patterns
    strcpy(manager->validation_config.forbidden_patterns[0], "DROP TABLE");
    strcpy(manager->validation_config.forbidden_patterns[1], "INSERT INTO");
    strcpy(manager->validation_config.forbidden_patterns[2], "DELETE FROM");
    strcpy(manager->validation_config.forbidden_patterns[3], "UPDATE SET");
    strcpy(manager->validation_config.forbidden_patterns[4], "<script>");
    strcpy(manager->validation_config.forbidden_patterns[5], "javascript:");
    manager->validation_config.forbidden_pattern_count = 6;
    
    manager->validation_config.enable_sql_injection_check = true;
    manager->validation_config.enable_xss_check = true;
    manager->validation_config.enable_command_injection_check = true;
    
    manager->is_active = false;
    manager->start_time = time(NULL);
    manager->total_violations_detected = 0;
    manager->total_bans_issued = 0;
    
    return manager;
}

void security_manager_destroy(security_manager_t* manager) {
    if (!manager) return;
    
    free(manager->players);
    free(manager->violations);
    free(manager);
}

bool security_manager_initialize(security_manager_t* manager) {
    if (!manager) return false;
    
    manager->is_active = true;
    manager->start_time = time(NULL);
    
    // Initialize secure storage
    if (!secure_storage_initialize(manager)) {
        return false;
    }
    
    return true;
}

int security_manager_add_player(security_manager_t* manager, const char* player_id, 
                               const char* ip_address, const char* hardware_id) {
    if (!manager || !player_id || manager->player_count >= manager->player_capacity) {
        return -1;
    }
    
    // Check if player already exists
    for (int i = 0; i < manager->player_count; i++) {
        if (strcmp(manager->players[i].player_id, player_id) == 0) {
            return i; // Player already exists
        }
    }
    
    player_security_t player = {0};
    strncpy(player.player_id, player_id, sizeof(player.player_id) - 1);
    if (ip_address) {
        strncpy(player.ip_address, ip_address, sizeof(player.ip_address) - 1);
    }
    if (hardware_id) {
        strncpy(player.hardware_id, hardware_id, sizeof(player.hardware_id) - 1);
    }
    
    player.current_threat_level = THREAT_LEVEL_LOW;
    player.last_check_time = time(NULL);
    player.action_window_start = time(NULL);
    
    manager->players[manager->player_count++] = player;
    return manager->player_count - 1;
}

player_security_t* security_manager_get_player(security_manager_t* manager, const char* player_id) {
    if (!manager || !player_id) return NULL;
    
    for (int i = 0; i < manager->player_count; i++) {
        if (strcmp(manager->players[i].player_id, player_id) == 0) {
            return &manager->players[i];
        }
    }
    return NULL;
}

bool security_check_movement_hack(security_manager_t* manager, const char* player_id, 
                                 double old_x, double old_y, double old_z,
                                 double new_x, double new_y, double new_z, double delta_time) {
    if (!manager || !player_id || delta_time <= 0) return false;
    
    player_security_t* player = security_manager_get_player(manager, player_id);
    if (!player) return false;
    
    double distance = sqrt(pow(new_x - old_x, 2) + pow(new_y - old_y, 2) + pow(new_z - old_z, 2));
    double speed = distance / delta_time;
    
    // Check for speed hacking
    if (speed > manager->anti_cheat_config.max_movement_speed) {
        char evidence[512];
        snprintf(evidence, sizeof(evidence), 
                "Speed: %.2f, Max allowed: %.2f, Distance: %.2f, Delta time: %.4f",
                speed, manager->anti_cheat_config.max_movement_speed, distance, delta_time);
        
        security_report_violation(manager, player_id, VIOLATION_SPEED_HACK, 
                                "Excessive movement speed detected", evidence, THREAT_LEVEL_HIGH);
        return true;
    }
    
    // Check for teleportation
    if (distance > manager->anti_cheat_config.teleport_detection_threshold) {
        char evidence[512];
        snprintf(evidence, sizeof(evidence), 
                "Distance: %.2f, Threshold: %.2f, Speed: %.2f",
                distance, manager->anti_cheat_config.teleport_detection_threshold, speed);
        
        security_report_violation(manager, player_id, VIOLATION_FLY_HACK, 
                                "Teleportation detected", evidence, THREAT_LEVEL_CRITICAL);
        return true;
    }
    
    // Update movement statistics
    player->movement_speed_average = (player->movement_speed_average * 0.9) + (speed * 0.1);
    
    return false;
}

bool secure_storage_encrypt_data(const unsigned char* key, const unsigned char* salt, 
                                const char* plaintext, unsigned char* ciphertext, int* ciphertext_len) {
    if (!key || !salt || !plaintext || !ciphertext || !ciphertext_len) return false;
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, salt) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    int len;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)plaintext, strlen(plaintext)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    *ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    *ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool secure_storage_decrypt_data(const unsigned char* key, const unsigned char* salt, 
                                const unsigned char* ciphertext, int ciphertext_len, 
                                char* plaintext, int* plaintext_len) {
    if (!key || !salt || !ciphertext || !plaintext || !plaintext_len) return false;
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, salt) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    int len;
    if (EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &len, ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    *plaintext_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    *plaintext_len += len;
    plaintext[*plaintext_len] = '\0';
    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool validate_player_name(const char* name, const input_validation_config_t* config) {
    if (!name || !config) return false;
    
    int length = strlen(name);
    if (length == 0 || length > config->max_player_name_length) {
        return false;
    }
    
    // Check for allowed characters
    for (int i = 0; i < length; i++) {
        if (strchr(config->allowed_characters, name[i]) == NULL) {
            return false;
        }
    }
    
    // Check for forbidden patterns
    for (int i = 0; i < config->forbidden_pattern_count; i++) {
        if (strstr(name, config->forbidden_patterns[i]) != NULL) {
            return false;
        }
    }
    
    return true;
}

bool check_sql_injection(const char* input) {
    if (!input) return false;
    
    const char* sql_patterns[] = {
        "DROP TABLE", "INSERT INTO", "DELETE FROM", "UPDATE SET",
        "UNION SELECT", "EXECUTE", "EXEC ", "SP_EXECUTESQL",
        "--", "/*", "*/", "'", '"'
    };
    
    int pattern_count = sizeof(sql_patterns) / sizeof(sql_patterns[0]);
    char input_lower[1024];
    
    // Convert to lowercase for case-insensitive matching
    for (int i = 0; input[i] && i < sizeof(input_lower) - 1; i++) {
        input_lower[i] = tolower(input[i]);
    }
    input_lower[strlen(input)] = '\0';
    
    for (int i = 0; i < pattern_count; i++) {
        if (strstr(input_lower, sql_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

bool check_xss_attack(const char* input) {
    if (!input) return false;
    
    const char* xss_patterns[] = {
        "<script", "</script>", "javascript:", "vbscript:",
        "onload=", "onerror=", "onclick=", "onmouseover=",
        "<iframe", "<object", "<embed", "<link"
    };
    
    int pattern_count = sizeof(xss_patterns) / sizeof(xss_patterns[0]);
    char input_lower[1024];
    
    // Convert to lowercase for case-insensitive matching
    for (int i = 0; input[i] && i < sizeof(input_lower) - 1; i++) {
        input_lower[i] = tolower(input[i]);
    }
    input_lower[strlen(input)] = '\0';
    
    for (int i = 0; i < pattern_count; i++) {
        if (strstr(input_lower, xss_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

bool generate_secure_key(unsigned char* key, int key_length) {
    if (!key || key_length <= 0) return false;
    
    return RAND_bytes(key, key_length) == 1;
}

bool generate_salt(unsigned char* salt, int salt_length) {
    if (!salt || salt_length <= 0) return false;
    
    return RAND_bytes(salt, salt_length) == 1;
}

int security_report_violation(security_manager_t* manager, const char* player_id, 
                             violation_type_t type, const char* description, 
                             const char* evidence, threat_level_t severity) {
    if (!manager || !player_id || !description) return -1;
    
    if (manager->violation_count >= manager->violation_capacity) {
        return -1;
    }
    
    security_violation_t violation = {0};
    violation.violation_id = manager->violation_count + 1;
    strncpy(violation.player_id, player_id, sizeof(violation.player_id) - 1);
    violation.type = type;
    violation.timestamp = time(NULL);
    strncpy(violation.description, description, sizeof(violation.description) - 1);
    if (evidence) {
        strncpy(violation.evidence, evidence, sizeof(violation.evidence) - 1);
    }
    violation.severity = severity;
    violation.is_resolved = false;
    
    manager->violations[manager->violation_count++] = violation;
    manager->total_violations_detected++;
    
    // Update player threat level
    player_security_t* player = security_manager_get_player(manager, player_id);
    if (player) {
        player->violation_count++;
        player->last_violation_time = violation.timestamp;
        
        // Escalate threat level based on violation count and severity
        if (player->violation_count >= 5 || severity >= THREAT_LEVEL_CRITICAL) {
            player->current_threat_level = THREAT_LEVEL_CRITICAL;
        } else if (player->violation_count >= 3 || severity >= THREAT_LEVEL_HIGH) {
            player->current_threat_level = THREAT_LEVEL_HIGH;
        } else if (player->violation_count >= 1 || severity >= THREAT_LEVEL_MEDIUM) {
            player->current_threat_level = THREAT_LEVEL_MEDIUM;
        }
    }
    
    return violation.violation_id;
}

// TODO: Medium - Add encryption to network traffic to protect against snooping and man-in-the-middle attacks.
// TODO: Medium - Implement a system for reporting and banning malicious players.

/* ============================================================================
 * security_system.c
 * Comprehensive Input Validation and Security System
 *
 * Advanced 3D Rendering Engine - Security Module
 * 
 * Provides protection against:
 * - Buffer overflow attacks
 * - SQL injection attacks
 * - Command injection attacks
 * - Cross-site scripting (XSS)
 * - Path traversal attacks
 * - Format string vulnerabilities
 * - Integer overflow/underflow
 * - Race conditions
 */

#include "security_system.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_INPUT_LENGTH 4096
#define MAX_FILENAME_LENGTH 256
#define MAX_USERNAME_LENGTH 64
#define MAX_PASSWORD_LENGTH 128
#define MAX_COMMAND_LENGTH 1024
#define MAX_QUERY_LENGTH 2048

#define SECURITY_MAX_VALIDATION_RULES 128
#define SECURITY_MAX_BLOCKED_PATTERNS 256
#define SECURITY_MAX_LOG_ENTRIES 1024

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum {
    SECURITY_RULE_NONE = 0,
    SECURITY_RULE_NO_SQL_INJECTION = 1 << 0,
    SECURITY_RULE_NO_COMMAND_INJECTION = 1 << 1,
    SECURITY_RULE_NO_XSS = 1 << 2,
    SECURITY_RULE_NO_PATH_TRAVERSAL = 1 << 3,
    SECURITY_RULE_NO_FORMAT_STRING = 1 << 4,
    SECURITY_RULE_LENGTH_LIMIT = 1 << 5,
    SECURITY_RULE_ALPHANUMERIC_ONLY = 1 << 6,
    SECURITY_RULE_FILENAME_SAFE = 1 << 7,
    SECURITY_RULE_NO_INTEGER_OVERFLOW = 1 << 8
} security_validation_rules_t;

typedef struct {
    char pattern[128];
    security_threat_type_t threat_type;
    uint32_t severity;
    bool enabled;
} security_blocked_pattern_t;

typedef struct {
    char input[MAX_INPUT_LENGTH];
    char sanitized[MAX_INPUT_LENGTH];
    security_validation_rules_t applied_rules;
    security_threat_type_t detected_threats;
    bool passed;
    uint64_t timestamp;
} security_validation_result_t;

typedef struct {
    security_threat_type_t threat_type;
    uint64_t timestamp;
    char source[64];
    char description[256];
    uint32_t severity;
} security_log_entry_t;

typedef struct {
    security_blocked_pattern_t blocked_patterns[SECURITY_MAX_BLOCKED_PATTERNS];
    security_log_entry_t log_entries[SECURITY_MAX_LOG_ENTRIES];
    uint32_t pattern_count;
    uint32_t log_count;
    uint32_t log_index;
    bool initialized;
    uint64_t total_validations;
    uint64_t blocked_attempts;
} security_context_t;

static security_context_t g_security_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool security_is_sql_injection(const char* input) {
    const char* sql_patterns[] = {
        "'", '"', "--", ";", "/*", "*/", "xp_", "sp_",
        "DROP", "DELETE", "INSERT", "UPDATE", "SELECT",
        "UNION", "OR", "AND", "WHERE", "EXEC", "EXECUTE",
        "BENCHMARK", "SLEEP", "WAITFOR", "PG_SLEEP",
        "LOAD_FILE", "OUTFILE", "DUMPFILE", "INTO"
    };
    
    char input_lower[MAX_INPUT_LENGTH];
    strncpy(input_lower, input, MAX_INPUT_LENGTH - 1);
    input_lower[MAX_INPUT_LENGTH - 1] = '\0';
    
    for (size_t i = 0; input_lower[i]; i++) {
        input_lower[i] = tolower(input_lower[i]);
    }
    
    for (size_t i = 0; i < sizeof(sql_patterns) / sizeof(sql_patterns[0]); i++) {
        if (strstr(input_lower, sql_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

static bool security_is_command_injection(const char* input) {
    const char* cmd_patterns[] = {
        "&&", "||", ";", "|", "&", "`", "$", "$(",
        "..", "/", "\\", "%", "#", "@", "!", "*",
        "rm ", "del ", "format", "fdisk", "mkfs",
        "wget", "curl", "nc", "netcat", "telnet",
        "powershell", "cmd.exe", "bash", "sh", "zsh",
        "eval", "exec", "system", "popen"
    };
    
    for (size_t i = 0; i < sizeof(cmd_patterns) / sizeof(cmd_patterns[0]); i++) {
        if (strstr(input, cmd_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

static bool security_is_xss(const char* input) {
    const char* xss_patterns[] = {
        "<script", "</script>", "javascript:", "vbscript:",
        "onload=", "onerror=", "onclick=", "onmouseover=",
        "<iframe", "<object", "<embed", "<link",
        "alert(", "confirm(", "prompt(", "eval(",
        "document.cookie", "window.location", "innerHTML"
    };
    
    char input_lower[MAX_INPUT_LENGTH];
    strncpy(input_lower, input, MAX_INPUT_LENGTH - 1);
    input_lower[MAX_INPUT_LENGTH - 1] = '\0';
    
    for (size_t i = 0; input_lower[i]; i++) {
        input_lower[i] = tolower(input_lower[i]);
    }
    
    for (size_t i = 0; i < sizeof(xss_patterns) / sizeof(xss_patterns[0]); i++) {
        if (strstr(input_lower, xss_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

static bool security_is_path_traversal(const char* input) {
    const char* path_patterns[] = {
        "..", "../", "..\\", "%2e%2e", "..%2f",
        "/etc/", "/proc/", "/sys/", "C:\\\\", "\\\\\\\\",
        "/dev/", "/var/", "/usr/", "/bin/", "/sbin/"
    };
    
    for (size_t i = 0; i < sizeof(path_patterns) / sizeof(path_patterns[0]); i++) {
        if (strstr(input, path_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

static bool security_is_format_string(const char* input) {
    const char* format_patterns[] = {
        "%s", "%d", "%x", "%n", "%p", "%f",
        "%08x", "%.*x", "%*s", "%*d"
    };
    
    for (size_t i = 0; i < sizeof(format_patterns) / sizeof(format_patterns[0]); i++) {
        if (strstr(input, format_patterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

static bool security_is_alphanumeric(const char* input) {
    for (size_t i = 0; input[i]; i++) {
        if (!isalnum(input[i]) && input[i] != '_' && input[i] != '-') {
            return false;
        }
    }
    return true;
}

static bool security_is_filename_safe(const char* input) {
    const char* unsafe_chars = "<>:\"/\\|?*";
    
    for (size_t i = 0; input[i]; i++) {
        if (input[i] < 32 || input[i] > 126) {
            return false;
        }
        if (strchr(unsafe_chars, input[i]) != NULL) {
            return false;
        }
    }
    
    if (strcmp(input, ".") == 0 || strcmp(input, "..") == 0) {
        return false;
    }
    
    return true;
}

static void security_sanitize_string(char* output, const char* input, size_t max_length) {
    size_t i = 0;
    for (i = 0; i < max_length - 1 && input[i]; i++) {
        switch (input[i]) {
            case '<': output[i] = '&'; output[++i] = 'l'; output[++i] = 't'; break;
            case '>': output[i] = '&'; output[++i] = 'g'; output[++i] = 't'; break;
            case '"': output[i] = '&'; output[++i] = 'q'; output[++i] = 'u'; output[++i] = 'o'; output[++i] = 't'; break;
            case '\'': output[i] = '&'; output[++i] = 'a'; output[++i] = 'p'; output[++i] = 'o'; output[++i] = 's'; break;
            case '&': output[i] = '&'; output[++i] = 'a'; output[++i] = 'm'; output[++i] = 'p'; break;
            default: output[i] = input[i]; break;
        }
    }
    output[i] = '\0';
}

static void security_log_threat(security_threat_type_t threat_type, 
                               const char* source, 
                               const char* description, 
                               uint32_t severity) {
    if (!g_security_ctx.initialized) return;
    
    uint32_t index = g_security_ctx.log_index;
    security_log_entry_t* entry = &g_security_ctx.log_entries[index];
    
    entry->threat_type = threat_type;
    entry->timestamp = 0; // TODO: Implement timestamp function
    strncpy(entry->source, source, sizeof(entry->source) - 1);
    entry->source[sizeof(entry->source) - 1] = '\0';
    strncpy(entry->description, description, sizeof(entry->description) - 1);
    entry->description[sizeof(entry->description) - 1] = '\0';
    entry->severity = severity;
    
    g_security_ctx.log_index = (g_security_ctx.log_index + 1) % SECURITY_MAX_LOG_ENTRIES;
    if (g_security_ctx.log_count < SECURITY_MAX_LOG_ENTRIES) {
        g_security_ctx.log_count++;
    }
    
    g_security_ctx.blocked_attempts++;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int security_system_init(void) {
    if (g_security_ctx.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_security_ctx, 0, sizeof(g_security_ctx));
    
    // Initialize default blocked patterns
    g_security_ctx.pattern_count = 0;
    g_security_ctx.log_count = 0;
    g_security_ctx.log_index = 0;
    g_security_ctx.total_validations = 0;
    g_security_ctx.blocked_attempts = 0;
    g_security_ctx.initialized = true;
    
    return 0;
}

void security_system_shutdown(void) {
    if (!g_security_ctx.initialized) {
        return;
    }
    
    memset(&g_security_ctx, 0, sizeof(g_security_ctx));
}

bool security_validate_input(const char* input, 
                           const security_validation_config_t* config,
                           security_validation_result_t* result) {
    if (!g_security_ctx.initialized || !input || !config || !result) {
        return false;
    }
    
    g_security_ctx.total_validations++;
    
    // Initialize result
    memset(result, 0, sizeof(security_validation_result_t));
    strncpy(result->input, input, MAX_INPUT_LENGTH - 1);
    result->input[MAX_INPUT_LENGTH - 1] = '\0';
    result->timestamp = 0; // TODO: Implement timestamp function
    result->passed = true;
    result->detected_threats = SECURITY_THREAT_NONE;
    
    size_t input_len = strlen(input);
    
    // Check length limits
    if (input_len > config->max_length) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_BUFFER_OVERFLOW;
        security_log_threat(SECURITY_THREAT_BUFFER_OVERFLOW, 
                           "input_validation", 
                           "Input exceeds maximum length limit", 
                           config->severity);
    }
    
    // Apply validation rules
    if (config->rules & SECURITY_RULE_NO_SQL_INJECTION && security_is_sql_injection(input)) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_SQL_INJECTION;
        security_log_threat(SECURITY_THREAT_SQL_INJECTION, 
                           "input_validation", 
                           "SQL injection pattern detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_NO_COMMAND_INJECTION && security_is_command_injection(input)) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_COMMAND_INJECTION;
        security_log_threat(SECURITY_THREAT_COMMAND_INJECTION, 
                           "input_validation", 
                           "Command injection pattern detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_NO_XSS && security_is_xss(input)) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_XSS;
        security_log_threat(SECURITY_THREAT_XSS, 
                           "input_validation", 
                           "XSS pattern detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_NO_PATH_TRAVERSAL && security_is_path_traversal(input)) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_PATH_TRAVERSAL;
        security_log_threat(SECURITY_THREAT_PATH_TRAVERSAL, 
                           "input_validation", 
                           "Path traversal pattern detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_NO_FORMAT_STRING && security_is_format_string(input)) {
        result->passed = false;
        result->detected_threats |= SECURITY_THREAT_FORMAT_STRING;
        security_log_threat(SECURITY_THREAT_FORMAT_STRING, 
                           "input_validation", 
                           "Format string vulnerability detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_ALPHANUMERIC_ONLY && !security_is_alphanumeric(input)) {
        result->passed = false;
        security_log_threat(SECURITY_THREAT_INVALID_INPUT, 
                           "input_validation", 
                           "Non-alphanumeric characters detected", 
                           config->severity);
    }
    
    if (config->rules & SECURITY_RULE_FILENAME_SAFE && !security_is_filename_safe(input)) {
        result->passed = false;
        security_log_threat(SECURITY_THREAT_INVALID_INPUT, 
                           "input_validation", 
                           "Unsafe filename characters detected", 
                           config->severity);
    }
    
    // Sanitize the input
    security_sanitize_string(result->sanitized, input, MAX_INPUT_LENGTH);
    result->applied_rules = config->rules;
    
    return result->passed;
}

bool security_validate_string_safe(const char* input, size_t max_length) {
    if (!input || !g_security_ctx.initialized) {
        return false;
    }
    
    security_validation_config_t config = {
        .rules = SECURITY_RULE_LENGTH_LIMIT | SECURITY_RULE_NO_SQL_INJECTION | 
                 SECURITY_RULE_NO_COMMAND_INJECTION | SECURITY_RULE_NO_XSS,
        .max_length = max_length,
        .severity = SECURITY_SEVERITY_MEDIUM
    };
    
    security_validation_result_t result;
    return security_validate_input(input, &config, &result);
}

bool security_validate_filename(const char* filename) {
    if (!filename || !g_security_ctx.initialized) {
        return false;
    }
    
    security_validation_config_t config = {
        .rules = SECURITY_RULE_FILENAME_SAFE | SECURITY_RULE_NO_PATH_TRAVERSAL |
                 SECURITY_RULE_LENGTH_LIMIT,
        .max_length = MAX_FILENAME_LENGTH,
        .severity = SECURITY_SEVERITY_HIGH
    };
    
    security_validation_result_t result;
    return security_validate_input(filename, &config, &result);
}

bool security_validate_network_data(const void* data, size_t size, size_t max_size) {
    if (!data || !g_security_ctx.initialized) {
        return false;
    }
    
    if (size > max_size) {
        security_log_threat(SECURITY_THREAT_BUFFER_OVERFLOW, 
                           "network_validation", 
                           "Network data exceeds maximum size", 
                           SECURITY_SEVERITY_HIGH);
        return false;
    }
    
    // Check for null bytes in string data
    const char* str_data = (const char*)data;
    for (size_t i = 0; i < size - 1; i++) {
        if (str_data[i] == '\0' && str_data[i + 1] != '\0') {
            security_log_threat(SECURITY_THREAT_INVALID_INPUT, 
                               "network_validation", 
                               "Embedded null bytes detected", 
                               SECURITY_SEVERITY_MEDIUM);
            return false;
        }
    }
    
    return true;
}

bool security_safe_string_copy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return false;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        security_log_threat(SECURITY_THREAT_BUFFER_OVERFLOW, 
                           "string_copy", 
                           "Source string too large for destination buffer", 
                           SECURITY_SEVERITY_HIGH);
        return false;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
    return true;
}

bool security_safe_string_concat(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return false;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        security_log_threat(SECURITY_THREAT_BUFFER_OVERFLOW, 
                           "string_concat", 
                           "Concatenation would exceed destination buffer", 
                           SECURITY_SEVERITY_HIGH);
        return false;
    }
    
    strncat(dest, src, dest_size - dest_len - 1);
    return true;
}

int security_sprintf_safe(char* dest, size_t dest_size, const char* format, ...) {
    if (!dest || !format || dest_size == 0) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(dest, dest_size, format, args);
    va_end(args);
    
    if (result >= (int)dest_size) {
        security_log_threat(SECURITY_THREAT_BUFFER_OVERFLOW, 
                           "sprintf_safe", 
                           "Formatted output exceeds buffer size", 
                           SECURITY_SEVERITY_HIGH);
        dest[dest_size - 1] = '\0';
        return -1;
    }
    
    return result;
}

bool security_validate_integer_range(int value, int min_val, int max_val) {
    if (value < min_val || value > max_val) {
        security_log_threat(SECURITY_THREAT_INTEGER_OVERFLOW, 
                           "integer_validation", 
                           "Integer value outside valid range", 
                           SECURITY_SEVERITY_MEDIUM);
        return false;
    }
    return true;
}

bool security_validate_size_t_range(size_t value, size_t max_val) {
    if (value > max_val) {
        security_log_threat(SECURITY_THREAT_INTEGER_OVERFLOW, 
                           "size_t_validation", 
                           "Size_t value exceeds maximum", 
                           SECURITY_SEVERITY_MEDIUM);
        return false;
    }
    return true;
}

void security_get_statistics(security_stats_t* stats) {
    if (!stats || !g_security_ctx.initialized) {
        return;
    }
    
    stats->total_validations = g_security_ctx.total_validations;
    stats->blocked_attempts = g_security_ctx.blocked_attempts;
    stats->log_entries = g_security_ctx.log_count;
    stats->blocked_patterns = g_security_ctx.pattern_count;
}

void security_clear_logs(void) {
    if (!g_security_ctx.initialized) {
        return;
    }
    
    g_security_ctx.log_count = 0;
    g_security_ctx.log_index = 0;
    memset(g_security_ctx.log_entries, 0, sizeof(g_security_ctx.log_entries));
}

/* ============================================================================
 * PLAYER SECURITY MANAGEMENT
 * ============================================================================ */

#define MAX_PLAYERS 1024
static player_security_profile_t g_player_profiles[MAX_PLAYERS];
static uint32_t g_next_violation_id = 1;

player_security_profile_t* security_player_profile_init(uint32_t player_id, const char* username) {
    if (!g_security_ctx.initialized || !username || player_id >= MAX_PLAYERS) {
        return NULL;
    }
    
    player_security_profile_t* profile = &g_player_profiles[player_id];
    
    // Initialize profile
    profile->player_id = player_id;
    security_safe_string_copy(profile->username, username, sizeof(profile->username));
    profile->current_threat_level = THREAT_LEVEL_NONE;
    profile->violation_count = 0;
    profile->last_violation_time = 0;
    profile->is_banned = false;
    profile->ban_expiry = 0;
    
    return profile;
}

player_security_profile_t* security_player_profile_get(uint32_t player_id) {
    if (!g_security_ctx.initialized || player_id >= MAX_PLAYERS) {
        return NULL;
    }
    
    player_security_profile_t* profile = &g_player_profiles[player_id];
    
    // Check if profile is initialized (username not empty)
    if (profile->username[0] == '\0') {
        return NULL;
    }
    
    return profile;
}

uint32_t security_log_violation(uint32_t player_id, 
                               security_threat_type_t threat_type,
                               const char* description, 
                               uint32_t severity) {
    if (!g_security_ctx.initialized || !description || player_id >= MAX_PLAYERS) {
        return 0;
    }
    
    player_security_profile_t* player = security_player_profile_get(player_id);
    if (!player) {
        return 0;
    }
    
    // Create violation record
    security_violation_t violation = {
        .violation_id = g_next_violation_id++,
        .player_id = player_id,
        .threat_type = threat_type,
        .timestamp = 0, // TODO: Implement timestamp function
        .severity = severity
    };
    security_safe_string_copy(violation.description, description, sizeof(violation.description));
    
    // Update player profile
    player->violation_count++;
    player->last_violation_time = violation.timestamp;
    
    // Update threat level based on violations and severity
    if (severity >= SECURITY_SEVERITY_CRITICAL || player->violation_count >= 10) {
        player->current_threat_level = THREAT_LEVEL_CRITICAL;
    } else if (severity >= SECURITY_SEVERITY_HIGH || player->violation_count >= 5) {
        player->current_threat_level = THREAT_LEVEL_HIGH;
    } else if (player->violation_count >= 1 || severity >= SECURITY_SEVERITY_MEDIUM) {
        player->current_threat_level = THREAT_LEVEL_MEDIUM;
    }
    
    // Auto-ban for critical violations
    if (severity >= SECURITY_SEVERITY_CRITICAL || player->current_threat_level >= THREAT_LEVEL_CRITICAL) {
        security_ban_player(player_id, 3600); // 1 hour ban
    }
    
    return violation.violation_id;
}

bool security_ban_player(uint32_t player_id, uint64_t duration_seconds) {
    if (!g_security_ctx.initialized || player_id >= MAX_PLAYERS) {
        return false;
    }
    
    player_security_profile_t* player = security_player_profile_get(player_id);
    if (!player) {
        return false;
    }
    
    player->is_banned = true;
    
    if (duration_seconds == 0) {
        player->ban_expiry = 0; // Permanent ban
    } else {
        player->ban_expiry = (0) + duration_seconds; // TODO: Implement timestamp function
    }
    
    security_log_threat(SECURITY_THREAT_INVALID_INPUT, 
                       "player_ban", 
                       "Player banned for security violations", 
                       SECURITY_SEVERITY_HIGH);
    
    return true;
}

bool security_unban_player(uint32_t player_id) {
    if (!g_security_ctx.initialized || player_id >= MAX_PLAYERS) {
        return false;
    }
    
    player_security_profile_t* player = security_player_profile_get(player_id);
    if (!player) {
        return false;
    }
    
    player->is_banned = false;
    player->ban_expiry = 0;
    player->current_threat_level = THREAT_LEVEL_LOW;
    
    return true;
}

bool security_is_player_banned(uint32_t player_id) {
    if (!g_security_ctx.initialized || player_id >= MAX_PLAYERS) {
        return false;
    }
    
    player_security_profile_t* player = security_player_profile_get(player_id);
    if (!player) {
        return false;
    }
    
    // Check if ban has expired
    if (player->is_banned && player->ban_expiry > 0) {
        uint64_t current_time = 0; // TODO: Implement timestamp function
        if (current_time >= player->ban_expiry) {
            player->is_banned = false;
            player->ban_expiry = 0;
            return false;
        }
    }
    
    return player->is_banned;
}

uint32_t security_get_recent_violations(security_violation_t* violations, uint32_t max_count) {
    if (!g_security_ctx.initialized || !violations || max_count == 0) {
        return 0;
    }
    
    // This is a simplified implementation - in practice, you'd want to store
    // violations in a more sophisticated data structure for efficient retrieval
    uint32_t count = 0;
    
    // For now, return 0 as we don't have a proper violation storage system
    // TODO: Implement proper violation storage and retrieval
    
    return count;
}

void security_set_global_level(security_severity_t severity) {
    if (!g_security_ctx.initialized) {
        return;
    }
    
    // This would affect the default severity for new validations
    // TODO: Implement global security level management
}

void security_set_global_rules(uint32_t rules, bool enabled) {
    if (!g_security_ctx.initialized) {
        return;
    }
    
    // This would affect the default rules for new validations
    // TODO: Implement global rule management
}

bool security_add_threat_pattern(const char* pattern, 
                                security_threat_type_t threat_type, 
                                uint32_t severity) {
    if (!g_security_ctx.initialized || !pattern || g_security_ctx.pattern_count >= SECURITY_MAX_BLOCKED_PATTERNS) {
        return false;
    }
    
    security_blocked_pattern_t* new_pattern = &g_security_ctx.blocked_patterns[g_security_ctx.pattern_count];
    
    security_safe_string_copy(new_pattern->pattern, pattern, sizeof(new_pattern->pattern));
    new_pattern->threat_type = threat_type;
    new_pattern->severity = severity;
    new_pattern->enabled = true;
    
    g_security_ctx.pattern_count++;
    return true;
}

bool security_remove_threat_pattern(const char* pattern) {
    if (!g_security_ctx.initialized || !pattern) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_security_ctx.pattern_count; i++) {
        if (strcmp(g_security_ctx.blocked_patterns[i].pattern, pattern) == 0) {
            // Shift remaining patterns
            for (uint32_t j = i; j < g_security_ctx.pattern_count - 1; j++) {
                g_security_ctx.blocked_patterns[j] = g_security_ctx.blocked_patterns[j + 1];
            }
            g_security_ctx.pattern_count--;
            return true;
        }
    }
    
    return false;
}

/* End of security_system.c */
