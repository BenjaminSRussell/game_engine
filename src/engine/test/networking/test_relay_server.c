#include "networking/relay_server.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/test.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_003 - Unit Tests for Relay Server Protocol
// Features: Comprehensive test coverage for relay server and client functionality

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
    RelayServer* server;
    RelayClient* client1;
    RelayClient* client2;
    NetAddress test_address1;
    NetAddress test_address2;
} RelayTestContext;

// Helper function to create test context
static RelayTestContext* create_relay_test_context(void) {
    RelayTestContext* ctx = memory_alloc(sizeof(RelayTestContext));
    if (!ctx) return NULL;
    
    memset(ctx, 0, sizeof(RelayTestContext));
    
    // Initialize relay server
    ctx->server = relay_create_server();
    ASSERT_NOT_NULL(ctx->server);
    
    // Start server
    bool result = relay_server_start(ctx->server);
    ASSERT(result);
    
    // Initialize relay clients
    ctx->client1 = relay_create_client();
    ASSERT_NOT_NULL(ctx->client1);
    
    ctx->client2 = relay_create_client();
    ASSERT_NOT_NULL(ctx->client2);
    
    // Set up test addresses
    ctx->test_address1.host = inet_addr("127.0.0.1");
    ctx->test_address1.port = 12345;
    
    ctx->test_address2.host = inet_addr("127.0.0.1");
    ctx->test_address2.port = 12346;
    
    return ctx;
}

// Helper function to destroy test context
static void destroy_relay_test_context(RelayTestContext* ctx) {
    if (!ctx) return;
    
    if (ctx->client1) {
        relay_destroy_client(ctx->client1);
    }
    
    if (ctx->client2) {
        relay_destroy_client(ctx->client2);
    }
    
    if (ctx->server) {
        relay_server_stop(ctx->server);
        relay_destroy_server(ctx->server);
    }
    
    memory_free(ctx);
}

// Test: Relay Server Initialization
static bool test_relay_server_init(void) {
    log_info("Testing relay server initialization...");
    
    RelayServer* server = relay_create_server();
    ASSERT_NOT_NULL(server);
    
    // Check server state
    uint32_t client_count = relay_server_get_client_count(server);
    ASSERT_EQ(0, client_count);
    
    uint32_t session_count = relay_server_get_session_count(server);
    ASSERT_EQ(0, session_count);
    
    relay_destroy_server(server);
    
    log_info("✅ Relay server initialization test passed");
    return true;
}

// Test: Relay Server Start/Stop
static bool test_relay_server_start_stop(void) {
    log_info("Testing relay server start/stop...");
    
    RelayServer* server = relay_create_server();
    ASSERT_NOT_NULL(server);
    
    // Start server
    bool result = relay_server_start(server);
    ASSERT(result);
    
    // Check server is running
    ASSERT(server->is_running);
    
    // Stop server
    relay_server_stop(server);
    
    // Check server is stopped
    ASSERT(!server->is_running);
    
    relay_destroy_server(server);
    
    log_info("✅ Relay server start/stop test passed");
    return true;
}

// Test: Client Registration
static bool test_client_registration(void) {
    log_info("Testing client registration...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register client
    uint32_t client_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    ASSERT(client_id != 0);
    
    // Verify client was registered
    RelayClient* client = relay_server_get_client(ctx->server, client_id);
    ASSERT_NOT_NULL(client);
    ASSERT_EQ(client_id, client->client_id);
    ASSERT_STR_EQ("testuser1", client->username);
    ASSERT_EQ(RELAY_SESSION_CONNECTED, client->state);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Client registration test passed");
    return true;
}

// Test: Client Authentication
static bool test_client_authentication(void) {
    log_info("Testing client authentication...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register client
    uint32_t client_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    ASSERT(client_id != 0);
    
    // Authenticate client
    bool result = relay_server_authenticate_client(ctx->server, client_id, "valid_token");
    ASSERT(result);
    
    // Verify client is authenticated
    RelayClient* client = relay_server_get_client(ctx->server, client_id);
    ASSERT_NOT_NULL(client);
    ASSERT(client->is_authenticated);
    ASSERT_EQ(RELAY_SESSION_AUTHENTICATED, client->state);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Client authentication test passed");
    return true;
}

// Test: Session Creation
static bool test_session_creation(void) {
    log_info("Testing session creation...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register two clients
    uint32_t client1_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    uint32_t client2_id = relay_server_register_client(ctx->server, &ctx->test_address2, "testuser2");
    
    ASSERT(client1_id != 0);
    ASSERT(client2_id != 0);
    
    // Create session between clients
    uint32_t session_id = relay_server_create_session(ctx->server, client1_id, client2_id);
    ASSERT(session_id != 0);
    
    // Verify session was created
    RelaySession* session = relay_server_get_session(ctx->server, session_id);
    ASSERT_NOT_NULL(session);
    ASSERT_EQ(session_id, session->session_id);
    ASSERT_EQ(client1_id, session->client1_id);
    ASSERT_EQ(client2_id, session->client2_id);
    ASSERT(session->is_active);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Session creation test passed");
    return true;
}

// Test: Message Relaying
static bool test_message_relaying(void) {
    log_info("Testing message relaying...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register two clients
    uint32_t client1_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    uint32_t client2_id = relay_server_register_client(ctx->server, &ctx->test_address2, "testuser2");
    
    ASSERT(client1_id != 0);
    ASSERT(client2_id != 0);
    
    // Create session
    uint32_t session_id = relay_server_create_session(ctx->server, client1_id, client2_id);
    ASSERT(session_id != 0);
    
    // Send message from client1 to client2
    const char* test_message = "Hello, World!";
    bool result = relay_server_relay_message(ctx->server, client1_id, client2_id, 
                                           test_message, strlen(test_message));
    ASSERT(result);
    
    // Verify statistics
    uint32_t bytes_relayed = relay_server_get_bytes_relayed(ctx->server);
    ASSERT(bytes_relayed > 0);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Message relaying test passed");
    return true;
}

// Test: Client Unregistration
static bool test_client_unregistration(void) {
    log_info("Testing client unregistration...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register client
    uint32_t client_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    ASSERT(client_id != 0);
    
    // Verify client exists
    RelayClient* client = relay_server_get_client(ctx->server, client_id);
    ASSERT_NOT_NULL(client);
    
    // Unregister client
    bool result = relay_server_unregister_client(ctx->server, client_id);
    ASSERT(result);
    
    // Verify client no longer exists
    client = relay_server_get_client(ctx->server, client_id);
    ASSERT(client == NULL);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Client unregistration test passed");
    return true;
}

// Test: Session Destruction
static bool test_session_destruction(void) {
    log_info("Testing session destruction...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register two clients
    uint32_t client1_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    uint32_t client2_id = relay_server_register_client(ctx->server, &ctx->test_address2, "testuser2");
    
    ASSERT(client1_id != 0);
    ASSERT(client2_id != 0);
    
    // Create session
    uint32_t session_id = relay_server_create_session(ctx->server, client1_id, client2_id);
    ASSERT(session_id != 0);
    
    // Verify session exists
    RelaySession* session = relay_server_get_session(ctx->server, session_id);
    ASSERT_NOT_NULL(session);
    
    // Destroy session
    bool result = relay_server_destroy_session(ctx->server, session_id);
    ASSERT(result);
    
    // Verify session no longer exists
    session = relay_server_get_session(ctx->server, session_id);
    ASSERT(session == NULL);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Session destruction test passed");
    return true;
}

// Test: Client Connection
static bool test_client_connection(void) {
    log_info("Testing client connection...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Initialize client with server address
    NetAddress server_addr;
    server_addr.host = inet_addr("127.0.0.1");
    server_addr.port = 3478; // Default relay port
    
    bool result = ctx->client1->init(ctx->client1, &server_addr);
    ASSERT(result);
    
    // Connect to server
    result = ctx->client1->connect(ctx->client1, "testuser1");
    ASSERT(result);
    
    // Verify connection
    ASSERT(ctx->client1->is_connected(ctx->client1));
    
    // Get client ID
    uint32_t client_id = ctx->client1->get_client_id(ctx->client1);
    ASSERT(client_id != 0);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Client connection test passed");
    return true;
}

// Test: Peer Communication
static bool test_peer_communication(void) {
    log_info("Testing peer communication...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Initialize clients
    NetAddress server_addr;
    server_addr.host = inet_addr("127.0.0.1");
    server_addr.port = 3478;
    
    ctx->client1->init(ctx->client1, &server_addr);
    ctx->client2->init(ctx->client2, &server_addr);
    
    // Connect clients
    ctx->client1->connect(ctx->client1, "testuser1");
    ctx->client2->connect(ctx->client2, "testuser2");
    
    uint32_t client1_id = ctx->client1->get_client_id(ctx->client1);
    uint32_t client2_id = ctx->client2->get_client_id(ctx->client2);
    
    // Connect peers
    bool result = ctx->client1->connect_to_peer(ctx->client1, client2_id);
    ASSERT(result);
    
    // Send message to peer
    const char* test_message = "Hello from peer!";
    result = ctx->client1->send_to_peer(ctx->client1, client2_id, test_message, strlen(test_message));
    ASSERT(result);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Peer communication test passed");
    return true;
}

// Test: Broadcast Message
static bool test_broadcast_message(void) {
    log_info("Testing broadcast message...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register multiple clients
    uint32_t client1_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    uint32_t client2_id = relay_server_register_client(ctx->server, &ctx->test_address2, "testuser2");
    
    ASSERT(client1_id != 0);
    ASSERT(client2_id != 0);
    
    // Broadcast message from client1
    const char* broadcast_message = "Broadcast to all!";
    bool result = relay_server_broadcast_message(ctx->server, client1_id, 
                                                broadcast_message, strlen(broadcast_message));
    ASSERT(result);
    
    // Verify bytes were relayed
    uint32_t bytes_relayed = relay_server_get_bytes_relayed(ctx->server);
    ASSERT(bytes_relayed > 0);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Broadcast message test passed");
    return true;
}

// Test: Statistics and Monitoring
static bool test_statistics(void) {
    log_info("Testing relay server statistics...");
    
    RelayTestContext* ctx = create_relay_test_context();
    ASSERT_NOT_NULL(ctx);
    
    // Register clients and create sessions
    uint32_t client1_id = relay_server_register_client(ctx->server, &ctx->test_address1, "testuser1");
    uint32_t client2_id = relay_server_register_client(ctx->server, &ctx->test_address2, "testuser2");
    
    ASSERT(client1_id != 0);
    ASSERT(client2_id != 0);
    
    uint32_t session_id = relay_server_create_session(ctx->server, client1_id, client2_id);
    ASSERT(session_id != 0);
    
    // Send some messages
    const char* message = "Test message";
    relay_server_relay_message(ctx->server, client1_id, client2_id, message, strlen(message));
    
    // Check statistics
    uint32_t client_count = relay_server_get_client_count(ctx->server);
    ASSERT_EQ(2, client_count);
    
    uint32_t session_count = relay_server_get_session_count(ctx->server);
    ASSERT_EQ(1, session_count);
    
    uint32_t bytes_relayed = relay_server_get_bytes_relayed(ctx->server);
    ASSERT(bytes_relayed > 0);
    
    float avg_latency = relay_server_get_average_latency(ctx->server);
    ASSERT(avg_latency >= 0.0f);
    
    destroy_relay_test_context(ctx);
    
    log_info("✅ Statistics test passed");
    return true;
}

// Test: Message Validation
static bool test_message_validation(void) {
    log_info("Testing message validation...");
    
    // Test message validation function
    RelayMessageHeader header;
    header.message_type = RELAY_MSG_DATA;
    header.flags = 0;
    header.session_id = 123;
    header.client_id = 456;
    header.sequence = 789;
    header.timestamp = (uint32_t)time_get_current_ms();
    header.data_length = 100;
    
    bool result = relay_validate_message(&header, 100);
    ASSERT(result);
    
    // Test invalid message (data length mismatch)
    result = relay_validate_message(&header, 50);
    ASSERT(!result);
    
    // Test checksum calculation
    uint32_t checksum = relay_calculate_checksum(&header, sizeof(header));
    ASSERT(checksum != 0);
    
    log_info("✅ Message validation test passed");
    return true;
}

// Main test runner
bool run_relay_server_tests(void) {
    log_info("=== Starting Relay Server Protocol Unit Tests ===");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_relay_server_init();
    all_passed &= test_relay_server_start_stop();
    all_passed &= test_client_registration();
    all_passed &= test_client_authentication();
    all_passed &= test_session_creation();
    all_passed &= test_message_relaying();
    all_passed &= test_client_unregistration();
    all_passed &= test_session_destruction();
    all_passed &= test_client_connection();
    all_passed &= test_peer_communication();
    all_passed &= test_broadcast_message();
    all_passed &= test_statistics();
    all_passed &= test_message_validation();
    
    if (all_passed) {
        log_info("🎉 All relay server tests passed!");
    } else {
        log_error("❌ Some relay server tests failed!");
    }
    
    log_info("=== Relay Server Unit Tests Complete ===");
    return all_passed;
}
