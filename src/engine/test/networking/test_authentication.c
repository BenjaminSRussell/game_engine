#include "networking/security/authentication.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/test.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_005 - Unit Tests for JWT Authentication
// Features: Comprehensive test coverage for JWT tokens, user management, and security

// Test framework macros
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            log_error("ASSERTION FAILED: %s:%d - %s", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    ASSERT((expected) == (actual))

#define ASSERT_NE(expected, actual) \
    ASSERT((expected) != (actual))

#define ASSERT_STR_EQ(expected, actual) \
    ASSERT(string_equals((expected), (actual)))

#define ASSERT_NOT_NULL(ptr) \
    ASSERT((ptr) != NULL)

// Test data structures
typedef struct {
    AuthenticationSystem* auth_system;
    uint32_t test_user_id;
    uint32_t admin_user_id;
    uint32_t test_session_id;
    uint32_t admin_session_id;
} AuthTestContext;

// Helper function to create test context
static AuthTestContext* create_auth_test_context(void) {
    AuthTestContext* ctx = memory_alloc(sizeof(AuthTestContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(AuthTestContext));
    
    // Initialize authentication system
    ctx->auth_system = auth_create_system();
    ASSERT_NOT_NULL(ctx->auth_system);
    
    // Create test users
    ASSERT(auth_create_user(ctx->auth_system, "testuser", "password123", USER_ROLE_PLAYER, "basic"));
    ASSERT(auth_create_user(ctx->auth_system, "admin", "adminpass", USER_ROLE_ADMIN, "*"));
    
    // Authenticate users
    ctx->test_session_id = auth_authenticate(ctx->auth_system, "testuser", "password123", NULL, NULL);
    ASSERT(ctx->test_session_id != 0);
    
    ctx->admin_session_id = auth_authenticate(ctx->auth_system, "admin", "adminpass", NULL, NULL);
    ASSERT(ctx->admin_session_id != 0);
    
    // Get user IDs
    AuthSession* test_session = auth_get_session(ctx->auth_system, ctx->test_session_id);
    AuthSession* admin_session = auth_get_session(ctx->auth_system, ctx->admin_session_id);
    
    ASSERT_NOT_NULL(test_session);
    ASSERT_NOT_NULL(admin_session);
    
    ctx->test_user_id = test_session->user_id;
    ctx->admin_user_id = admin_session->user_id;
    
    return ctx;
}

// Helper function to destroy test context
static void destroy_auth_test_context(AuthTestContext* ctx) {
    if (!ctx) return;
    
    if (ctx->auth_system) {
        auth_destroy_system(ctx->auth_system);
    }
    
    memory_free(ctx);
}

// Test: Authentication System Initialization
static bool test_auth_system_init(void) {
    log_info("Testing authentication system initialization...");
    
    AuthenticationSystem* auth = auth_create_system();
    ASSERT_NOT_NULL(auth);
    
    // Check system state
    uint32_t user_count = auth_get_user_count(auth);
    ASSERT(user_count > 0); // Should have default admin user
    
    auth_destroy_system(auth);
    
    log_info("✅ Authentication system initialization test passed");
    return true;
}

// Test: User Creation
static bool test_user_creation(void) {
    log_info("Testing user creation...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create new user
    bool result = auth_create_user(ctx->auth_system, "newuser", "newpass123", USER_ROLE_MODERATOR, "moderate");
    ASSERT(result);
    
    // Verify user exists
    UserCredentials* user = auth_find_user_by_username(ctx->auth_system, "newuser");
    ASSERT_NOT_NULL(user);
    ASSERT_STR_EQ("newuser", user->username);
    ASSERT_EQ(USER_ROLE_MODERATOR, user->role);
    ASSERT_STR_EQ("moderate", user->permissions);
    ASSERT(user->is_active);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ User creation test passed");
    return true;
}

// Test: User Authentication
static bool test_user_authentication(void) {
    log_info("Testing user authentication...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Test valid authentication
    uint32_t session_id = auth_authenticate(ctx->auth_system, "testuser", "password123", NULL, NULL);
    ASSERT(session_id != 0);
    
    // Verify session
    AuthSession* session = auth_get_session(ctx->auth_system, session_id);
    ASSERT_NOT_NULL(session);
    ASSERT(session->is_authenticated);
    ASSERT(session->is_active);
    ASSERT_STR_EQ("testuser", session->username);
    ASSERT_EQ(USER_ROLE_PLAYER, session->role);
    
    // Test invalid password
    session_id = auth_authenticate(ctx->auth_system, "testuser", "wrongpass", NULL, NULL);
    ASSERT_EQ(0, session_id);
    
    // Test non-existent user
    session_id = auth_authenticate(ctx->auth_system, "nonexistent", "password", NULL, NULL);
    ASSERT_EQ(0, session_id);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ User authentication test passed");
    return true;
}

// Test: JWT Token Generation
static bool test_jwt_token_generation(void) {
    log_info("Testing JWT token generation...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Generate JWT token
    char token[2048];
    bool result = auth_generate_token(ctx->auth_system, ctx->test_user_id, "testuser", 
                                     USER_ROLE_PLAYER, "basic", token, sizeof(token));
    ASSERT(result);
    
    // Verify token format (should have 3 parts separated by dots)
    int dot_count = 0;
    for (char* p = token; *p; p++) {
        if (*p == '.') dot_count++;
    }
    ASSERT_EQ(2, dot_count); // Header.Payload.Signature
    
    // Verify token is not empty
    ASSERT(strlen(token) > 0);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ JWT token generation test passed");
    return true;
}

// Test: JWT Token Validation
static bool test_jwt_token_validation(void) {
    log_info("Testing JWT token validation...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Generate JWT token
    char token[2048];
    bool result = auth_generate_token(ctx->auth_system, ctx->test_user_id, "testuser", 
                                     USER_ROLE_PLAYER, "basic", token, sizeof(token));
    ASSERT(result);
    
    // Validate token
    uint32_t user_id;
    char username[64];
    UserRole role;
    
    TokenValidationResult validation_result = auth_validate_token(ctx->auth_system, token, 
                                                                &user_id, username, &role);
    ASSERT_EQ(TOKEN_VALID, validation_result);
    ASSERT_EQ(ctx->test_user_id, user_id);
    ASSERT_STR_EQ("testuser", username);
    ASSERT_EQ(USER_ROLE_PLAYER, role);
    
    // Test invalid token
    validation_result = auth_validate_token(ctx->auth_system, "invalid.token.here", 
                                           &user_id, username, &role);
    ASSERT_EQ(TOKEN_MALFORMED, validation_result);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ JWT token validation test passed");
    return true;
}

// Test: Session Management
static bool test_session_management(void) {
    log_info("Testing session management...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Verify session is valid
    bool result = auth_is_session_valid(ctx->auth_system, ctx->test_session_id);
    ASSERT(result);
    
    // Update session activity
    result = auth_update_session_activity(ctx->auth_system, ctx->test_session_id);
    ASSERT(result);
    
    // Logout user
    result = auth_logout(ctx->auth_system, ctx->test_session_id);
    ASSERT(result);
    
    // Verify session is no longer valid
    result = auth_is_session_valid(ctx->auth_system, ctx->test_session_id);
    ASSERT(!result);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ Session management test passed");
    return true;
}

// Test: Password Hashing
static bool test_password_hashing(void) {
    log_info("Testing password hashing...");
    
    const char* password = "testpassword123";
    char salt[32];
    char hash[128];
    
    // Generate salt
    bool result = auth_generate_salt(salt, sizeof(salt));
    ASSERT(result);
    
    // Hash password
    result = auth_hash_password(password, salt, hash, sizeof(hash));
    ASSERT(result);
    
    // Verify password
    result = auth_verify_password(password, hash, salt);
    ASSERT(result);
    
    // Verify wrong password fails
    result = auth_verify_password("wrongpassword", hash, salt);
    ASSERT(!result);
    
    log_info("✅ Password hashing test passed");
    return true;
}

// Test: Username Validation
static bool test_username_validation(void) {
    log_info("Testing username validation...");
    
    // Valid usernames
    ASSERT(auth_is_valid_username("user123"));
    ASSERT(auth_is_valid_username("test_user"));
    ASSERT(auth_is_valid_username("PlayerOne"));
    
    // Invalid usernames
    ASSERT(!auth_is_valid_username("")); // Empty
    ASSERT(!auth_is_valid_username("ab")); // Too short
    ASSERT(!auth_is_valid_username("user@domain")); // Invalid character
    ASSERT(!auth_is_valid_username("user with spaces")); // Spaces
    
    log_info("✅ Username validation test passed");
    return true;
}

// Test: Password Validation
static bool test_password_validation(void) {
    log_info("Testing password validation...");
    
    // Valid passwords
    ASSERT(auth_is_valid_password("Password123"));
    ASSERT(auth_is_valid_password("MySecurePass1"));
    ASSERT(auth_is_valid_password("ComplexP@ssw0rd"));
    
    // Invalid passwords
    ASSERT(!auth_is_valid_password("")); // Empty
    ASSERT(!auth_is_valid_password("short")); // Too short
    ASSERT(!auth_is_valid_password("alllowercase")); // No uppercase
    ASSERT(!auth_is_valid_password("ALLUPPERCASE")); // No lowercase
    ASSERT(!auth_is_valid_password("NoDigitsHere")); // No digits
    
    log_info("✅ Password validation test passed");
    return true;
}

// Test: Role-Based Access Control
static bool test_role_based_access(void) {
    log_info("Testing role-based access control...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Test permission checking
    bool result = auth_has_permission(ctx->auth_system, ctx->test_user_id, "basic");
    ASSERT(result);
    
    result = auth_has_permission(ctx->auth_system, ctx->test_user_id, "admin");
    ASSERT(!result); // Player shouldn't have admin permission
    
    result = auth_has_permission(ctx->auth_system, ctx->admin_user_id, "admin");
    ASSERT(result); // Admin should have admin permission
    
    // Test role hierarchy
    result = auth_has_role_or_higher(ctx->auth_system, ctx->test_user_id, USER_ROLE_PLAYER);
    ASSERT(result);
    
    result = auth_has_role_or_higher(ctx->auth_system, ctx->test_user_id, USER_ROLE_ADMIN);
    ASSERT(!result); // Player is not admin or higher
    
    result = auth_has_role_or_higher(ctx->auth_system, ctx->admin_user_id, USER_ROLE_MODERATOR);
    ASSERT(result); // Admin is higher than moderator
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ Role-based access control test passed");
    return true;
}

// Test: User Update
static bool test_user_update(void) {
    log_info("Testing user update...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Update user
    bool result = auth_update_user(ctx->auth_system, ctx->test_user_id, "updateduser", 
                                  USER_ROLE_MODERATOR, "moderate,advanced");
    ASSERT(result);
    
    // Verify update
    UserCredentials* user = auth_find_user_by_username(ctx->auth_system, "updateduser");
    ASSERT_NOT_NULL(user);
    ASSERT_STR_EQ("updateduser", user->username);
    ASSERT_EQ(USER_ROLE_MODERATOR, user->role);
    ASSERT_STR_EQ("moderate,advanced", user->permissions);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ User update test passed");
    return true;
}

// Test: User Deletion
static bool test_user_deletion(void) {
    log_info("Testing user deletion...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create temporary user
    ASSERT(auth_create_user(ctx->auth_system, "tempuser", "temppass", USER_ROLE_PLAYER, "basic"));
    
    UserCredentials* user = auth_find_user_by_username(ctx->auth_system, "tempuser");
    ASSERT_NOT_NULL(user);
    uint32_t temp_user_id = user->user_id;
    
    // Delete user
    bool result = auth_delete_user(ctx->auth_system, temp_user_id);
    ASSERT(result);
    
    // Verify user no longer exists
    user = auth_find_user_by_username(ctx->auth_system, "tempuser");
    ASSERT(user == NULL);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ User deletion test passed");
    return true;
}

// Test: Session Cleanup
static bool test_session_cleanup(void) {
    log_info("Testing session cleanup...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Get initial session count
    uint32_t initial_sessions = auth_get_active_sessions(ctx->auth_system);
    
    // Create additional sessions
    uint32_t session1 = auth_authenticate(ctx->auth_system, "testuser", "password123", NULL, NULL);
    uint32_t session2 = auth_authenticate(ctx->auth_system, "admin", "adminpass", NULL, NULL);
    
    ASSERT(session1 != 0);
    ASSERT(session2 != 0);
    
    // Verify session count increased
    uint32_t current_sessions = auth_get_active_sessions(ctx->auth_system);
    ASSERT(current_sessions > initial_sessions);
    
    // Cleanup expired sessions (none should be expired yet)
    bool result = auth_cleanup_expired_sessions(ctx->auth_system);
    ASSERT(result);
    
    // Logout sessions
    auth_logout(ctx->auth_system, session1);
    auth_logout(ctx->auth_system, session2);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ Session cleanup test passed");
    return true;
}

// Test: Failed Login Tracking
static bool test_failed_login_tracking(void) {
    log_info("Testing failed login tracking...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Attempt multiple failed logins
    for (int i = 0; i < 3; i++) {
        uint32_t session_id = auth_authenticate(ctx->auth_system, "testuser", "wrongpass", NULL, NULL);
        ASSERT_EQ(0, session_id);
    }
    
    // Check failed login attempts
    uint32_t failed_attempts = auth_get_failed_login_attempts(ctx->auth_system);
    ASSERT(failed_attempts >= 3);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ Failed login tracking test passed");
    return true;
}

// Test: Token Refresh
static bool test_token_refresh(void) {
    log_info("Testing token refresh...");
    
    AuthTestContext* ctx = create_auth_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Generate original token
    char original_token[2048];
    bool result = auth_generate_token(ctx->auth_system, ctx->test_user_id, "testuser", 
                                     USER_ROLE_PLAYER, "basic", original_token, sizeof(original_token));
    ASSERT(result);
    
    // Generate refresh token (simplified)
    char refresh_token[256];
    snprintf(refresh_token, sizeof(refresh_token), "refresh_%u", ctx->test_user_id);
    
    // Refresh token
    char new_token[2048];
    result = auth_refresh_token(ctx->auth_system, refresh_token, new_token, sizeof(new_token));
    ASSERT(result);
    
    // Verify new token is different
    ASSERT(!string_equals(original_token, new_token));
    
    // Validate new token
    uint32_t user_id;
    char username[64];
    UserRole role;
    
    TokenValidationResult validation_result = auth_validate_token(ctx->auth_system, new_token, 
                                                                &user_id, username, &role);
    ASSERT_EQ(TOKEN_VALID, validation_result);
    ASSERT_EQ(ctx->test_user_id, user_id);
    
    destroy_auth_test_context(ctx);
    
    log_info("✅ Token refresh test passed");
    return true;
}

// Main test runner
bool run_authentication_tests(void) {
    log_info("=== Starting JWT Authentication Unit Tests ===");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_auth_system_init();
    all_passed &= test_user_creation();
    all_passed &= test_user_authentication();
    all_passed &= test_jwt_token_generation();
    all_passed &= test_jwt_token_validation();
    all_passed &= test_session_management();
    all_passed &= test_password_hashing();
    all_passed &= test_username_validation();
    all_passed &= test_password_validation();
    all_passed &= test_role_based_access();
    all_passed &= test_user_update();
    all_passed &= test_user_deletion();
    all_passed &= test_session_cleanup();
    all_passed &= test_failed_login_tracking();
    all_passed &= test_token_refresh();
    
    if (all_passed) {
        log_info("🎉 All authentication tests passed!");
    } else {
        log_error("❌ Some authentication tests failed!");
    }
    
    log_info("=== JWT Authentication Unit Tests Complete ===");
    return all_passed;
}
