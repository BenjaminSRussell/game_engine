/**
 * Automated Network Stress Testing Framework
 * Header file for comprehensive network stress testing
 */

#ifndef NETWORK_STRESS_TEST_H
#define NETWORK_STRESS_TEST_H

#include <stdint.h>
#include <stdbool.h>

// Stress test configuration constants
#define MAX_STRESS_CLIENTS 100
#define MAX_STRESS_ENTITIES 50
#define STRESS_TEST_DURATION_MS 30000
#define PACKET_SIZE_STRESS_LARGE 2048
#define STRESS_REPORT_INTERVAL_MS 5000

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
    float latency_ms;           // Artificial latency in milliseconds
    float packet_loss_rate;      // Packet loss rate (0.0 to 1.0)
    float bandwidth_limit_mbps; // Bandwidth limit in Mbps
    float jitter_ms;            // Latency jitter in milliseconds
    bool enable_corruption;     // Enable packet corruption simulation
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
    struct ClientPrediction *prediction;  // Forward declaration
    uint64_t last_packet_time;
    uint32_t stress_level;
} StressClient;

// Public API functions

/**
 * Initialize the network stress testing framework
 * @param num_clients Number of clients to simulate
 * @return true if initialization successful, false otherwise
 */
bool network_stress_test_init(uint32_t num_clients);

/**
 * Run all stress test scenarios
 */
void network_stress_test_run_all_scenarios(void);

/**
 * Run a specific stress test scenario
 * @param scenario The scenario to run
 */
void network_stress_test_run_scenario(StressScenario scenario);

/**
 * Set network conditions for stress testing
 * @param conditions Pointer to network conditions structure
 */
void network_stress_test_set_conditions(const NetworkConditions *conditions);

/**
 * Shutdown the network stress testing framework
 */
void network_stress_test_shutdown(void);

// Utility functions

/**
 * Get stress test scenario name
 * @param scenario The scenario enum
 * @return String name of the scenario
 */
static inline const char* stress_test_get_scenario_name(StressScenario scenario) {
    switch (scenario) {
        case STRESS_SCENARIO_BASIC_CONNECTIVITY: return "Basic Connectivity";
        case STRESS_SCENARIO_HIGH_THROUGHPUT: return "High Throughput";
        case STRESS_SCENARIO_PACKET_LOSS: return "Packet Loss";
        case STRESS_SCENARIO_LATENCY_SPIKES: return "Latency Spikes";
        case STRESS_SCENARIO_FRAGMENTATION: return "Fragmentation";
        case STRESS_SCENARIO_COMPRESSION: return "Compression";
        case STRESS_SCENARIO_PREDICTION_STRESS: return "Prediction Stress";
        case STRESS_SCENARIO_RPC_STRESS: return "RPC Stress";
        case STRESS_SCENARIO_MIXED_LOAD: return "Mixed Load";
        case STRESS_SCENARIO_CONNECTION_MIGRATION: return "Connection Migration";
        default: return "Unknown";
    }
}

/**
 * Create default network conditions
 * @return NetworkConditions structure with default values
 */
static inline NetworkConditions stress_test_default_conditions(void) {
    NetworkConditions conditions = {0};
    conditions.latency_ms = 10.0f;
    conditions.packet_loss_rate = 0.01f; // 1%
    conditions.bandwidth_limit_mbps = 100.0f;
    conditions.jitter_ms = 2.0f;
    conditions.enable_corruption = false;
    return conditions;
}

/**
 * Create high-latency network conditions
 * @return NetworkConditions structure with high latency values
 */
static inline NetworkConditions stress_test_high_latency_conditions(void) {
    NetworkConditions conditions = {0};
    conditions.latency_ms = 200.0f;
    conditions.packet_loss_rate = 0.05f; // 5%
    conditions.bandwidth_limit_mbps = 10.0f;
    conditions.jitter_ms = 50.0f;
    conditions.enable_corruption = false;
    return conditions;
}

/**
 * Create packet loss network conditions
 * @return NetworkConditions structure with high packet loss
 */
static inline NetworkConditions stress_test_packet_loss_conditions(void) {
    NetworkConditions conditions = {0};
    conditions.latency_ms = 50.0f;
    conditions.packet_loss_rate = 0.20f; // 20%
    conditions.bandwidth_limit_mbps = 50.0f;
    conditions.jitter_ms = 10.0f;
    conditions.enable_corruption = false;
    return conditions;
}

/**
 * Create bandwidth-limited network conditions
 * @return NetworkConditions structure with low bandwidth
 */
static inline NetworkConditions stress_test_bandwidth_limited_conditions(void) {
    NetworkConditions conditions = {0};
    conditions.latency_ms = 100.0f;
    conditions.packet_loss_rate = 0.02f; // 2%
    conditions.bandwidth_limit_mbps = 1.0f; // 1 Mbps
    conditions.jitter_ms = 20.0f;
    conditions.enable_corruption = false;
    return conditions;
}

#endif // NETWORK_STRESS_TEST_H
