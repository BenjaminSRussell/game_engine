/**
 * Network Condition Simulator
 * Realistic network condition simulation for stress testing
 */

#include "network_stress_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Network condition presets
typedef enum {
    NETWORK_PRESET_PERFECT = 0,
    NETWORK_PRESET_MOBILE_3G,
    NETWORK_PRESET_MOBILE_4G,
    NETWORK_PRESET_WIFI_GOOD,
    NETWORK_PRESET_WIFI_POOR,
    NETWORK_PRESET_BROADBAND,
    NETWORK_PRESET_SATELLITE,
    NETWORK_PRESET_DIALUP,
    NETWORK_PRESET_COUNT
} NetworkPreset;

// Packet corruption simulation
typedef struct {
    float corruption_rate;
    uint32_t corruption_seed;
    bool enable_bit_flips;
    bool enable_byte_reordering;
    bool enable_packet_duplication;
} CorruptionSimulator;

// Bandwidth throttling simulation
typedef struct {
    float current_bandwidth_mbps;
    float target_bandwidth_mbps;
    uint32_t bytes_sent_in_window;
    uint64_t window_start_time;
    uint64_t window_duration_ms;
    bool bandwidth_limited;
} BandwidthThrottler;

// Network condition simulator state
typedef struct {
    NetworkPreset preset;
    NetworkConditions base_conditions;
    NetworkConditions current_conditions;
    CorruptionSimulator corruption;
    BandwidthThrottler bandwidth;
    uint64_t simulation_start_time;
    uint64_t last_update_time;
    float condition_variation_intensity;
    bool enable_dynamic_conditions;
} NetworkSimulator;

// Global simulator instance
static NetworkSimulator g_simulator = {0};

// Helper functions
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static float random_float_range(float min, float max) {
    float random = (float)rand() / RAND_MAX;
    return min + random * (max - min);
}

static bool should_corrupt_packet(const CorruptionSimulator *corruption) {
    if (corruption->corruption_rate <= 0.0f) return false;
    
    float random = (float)rand() / RAND_MAX;
    return random < corruption->corruption_rate;
}

static void corrupt_packet_data(uint8_t *data, uint32_t size, const CorruptionSimulator *corruption) {
    if (!data || size == 0) return;
    
    // Use seeded random for reproducible corruption
    srand(corruption->corruption_seed);
    
    if (corruption->enable_bit_flips) {
        // Flip random bits
        uint32_t num_flips = rand() % 3 + 1; // 1-3 bit flips
        for (uint32_t i = 0; i < num_flips; i++) {
            uint32_t byte_index = rand() % size;
            uint8_t bit_index = rand() % 8;
            data[byte_index] ^= (1 << bit_index);
        }
    }
    
    if (corruption->enable_byte_reordering && size > 1) {
        // Swap two random bytes
        uint32_t index1 = rand() % size;
        uint32_t index2 = rand() % size;
        if (index1 != index2) {
            uint8_t temp = data[index1];
            data[index1] = data[index2];
            data[index2] = temp;
        }
    }
}

// Network preset definitions
static NetworkConditions get_network_preset(NetworkPreset preset) {
    NetworkConditions conditions = {0};
    
    switch (preset) {
        case NETWORK_PRESET_PERFECT:
            conditions.latency_ms = 1.0f;
            conditions.packet_loss_rate = 0.001f; // 0.1%
            conditions.bandwidth_limit_mbps = 1000.0f;
            conditions.jitter_ms = 0.1f;
            conditions.enable_corruption = false;
            break;
            
        case NETWORK_PRESET_MOBILE_3G:
            conditions.latency_ms = 200.0f;
            conditions.packet_loss_rate = 0.02f; // 2%
            conditions.bandwidth_limit_mbps = 2.0f;
            conditions.jitter_ms = 50.0f;
            conditions.enable_corruption = true;
            break;
            
        case NETWORK_PRESET_MOBILE_4G:
            conditions.latency_ms = 50.0f;
            conditions.packet_loss_rate = 0.01f; // 1%
            conditions.bandwidth_limit_mbps = 20.0f;
            conditions.jitter_ms = 10.0f;
            conditions.enable_corruption = false;
            break;
            
        case NETWORK_PRESET_WIFI_GOOD:
            conditions.latency_ms = 10.0f;
            conditions.packet_loss_rate = 0.005f; // 0.5%
            conditions.bandwidth_limit_mbps = 100.0f;
            conditions.jitter_ms = 5.0f;
            conditions.enable_corruption = false;
            break;
            
        case NETWORK_PRESET_WIFI_POOR:
            conditions.latency_ms = 100.0f;
            conditions.packet_loss_rate = 0.05f; // 5%
            conditions.bandwidth_limit_mbps = 10.0f;
            conditions.jitter_ms = 30.0f;
            conditions.enable_corruption = true;
            break;
            
        case NETWORK_PRESET_BROADBAND:
            conditions.latency_ms = 20.0f;
            conditions.packet_loss_rate = 0.002f; // 0.2%
            conditions.bandwidth_limit_mbps = 100.0f;
            conditions.jitter_ms = 2.0f;
            conditions.enable_corruption = false;
            break;
            
        case NETWORK_PRESET_SATELLITE:
            conditions.latency_ms = 600.0f;
            conditions.packet_loss_rate = 0.03f; // 3%
            conditions.bandwidth_limit_mbps = 5.0f;
            conditions.jitter_ms = 100.0f;
            conditions.enable_corruption = true;
            break;
            
        case NETWORK_PRESET_DIALUP:
            conditions.latency_ms = 150.0f;
            conditions.packet_loss_rate = 0.08f; // 8%
            conditions.bandwidth_limit_mbps = 0.056f; // 56 Kbps
            conditions.jitter_ms = 40.0f;
            conditions.enable_corruption = true;
            break;
            
        default:
            conditions = stress_test_default_conditions();
            break;
    }
    
    return conditions;
}

// Dynamic condition variation
static void update_dynamic_conditions(NetworkSimulator *simulator) {
    if (!simulator->enable_dynamic_conditions) return;
    
    uint64_t current_time = get_timestamp_ms();
    uint64_t time_since_start = current_time - simulator->simulation_start_time;
    
    // Create realistic network condition variations over time
    float time_factor = (float)(time_since_start % 60000) / 60000.0f; // 60-second cycle
    float variation = sinf(time_factor * 2.0f * M_PI) * simulator->condition_variation_intensity;
    
    // Apply variations to base conditions
    simulator->current_conditions.latency_ms = 
        simulator->base_conditions.latency_ms * (1.0f + variation * 0.5f);
    simulator->current_conditions.jitter_ms = 
        simulator->base_conditions.jitter_ms * (1.0f + variation * 0.3f);
    
    // Packet loss varies less dramatically
    float loss_variation = sinf(time_factor * 4.0f * M_PI) * simulator->condition_variation_intensity * 0.2f;
    simulator->current_conditions.packet_loss_rate = 
        fmaxf(0.0f, simulator->base_conditions.packet_loss_rate * (1.0f + loss_variation));
    
    // Bandwidth can fluctuate significantly
    float bandwidth_variation = cosf(time_factor * 3.0f * M_PI) * simulator->condition_variation_intensity;
    simulator->current_conditions.bandwidth_limit_mbps = 
        fmaxf(0.1f, simulator->base_conditions.bandwidth_limit_mbps * (1.0f + bandwidth_variation));
}

// Bandwidth throttling
static bool check_bandwidth_limit(BandwidthThrottler *throttler, uint32_t packet_size) {
    if (!throttler->bandwidth_limited) return true;
    
    uint64_t current_time = get_timestamp_ms();
    
    // Reset window if expired
    if (current_time - throttler->window_start_time >= throttler->window_duration_ms) {
        throttler->bytes_sent_in_window = 0;
        throttler->window_start_time = current_time;
    }
    
    // Calculate if packet fits in remaining bandwidth
    float remaining_bandwidth_mbps = throttler->target_bandwidth_mbps;
    float remaining_bytes = remaining_bandwidth_mbps * 1024.0f * 1024.0f / 8.0f / 1000.0f * 
                            (throttler->window_duration_ms - (current_time - throttler->window_start_time));
    
    if (throttler->bytes_sent_in_window + packet_size <= (uint32_t)remaining_bytes) {
        throttler->bytes_sent_in_window += packet_size;
        return true;
    }
    
    return false; // Packet would exceed bandwidth limit
}

// Public API functions
bool network_simulator_init(NetworkPreset preset, bool enable_dynamic_conditions) {
    memset(&g_simulator, 0, sizeof(NetworkSimulator));
    
    g_simulator.preset = preset;
    g_simulator.base_conditions = get_network_preset(preset);
    g_simulator.current_conditions = g_simulator.base_conditions;
    g_simulator.enable_dynamic_conditions = enable_dynamic_conditions;
    g_simulator.condition_variation_intensity = 0.3f; // 30% variation
    g_simulator.simulation_start_time = get_timestamp_ms();
    g_simulator.last_update_time = g_simulator.simulation_start_time;
    
    // Initialize corruption simulator
    g_simulator.corruption.corruption_rate = g_simulator.base_conditions.enable_corruption ? 0.001f : 0.0f;
    g_simulator.corruption.corruption_seed = (uint32_t)time(NULL);
    g_simulator.corruption.enable_bit_flips = true;
    g_simulator.corruption.enable_byte_reordering = false;
    g_simulator.corruption.enable_packet_duplication = false;
    
    // Initialize bandwidth throttler
    g_simulator.bandwidth.target_bandwidth_mbps = g_simulator.base_conditions.bandwidth_limit_mbps;
    g_simulator.bandwidth.current_bandwidth_mbps = g_simulator.bandwidth.target_bandwidth_mbps;
    g_simulator.bandwidth.window_duration_ms = 1000; // 1-second window
    g_simulator.bandwidth.window_start_time = get_timestamp_ms();
    g_simulator.bandwidth.bandwidth_limited = (g_simulator.bandwidth.target_bandwidth_mbps < 1000.0f);
    
    printf("Network simulator initialized with preset: %d\n", preset);
    printf("Base conditions: Latency=%.1fms, Loss=%.1f%%, Bandwidth=%.1fMbps\n",
           g_simulator.base_conditions.latency_ms,
           g_simulator.base_conditions.packet_loss_rate * 100.0f,
           g_simulator.base_conditions.bandwidth_limit_mbps);
    
    return true;
}

void network_simulator_update(void) {
    update_dynamic_conditions(&g_simulator);
    g_simulator.last_update_time = get_timestamp_ms();
}

bool network_simulator_process_packet(uint8_t *packet_data, uint32_t packet_size) {
    if (!packet_data || packet_size == 0) return false;
    
    // Update simulator state
    network_simulator_update();
    
    // Check bandwidth limit
    if (!check_bandwidth_limit(&g_simulator.bandwidth, packet_size)) {
        return false; // Packet dropped due to bandwidth limit
    }
    
    // Check packet loss
    float random = (float)rand() / RAND_MAX;
    if (random < g_simulator.current_conditions.packet_loss_rate) {
        return false; // Packet dropped due to loss
    }
    
    // Apply corruption if enabled
    if (should_corrupt_packet(&g_simulator.corruption)) {
        corrupt_packet_data(packet_data, packet_size, &g_simulator.corruption);
    }
    
    // Apply latency delay
    if (g_simulator.current_conditions.latency_ms > 0.0f) {
        // Add jitter to latency
        float jitter = ((float)rand() / RAND_MAX) * g_simulator.current_conditions.jitter_ms;
        float total_delay = g_simulator.current_conditions.latency_ms + jitter;
        usleep((useconds_t)(total_delay * 1000));
    }
    
    return true; // Packet processed successfully
}

NetworkConditions network_simulator_get_current_conditions(void) {
    return g_simulator.current_conditions;
}

void network_simulator_set_conditions(const NetworkConditions *conditions) {
    if (conditions) {
        g_simulator.base_conditions = *conditions;
        g_simulator.current_conditions = *conditions;
        g_simulator.bandwidth.target_bandwidth_mbps = conditions->bandwidth_limit_mbps;
        g_simulator.bandwidth.current_bandwidth_mbps = conditions->bandwidth_limit_mbps;
        g_simulator.bandwidth.bandwidth_limited = (conditions->bandwidth_limit_mbps < 1000.0f);
    }
}

void network_simulator_set_variation_intensity(float intensity) {
    g_simulator.condition_variation_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

void network_simulator_enable_dynamic_conditions(bool enable) {
    g_simulator.enable_dynamic_conditions = enable;
}

void network_simulator_set_corruption_rate(float rate) {
    g_simulator.corruption.corruption_rate = fmaxf(0.0f, fminf(1.0f, rate));
}

void network_simulator_shutdown(void) {
    memset(&g_simulator, 0, sizeof(NetworkSimulator));
    printf("Network simulator shutdown complete\n");
}

// Utility functions
const char* network_simulator_get_preset_name(NetworkPreset preset) {
    switch (preset) {
        case NETWORK_PRESET_PERFECT: return "Perfect";
        case NETWORK_PRESET_MOBILE_3G: return "Mobile 3G";
        case NETWORK_PRESET_MOBILE_4G: return "Mobile 4G";
        case NETWORK_PRESET_WIFI_GOOD: return "WiFi Good";
        case NETWORK_PRESET_WIFI_POOR: return "WiFi Poor";
        case NETWORK_PRESET_BROADBAND: return "Broadband";
        case NETWORK_PRESET_SATELLITE: return "Satellite";
        case NETWORK_PRESET_DIALUP: return "Dial-up";
        default: return "Unknown";
    }
}

void network_simulator_print_status(void) {
    printf("=== Network Simulator Status ===\n");
    printf("Preset: %s\n", network_simulator_get_preset_name(g_simulator.preset));
    printf("Dynamic Conditions: %s\n", g_simulator.enable_dynamic_conditions ? "Enabled" : "Disabled");
    printf("Variation Intensity: %.1f%%\n", g_simulator.condition_variation_intensity * 100.0f);
    printf("Current Conditions:\n");
    printf("  Latency: %.1f ms\n", g_simulator.current_conditions.latency_ms);
    printf("  Packet Loss: %.1f%%\n", g_simulator.current_conditions.packet_loss_rate * 100.0f);
    printf("  Jitter: %.1f ms\n", g_simulator.current_conditions.jitter_ms);
    printf("  Bandwidth: %.1f Mbps\n", g_simulator.current_conditions.bandwidth_limit_mbps);
    printf("  Corruption: %s\n", g_simulator.current_conditions.enable_corruption ? "Enabled" : "Disabled");
    printf("Corruption Rate: %.3f%%\n", g_simulator.corruption.corruption_rate * 100.0f);
    printf("Bandwidth Limited: %s\n", g_simulator.bandwidth.bandwidth_limited ? "Yes" : "No");
    printf("===============================\n");
}
