#include "networking/nat_traversal.h"
#include "networking/relay_server.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/time.h"
#include "core/random.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_008 - Performance Benchmarks for NAT Traversal Success Rates
// Features: Comprehensive testing of NAT traversal performance, success rates, and latency

// NAT traversal benchmark configuration
typedef struct {
    uint32_t test_sessions;
    uint32_t concurrent_sessions;
    uint32_t timeout_ms;
    bool simulate_network_conditions;
    bool detailed_logging;
} NatBenchmarkConfig;

// NAT traversal benchmark results
typedef struct {
    uint32_t total_sessions;
    uint32_t successful_connections;
    uint32_t failed_connections;
    uint32_t relay_fallbacks;
    double success_rate_percent;
    double average_connection_time_ms;
    double min_connection_time_ms;
    double max_connection_time_ms;
    double average_rtt_ms;
    NatType most_common_nat_type;
    uint32_t nat_type_counts[NAT_TYPE_COUNT];
    double throughput_mbps;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint64_t total_benchmark_time_ms;
} NatBenchmarkResults;

// Simulated network conditions
typedef struct {
    double packet_loss_rate;        // 0.0 to 1.0
    uint32_t latency_ms;           // Base latency in ms
    uint32_t jitter_ms;            // Latency variation
    uint32_t bandwidth_kbps;       // Available bandwidth
} NetworkConditions;

// Test session tracking
typedef struct {
    uint32_t session_id;
    uint64_t start_time_ms;
    uint64_t end_time_ms;
    bool successful;
    bool used_relay;
    NatType nat_type;
    uint32_t rtt_ms;
    uint32_t packets_exchanged;
} TestSession;

// Helper function to generate random NAT type
static NatType generate_random_nat_type(void) {
    uint32_t rand_val = random_get_uint32() % 100;
    
    if (rand_val < 20) return NAT_TYPE_OPEN;           // 20%
    if (rand_val < 40) return NAT_TYPE_FULL_CONE;      // 20%
    if (rand_val < 60) return NAT_TYPE_RESTRICTED;     // 20%
    if (rand_val < 80) return NAT_TYPE_PORT_RESTRICTED; // 20%
    if (rand_val < 95) return NAT_TYPE_SYMMETRIC;      // 15%
    return NAT_TYPE_BLOCKED;                           // 5%
}

// Helper function to simulate network delay
static void simulate_network_delay(const NetworkConditions* conditions) {
    if (!conditions) return;
    
    uint32_t base_delay = conditions->latency_ms;
    uint32_t jitter = conditions->jitter_ms;
    
    // Add jitter
    if (jitter > 0) {
        int32_t jitter_variation = (int32_t)(random_get_uint32() % (jitter * 2)) - jitter;
        base_delay += (jitter_variation > 0) ? jitter_variation : 0;
    }
    
    // Simulate packet loss
    if (conditions->packet_loss_rate > 0.0) {
        double rand_val = (double)(random_get_uint32() % 1000) / 1000.0;
        if (rand_val < conditions->packet_loss_rate) {
            // Simulate packet loss with longer delay
            base_delay *= 3;
        }
    }
    
    // Apply delay (simplified - in real implementation would use proper timing)
    if (base_delay > 0) {
        // Simulate delay with busy wait (for testing only)
        volatile uint64_t end_time = time_get_current_ms() + base_delay;
        while (time_get_current_ms() < end_time) {
            // Wait
        }
    }
}

// Helper function to test NAT type compatibility
static bool test_nat_compatibility(NatType local_type, NatType remote_type) {
    return nat_supports_direct_connection(local_type, remote_type);
}

// Benchmark: NAT Discovery Performance
static bool benchmark_nat_discovery(const NatBenchmarkConfig* config, NatBenchmarkResults* results) {
    log_info("Benchmarking NAT discovery performance...");
    
    NatTraversalSystem* nat_system = nat_create_system();
    if (!nat_system) {
        log_error("Failed to create NAT traversal system");
        return false;
    }
    
    uint32_t successful_discoveries = 0;
    uint64_t total_discovery_time = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    // Test NAT discovery for multiple sessions
    for (uint32_t i = 0; i < config->test_sessions; i++) {
        uint64_t start_time = time_get_current_ms();
        
        bool result = nat_discover_nat_type(nat_system);
        
        // Wait for discovery completion (simulated)
        for (int j = 0; j < 100; j++) {
            nat_system_update(nat_system, 0.1f);
            if (nat_is_nat_discovery_complete(nat_system)) {
                break;
            }
        }
        
        uint64_t end_time = time_get_current_ms();
        uint64_t discovery_time = end_time - start_time;
        
        if (result && nat_is_nat_discovery_complete(nat_system)) {
            successful_discoveries++;
            total_discovery_time += discovery_time;
            
            if (discovery_time < min_time) min_time = discovery_time;
            if (discovery_time > max_time) max_time = discovery_time;
            
            // Track NAT type distribution
            NatTraversalResult nat_result = nat_get_nat_result(nat_system);
            if (nat_result.success) {
                results->nat_type_counts[nat_result.nat_type]++;
            }
        }
    }
    
    // Calculate results
    results->average_connection_time_ms = successful_discoveries > 0 ? 
        (double)total_discovery_time / successful_discoveries : 0.0;
    results->min_connection_time_ms = (double)min_time;
    results->max_connection_time_ms = (double)max_time;
    
    nat_destroy_system(nat_system);
    
    log_info("NAT discovery: %u/%u successful (%.1f%% success rate)", 
           successful_discoveries, config->test_sessions,
           (double)successful_discoveries / config->test_sessions * 100.0);
    
    return true;
}

// Benchmark: Hole Punch Success Rate
static bool benchmark_hole_punch_success_rate(const NatBenchmarkConfig* config, 
                                             NatBenchmarkResults* results) {
    log_info("Benchmarking hole punch success rate...");
    
    NatTraversalSystem* nat_system = nat_create_system();
    if (!nat_system) {
        log_error("Failed to create NAT traversal system");
        return false;
    }
    
    TestSession* sessions = memory_alloc(sizeof(TestSession) * config->test_sessions);
    if (!sessions) {
        nat_destroy_system(nat_system);
        return false;
    }
    
    memset(sessions, 0, sizeof(TestSession) * config->test_sessions);
    
    uint32_t successful_connections = 0;
    uint32_t relay_fallbacks = 0;
    uint64_t total_connection_time = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    // Enable relay fallback
    nat_enable_relay_fallback(nat_system, true);
    
    // Test hole punching for multiple session pairs
    for (uint32_t i = 0; i < config->test_sessions; i++) {
        sessions[i].session_id = i + 1;
        sessions[i].start_time_ms = time_get_current_ms();
        sessions[i].nat_type = generate_random_nat_type();
        
        // Simulate peer address
        NetAddress peer_address;
        peer_address.host = inet_addr("127.0.0.1");
        peer_address.port = 12345 + (i % 1000);
        
        // Start hole punch session
        uint32_t hole_punch_id = nat_start_hole_punch(nat_system, &peer_address, i % 2 == 0);
        sessions[i].session_id = hole_punch_id;
        
        if (hole_punch_id == 0) {
            sessions[i].successful = false;
            continue;
        }
        
        // Simulate connection process
        bool connected = false;
        bool used_relay = false;
        
        for (int j = 0; j < 100; j++) {
            nat_system_update(nat_system, 0.1f);
            
            // Check if hole punch completed
            if (nat_is_hole_punch_complete(nat_system, hole_punch_id)) {
                connected = true;
                break;
            }
            
            // Simulate timeout and relay fallback
            if (j == 50 && !connected) {
                used_relay = true;
                relay_fallbacks++;
                connected = nat_connect_via_relay(nat_system, &peer_address);
                break;
            }
        }
        
        sessions[i].end_time_ms = time_get_current_ms();
        sessions[i].successful = connected;
        sessions[i].used_relay = used_relay;
        
        uint64_t connection_time = sessions[i].end_time_ms - sessions[i].start_time_ms;
        
        if (connected) {
            successful_connections++;
            total_connection_time += connection_time;
            
            if (connection_time < min_time) min_time = connection_time;
            if (connection_time > max_time) max_time = connection_time;
        }
    }
    
    // Calculate results
    results->successful_connections = successful_connections;
    results->failed_connections = config->test_sessions - successful_connections;
    results->relay_fallbacks = relay_fallbacks;
    results->success_rate_percent = (double)successful_connections / config->test_sessions * 100.0;
    results->average_connection_time_ms = successful_connections > 0 ? 
        (double)total_connection_time / successful_connections : 0.0;
    results->min_connection_time_ms = (double)min_time;
    results->max_connection_time_ms = (double)max_time;
    
    memory_free(sessions);
    nat_destroy_system(nat_system);
    
    log_info("Hole punch: %u/%u successful (%.1f%% success rate, %u relay fallbacks)", 
           successful_connections, config->test_sessions, results->success_rate_percent, relay_fallbacks);
    
    return true;
}

// Benchmark: Concurrent Sessions Performance
static bool benchmark_concurrent_sessions(const NatBenchmarkConfig* config, 
                                        NatBenchmarkResults* results) {
    log_info("Benchmarking concurrent sessions performance...");
    
    NatTraversalSystem* nat_system = nat_create_system();
    if (!nat_system) {
        log_error("Failed to create NAT traversal system");
        return false;
    }
    
    uint32_t* session_ids = memory_alloc(sizeof(uint32_t) * config->concurrent_sessions);
    uint64_t* start_times = memory_alloc(sizeof(uint64_t) * config->concurrent_sessions);
    
    if (!session_ids || !start_times) {
        memory_free(session_ids);
        memory_free(start_times);
        nat_destroy_system(nat_system);
        return false;
    }
    
    // Start all sessions concurrently
    uint64_t benchmark_start = time_get_current_ms();
    
    for (uint32_t i = 0; i < config->concurrent_sessions; i++) {
        start_times[i] = time_get_current_ms();
        
        NetAddress peer_address;
        peer_address.host = inet_addr("127.0.0.1");
        peer_address.port = 20000 + i;
        
        session_ids[i] = nat_start_hole_punch(nat_system, &peer_address, true);
    }
    
    // Wait for all sessions to complete or timeout
    uint32_t completed_sessions = 0;
    uint64_t total_completion_time = 0;
    
    for (int iteration = 0; iteration < 1000; iteration++) {
        nat_system_update(nat_system, 0.01f);
        
        for (uint32_t i = 0; i < config->concurrent_sessions; i++) {
            if (session_ids[i] != 0 && !nat_is_hole_punch_complete(nat_system, session_ids[i])) {
                // Check timeout
                if (time_get_current_ms() - start_times[i] > config->timeout_ms) {
                    session_ids[i] = 0; // Mark as timed out
                }
            } else if (session_ids[i] != 0) {
                // Session completed
                uint64_t completion_time = time_get_current_ms() - start_times[i];
                total_completion_time += completion_time;
                completed_sessions++;
                session_ids[i] = 0; // Mark as processed
            }
        }
        
        if (completed_sessions >= config->concurrent_sessions) {
            break; // All sessions completed
        }
    }
    
    uint64_t benchmark_end = time_get_current_ms();
    
    // Calculate results
    results->successful_connections = completed_sessions;
    results->failed_connections = config->concurrent_sessions - completed_sessions;
    results->success_rate_percent = (double)completed_sessions / config->concurrent_sessions * 100.0;
    results->average_connection_time_ms = completed_sessions > 0 ? 
        (double)total_completion_time / completed_sessions : 0.0;
    results->total_benchmark_time_ms = benchmark_end - benchmark_start;
    
    // Calculate throughput (sessions per second)
    if (results->total_benchmark_time_ms > 0) {
        results->throughput_mbps = (double)completed_sessions / (results->total_benchmark_time_ms / 1000.0);
    }
    
    memory_free(session_ids);
    memory_free(start_times);
    nat_destroy_system(nat_system);
    
    log_info("Concurrent sessions: %u/%u completed in %llu ms (%.1f sessions/sec)", 
           completed_sessions, config->concurrent_sessions, results->total_benchmark_time_ms,
           results->throughput_mbps);
    
    return true;
}

// Benchmark: Network Conditions Impact
static bool benchmark_network_conditions_impact(const NatBenchmarkConfig* config, 
                                               NatBenchmarkResults* results) {
    log_info("Benchmarking network conditions impact...");
    
    // Test different network conditions
    NetworkConditions conditions[] = {
        {0.0, 10, 5, 10000},    // Perfect network
        {0.01, 50, 20, 5000},  // Slight degradation
        {0.05, 100, 50, 2000}, // Moderate degradation
        {0.1, 200, 100, 1000}  // Poor network
    };
    
    size_t num_conditions = sizeof(conditions) / sizeof(conditions[0]);
    
    for (size_t i = 0; i < num_conditions; i++) {
        log_info("Testing network condition %zu: %.1f%% loss, %u ms latency", 
               i + 1, conditions[i].packet_loss_rate * 100, conditions[i].latency_ms);
        
        NatTraversalSystem* nat_system = nat_create_system();
        if (!nat_system) continue;
        
        uint32_t successful = 0;
        uint64_t total_time = 0;
        
        for (uint32_t j = 0; j < config->test_sessions / num_conditions; j++) {
            uint64_t start_time = time_get_current_ms();
            
            // Simulate network conditions
            simulate_network_delay(&conditions[i]);
            
            NetAddress peer_address;
            peer_address.host = inet_addr("127.0.0.1");
            peer_address.port = 30000 + j;
            
            uint32_t session_id = nat_start_hole_punch(nat_system, &peer_address, true);
            
            if (session_id != 0) {
                // Wait for completion with network simulation
                for (int k = 0; k < 100; k++) {
                    nat_system_update(nat_system, 0.01f);
                    simulate_network_delay(&conditions[i]);
                    
                    if (nat_is_hole_punch_complete(nat_system, session_id)) {
                        successful++;
                        total_time += time_get_current_ms() - start_time;
                        break;
                    }
                    
                    if (time_get_current_ms() - start_time > config->timeout_ms) {
                        break;
                    }
                }
            }
        }
        
        double success_rate = (double)successful / (config->test_sessions / num_conditions) * 100.0;
        double avg_time = successful > 0 ? (double)total_time / successful : 0.0;
        
        log_info("  Results: %.1f%% success rate, %.1f ms avg time", success_rate, avg_time);
        
        nat_destroy_system(nat_system);
    }
    
    return true;
}

// Comprehensive NAT traversal benchmark
static bool run_comprehensive_nat_benchmark(const NatBenchmarkConfig* config) {
    log_info("=== Starting Comprehensive NAT Traversal Benchmark ===");
    log_info("Test sessions: %u, Concurrent: %u, Timeout: %u ms", 
           config->test_sessions, config->concurrent_sessions, config->timeout_ms);
    
    NatBenchmarkResults results = {0};
    bool success = true;
    
    // Run all benchmark tests
    success &= benchmark_nat_discovery(config, &results);
    success &= benchmark_hole_punch_success_rate(config, &results);
    success &= benchmark_concurrent_sessions(config, &results);
    success &= benchmark_network_conditions_impact(config, &results);
    
    // Print comprehensive results
    log_info("\n=== NAT TRAVERSAL BENCHMARK RESULTS ===");
    log_info("Total Sessions Tested: %u", config->test_sessions);
    log_info("Successful Connections: %u", results.successful_connections);
    log_info("Failed Connections: %u", results.failed_connections);
    log_info("Relay Fallbacks: %u", results.relay_fallbacks);
    
    log_info("\n=== PERFORMANCE METRICS ===");
    log_info("Overall Success Rate: %.2f%%", results.success_rate_percent);
    log_info("Average Connection Time: %.2f ms", results.average_connection_time_ms);
    log_info("Min Connection Time: %.2f ms", results.min_connection_time_ms);
    log_info("Max Connection Time: %.2f ms", results.max_connection_time_ms);
    log_info("Throughput: %.2f sessions/sec", results.throughput_mbps);
    
    log_info("\n=== NAT TYPE DISTRIBUTION ===");
    const char* nat_type_names[] = {"Open", "Full Cone", "Restricted", "Port Restricted", "Symmetric", "Blocked"};
    for (int i = 0; i < NAT_TYPE_COUNT; i++) {
        if (results.nat_type_counts[i] > 0) {
            log_info("%s: %u sessions (%.1f%%)", 
                   nat_type_names[i], results.nat_type_counts[i],
                   (double)results.nat_type_counts[i] / config->test_sessions * 100.0);
        }
    }
    
    // Performance assessment
    log_info("\n=== PERFORMANCE ASSESSMENT ===");
    
    if (results.success_rate_percent > 90) {
        log_info("✅ Success rate: EXCELLENT (>90%%)");
    } else if (results.success_rate_percent > 75) {
        log_info("✅ Success rate: GOOD (>75%%)");
    } else if (results.success_rate_percent > 50) {
        log_info("⚠️  Success rate: ACCEPTABLE (>50%%)");
    } else {
        log_info("❌ Success rate: POOR (<50%%)");
    }
    
    if (results.average_connection_time_ms < 100) {
        log_info("✅ Connection time: EXCELLENT (<100ms)");
    } else if (results.average_connection_time_ms < 500) {
        log_info("✅ Connection time: GOOD (<500ms)");
    } else if (results.average_connection_time_ms < 2000) {
        log_info("⚠️  Connection time: ACCEPTABLE (<2s)");
    } else {
        log_info("❌ Connection time: POOR (>2s)");
    }
    
    if (results.relay_fallbacks < (config->test_sessions * 0.1)) {
        log_info("✅ Relay usage: EXCELLENT (<10%%)");
    } else if (results.relay_fallbacks < (config->test_sessions * 0.25)) {
        log_info("✅ Relay usage: GOOD (<25%%)");
    } else if (results.relay_fallbacks < (config->test_sessions * 0.5)) {
        log_info("⚠️  Relay usage: ACCEPTABLE (<50%%)");
    } else {
        log_info("❌ Relay usage: POOR (>50%%)");
    }
    
    log_info("=== NAT Traversal Benchmark Complete ===\n");
    
    return success;
}

// Stress test for NAT traversal
static bool run_nat_stress_test(void) {
    log_info("=== Running NAT Traversal Stress Test ===");
    
    NatBenchmarkConfig stress_config = {0};
    stress_config.test_sessions = 1000;
    stress_config.concurrent_sessions = 100;
    stress_config.timeout_ms = 30000; // 30 seconds
    stress_config.simulate_network_conditions = true;
    stress_config.detailed_logging = false;
    
    return run_comprehensive_nat_benchmark(&stress_config);
}

// Main benchmark runner
bool run_nat_traversal_benchmarks(void) {
    log_info("=== Starting NAT Traversal Performance Benchmarks ===");
    
    bool all_passed = true;
    
    // Run standard benchmark suite
    NatBenchmarkConfig standard_config = {0};
    standard_config.test_sessions = 100;
    standard_config.concurrent_sessions = 10;
    standard_config.timeout_ms = 10000; // 10 seconds
    standard_config.simulate_network_conditions = false;
    standard_config.detailed_logging = true;
    
    all_passed &= run_comprehensive_nat_benchmark(&standard_config);
    
    // Run stress test
    all_passed &= run_nat_stress_test();
    
    if (all_passed) {
        log_info("🎉 All NAT traversal benchmarks completed successfully!");
    } else {
        log_error("❌ Some NAT traversal benchmarks failed!");
    }
    
    log_info("=== NAT Traversal Performance Benchmarks Complete ===");
    return all_passed;
}
