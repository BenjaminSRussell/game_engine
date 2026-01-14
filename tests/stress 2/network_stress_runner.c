/**
 * Network Stress Test Runner
 * Main executable for running automated network stress tests
 */

#include "network_stress_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

// Test configuration
typedef struct {
    uint32_t num_clients;
    int specific_scenario;
    bool run_all_scenarios;
    bool verbose_output;
    bool continuous_mode;
    uint32_t test_iterations;
    NetworkConditions custom_conditions;
    bool use_custom_conditions;
} StressTestConfig;

static volatile bool g_running = true;
static StressTestConfig g_config = {0};

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    g_running = false;
}

// Print usage information
static void print_usage(const char *program_name) {
    printf("Network Stress Test Runner\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -c, --clients <num>     Number of clients to simulate (default: 10)\n");
    printf("  -s, --scenario <num>    Run specific scenario (0-%d)\n", STRESS_SCENARIO_COUNT - 1);
    printf("  -a, --all              Run all scenarios (default)\n");
    printf("  -v, --verbose          Enable verbose output\n");
    printf("  -i, --iterations <num> Number of test iterations (default: 1)\n");
    printf("  -l, --latency <ms>     Set custom latency in ms\n");
    printf("  -p, --packet-loss <%%>  Set custom packet loss percentage\n");
    printf("  -j, --jitter <ms>      Set custom jitter in ms\n");
    printf("  -b, --bandwidth <mbps> Set custom bandwidth limit in Mbps\n");
    printf("  -h, --help             Show this help message\n\n");
    
    printf("Available Scenarios:\n");
    for (int i = 0; i < STRESS_SCENARIO_COUNT; i++) {
        printf("  %d: %s\n", i, stress_test_get_scenario_name((StressScenario)i));
    }
    printf("\n");
}

// Parse command line arguments
static bool parse_arguments(int argc, char *argv[], StressTestConfig *config) {
    static struct option long_options[] = {
        {"clients", required_argument, 0, 'c'},
        {"scenario", required_argument, 0, 's'},
        {"all", no_argument, 0, 'a'},
        {"verbose", no_argument, 0, 'v'},
        {"iterations", required_argument, 0, 'i'},
        {"latency", required_argument, 0, 'l'},
        {"packet-loss", required_argument, 0, 'p'},
        {"jitter", required_argument, 0, 'j'},
        {"bandwidth", required_argument, 0, 'b'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Set defaults
    config->num_clients = 10;
    config->specific_scenario = -1;
    config->run_all_scenarios = true;
    config->verbose_output = false;
    config->continuous_mode = false;
    config->test_iterations = 1;
    config->use_custom_conditions = false;
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "c:s:avi:l:p:j:b:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                config->num_clients = (uint32_t)atoi(optarg);
                if (config->num_clients == 0 || config->num_clients > MAX_STRESS_CLIENTS) {
                    printf("Error: Number of clients must be between 1 and %d\n", MAX_STRESS_CLIENTS);
                    return false;
                }
                break;
                
            case 's':
                config->specific_scenario = atoi(optarg);
                if (config->specific_scenario < 0 || config->specific_scenario >= STRESS_SCENARIO_COUNT) {
                    printf("Error: Scenario must be between 0 and %d\n", STRESS_SCENARIO_COUNT - 1);
                    return false;
                }
                config->run_all_scenarios = false;
                break;
                
            case 'a':
                config->run_all_scenarios = true;
                break;
                
            case 'v':
                config->verbose_output = true;
                break;
                
            case 'i':
                config->test_iterations = (uint32_t)atoi(optarg);
                if (config->test_iterations == 0) {
                    printf("Error: Number of iterations must be greater than 0\n");
                    return false;
                }
                break;
                
            case 'l':
                config->custom_conditions.latency_ms = (float)atof(optarg);
                config->use_custom_conditions = true;
                break;
                
            case 'p':
                config->custom_conditions.packet_loss_rate = (float)atof(optarg) / 100.0f;
                config->use_custom_conditions = true;
                break;
                
            case 'j':
                config->custom_conditions.jitter_ms = (float)atof(optarg);
                config->use_custom_conditions = true;
                break;
                
            case 'b':
                config->custom_conditions.bandwidth_limit_mbps = (float)atof(optarg);
                config->use_custom_conditions = true;
                break;
                
            case 'h':
                print_usage(argv[0]);
                return false;
                
            case '?':
                printf("Unknown option. Use -h for help.\n");
                return false;
                
            default:
                break;
        }
    }
    
    return true;
}

// Print test configuration
static void print_config(const StressTestConfig *config) {
    printf("=== Network Stress Test Configuration ===\n");
    printf("Clients: %u\n", config->num_clients);
    printf("Iterations: %u\n", config->test_iterations);
    printf("Verbose Output: %s\n", config->verbose_output ? "Yes" : "No");
    
    if (config->run_all_scenarios) {
        printf("Scenarios: All\n");
    } else {
        printf("Scenario: %d (%s)\n", config->specific_scenario,
               stress_test_get_scenario_name((StressScenario)config->specific_scenario));
    }
    
    if (config->use_custom_conditions) {
        printf("Custom Conditions:\n");
        printf("  Latency: %.1f ms\n", config->custom_conditions.latency_ms);
        printf("  Packet Loss: %.1f%%\n", config->custom_conditions.packet_loss_rate * 100.0f);
        printf("  Jitter: %.1f ms\n", config->custom_conditions.jitter_ms);
        printf("  Bandwidth: %.1f Mbps\n", config->custom_conditions.bandwidth_limit_mbps);
    } else {
        printf("Conditions: Default\n");
    }
    
    printf("==========================================\n\n");
}

// Run a single test iteration
static bool run_test_iteration(const StressTestConfig *config) {
    printf("--- Test Iteration %u ---\n", config->test_iterations);
    
    // Initialize stress test
    if (!network_stress_test_init(config->num_clients)) {
        printf("Failed to initialize network stress test\n");
        return false;
    }
    
    // Set custom conditions if specified
    if (config->use_custom_conditions) {
        network_stress_test_set_conditions(&config->custom_conditions);
    }
    
    // Run tests
    if (config->run_all_scenarios) {
        network_stress_test_run_all_scenarios();
    } else {
        network_stress_test_run_scenario((StressScenario)config->specific_scenario);
    }
    
    // Cleanup
    network_stress_test_shutdown();
    
    return true;
}

// Main function
int main(int argc, char *argv[]) {
    printf("Network Stress Test Runner v1.0\n");
    printf("===============================\n\n");
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse arguments
    if (!parse_arguments(argc, argv, &g_config)) {
        return 1;
    }
    
    // Print configuration
    print_config(&g_config);
    
    // Run test iterations
    bool all_passed = true;
    for (uint32_t iteration = 1; iteration <= g_config.test_iterations && g_running; iteration++) {
        g_config.test_iterations = iteration; // Update for current iteration display
        
        if (!run_test_iteration(&g_config)) {
            all_passed = false;
            break;
        }
        
        // Brief pause between iterations
        if (iteration < g_config.test_iterations && g_running) {
            printf("Waiting 2 seconds before next iteration...\n");
            sleep(2);
        }
    }
    
    // Print final results
    printf("\n=== Final Results ===\n");
    if (all_passed && g_running) {
        printf("All network stress tests completed successfully!\n");
        printf("Total iterations completed: %u\n", g_config.test_iterations);
    } else if (!g_running) {
        printf("Tests interrupted by user signal.\n");
    } else {
        printf("Some tests failed.\n");
    }
    printf("====================\n");
    
    return all_passed ? 0 : 1;
}
