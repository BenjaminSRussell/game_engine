/**
 * Automated Network Stress Testing Framework
 * Comprehensive stress testing for networking systems with configurable scenarios
 */

#include "include/network/network_manager.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"
#include "include/network/network_prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define MAX_STRESS_CLIENTS 100
#define MAX_STRESS_ENTITIES 50
#define STRESS_TEST_DURATION_MS 30000 // 30 seconds
#define PACKET_SIZE_STRESS_LARGE 2048
#define STRESS_REPORT_INTERVAL_MS 5000 // 5 seconds

// Stress test scenarios
typedef enum {
    STRESS_SCENARIO_BASIC_CONNECTIVITY = 0,
    STRESS_SCENARIO_HIGH_THROUGHPUT,
    STRESS_SCENARIO_PACKET_LOSS,
    STRESS_SCENARIO_LATENCY_SPIKES,
    STRESS_SCENARIO_FRAGMENTATION,
    STRESS_SCENARIO_COMPRESSION,
    STRESS_SCENARIO_PREDICTION_STRESS,
    STRESS_SCENARIO_RPC_STRESS,
    STRESS_SCENARIO_MIXED_LOAD,
    STRESS_SCENARIO_CONNECTION_MIGRATION,
    STRESS_SCENARIO_COUNT
} StressScenario;

// Network condition simulation
typedef struct {
    float latency_ms;
    float packet_loss_rate;
    float bandwidth_limit_mbps;
    float jitter_ms;
    bool enable_corruption;
} NetworkConditions;

// Stress test metrics
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t rpc_calls_sent;
    uint32_t rpc_calls_completed;
    uint32_t rpc_timeouts;
    uint32_t prediction_mispredictions;
    uint32_t connection_failures;
    uint32_t reconnections;
    float avg_latency_ms;
    float max_latency_ms;
    float min_latency_ms;
    float packet_loss_rate;
    float throughput_mbps;
    uint64_t test_start_time;
    uint64_t test_duration_ms;
    bool test_completed;
} StressMetrics;

// Stress test client
typedef struct {
    uint32_t client_id;
    bool is_active;
    bool is_server;
    NetworkConditions conditions;
    StressMetrics metrics;
    ClientPrediction *prediction;
    uint64_t last_packet_time;
    uint32_t stress_level;
} StressClient;

// Global stress test state
static StressClient g_stress_clients[MAX_STRESS_CLIENTS];
static uint32_t g_active_clients = 0;
static StressScenario g_current_scenario = STRESS_SCENARIO_BASIC_CONNECTIVITY;
static NetworkConditions g_global_conditions = {0.0f};
static bool g_stress_test_running = false;
static uint64_t g_test_start_time = 0;

// Helper functions
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void apply_network_conditions(StressClient *client) {
    // Simulate network conditions by modifying packet processing
    if (client->conditions.latency_ms > 0.0f) {
        // Add artificial delay to packet processing
        usleep((useconds_t)(client->conditions.latency_ms * 1000));
    }
    
    // Simulate packet loss
    if (client->conditions.packet_loss_rate > 0.0f) {
        float random = (float)rand() / RAND_MAX;
        if (random < client->conditions.packet_loss_rate) {
            // Drop this packet
            return;
        }
    }
    
    // Simulate jitter
    if (client->conditions.jitter_ms > 0.0f) {
        float jitter = ((float)rand() / RAND_MAX) * client->conditions.jitter_ms;
        usleep((useconds_t)(jitter * 1000));
    }
}

static void update_stress_metrics(StressClient *client, bool packet_sent, bool packet_received, 
                           uint32_t bytes, float latency_ms) {
    if (packet_sent) {
        client->metrics.packets_sent++;
        client->metrics.bytes_sent += bytes;
    }
    
    if (packet_received) {
        client->metrics.packets_received++;
        client->metrics.bytes_received += bytes;
        
        // Update latency metrics
        if (latency_ms > 0.0f) {
            if (client->metrics.avg_latency_ms == 0.0f) {
                client->metrics.avg_latency_ms = latency_ms;
                client->metrics.max_latency_ms = latency_ms;
                client->metrics.min_latency_ms = latency_ms;
            } else {
                client->metrics.avg_latency_ms = 
                    (client->metrics.avg_latency_ms * 0.9f) + (latency_ms * 0.1f);
                client->metrics.max_latency_ms = 
                    fmaxf(client->metrics.max_latency_ms, latency_ms);
                client->metrics.min_latency_ms = 
                    fminf(client->metrics.min_latency_ms, latency_ms);
            }
        }
    }
    
    // Calculate packet loss rate
    if (client->metrics.packets_sent > 0) {
        uint32_t lost_packets = client->metrics.packets_sent - client->metrics.packets_received;
        client->metrics.packet_loss_rate = 
            (float)lost_packets / (float)client->metrics.packets_sent;
    }
    
    // Calculate throughput
    uint64_t current_time = get_timestamp_ms();
    if (current_time > client->metrics.test_start_time) {
        client->metrics.test_duration_ms = current_time - client->metrics.test_start_time;
        if (client->metrics.test_duration_ms > 0) {
            client->metrics.throughput_mbps = 
                (float)client->metrics.bytes_received * 8.0f / 1000.0f / 1000.0f / 
                (client->metrics.test_duration_ms / 1000.0f);
        }
    }
}

static void stress_test_basic_connectivity(void) {
    printf("Running basic connectivity stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Simulate connection/disconnection cycles
        if (rand() % 100 < 10) { // 10% chance of disconnection
            client->metrics.connection_failures++;
            printf("Client %u: Simulated connection failure\n", client->client_id);
        }
    }
}

static void stress_test_high_throughput(void) {
    printf("Running high throughput stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Send large packets rapidly
        for (int j = 0; j < 100; j++) {
            uint8_t large_packet[PACKET_SIZE_STRESS_LARGE];
            memset(large_packet, 0xAA, sizeof(large_packet));
            
            update_stress_metrics(client, true, false, sizeof(large_packet), 0.0f);
            apply_network_conditions(client);
        }
    }
}

static void stress_test_packet_loss(void) {
    printf("Running packet loss stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Set high packet loss conditions
        client->conditions.packet_loss_rate = 0.2f; // 20% packet loss
        
        for (int j = 0; j < 50; j++) {
            uint8_t test_packet[64];
            memset(test_packet, 0xBB, sizeof(test_packet));
            
            apply_network_conditions(client);
            update_stress_metrics(client, true, false, sizeof(test_packet), 0.0f);
        }
    }
}

static void stress_test_latency_spikes(void) {
    printf("Running latency spike stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Simulate latency spikes
        client->conditions.latency_ms = 200.0f; // 200ms latency
        client->conditions.jitter_ms = 50.0f;  // 50ms jitter
        
        for (int j = 0; j < 20; j++) {
            uint8_t test_packet[32];
            memset(test_packet, 0xCC, sizeof(test_packet));
            
            apply_network_conditions(client);
            update_stress_metrics(client, true, false, sizeof(test_packet), 
                           client->conditions.latency_ms);
        }
    }
}

static void stress_test_fragmentation(void) {
    printf("Running fragmentation stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Send oversized packets that require fragmentation
        uint8_t oversized_packet[3000];
        memset(oversized_packet, 0xDD, sizeof(oversized_packet));
        
        update_stress_metrics(client, true, false, sizeof(oversized_packet), 0.0f);
        apply_network_conditions(client);
    }
}

static void stress_test_compression(void) {
    printf("Running compression stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Send compressible data
        uint8_t compressible_data[1024];
        for (int j = 0; j < 1024; j++) {
            compressible_data[j] = (uint8_t)(j % 256); // Repeating pattern
        }
        
        update_stress_metrics(client, true, false, sizeof(compressible_data), 0.0f);
        apply_network_conditions(client);
    }
}

static void stress_test_prediction_stress(void) {
    printf("Running prediction stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active || !client->prediction) continue;
        
        // Simulate rapid entity state changes
        for (int j = 0; j < 50; j++) {
            Vec3 pos = {(float)(j * 10), (float)(i * 5), 0.0f};
            Vec3 vel = {(float)(j % 20 - 10), 0.0f, 0.0f};
            
            // Add prediction frame
            prediction_add_frame(client->prediction, j, pos, vel);
            
            // Simulate server correction to trigger reconciliation
            if (j % 10 == 0) {
                Vec3 server_pos = {(float)(j * 10 + 5), (float)(i * 5 + 2), 0.0f};
                prediction_reconcile(client->prediction, i, j, server_pos, vel);
                client->metrics.prediction_mispredictions++;
            }
        }
    }
}

static void stress_test_rpc_stress(void) {
    printf("Running RPC stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Send rapid RPC calls
        for (int j = 0; j < 30; j++) {
            char rpc_data[128];
            snprintf(rpc_data, sizeof(rpc_data), "RPC_Call_%u_%u", i, j);
            
            bool sent = rpc_send(i, "StressRPC", rpc_data, strlen(rpc_data));
            if (sent) {
                client->metrics.rpc_calls_sent++;
            } else {
                client->metrics.rpc_timeouts++;
            }
            
            apply_network_conditions(client);
        }
    }
}

static void stress_test_mixed_load(void) {
    printf("Running mixed load stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Mix of different stress types
        client->conditions.latency_ms = 50.0f + (float)(i % 100);
        client->conditions.packet_loss_rate = 0.05f + (float)(i % 20) * 0.01f;
        client->conditions.jitter_ms = 10.0f;
        
        for (int j = 0; j < 25; j++) {
            uint8_t mixed_packet[256];
            memset(mixed_packet, 0xEE, sizeof(mixed_packet));
            
            update_stress_metrics(client, true, false, sizeof(mixed_packet), 
                           client->conditions.latency_ms);
            apply_network_conditions(client);
        }
    }
}

static void stress_test_connection_migration(void) {
    printf("Running connection migration stress test...\n");
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (!client->is_active) continue;
        
        // Simulate connection failures and migrations
        if (rand() % 50 < 5) { // 10% chance of migration
            client->metrics.reconnections++;
            printf("Client %u: Simulating connection migration\n", client->client_id);
        }
        
        uint8_t migration_packet[64];
        memset(migration_packet, 0xFF, sizeof(migration_packet));
        update_stress_metrics(client, true, false, sizeof(migration_packet), 0.0f);
        apply_network_conditions(client);
    }
}

static void print_stress_report(void) {
    printf("\n=== NETWORK STRESS TEST REPORT ===\n");
    printf("Scenario: %u\n", g_current_scenario);
    printf("Active Clients: %u\n", g_active_clients);
    printf("Test Duration: %lu ms\n", (unsigned long)(get_timestamp_ms() - g_test_start_time));
    
    uint32_t total_packets_sent = 0, total_packets_received = 0;
    uint32_t total_bytes_sent = 0, total_bytes_received = 0;
    uint32_t total_rpc_calls = 0, total_rpc_timeouts = 0;
    uint32_t total_mispredictions = 0, total_failures = 0, total_reconnections = 0;
    
    for (uint32_t i = 0; i < g_active_clients; i++) {
        StressClient *client = &g_stress_clients[i];
        if (client->is_active) {
            total_packets_sent += client->metrics.packets_sent;
            total_packets_received += client->metrics.packets_received;
            total_bytes_sent += client->metrics.bytes_sent;
            total_bytes_received += client->metrics.bytes_received;
            total_rpc_calls += client->metrics.rpc_calls_sent;
            total_rpc_timeouts += client->metrics.rpc_timeouts;
            total_mispredictions += client->metrics.prediction_mispredictions;
            total_failures += client->metrics.connection_failures;
            total_reconnections += client->metrics.reconnections;
            
            printf("Client %u: Sent: %u packets (%u bytes), Received: %u packets (%u bytes), "
                   "RPC: %u/%u, Mispredictions: %u, Failures: %u, Reconnections: %u\n",
                   client->client_id,
                   client->metrics.packets_sent, client->metrics.bytes_sent,
                   client->metrics.packets_received, client->metrics.bytes_received,
                   client->metrics.rpc_calls_sent, client->metrics.rpc_timeouts,
                   client->metrics.prediction_mispredictions,
                   client->metrics.connection_failures, client->metrics.reconnections);
        }
    }
    
    if (total_packets_sent > 0) {
        float overall_packet_loss = (float)(total_packets_sent - total_packets_received) / total_packets_sent;
        float overall_throughput = (float)total_bytes_received * 8.0f / 1000.0f / 1000.0f / 
                               ((get_timestamp_ms() - g_test_start_time) / 1000.0f);
        
        printf("\nOverall Statistics:\n");
        printf("Total Packets Sent: %u\n", total_packets_sent);
        printf("Total Packets Received: %u\n", total_packets_received);
        printf("Packet Loss Rate: %.2f%%\n", overall_packet_loss * 100.0f);
        printf("Total Bytes Sent: %u\n", total_bytes_sent);
        printf("Total Bytes Received: %u\n", total_bytes_received);
        printf("Throughput: %.2f Mbps\n", overall_throughput);
        printf("Total RPC Calls: %u\n", total_rpc_calls);
        printf("RPC Timeouts: %u\n", total_rpc_timeouts);
        printf("Total Mispredictions: %u\n", total_mispredictions);
        printf("Total Connection Failures: %u\n", total_failures);
        printf("Total Reconnections: %u\n", total_reconnections);
    }
    
    printf("=====================================\n\n");
}

// Main stress test execution
static void run_stress_test_scenario(StressScenario scenario) {
    g_current_scenario = scenario;
    g_test_start_time = get_timestamp_ms();
    
    // Reset client metrics
    for (uint32_t i = 0; i < g_active_clients; i++) {
        memset(&g_stress_clients[i].metrics, 0, sizeof(StressMetrics));
        g_stress_clients[i].metrics.test_start_time = g_test_start_time;
        g_stress_clients[i].conditions = g_global_conditions;
    }
    
    switch (scenario) {
        case STRESS_SCENARIO_BASIC_CONNECTIVITY:
            stress_test_basic_connectivity();
            break;
        case STRESS_SCENARIO_HIGH_THROUGHPUT:
            stress_test_high_throughput();
            break;
        case STRESS_SCENARIO_PACKET_LOSS:
            stress_test_packet_loss();
            break;
        case STRESS_SCENARIO_LATENCY_SPIKES:
            stress_test_latency_spikes();
            break;
        case STRESS_SCENARIO_FRAGMENTATION:
            stress_test_fragmentation();
            break;
        case STRESS_SCENARIO_COMPRESSION:
            stress_test_compression();
            break;
        case STRESS_SCENARIO_PREDICTION_STRESS:
            stress_test_prediction_stress();
            break;
        case STRESS_SCENARIO_RPC_STRESS:
            stress_test_rpc_stress();
            break;
        case STRESS_SCENARIO_MIXED_LOAD:
            stress_test_mixed_load();
            break;
        case STRESS_SCENARIO_CONNECTION_MIGRATION:
            stress_test_connection_migration();
            break;
        default:
            printf("Unknown stress test scenario: %u\n", scenario);
            break;
    }
    
    print_stress_report();
}

// Public API
bool network_stress_test_init(uint32_t num_clients) {
    printf("Initializing network stress test with %u clients...\n", num_clients);
    
    if (num_clients > MAX_STRESS_CLIENTS) {
        num_clients = MAX_STRESS_CLIENTS;
    }
    
    g_active_clients = num_clients;
    g_stress_test_running = true;
    
    // Initialize stress clients
    for (uint32_t i = 0; i < num_clients; i++) {
        memset(&g_stress_clients[i], 0, sizeof(StressClient));
        g_stress_clients[i].client_id = i;
        g_stress_clients[i].is_active = true;
        g_stress_clients[i].is_server = (i == 0); // First client is server
        g_stress_clients[i].stress_level = 1;
        
        // Initialize prediction for clients
        if (!g_stress_clients[i].is_server) {
            g_stress_clients[i].prediction = prediction_create();
        }
    }
    
    // Initialize networking systems
    network_manager_init(num_clients > 0);
    rpc_system_init();
    
    printf("Network stress test initialized successfully\n");
    return true;
}

void network_stress_test_run_all_scenarios(void) {
    if (!g_stress_test_running) {
        printf("Stress test not initialized. Call network_stress_test_init() first.\n");
        return;
    }
    
    printf("Running all network stress test scenarios...\n\n");
    
    for (int scenario = 0; scenario < STRESS_SCENARIO_COUNT; scenario++) {
        printf("\n--- Starting Scenario %d ---\n", scenario);
        run_stress_test_scenario((StressScenario)scenario);
        
        // Brief pause between scenarios
        usleep(1000000); // 1 second
    }
    
    printf("\nAll network stress test scenarios completed.\n");
}

void network_stress_test_run_scenario(StressScenario scenario) {
    if (!g_stress_test_running) {
        printf("Stress test not initialized. Call network_stress_test_init() first.\n");
        return;
    }
    
    run_stress_test_scenario(scenario);
}

void network_stress_test_set_conditions(const NetworkConditions *conditions) {
    if (conditions) {
        g_global_conditions = *conditions;
        printf("Network conditions updated: Latency=%.1fms, Loss=%.1f%%, Jitter=%.1fms\n",
               conditions->latency_ms, conditions->packet_loss_rate * 100.0f, 
               conditions->jitter_ms);
    }
}

void network_stress_test_shutdown(void) {
    printf("Shutting down network stress test...\n");
    
    // Cleanup prediction systems
    for (uint32_t i = 0; i < g_active_clients; i++) {
        if (g_stress_clients[i].prediction) {
            prediction_destroy(g_stress_clients[i].prediction);
        }
    }
    
    // Shutdown networking systems
    rpc_system_shutdown();
    network_manager_shutdown();
    
    g_stress_test_running = false;
    g_active_clients = 0;
    
    printf("Network stress test shutdown complete\n");
}
