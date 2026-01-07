#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include "../core/core.h"
#include "include/network/network_types.h"

// JWT token parts
typedef enum {
    JWT_HEADER = 0,
    JWT_PAYLOAD,
    JWT_SIGNATURE,
    JWT_PART_COUNT
} JwtTokenPart;

// JWT claims
typedef enum {
    JWT_CLAIM_ISS = 0,    // Issuer
    JWT_CLAIM_SUB,        // Subject
    JWT_CLAIM_AUD,        // Audience
    JWT_CLAIM_EXP,        // Expiration time
    JWT_CLAIM_NBF,        // Not before
    JWT_CLAIM_IAT,        // Issued at
    JWT_CLAIM_JTI,        // JWT ID
    JWT_CLAIM_USER_ID,    // User ID
    JWT_CLAIM_USERNAME,   // Username
    JWT_CLAIM_ROLE,       // User role
    JWT_CLAIM_PERMISSIONS,// Permissions
    JWT_CLAIM_SESSION_ID, // Session ID
    JWT_CLAIM_COUNT
} JwtClaimType;

// User roles
typedef enum {
    USER_ROLE_GUEST = 0,
    USER_ROLE_PLAYER,
    USER_ROLE_MODERATOR,
    USER_ROLE_ADMIN,
    USER_ROLE_SYSTEM,
    USER_ROLE_COUNT
} UserRole;

// Authentication methods
typedef enum {
    AUTH_METHOD_PASSWORD = 0,
    AUTH_METHOD_TOKEN,
    AUTH_METHOD_OAUTH,
    AUTH_METHOD_CERTIFICATE,
    AUTH_METHOD_BIOMETRIC,
    AUTH_METHOD_COUNT
} AuthenticationMethod;

// Token validation result
typedef enum {
    TOKEN_VALID = 0,
    TOKEN_INVALID_SIGNATURE,
    TOKEN_EXPIRED,
    TOKEN_NOT_YET_VALID,
    TOKEN_MALFORMED,
    TOKEN_MISSING_CLAIMS,
    TOKEN_REVOKED,
    TOKEN_UNKNOWN_ERROR
} TokenValidationResult;

// Authentication session
typedef struct {
    uint32_t session_id;
    uint32_t user_id;
    char username[64];
    UserRole role;
    char permissions[256];
    uint64_t created_time;
    uint64_t expires_time;
    uint64_t last_activity;
    NetAddress client_address;
    char user_agent[128];
    bool is_authenticated;
    bool is_active;
} AuthSession;

// JWT token structure
typedef struct {
    char header[512];
    char payload[1024];
    char signature[512];
    char full_token[2048];
    uint64_t issued_at;
    uint64_t expires_at;
    bool is_valid;
} JwtToken;

// User credentials
typedef struct {
    uint32_t user_id;
    char username[64];
    char password_hash[128];
    char salt[32];
    UserRole role;
    char permissions[256];
    bool is_active;
    uint64_t created_time;
    uint64_t last_login;
    uint32_t failed_attempts;
    uint64_t locked_until;
} UserCredentials;

// Authentication system configuration
typedef struct {
    char secret_key[256];
    char issuer[128];
    char audience[128];
    uint32_t token_expiry_minutes;
    uint32_t refresh_token_expiry_days;
    uint32_t max_failed_attempts;
    uint32_t lockout_duration_minutes;
    bool require_email_verification;
    bool enable_two_factor;
    bool enable_session_management;
} AuthConfig;

// Authentication system interface
typedef struct AuthenticationSystem {
    // Lifecycle
    bool (*init)(struct AuthenticationSystem* auth, const AuthConfig* config);
    void (*shutdown)(struct AuthenticationSystem* auth);
    void (*update)(struct AuthenticationSystem* auth, float delta_time);
    
    // User management
    bool (*create_user)(struct AuthenticationSystem* auth, const char* username, const char* password,
                       UserRole role, const char* permissions);
    bool (*delete_user)(struct AuthenticationSystem* auth, uint32_t user_id);
    bool (*update_user)(struct AuthenticationSystem* auth, uint32_t user_id, const char* username,
                       UserRole role, const char* permissions);
    UserCredentials* (*get_user)(struct AuthenticationSystem* auth, uint32_t user_id);
    UserCredentials* (*find_user_by_username)(struct AuthenticationSystem* auth, const char* username);
    
    // Authentication
    uint32_t (*authenticate)(struct AuthenticationSystem* auth, const char* username, const char* password,
                           const NetAddress* client_address, const char* user_agent);
    bool (*authenticate_token)(struct AuthenticationSystem* auth, const char* token,
                              const NetAddress* client_address);
    bool (*logout)(struct AuthenticationSystem* auth, uint32_t session_id);
    bool (*refresh_token)(struct AuthenticationSystem* auth, const char* refresh_token, char* new_token);
    
    // JWT token operations
    bool (*generate_token)(struct AuthenticationSystem* auth, uint32_t user_id, const char* username,
                          UserRole role, const char* permissions, char* token, size_t token_size);
    TokenValidationResult (*validate_token)(struct AuthenticationSystem* auth, const char* token,
                                         uint32_t* user_id, char* username, UserRole* role);
    bool (*revoke_token)(struct AuthenticationSystem* auth, const char* token);
    bool (*refresh_jwt_token)(struct AuthenticationSystem* auth, const char* old_token, char* new_token);
    
    // Session management
    AuthSession* (*get_session)(struct AuthenticationSystem* auth, uint32_t session_id);
    bool (*is_session_valid)(struct AuthenticationSystem* auth, uint32_t session_id);
    bool (*update_session_activity)(struct AuthenticationSystem* auth, uint32_t session_id);
    uint32_t (*get_active_sessions)(struct AuthenticationSystem* auth);
    bool (*cleanup_expired_sessions)(struct AuthenticationSystem* auth);
    
    // Security
    bool (*change_password)(struct AuthenticationSystem* auth, uint32_t user_id, const char* old_password,
                           const char* new_password);
    bool (*reset_password)(struct AuthenticationSystem* auth, const char* username, const char* reset_token,
                          const char* new_password);
    bool (*lock_user)(struct AuthenticationSystem* auth, uint32_t user_id, uint32_t duration_minutes);
    bool (*unlock_user)(struct AuthenticationSystem* auth, uint32_t user_id);
    
    // Authorization
    bool (*has_permission)(struct AuthenticationSystem* auth, uint32_t user_id, const char* permission);
    bool (*has_role_or_higher)(struct AuthenticationSystem* auth, uint32_t user_id, UserRole min_role);
    
    // Statistics
    uint32_t (*get_user_count)(struct AuthenticationSystem* auth);
    uint32_t (*get_active_user_count)(struct AuthenticationSystem* auth);
    uint32_t (*get_failed_login_attempts)(struct AuthenticationSystem* auth);
    
    // Internal state
    UserCredentials* users;
    uint32_t user_count;
    uint32_t user_capacity;
    AuthSession* sessions;
    uint32_t session_count;
    uint32_t session_capacity;
    AuthConfig config;
    uint32_t next_user_id;
    uint32_t next_session_id;
    uint64_t last_cleanup;
    bool initialized;
} AuthenticationSystem;

// Global authentication functions (simplified interface)
bool auth_init(const AuthConfig* config);
bool auth_create_user(const char* username, const char* password, UserRole role, const char* permissions);
bool auth_authenticate(const char* username, const char* password, const NetAddress* client_address,
                      const char* user_agent);
bool auth_generate_token(uint32_t user_id, const char* username, UserRole role, const char* permissions,
                         char* token, size_t token_size);
TokenValidationResult auth_validate_token(const char* token, uint32_t* user_id, char* username, UserRole* role);
bool auth_logout(uint32_t session_id);
AuthSession* auth_get_session(uint32_t session_id);
bool auth_has_permission(uint32_t user_id, const char* permission);

// JWT utility functions
bool jwt_create_token(const char* header, const char* payload, const char* secret, char* token, size_t token_size);
bool jwt_parse_token(const char* token, char* header, size_t header_size, char* payload, size_t payload_size,
                    char* signature, size_t signature_size);
bool jwt_verify_signature(const char* header, const char* payload, const char* signature, const char* secret);
bool jwt_decode_payload(const char* payload, char* claims[JWT_CLAIM_COUNT][256]);
bool jwt_encode_payload(const char* claims[JWT_CLAIM_COUNT][256], char* payload, size_t payload_size);

// Password utilities
bool auth_hash_password(const char* password, const char* salt, char* hash, size_t hash_size);
bool auth_verify_password(const char* password, const char* hash, const char* salt);
bool auth_generate_salt(char* salt, size_t salt_size);
bool auth_generate_reset_token(char* token, size_t token_size);

// Factory functions
AuthenticationSystem* auth_create_system(void);
void auth_destroy_system(AuthenticationSystem* auth);

// Utility functions
const char* user_role_to_string(UserRole role);
const char* auth_method_to_string(AuthenticationMethod method);
const char* token_validation_to_string(TokenValidationResult result);
const char* jwt_claim_to_string(JwtClaimType claim);

bool auth_is_valid_username(const char* username);
bool auth_is_valid_password(const char* password);
bool auth_is_valid_email(const char* email);
UserRole auth_role_from_string(const char* role_string);

#endif // AUTHENTICATION_H
