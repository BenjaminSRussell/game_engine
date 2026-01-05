#include "networking/matchmaking/lobby_system.h"
#include "networking/security/authentication.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/time.h"
#include "core/test.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_001 - Unit Tests for Lobby System
// Features: Comprehensive test coverage for all lobby functionality

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

#define ASSERT_STR_EQ(expected, actual) \
    ASSERT(string_equals((expected), (actual)))

#define ASSERT_NOT_NULL(ptr) \
    ASSERT((ptr) != NULL)

// Test data structures
typedef struct {
    LobbySystem* lobby_system;
    AuthenticationSystem* auth_system;
    uint32_t test_user_id;
    uint32_t test_session_id;
} TestContext;

// Helper function to create test context
static TestContext* create_test_context(void) {
    TestContext* ctx = memory_alloc(sizeof(TestContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(TestContext));
    
    // Initialize lobby system
    ctx->lobby_system = lobby_create_system();
    ASSERT_NOT_NULL(ctx->lobby_system);
    
    // Initialize authentication system
    ctx->auth_system = auth_create_system();
    ASSERT_NOT_NULL(ctx->auth_system);
    
    // Create test user
    ASSERT(auth_create_user(ctx->auth_system, "testuser", "password123", USER_ROLE_PLAYER, "*"));
    
    // Authenticate test user
    ctx->test_session_id = auth_authenticate(ctx->auth_system, "testuser", "password123", NULL, NULL);
    ASSERT(ctx->test_session_id != 0);
    
    AuthSession* session = auth_get_session(ctx->auth_system, ctx->test_session_id);
    ASSERT_NOT_NULL(session);
    ctx->test_user_id = session->user_id;
    
    return ctx;
}

// Helper function to destroy test context
static void destroy_test_context(TestContext* ctx) {
    if (!ctx) return;
    
    if (ctx->lobby_system) {
        lobby_destroy_system(ctx->lobby_system);
    }
    
    if (ctx->auth_system) {
        auth_destroy_system(ctx->auth_system);
    }
    
    memory_free(ctx);
}

// Test: Lobby System Initialization
static bool test_lobby_system_init(void) {
    log_info("Testing lobby system initialization...");
    
    LobbySystem* system = lobby_create_system();
    ASSERT_NOT_NULL(system);
    
    // Check system state
    uint32_t lobby_count = lobby_get_lobby_count(system);
    ASSERT_EQ(0, lobby_count);
    
    lobby_destroy_system(system);
    
    log_info("✅ Lobby system initialization test passed");
    return true;
}

// Test: Create Lobby
static bool test_create_lobby(void) {
    log_info("Testing lobby creation...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 8;
    create_info.is_private = false;
    create_info.has_password = false;
    string_copy(create_info.game_mode, "survival", sizeof(create_info.game_mode));
    string_copy(create_info.map, "world", sizeof(create_info.map));
    string_copy(create_info.region, "us-east", sizeof(create_info.region));
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Verify lobby was created
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_STR_EQ("Test Lobby", lobby_info.name);
    ASSERT_EQ(8, lobby_info.max_players);
    ASSERT_EQ(1, lobby_info.current_players);
    ASSERT_EQ(ctx->test_user_id, lobby_info.host_id);
    ASSERT_STR_EQ("survival", lobby_info.game_mode);
    
    destroy_test_context(ctx);
    
    log_info("✅ Lobby creation test passed");
    return true;
}

// Test: Join Lobby
static bool test_join_lobby(void) {
    log_info("Testing lobby join...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    create_info.is_private = false;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Create second user
    ASSERT(auth_create_user(ctx->auth_system, "testuser2", "password123", USER_ROLE_PLAYER, "*"));
    uint32_t session2 = auth_authenticate(ctx->auth_system, "testuser2", "password123", NULL, NULL);
    ASSERT(session2 != 0);
    
    AuthSession* session2_info = auth_get_session(ctx->auth_system, session2);
    ASSERT_NOT_NULL(session2_info);
    
    // Join lobby
    LobbyJoinResult result = lobby_join_lobby(ctx->lobby_system, session2_info->user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, result);
    
    // Verify player joined
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(2, lobby_info.current_players);
    
    destroy_test_context(ctx);
    
    log_info("✅ Lobby join test passed");
    return true;
}

// Test: Leave Lobby
static bool test_leave_lobby(void) {
    log_info("Testing lobby leave...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Leave lobby
    bool result = lobby_leave_lobby(ctx->lobby_system, ctx->test_user_id, lobby_id);
    ASSERT(result);
    
    // Verify lobby was destroyed (host left)
    LobbyInfo lobby_info;
    ASSERT(!lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    
    destroy_test_context(ctx);
    
    log_info("✅ Lobby leave test passed");
    return true;
}

// Test: Kick Player
static bool test_kick_player(void) {
    log_info("Testing player kick...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Create second user
    ASSERT(auth_create_user(ctx->auth_system, "testuser2", "password123", USER_ROLE_PLAYER, "*"));
    uint32_t session2 = auth_authenticate(ctx->auth_system, "testuser2", "password123", NULL, NULL);
    ASSERT(session2 != 0);
    
    AuthSession* session2_info = auth_get_session(ctx->auth_system, session2);
    ASSERT_NOT_NULL(session2_info);
    
    // Join lobby
    lobby_join_lobby(ctx->lobby_system, session2_info->user_id, lobby_id, NULL);
    
    // Kick player
    bool result = lobby_kick_player(ctx->lobby_system, ctx->test_user_id, lobby_id, session2_info->user_id);
    ASSERT(result);
    
    // Verify player was removed
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(1, lobby_info.current_players);
    
    destroy_test_context(ctx);
    
    log_info("✅ Player kick test passed");
    return true;
}

// Test: Promote to Host
static bool test_promote_host(void) {
    log_info("Testing host promotion...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Create second user
    ASSERT(auth_create_user(ctx->auth_system, "testuser2", "password123", USER_ROLE_PLAYER, "*"));
    uint32_t session2 = auth_authenticate(ctx->auth_system, "testuser2", "password123", NULL, NULL);
    ASSERT(session2 != 0);
    
    AuthSession* session2_info = auth_get_session(ctx->auth_system, session2);
    ASSERT_NOT_NULL(session2_info);
    
    // Join lobby
    lobby_join_lobby(ctx->lobby_system, session2_info->user_id, lobby_id, NULL);
    
    // Promote to host
    bool result = lobby_promote_to_host(ctx->lobby_system, ctx->test_user_id, lobby_id, session2_info->user_id);
    ASSERT(result);
    
    // Verify host changed
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(session2_info->user_id, lobby_info.host_id);
    
    destroy_test_context(ctx);
    
    log_info("✅ Host promotion test passed");
    return true;
}

// Test: Find Lobbies
static bool test_find_lobbies(void) {
    log_info("Testing lobby search...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create multiple lobbies
    for (int i = 0; i < 3; i++) {
        LobbyCreateInfo create_info = {0};
        snprintf(create_info.name, sizeof(create_info.name), "Test Lobby %d", i);
        create_info.max_players = 8;
        create_info.is_private = false;
        string_copy(create_info.game_mode, i % 2 == 0 ? "survival" : "creative", sizeof(create_info.game_mode));
        
        uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
        ASSERT(lobby_id != 0);
    }
    
    // Search for lobbies
    LobbySearchCriteria criteria = {0};
    string_copy(criteria.game_mode, "survival", sizeof(criteria.game_mode));
    criteria.include_empty = true;
    criteria.include_full = false;
    
    LobbyInfo results[10];
    uint32_t found = lobby_find_lobbies(ctx->lobby_system, &criteria, results, 10);
    ASSERT(found >= 1);
    
    // Verify results
    for (uint32_t i = 0; i < found; i++) {
        ASSERT_STR_EQ("survival", results[i].game_mode);
    }
    
    destroy_test_context(ctx);
    
    log_info("✅ Lobby search test passed");
    return true;
}

// Test: Lobby Events
static bool test_lobby_events(void) {
    log_info("Testing lobby events...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Check for events
    LobbyEvent events[10];
    uint32_t event_count = lobby_get_events(ctx->lobby_system, lobby_id, events, 10);
    ASSERT(event_count > 0);
    
    // Verify lobby created event
    bool found_create_event = false;
    for (uint32_t i = 0; i < event_count; i++) {
        if (events[i].event_type == LOBBY_EVENT_CREATED) {
            found_create_event = true;
            break;
        }
    }
    ASSERT(found_create_event);
    
    destroy_test_context(ctx);
    
    log_info("✅ Lobby events test passed");
    return true;
}

// Test: Password Protection
static bool test_password_protection(void) {
    log_info("Testing password protection...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create private lobby with password
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Private Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    create_info.is_private = true;
    create_info.has_password = true;
    string_copy(create_info.password, "secret123", sizeof(create_info.password));
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Create second user
    ASSERT(auth_create_user(ctx->auth_system, "testuser2", "password123", USER_ROLE_PLAYER, "*"));
    uint32_t session2 = auth_authenticate(ctx->auth_system, "testuser2", "password123", NULL, NULL);
    ASSERT(session2 != 0);
    
    AuthSession* session2_info = auth_get_session(ctx->auth_system, session2);
    ASSERT_NOT_NULL(session2_info);
    
    // Try to join with wrong password
    LobbyJoinResult result = lobby_join_lobby(ctx->lobby_system, session2_info->user_id, lobby_id, "wrongpass");
    ASSERT_EQ(LOBBY_JOIN_INVALID_PASSWORD, result);
    
    // Join with correct password
    result = lobby_join_lobby(ctx->lobby_system, session2_info->user_id, lobby_id, "secret123");
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, result);
    
    destroy_test_context(ctx);
    
    log_info("✅ Password protection test passed");
    return true;
}

// Test: Ready Status
static bool test_ready_status(void) {
    log_info("Testing ready status...");
    
    TestContext* ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->test_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Set ready status
    bool result = lobby_set_ready_status(ctx->lobby_system, ctx->test_user_id, lobby_id, true);
    ASSERT(result);
    
    // Verify ready status
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(1, lobby_info.ready_players);
    
    destroy_test_context(ctx);
    
    log_info("✅ Ready status test passed");
    return true;
}

// Main test runner
bool run_lobby_system_tests(void) {
    log_info("=== Starting Lobby System Unit Tests ===");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_lobby_system_init();
    all_passed &= test_create_lobby();
    all_passed &= test_join_lobby();
    all_passed &= test_leave_lobby();
    all_passed &= test_kick_player();
    all_passed &= test_promote_host();
    all_passed &= test_find_lobbies();
    all_passed &= test_lobby_events();
    all_passed &= test_password_protection();
    all_passed &= test_ready_status();
    
    if (all_passed) {
        log_info("🎉 All lobby system tests passed!");
    } else {
        log_error("❌ Some lobby system tests failed!");
    }
    
    log_info("=== Lobby System Unit Tests Complete ===");
    return all_passed;
}
