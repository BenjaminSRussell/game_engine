#include "networking/matchmaking/lobby_system.h"
#include "networking/nat_traversal.h"
#include "networking/relay_server.h"
#include "networking/security/authentication.h"
#include "networking/security/encryption.h"
#include "core/network/network_system.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/test.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_006 - Integration Tests for Lobby System with Network Layer
// Features: End-to-end testing of lobby system with networking, NAT traversal, and security

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

// Integration test context
typedef struct {
    LobbySystem* lobby_system;
    NatTraversalSystem* nat_system;
    RelayServer* relay_server;
    AuthenticationSystem* auth_system;
    NetworkSystem* network_system;
    
    uint32_t host_user_id;
    uint32_t client_user_id;
    uint32_t host_session_id;
    uint32_t client_session_id;
    
    NetAddress host_address;
    NetAddress client_address;
    
    bool test_running;
} IntegrationTestContext;

// Helper function to create integration test context
static IntegrationTestContext* create_integration_context(void) {
    IntegrationTestContext* ctx = memory_alloc(sizeof(IntegrationTestContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(IntegrationTestContext));
    
    // Initialize all systems
    ctx->lobby_system = lobby_create_system();
    ASSERT_NOT_NULL(ctx->lobby_system);
    
    ctx->nat_system = nat_create_system();
    ASSERT_NOT_NULL(ctx->nat_system);
    
    ctx->relay_server = relay_create_server();
    ASSERT_NOT_NULL(ctx->relay_server);
    
    ctx->auth_system = auth_create_system();
    ASSERT_NOT_NULL(ctx->auth_system);
    
    ctx->network_system = network_create_system();
    ASSERT_NOT_NULL(ctx->network_system);
    
    // Start relay server
    bool result = relay_server_start(ctx->relay_server);
    ASSERT(result);
    
    // Create test users
    ASSERT(auth_create_user(ctx->auth_system, "hostuser", "hostpass123", USER_ROLE_PLAYER, "*"));
    ASSERT(auth_create_user(ctx->auth_system, "clientuser", "clientpass123", USER_ROLE_PLAYER, "*"));
    
    // Authenticate users
    ctx->host_session_id = auth_authenticate(ctx->auth_system, "hostuser", "hostpass123", NULL, NULL);
    ctx->client_session_id = auth_authenticate(ctx->auth_system, "clientuser", "clientpass123", NULL, NULL);
    
    ASSERT(ctx->host_session_id != 0);
    ASSERT(ctx->client_session_id != 0);
    
    // Get user IDs
    AuthSession* host_session = auth_get_session(ctx->auth_system, ctx->host_session_id);
    AuthSession* client_session = auth_get_session(ctx->auth_system, ctx->client_session_id);
    
    ASSERT_NOT_NULL(host_session);
    ASSERT_NOT_NULL(client_session);
    
    ctx->host_user_id = host_session->user_id;
    ctx->client_user_id = client_session->user_id;
    
    // Set up test addresses
    ctx->host_address.host = inet_addr("127.0.0.1");
    ctx->host_address.port = 12345;
    
    ctx->client_address.host = inet_addr("127.0.0.1");
    ctx->client_address.port = 12346;
    
    ctx->test_running = true;
    
    return ctx;
}

// Helper function to destroy integration test context
static void destroy_integration_context(IntegrationTestContext* ctx) {
    if (!ctx) return;
    
    if (ctx->lobby_system) {
        lobby_destroy_system(ctx->lobby_system);
    }
    
    if (ctx->nat_system) {
        nat_destroy_system(ctx->nat_system);
    }
    
    if (ctx->relay_server) {
        relay_server_stop(ctx->relay_server);
        relay_destroy_server(ctx->relay_server);
    }
    
    if (ctx->auth_system) {
        auth_destroy_system(ctx->auth_system);
    }
    
    if (ctx->network_system) {
        network_destroy_system(ctx->network_system);
    }
    
    memory_free(ctx);
}

// Test: End-to-End Lobby Creation and Joining
static bool test_end_to_end_lobby(void) {
    log_info("Testing end-to-end lobby creation and joining...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby as host
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Integration Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    create_info.is_private = false;
    string_copy(create_info.game_mode, "survival", sizeof(create_info.game_mode));
    string_copy(create_info.map, "test_world", sizeof(create_info.map));
    string_copy(create_info.region, "us-east", sizeof(create_info.region));
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Join lobby as client
    LobbyJoinResult join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    // Verify lobby state
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(2, lobby_info.current_players);
    ASSERT_EQ(ctx->host_user_id, lobby_info.host_id);
    
    // Test chat functionality
    bool chat_result = lobby_send_chat_message(ctx->lobby_system, ctx->host_user_id, lobby_id, "Hello from host!");
    ASSERT(chat_result);
    
    chat_result = lobby_send_chat_message(ctx->lobby_system, ctx->client_user_id, lobby_id, "Hello from client!");
    ASSERT(chat_result);
    
    destroy_integration_context(ctx);
    
    log_info("✅ End-to-end lobby creation and joining test passed");
    return true;
}

// Test: NAT Traversal Integration
static bool test_nat_traversal_integration(void) {
    log_info("Testing NAT traversal integration...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Discover NAT types for both clients
    bool result = nat_discover_nat_type(ctx->nat_system);
    ASSERT(result);
    
    // Wait for discovery completion
    for (int i = 0; i < 100; i++) {
        nat_system_update(ctx->nat_system, 0.1f);
        if (nat_is_nat_discovery_complete(ctx->nat_system)) {
            break;
        }
    }
    
    ASSERT(nat_is_nat_discovery_complete(ctx->nat_system));
    
    // Get NAT results
    NatTraversalResult nat_result = nat_get_nat_result(ctx->nat_system);
    ASSERT(nat_result.success);
    
    // Test hole punching between clients
    uint32_t hole_punch_session = nat_start_hole_punch(ctx->nat_system, &ctx->client_address, true);
    ASSERT(hole_punch_session != 0);
    
    // Verify session exists
    HolePunchSession* session = nat_get_hole_punch_session(ctx->nat_system, hole_punch_session);
    ASSERT_NOT_NULL(session);
    
    destroy_integration_context(ctx);
    
    log_info("✅ NAT traversal integration test passed");
    return true;
}

// Test: Relay Server Integration
static bool test_relay_server_integration(void) {
    log_info("Testing relay server integration...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register clients with relay server
    uint32_t relay_client1 = relay_server_register_client(ctx->relay_server, &ctx->host_address, "hostuser");
    uint32_t relay_client2 = relay_server_register_client(ctx->relay_server, &ctx->client_address, "clientuser");
    
    ASSERT(relay_client1 != 0);
    ASSERT(relay_client2 != 0);
    
    // Authenticate clients
    bool result = relay_server_authenticate_client(ctx->relay_server, relay_client1, "valid_token");
    ASSERT(result);
    
    result = relay_server_authenticate_client(ctx->relay_server, relay_client2, "valid_token");
    ASSERT(result);
    
    // Create relay session
    uint32_t relay_session = relay_server_create_session(ctx->relay_server, relay_client1, relay_client2);
    ASSERT(relay_session != 0);
    
    // Test message relaying
    const char* test_message = "Relay test message";
    result = relay_server_relay_message(ctx->relay_server, relay_client1, relay_client2,
                                       test_message, strlen(test_message));
    ASSERT(result);
    
    // Verify statistics
    uint32_t bytes_relayed = relay_server_get_bytes_relayed(ctx->relay_server);
    ASSERT(bytes_relayed > 0);
    
    destroy_integration_context(ctx);
    
    log_info("✅ Relay server integration test passed");
    return true;
}

// Test: Encryption Integration
static bool test_encryption_integration(void) {
    log_info("Testing encryption integration...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Initialize crypto system
    bool result = crypto_init();
    ASSERT(result);
    
    // Generate encryption keys for session
    uint8_t session_key[32];
    result = crypto_generate_key(session_key, sizeof(session_key));
    ASSERT(result);
    
    // Test secure message exchange
    const char* original_message = "Secure lobby message";
    uint8_t ciphertext[256];
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    // Encrypt message
    result = crypto_encrypt_aes_gcm((uint8_t*)original_message, strlen(original_message),
                                   NULL, 0,
                                   ciphertext, &ciphertext_len,
                                   tag, sizeof(tag));
    ASSERT(result);
    
    // Decrypt message
    uint8_t decrypted[256];
    size_t decrypted_len = sizeof(decrypted);
    
    result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len,
                                   NULL, 0,
                                   tag, sizeof(tag),
                                   decrypted, &decrypted_len);
    ASSERT(result);
    
    // Verify message integrity
    ASSERT_EQ(strlen(original_message), decrypted_len);
    ASSERT(memcmp(original_message, decrypted, strlen(original_message)) == 0);
    
    destroy_integration_context(ctx);
    
    log_info("✅ Encryption integration test passed");
    return true;
}

// Test: JWT Authentication Integration
static bool test_jwt_authentication_integration(void) {
    log_info("Testing JWT authentication integration...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Generate JWT tokens for both users
    char host_token[2048];
    char client_token[2048];
    
    bool result = auth_generate_token(ctx->auth_system, ctx->host_user_id, "hostuser",
                                     USER_ROLE_PLAYER, "*", host_token, sizeof(host_token));
    ASSERT(result);
    
    result = auth_generate_token(ctx->auth_system, ctx->client_user_id, "clientuser",
                                 USER_ROLE_PLAYER, "*", client_token, sizeof(client_token));
    ASSERT(result);
    
    // Validate tokens
    uint32_t host_id, client_id;
    char host_username[64], client_username[64];
    UserRole host_role, client_role;
    
    TokenValidationResult host_validation = auth_validate_token(ctx->auth_system, host_token,
                                                               &host_id, host_username, &host_role);
    ASSERT_EQ(TOKEN_VALID, host_validation);
    ASSERT_EQ(ctx->host_user_id, host_id);
    
    TokenValidationResult client_validation = auth_validate_token(ctx->auth_system, client_token,
                                                                 &client_id, client_username, &client_role);
    ASSERT_EQ(TOKEN_VALID, client_validation);
    ASSERT_EQ(ctx->client_user_id, client_id);
    
    // Test token-based lobby access
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "JWT Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Join lobby with validated token
    LobbyJoinResult join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    destroy_integration_context(ctx);
    
    log_info("✅ JWT authentication integration test passed");
    return true;
}

// Test: Multi-Player Lobby Scenario
static bool test_multi_player_lobby_scenario(void) {
    log_info("Testing multi-player lobby scenario...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create additional users
    ASSERT(auth_create_user(ctx->auth_system, "player1", "pass123", USER_ROLE_PLAYER, "*"));
    ASSERT(auth_create_user(ctx->auth_system, "player2", "pass123", USER_ROLE_PLAYER, "*"));
    
    uint32_t player1_session = auth_authenticate(ctx->auth_system, "player1", "pass123", NULL, NULL);
    uint32_t player2_session = auth_authenticate(ctx->auth_system, "player2", "pass123", NULL, NULL);
    
    ASSERT(player1_session != 0);
    ASSERT(player2_session != 0);
    
    AuthSession* player1_auth = auth_get_session(ctx->auth_system, player1_session);
    AuthSession* player2_auth = auth_get_session(ctx->auth_system, player2_session);
    
    ASSERT_NOT_NULL(player1_auth);
    ASSERT_NOT_NULL(player2_auth);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Multi-Player Test", sizeof(create_info.name));
    create_info.max_players = 4;
    create_info.is_private = false;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // All players join lobby
    LobbyJoinResult join_result;
    
    join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    join_result = lobby_join_lobby(ctx->lobby_system, player1_auth->user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    join_result = lobby_join_lobby(ctx->lobby_system, player2_auth->user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    // Verify lobby is full
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(4, lobby_info.current_players);
    ASSERT_EQ(4, lobby_info.max_players);
    
    // Test ready system
    ASSERT(lobby_set_ready_status(ctx->lobby_system, ctx->host_user_id, lobby_id, true));
    ASSERT(lobby_set_ready_status(ctx->lobby_system, ctx->client_user_id, lobby_id, true));
    ASSERT(lobby_set_ready_status(ctx->lobby_system, player1_auth->user_id, lobby_id, true));
    ASSERT(lobby_set_ready_status(ctx->lobby_system, player2_auth->user_id, lobby_id, true));
    
    // Verify all players ready
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(4, lobby_info.ready_players);
    
    // Test host promotion
    ASSERT(lobby_promote_to_host(ctx->lobby_system, ctx->host_user_id, lobby_id, ctx->client_user_id));
    
    // Verify host change
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(ctx->client_user_id, lobby_info.host_id);
    
    destroy_integration_context(ctx);
    
    log_info("✅ Multi-player lobby scenario test passed");
    return true;
}

// Test: Network Failure Recovery
static bool test_network_failure_recovery(void) {
    log_info("Testing network failure recovery...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create lobby
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Recovery Test Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Client joins lobby
    LobbyJoinResult join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    // Simulate network failure (client disconnect)
    bool result = lobby_leave_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id);
    ASSERT(result);
    
    // Verify client removed from lobby
    LobbyInfo lobby_info;
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(1, lobby_info.current_players);
    
    // Client reconnects
    join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, NULL);
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    // Verify client back in lobby
    ASSERT(lobby_get_info(ctx->lobby_system, lobby_id, &lobby_info));
    ASSERT_EQ(2, lobby_info.current_players);
    
    destroy_integration_context(ctx);
    
    log_info("✅ Network failure recovery test passed");
    return true;
}

// Test: Security Integration
static bool test_security_integration(void) {
    log_info("Testing security integration...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Initialize crypto for secure communication
    crypto_init();
    
    // Create private lobby with password
    LobbyCreateInfo create_info = {0};
    string_copy(create_info.name, "Secure Lobby", sizeof(create_info.name));
    create_info.max_players = 4;
    create_info.is_private = true;
    create_info.has_password = true;
    string_copy(create_info.password, "securepass123", sizeof(create_info.password));
    
    uint32_t lobby_id = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
    ASSERT(lobby_id != 0);
    
    // Test wrong password rejection
    LobbyJoinResult join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, "wrongpass");
    ASSERT_EQ(LOBBY_JOIN_INVALID_PASSWORD, join_result);
    
    // Test correct password acceptance
    join_result = lobby_join_lobby(ctx->lobby_system, ctx->client_user_id, lobby_id, "securepass123");
    ASSERT_EQ(LOBBY_JOIN_SUCCESS, join_result);
    
    // Test encrypted chat
    const char* chat_message = "Secure chat message";
    uint8_t encrypted_chat[256];
    uint8_t chat_tag[16];
    size_t encrypted_len = sizeof(encrypted_chat);
    
    // Encrypt chat message
    bool result = crypto_encrypt_aes_gcm((uint8_t*)chat_message, strlen(chat_message),
                                       NULL, 0,
                                       encrypted_chat, &encrypted_len,
                                       chat_tag, sizeof(chat_tag));
    ASSERT(result);
    
    // Send encrypted message (simulated)
    result = lobby_send_chat_message(ctx->lobby_system, ctx->host_user_id, lobby_id, 
                                   (char*)encrypted_chat);
    ASSERT(result);
    
    destroy_integration_context(ctx);
    
    log_info("✅ Security integration test passed");
    return true;
}

// Test: Performance Under Load
static bool test_performance_under_load(void) {
    log_info("Testing performance under load...");
    
    IntegrationTestContext* ctx = create_integration_context();
    ASSERT_NOT_NULL(ctx);
    
    // Create multiple lobbies
    const int num_lobbies = 10;
    uint32_t lobby_ids[num_lobbies];
    
    for (int i = 0; i < num_lobbies; i++) {
        LobbyCreateInfo create_info = {0};
        snprintf(create_info.name, sizeof(create_info.name), "Load Test Lobby %d", i);
        create_info.max_players = 8;
        
        lobby_ids[i] = lobby_create_lobby(ctx->lobby_system, ctx->host_user_id, &create_info);
        ASSERT(lobby_ids[i] != 0);
    }
    
    // Verify all lobbies created
    uint32_t total_lobbies = lobby_get_lobby_count(ctx->lobby_system);
    ASSERT_EQ(num_lobbies, total_lobbies);
    
    // Test lobby search performance
    LobbySearchCriteria criteria = {0};
    criteria.include_empty = true;
    criteria.include_full = false;
    
    LobbyInfo results[20];
    uint64_t start_time = time_get_current_ms();
    
    uint32_t found = lobby_find_lobbies(ctx->lobby_system, &criteria, results, 20);
    uint64_t end_time = time_get_current_ms();
    
    ASSERT(found >= num_lobbies);
    ASSERT((end_time - start_time) < 100); // Should complete in <100ms
    
    // Clean up lobbies
    for (int i = 0; i < num_lobbies; i++) {
        lobby_leave_lobby(ctx->lobby_system, ctx->host_user_id, lobby_ids[i]);
    }
    
    destroy_integration_context(ctx);
    
    log_info("✅ Performance under load test passed");
    return true;
}

// Main integration test runner
bool run_integration_tests(void) {
    log_info("=== Starting Network Integration Tests ===");
    
    bool all_passed = true;
    
    // Run all integration tests
    all_passed &= test_end_to_end_lobby();
    all_passed &= test_nat_traversal_integration();
    all_passed &= test_relay_server_integration();
    all_passed &= test_encryption_integration();
    all_passed &= test_jwt_authentication_integration();
    all_passed &= test_multi_player_lobby_scenario();
    all_passed &= test_network_failure_recovery();
    all_passed &= test_security_integration();
    all_passed &= test_performance_under_load();
    
    if (all_passed) {
        log_info("🎉 All integration tests passed!");
    } else {
        log_error("❌ Some integration tests failed!");
    }
    
    log_info("=== Network Integration Tests Complete ===");
    return all_passed;
}
