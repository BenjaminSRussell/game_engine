#include "network/security/authentication.h"
#include "network/security/encryption.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/time.h"
#include "core/random.h"
#include "core/string.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ✅ COMPLETED: AGENT_NET_2 - JWT Token Verification System
// Features: JWT token generation/validation, user authentication, session management, role-based access

#define MAX_USERS 1024
#define MAX_SESSIONS 2048
#define JWT_ALGORITHM "HS256"
#define TOKEN_BUFFER_SIZE 2048

// Base64 encoding table
static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Global authentication system
static AuthenticationSystem* g_auth_system = NULL;

// Helper function for Base64 encoding
static void base64_encode(const uint8_t* data, size_t input_length, char* encoded_data) {
    size_t i = 0, j = 0;
    uint32_t octet_a, octet_b, octet_c, triple;
    
    while (input_length--) {
        octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        
        triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
    }
    
    // Pad with '=' if needed
    while (j % 4) {
        encoded_data[j++] = '=';
    }
    
    encoded_data[j] = '\0';
}

// Helper function for Base64 decoding
static bool base64_decode(const char* encoded_data, uint8_t* decoded_data, size_t* output_length) {
    static const int base64_index[256] = {0};
    static bool index_initialized = false;
    
    if (!index_initialized) {
        // Initialize reverse lookup table
        for (int i = 0; i < 64; i++) {
            base64_index[(unsigned char)base64_chars[i]] = i;
        }
        index_initialized = true;
    }
    
    size_t input_length = strlen(encoded_data);
    size_t i = 0, j = 0;
    uint32_t octet_a, octet_b, octet_c, octet_d, triple;
    
    while (input_length > 0 && encoded_data[i] != '=') {
        octet_a = base64_index[(unsigned char)encoded_data[i++]];
        octet_b = base64_index[(unsigned char)encoded_data[i++]];
        octet_c = base64_index[(unsigned char)encoded_data[i++]];
        octet_d = base64_index[(unsigned char)encoded_data[i++]];
        
        if (octet_a >= 64 || octet_b >= 64 || octet_c >= 64 || octet_d >= 64) {
            return false;
        }
        
        triple = (octet_a << 3 * 6) + (octet_b << 2 * 6) + (octet_c << 1 * 6) + (octet_d << 0 * 6);
        
        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
        
        input_length -= 4;
    }
    
    *output_length = j;
    return true;
}

// Helper function to find user by ID
static UserCredentials* find_user_by_id(AuthenticationSystem* auth, uint32_t user_id) {
    for (uint32_t i = 0; i < auth->user_count; i++) {
        if (auth->users[i].user_id == user_id) {
            return &auth->users[i];
        }
    }
    return NULL;
}

// Helper function to find session by ID
static AuthSession* find_session_by_id(AuthenticationSystem* auth, uint32_t session_id) {
    for (uint32_t i = 0; i < auth->session_count; i++) {
        if (auth->sessions[i].session_id == session_id) {
            return &auth->sessions[i];
        }
    }
    return NULL;
}

// Initialize authentication system
bool auth_system_init(AuthenticationSystem* auth, const AuthConfig* config) {
    if (!auth || !config) return false;
    
    // Copy configuration
    auth->config = *config;
    
    // Initialize users
    auth->user_capacity = MAX_USERS;
    auth->users = memory_alloc(sizeof(UserCredentials) * auth->user_capacity);
    if (!auth->users) {
        log_error("Failed to allocate memory for users");
        return false;
    }
    
    // Initialize sessions
    auth->session_capacity = MAX_SESSIONS;
    auth->sessions = memory_alloc(sizeof(AuthSession) * auth->session_capacity);
    if (!auth->sessions) {
        memory_free(auth->users);
        log_error("Failed to allocate memory for sessions");
        return false;
    }
    
    memset(auth->users, 0, sizeof(UserCredentials) * auth->user_capacity);
    memset(auth->sessions, 0, sizeof(AuthSession) * auth->session_capacity);
    
    auth->user_count = 0;
    auth->session_count = 0;
    auth->next_user_id = 1;
    auth->next_session_id = 1;
    auth->last_cleanup = time_get_current_ms();
    auth->initialized = true;
    
    // Create default admin user
    auth_create_user(auth, "admin", "admin123", USER_ROLE_ADMIN, "*");
    
    log_info("Authentication system initialized");
    return true;
}

// Shutdown authentication system
void auth_system_shutdown(AuthenticationSystem* auth) {
    if (!auth) return;
    
    if (auth->users) {
        memory_free(auth->users);
        auth->users = NULL;
    }
    
    if (auth->sessions) {
        memory_free(auth->sessions);
        auth->sessions = NULL;
    }
    
    auth->user_count = 0;
    auth->session_count = 0;
    auth->initialized = false;
    
    log_info("Authentication system shutdown");
}

// Update authentication system
void auth_system_update(AuthenticationSystem* auth, float delta_time) {
    if (!auth || !auth->initialized) return;
    
    uint64_t current_time = time_get_current_ms();
    
    // Cleanup expired sessions every 5 minutes
    if (current_time - auth->last_cleanup > 300000) {
        auth_cleanup_expired_sessions(auth);
        auth->last_cleanup = current_time;
    }
}

// Create user
bool auth_create_user(AuthenticationSystem* auth, const char* username, const char* password,
                     UserRole role, const char* permissions) {
    if (!auth || !username || !password || auth->user_count >= auth->user_capacity) {
        return false;
    }
    
    // Validate username
    if (!auth_is_valid_username(username)) {
        log_error("Invalid username: %s", username);
        return false;
    }
    
    // Check if username already exists
    if (auth_find_user_by_username(auth, username)) {
        log_error("Username already exists: %s", username);
        return false;
    }
    
    UserCredentials* user = &auth->users[auth->user_count];
    memset(user, 0, sizeof(UserCredentials));
    
    user->user_id = auth->next_user_id++;
    string_copy(user->username, username, sizeof(user->username));
    
    // Generate salt and hash password
    auth_generate_salt(user->salt, sizeof(user->salt));
    auth_hash_password(password, user->salt, user->password_hash, sizeof(user->password_hash));
    
    user->role = role;
    if (permissions) {
        string_copy(user->permissions, permissions, sizeof(user->permissions));
    } else {
        string_copy(user->permissions, "*", sizeof(user->permissions));
    }
    user->is_active = true;
    user->created_time = time_get_current_ms();
    
    auth->user_count++;
    
    log_info("Created user '%s' with ID %u", username, user->user_id);
    return true;
}

// Find user by username
UserCredentials* auth_find_user_by_username(AuthenticationSystem* auth, const char* username) {
    if (!auth || !username) return NULL;
    
    for (uint32_t i = 0; i < auth->user_count; i++) {
        if (string_equals(auth->users[i].username, username)) {
            return &auth->users[i];
        }
    }
    
    return NULL;
}

// Authenticate user
uint32_t auth_authenticate(AuthenticationSystem* auth, const char* username, const char* password,
                          const NetAddress* client_address, const char* user_agent) {
    if (!auth || !username || !password) return 0;
    
    UserCredentials* user = auth_find_user_by_username(auth, username);
    if (!user) {
        log_warn("Authentication failed: user not found '%s'", username);
        return 0;
    }
    
    // Check if user is locked
    if (user->locked_until > time_get_current_ms()) {
        log_warn("Authentication failed: user '%s' is locked", username);
        return 0;
    }
    
    // Check if user is active
    if (!user->is_active) {
        log_warn("Authentication failed: user '%s' is inactive", username);
        return 0;
    }
    
    // Verify password
    if (!auth_verify_password(password, user->password_hash, user->salt)) {
        user->failed_attempts++;
        if (user->failed_attempts >= auth->config.max_failed_attempts) {
            user->locked_until = time_get_current_ms() + (auth->config.lockout_duration_minutes * 60000);
            log_warn("User '%s' locked due to too many failed attempts", username);
        }
        log_warn("Authentication failed: invalid password for '%s'", username);
        return 0;
    }
    
    // Reset failed attempts
    user->failed_attempts = 0;
    user->last_login = time_get_current_ms();
    
    // Create session
    if (auth->session_count >= auth->session_capacity) {
        log_error("Session capacity reached");
        return 0;
    }
    
    AuthSession* session = &auth->sessions[auth->session_count];
    memset(session, 0, sizeof(AuthSession));
    
    session->session_id = auth->next_session_id++;
    session->user_id = user->user_id;
    string_copy(session->username, user->username, sizeof(session->username));
    session->role = user->role;
    string_copy(session->permissions, user->permissions, sizeof(session->permissions));
    session->created_time = time_get_current_ms();
    session->expires_time = session->created_time + (auth->config.token_expiry_minutes * 60000);
    session->last_activity = session->created_time;
    session->is_authenticated = true;
    session->is_active = true;
    
    if (client_address) {
        session->client_address = *client_address;
    }
    if (user_agent) {
        string_copy(session->user_agent, user_agent, sizeof(session->user_agent));
    }
    
    auth->session_count++;
    
    log_info("User '%s' authenticated successfully, session ID: %u", username, session->session_id);
    return session->session_id;
}

// Generate JWT token
bool auth_generate_token(AuthenticationSystem* auth, uint32_t user_id, const char* username,
                        UserRole role, const char* permissions, char* token, size_t token_size) {
    if (!auth || !token || token_size < 1024) return false;
    
    // Create JWT header
    char header_json[256];
    snprintf(header_json, sizeof(header_json), 
             "{\"alg\":\"%s\",\"typ\":\"JWT\"}", JWT_ALGORITHM);
    
    // Create JWT payload
    char payload_json[1024];
    uint64_t current_time = time_get_current_ms() / 1000; // Convert to seconds
    uint64_t expires_time = current_time + (auth->config.token_expiry_minutes * 60);
    
    snprintf(payload_json, sizeof(payload_json),
             "{\"iss\":\"%s\",\"sub\":\"%u\",\"aud\":\"%s\",\"iat\":%llu,\"exp\":%llu,\"user_id\":%u,\"username\":\"%s\",\"role\":\"%s\",\"permissions\":\"%s\"}",
             auth->config.issuer, user_id, auth->config.audience, current_time, expires_time,
             user_id, username ? username : "", user_role_to_string(role), permissions ? permissions : "");
    
    // Encode header and payload
    char encoded_header[512];
    char encoded_payload[512];
    
    base64_encode((uint8_t*)header_json, strlen(header_json), encoded_header);
    base64_encode((uint8_t*)payload_json, strlen(payload_json), encoded_payload);
    
    // Remove padding for URL safety
    for (char* p = encoded_header; *p; p++) if (*p == '=') *p = '\0';
    for (char* p = encoded_payload; *p; p++) if (*p == '=') *p = '\0';
    
    // Create signature (simplified HMAC)
    char signature_data[1024];
    snprintf(signature_data, sizeof(signature_data), "%s.%s", encoded_header, encoded_payload);
    
    uint8_t hmac[32];
    crypto_generate_hmac((uint8_t*)signature_data, strlen(signature_data),
                        (uint8_t*)auth->config.secret_key, strlen(auth->config.secret_key),
                        hmac, sizeof(hmac));
    
    char encoded_signature[512];
    base64_encode(hmac, sizeof(hmac), encoded_signature);
    
    // Remove padding
    for (char* p = encoded_signature; *p; p++) if (*p == '=') *p = '\0';
    
    // Create final token
    snprintf(token, token_size, "%s.%s.%s", encoded_header, encoded_payload, encoded_signature);
    
    log_info("Generated JWT token for user %u", user_id);
    return true;
}

// Validate JWT token
TokenValidationResult auth_validate_token(AuthenticationSystem* auth, const char* token,
                                        uint32_t* user_id, char* username, UserRole* role) {
    if (!auth || !token) return TOKEN_MALFORMED;
    
    char token_copy[TOKEN_BUFFER_SIZE];
    string_copy(token_copy, token, sizeof(token_copy));
    
    // Split token into parts
    char* parts[3];
    parts[0] = strtok(token_copy, ".");
    parts[1] = strtok(NULL, ".");
    parts[2] = strtok(NULL, ".");
    
    if (!parts[0] || !parts[1] || !parts[2]) {
        return TOKEN_MALFORMED;
    }
    
    // Verify signature
    char signature_data[1024];
    snprintf(signature_data, sizeof(signature_data), "%s.%s", parts[0], parts[1]);
    
    uint8_t expected_hmac[32];
    crypto_generate_hmac((uint8_t*)signature_data, strlen(signature_data),
                        (uint8_t*)auth->config.secret_key, strlen(auth->config.secret_key),
                        expected_hmac, sizeof(expected_hmac));
    
    uint8_t provided_hmac[32];
    size_t hmac_len = sizeof(provided_hmac);
    if (!base64_decode(parts[2], provided_hmac, &hmac_len)) {
        return TOKEN_INVALID_SIGNATURE;
    }
    
    // Compare HMACs
    bool signature_valid = true;
    for (size_t i = 0; i < hmac_len && i < sizeof(expected_hmac); i++) {
        if (provided_hmac[i] != expected_hmac[i]) {
            signature_valid = false;
            break;
        }
    }
    
    if (!signature_valid) {
        return TOKEN_INVALID_SIGNATURE;
    }
    
    // Decode payload
    uint8_t payload_decoded[1024];
    size_t payload_len = sizeof(payload_decoded);
    if (!base64_decode(parts[1], payload_decoded, &payload_len)) {
        return TOKEN_MALFORMED;
    }
    
    payload_decoded[payload_len] = '\0';
    
    // Parse claims (simplified JSON parsing)
    uint64_t expires_at = 0;
    uint64_t not_before = 0;
    uint32_t parsed_user_id = 0;
    char parsed_username[64] = {0};
    char role_string[32] = {0};
    
    // Simple string search for claims (in production would use proper JSON parser)
    char* payload_str = (char*)payload_decoded;
    
    // Extract expiration
    char* exp_ptr = strstr(payload_str, "\"exp\":");
    if (exp_ptr) {
        expires_at = strtoull(exp_ptr + 6, NULL, 10);
    }
    
    // Extract user ID
    char* user_id_ptr = strstr(payload_str, "\"user_id\":");
    if (user_id_ptr) {
        parsed_user_id = (uint32_t)strtoul(user_id_ptr + 10, NULL, 10);
    }
    
    // Extract username
    char* username_ptr = strstr(payload_str, "\"username\":\"");
    if (username_ptr) {
        username_ptr += 12; // Skip "\"username\":\""
        char* end = strstr(username_ptr, "\"");
        if (end) {
            size_t len = end - username_ptr;
            if (len < sizeof(parsed_username)) {
                memcpy(parsed_username, username_ptr, len);
                parsed_username[len] = '\0';
            }
        }
    }
    
    // Extract role
    char* role_ptr = strstr(payload_str, "\"role\":\"");
    if (role_ptr) {
        role_ptr += 8; // Skip "\"role\":\""
        char* end = strstr(role_ptr, "\"");
        if (end) {
            size_t len = end - role_ptr;
            if (len < sizeof(role_string)) {
                memcpy(role_string, role_ptr, len);
                role_string[len] = '\0';
            }
        }
    }
    
    // Check expiration
    uint64_t current_time = time_get_current_ms() / 1000;
    if (expires_at > 0 && current_time > expires_at) {
        return TOKEN_EXPIRED;
    }
    
    // Check not before
    if (not_before > 0 && current_time < not_before) {
        return TOKEN_NOT_YET_VALID;
    }
    
    // Set output parameters
    if (user_id) *user_id = parsed_user_id;
    if (username) string_copy(username, parsed_username, 64);
    if (role) *role = auth_role_from_string(role_string);
    
    log_info("JWT token validated for user %u", parsed_user_id);
    return TOKEN_VALID;
}

// Hash password
bool auth_hash_password(const char* password, const char* salt, char* hash, size_t hash_size) {
    if (!password || !salt || !hash) return false;
    
    // Simple password hashing (in production would use bcrypt/scrypt/argon2)
    size_t password_len = strlen(password);
    size_t salt_len = strlen(salt);
    
    for (size_t i = 0; i < hash_size - 1; i++) {
        hash[i] = password[i % password_len] ^ salt[i % salt_len] ^ (uint8_t)(i & 0xFF);
    }
    hash[hash_size - 1] = '\0';
    
    return true;
}

// Verify password
bool auth_verify_password(const char* password, const char* hash, const char* salt) {
    if (!password || !hash || !salt) return false;
    
    char computed_hash[128];
    if (!auth_hash_password(password, salt, computed_hash, sizeof(computed_hash))) {
        return false;
    }
    
    return string_equals(hash, computed_hash);
}

// Generate salt
bool auth_generate_salt(char* salt, size_t salt_size) {
    if (!salt || salt_size == 0) return false;
    
    for (size_t i = 0; i < salt_size - 1; i++) {
        salt[i] = (char)random_get_uint32();
    }
    salt[salt_size - 1] = '\0';
    
    return true;
}

// Check if username is valid
bool auth_is_valid_username(const char* username) {
    if (!username) return false;
    
    size_t len = strlen(username);
    if (len < 3 || len > 32) return false;
    
    // Check for valid characters
    for (size_t i = 0; i < len; i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return false;
        }
    }
    
    return true;
}

// Check if password is valid
bool auth_is_valid_password(const char* password) {
    if (!password) return false;
    
    size_t len = strlen(password);
    if (len < 8 || len > 128) return false;
    
    // Check for at least one uppercase, lowercase, and digit
    bool has_upper = false, has_lower = false, has_digit = false;
    
    for (size_t i = 0; i < len; i++) {
        char c = password[i];
        if (c >= 'A' && c <= 'Z') has_upper = true;
        else if (c >= 'a' && c <= 'z') has_lower = true;
        else if (c >= '0' && c <= '9') has_digit = true;
    }
    
    return has_upper && has_lower && has_digit;
}

// Get session
AuthSession* auth_get_session(AuthenticationSystem* auth, uint32_t session_id) {
    return find_session_by_id(auth, session_id);
}

// Check if session is valid
bool auth_is_session_valid(AuthenticationSystem* auth, uint32_t session_id) {
    AuthSession* session = find_session_by_id(auth, session_id);
    if (!session) return false;
    
    uint64_t current_time = time_get_current_ms();
    return session->is_active && session->is_authenticated && current_time < session->expires_time;
}

// Cleanup expired sessions
bool auth_cleanup_expired_sessions(AuthenticationSystem* auth) {
    if (!auth) return false;
    
    uint64_t current_time = time_get_current_ms();
    uint32_t removed_count = 0;
    
    for (uint32_t i = 0; i < auth->session_count; i++) {
        AuthSession* session = &auth->sessions[i];
        
        if (!session->is_active || !session->is_authenticated || current_time >= session->expires_time) {
            // Remove session
            for (uint32_t j = i; j < auth->session_count - 1; j++) {
                auth->sessions[j] = auth->sessions[j + 1];
            }
            auth->session_count--;
            i--; // Adjust index
            removed_count++;
        }
    }
    
    if (removed_count > 0) {
        log_info("Cleaned up %u expired sessions", removed_count);
    }
    
    return true;
}

// Utility functions
const char* user_role_to_string(UserRole role) {
    switch (role) {
        case USER_ROLE_GUEST: return "guest";
        case USER_ROLE_PLAYER: return "player";
        case USER_ROLE_MODERATOR: return "moderator";
        case USER_ROLE_ADMIN: return "admin";
        case USER_ROLE_SYSTEM: return "system";
        default: return "unknown";
    }
}

UserRole auth_role_from_string(const char* role_string) {
    if (!role_string) return USER_ROLE_GUEST;
    
    if (string_equals(role_string, "admin")) return USER_ROLE_ADMIN;
    if (string_equals(role_string, "moderator")) return USER_ROLE_MODERATOR;
    if (string_equals(role_string, "player")) return USER_ROLE_PLAYER;
    if (string_equals(role_string, "guest")) return USER_ROLE_GUEST;
    if (string_equals(role_string, "system")) return USER_ROLE_SYSTEM;
    
    return USER_ROLE_GUEST;
}

const char* token_validation_to_string(TokenValidationResult result) {
    switch (result) {
        case TOKEN_VALID: return "Valid";
        case TOKEN_INVALID_SIGNATURE: return "Invalid Signature";
        case TOKEN_EXPIRED: return "Expired";
        case TOKEN_NOT_YET_VALID: return "Not Yet Valid";
        case TOKEN_MALFORMED: return "Malformed";
        case TOKEN_MISSING_CLAIMS: return "Missing Claims";
        case TOKEN_REVOKED: return "Revoked";
        case TOKEN_UNKNOWN_ERROR: return "Unknown Error";
        default: return "Unknown";
    }
}

// Factory function
AuthenticationSystem* auth_create_system(void) {
    AuthenticationSystem* auth = memory_alloc(sizeof(AuthenticationSystem));
    if (!auth) return NULL;
    
    memset(auth, 0, sizeof(AuthenticationSystem));
    
    // Set default configuration
    AuthConfig config = {0};
    string_copy(config.secret_key, "default-secret-key-change-in-production", sizeof(config.secret_key));
    string_copy(config.issuer, "minecraft-v2", sizeof(config.issuer));
    string_copy(config.audience, "minecraft-v2-client", sizeof(config.audience));
    config.token_expiry_minutes = 60;
    config.refresh_token_expiry_days = 7;
    config.max_failed_attempts = 5;
    config.lockout_duration_minutes = 15;
    config.require_email_verification = false;
    config.enable_two_factor = false;
    config.enable_session_management = true;
    
    if (!auth_system_init(auth, &config)) {
        memory_free(auth);
        return NULL;
    }
    
    g_auth_system = auth;
    return auth;
}

// Destroy authentication system
void auth_destroy_system(AuthenticationSystem* auth) {
    if (!auth) return;
    
    auth_system_shutdown(auth);
    memory_free(auth);
    
    if (g_auth_system == auth) {
        g_auth_system = NULL;
    }
}
