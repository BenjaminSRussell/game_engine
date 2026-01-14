/**
 * Network Condition Simulator
 * Header file for realistic network condition simulation
 */

#ifndef NETWORK_CONDITION_SIMULATOR_H
#define NETWORK_CONDITION_SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>

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

// Forward declaration of NetworkConditions from stress test
typedef struct {
    float latency_ms;
    float packet_loss_rate;
    float bandwidth_limit_mbps;
    float jitter_ms;
    bool enable_corruption;
} NetworkConditions;

// Public API functions

/**
 * Initialize the network condition simulator
 * @param preset Network preset to use
 * @param enable_dynamic_conditions Enable realistic condition variations over time
 * @return true if initialization successful, false otherwise
 */
bool network_simulator_init(NetworkPreset preset, bool enable_dynamic_conditions);

/**
 * Update simulator state (call regularly for dynamic conditions)
 */
void network_simulator_update(void);

/**
 * Process a packet through the network simulator
 * @param packet_data Buffer containing packet data (may be modified by corruption)
 * @param packet_size Size of packet data in bytes
 * @return true if packet should be delivered, false if dropped
 */
bool network_simulator_process_packet(uint8_t *packet_data, uint32_t packet_size);

/**
 * Get current network conditions
 * @return Current network conditions
 */
NetworkConditions network_simulator_get_current_conditions(void);

/**
 * Set custom network conditions
 * @param conditions Pointer to network conditions structure
 */
void network_simulator_set_conditions(const NetworkConditions *conditions);

/**
 * Set variation intensity for dynamic conditions
 * @param intensity Variation intensity (0.0 to 1.0)
 */
void network_simulator_set_variation_intensity(float intensity);

/**
 * Enable or disable dynamic condition variations
 * @param enable true to enable dynamic conditions, false to disable
 */
void network_simulator_enable_dynamic_conditions(bool enable);

/**
 * Set packet corruption rate
 * @param rate Corruption rate (0.0 to 1.0)
 */
void network_simulator_set_corruption_rate(float rate);

/**
 * Shutdown the network condition simulator
 */
void network_simulator_shutdown(void);

// Utility functions

/**
 * Get network preset name
 * @param preset Network preset enum
 * @return String name of the preset
 */
const char* network_simulator_get_preset_name(NetworkPreset preset);

/**
 * Print current simulator status
 */
void network_simulator_print_status(void);

// Helper functions for creating network conditions

/**
 * Create network conditions for a specific preset
 * @param preset Network preset
 * @return NetworkConditions structure
 */
static inline NetworkConditions network_simulator_preset_conditions(NetworkPreset preset) {
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
            conditions.latency_ms = 10.0f;
            conditions.packet_loss_rate = 0.01f;
            conditions.bandwidth_limit_mbps = 100.0f;
            conditions.jitter_ms = 5.0f;
            conditions.enable_corruption = false;
            break;
    }
    
    return conditions;
}

/**
 * Check if a network preset is considered "poor" quality
 * @param preset Network preset
 * @return true if poor quality, false otherwise
 */
static inline bool network_simulator_is_poor_quality(NetworkPreset preset) {
    return (preset == NETWORK_PRESET_MOBILE_3G || 
            preset == NETWORK_PRESET_WIFI_POOR || 
            preset == NETWORK_PRESET_SATELLITE || 
            preset == NETWORK_PRESET_DIALUP);
}

/**
 * Check if a network preset is considered "mobile"
 * @param preset Network preset
 * @return true if mobile network, false otherwise
 */
static inline bool network_simulator_is_mobile(NetworkPreset preset) {
    return (preset == NETWORK_PRESET_MOBILE_3G || preset == NETWORK_PRESET_MOBILE_4G);
}

/**
 * Check if a network preset is considered "wireless"
 * @param preset Network preset
 * @return true if wireless network, false otherwise
 */
static inline bool network_simulator_is_wireless(NetworkPreset preset) {
    return (preset == NETWORK_PRESET_MOBILE_3G || 
            preset == NETWORK_PRESET_MOBILE_4G || 
            preset == NETWORK_PRESET_WIFI_GOOD || 
            preset == NETWORK_PRESET_WIFI_POOR);
}

/**
 * Get expected latency range for a preset
 * @param preset Network preset
 * @param min_latency Output for minimum expected latency
 * @param max_latency Output for maximum expected latency
 */
static inline void network_simulator_get_latency_range(NetworkPreset preset, 
                                                     float *min_latency, float *max_latency) {
    switch (preset) {
        case NETWORK_PRESET_PERFECT:
            *min_latency = 0.5f; *max_latency = 2.0f; break;
        case NETWORK_PRESET_MOBILE_3G:
            *min_latency = 100.0f; *max_latency = 300.0f; break;
        case NETWORK_PRESET_MOBILE_4G:
            *min_latency = 30.0f; *max_latency = 80.0f; break;
        case NETWORK_PRESET_WIFI_GOOD:
            *min_latency = 5.0f; *max_latency = 20.0f; break;
        case NETWORK_PRESET_WIFI_POOR:
            *min_latency = 50.0f; *max_latency = 200.0f; break;
        case NETWORK_PRESET_BROADBAND:
            *min_latency = 10.0f; *max_latency = 40.0f; break;
        case NETWORK_PRESET_SATELLITE:
            *min_latency = 500.0f; *max_latency = 800.0f; break;
        case NETWORK_PRESET_DIALUP:
            *min_latency = 100.0f; *max_latency = 250.0f; break;
        default:
            *min_latency = 10.0f; *max_latency = 50.0f; break;
    }
}

#endif // NETWORK_CONDITION_SIMULATOR_H
