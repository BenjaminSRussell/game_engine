// Anti-Cheat and Security Implementation
#include "network/security/authentication.h"
#include "network/security/encryption.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// =================================================================================================
//                              ANTI-CHEAT SYSTEM
// =================================================================================================

#define MAX_VIOLATIONS_BEFORE_BAN 10
#define SPEED_HACK_THRESHOLD 2.5f
#define FLY_HACK_DETECTION_TIME 3000 // ms
#define COMBAT_HACK_WINDOW 1000 // ms
#define POSITION_TOLERANCE 0.1f

typedef struct PlayerSecurityProfile {
    uint64_t player_id;
    char username[64];
    
    // Movement tracking
    Vec3 last_position;
    Vec3 last_velocity;
    uint64_t last_move_time;
    float move_distance_total;
    uint32_t move_count;
    
    // Speed tracking
    float average_speed;
    float max_speed;
    uint32_t speed_violations;
    
    // Combat tracking
    uint64_t last_attack_time;
    uint32_t attacks_in_window;
    uint32_t combat_violations;
    
    // Anomaly detection
    uint32_t total_violations;
    uint64_t last_violation_time;
    bool is_suspicious;
    bool is_banned;
    
    // Client validation
    uint32_t client_hash;
    uint32_t expected_client_hash;
    bool client_validated;
} PlayerSecurityProfile;

typedef struct AntiCheatSystem {
    PlayerSecurityProfile* players;
    uint32_t player_count;
    uint32_t player_capacity;
    
    // Global settings
    bool enabled;
    float max_allowed_speed;
    float max_vertical_speed;
    uint32_t max_attacks_per_second;
    
    // Detection thresholds
    float teleport_distance_threshold;
    uint32_t violation_threshold;
    uint64_t ban_duration_ms;
    
    // Statistics
    uint32_t total_bans;
    uint32_t total_kicks;
    uint32_t speed_hacks_detected;
    uint32_t fly_hacks_detected;
    uint32_t combat_hacks_detected;
} AntiCheatSystem;

static AntiCheatSystem g_anti_cheat = {0};

// Helper functions
static float calculate_distance(Vec3 a, Vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static float calculate_speed(Vec3 velocity) {
    return sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
}

static PlayerSecurityProfile* find_or_create_profile(uint64_t player_id, const char* username) {
    // Find existing profile
    for (uint32_t i = 0; i < g_anti_cheat.player_count; i++) {
        if (g_anti_cheat.players[i].player_id == player_id) {
            return &g_anti_cheat.players[i];
        }
    }
    
    // Create new profile
    if (g_anti_cheat.player_count >= g_anti_cheat.player_capacity) {
        uint32_t new_capacity = g_anti_cheat.player_capacity == 0 ? 64 : g_anti_cheat.player_capacity * 2;
        PlayerSecurityProfile* new_players = realloc(g_anti_cheat.players, new_capacity * sizeof(PlayerSecurityProfile));
        if (!new_players) {
            return NULL;
        }
        g_anti_cheat.players = new_players;
        g_anti_cheat.player_capacity = new_capacity;
    }
    
    PlayerSecurityProfile* profile = &g_anti_cheat.players[g_anti_cheat.player_count++];
    memset(profile, 0, sizeof(PlayerSecurityProfile));
    profile->player_id = player_id;
    if (username) {
        strncpy(profile->username, username, sizeof(profile->username) - 1);
    }
    profile->last_move_time = get_current_time_ms();
    
    return profile;
}

// Main anti-cheat functions
bool anticheat_init(uint32_t max_players) {
    memset(&g_anti_cheat, 0, sizeof(AntiCheatSystem));
    
    g_anti_cheat.players = calloc(max_players, sizeof(PlayerSecurityProfile));
    if (!g_anti_cheat.players) {
        LOG_ERROR("Failed to allocate anti-cheat player profiles");
        return false;
    }
    
    g_anti_cheat.player_capacity = max_players;
    g_anti_cheat.enabled = true;
    g_anti_cheat.max_allowed_speed = 10.0f; // blocks per second
    g_anti_cheat.max_vertical_speed = 5.0f;
    g_anti_cheat.max_attacks_per_second = 5;
    g_anti_cheat.teleport_distance_threshold = 50.0f;
    g_anti_cheat.violation_threshold = MAX_VIOLATIONS_BEFORE_BAN;
    g_anti_cheat.ban_duration_ms = 3600000; // 1 hour
    
    LOG_INFO("Anti-cheat system initialized for %u players", max_players);
    return true;
}

void anticheat_shutdown(void) {
    free(g_anti_cheat.players);
    memset(&g_anti_cheat, 0, sizeof(AntiCheatSystem));
    LOG_INFO("Anti-cheat system shutdown");
}

bool anticheat_validate_movement(uint64_t player_id, const char* username, 
                                 Vec3 new_position, Vec3 new_velocity, uint64_t timestamp) {
    if (!g_anti_cheat.enabled) {
        return true;
    }
    
    PlayerSecurityProfile* profile = find_or_create_profile(player_id, username);
    if (!profile || profile->is_banned) {
        return false;
    }
    
    uint64_t time_delta = timestamp - profile->last_move_time;
    if (time_delta == 0) return true; // Ignore same-timestamp updates
    
    float distance = calculate_distance(profile->last_position, new_position);
    float speed = distance / (time_delta / 1000.0f); // Convert to units per second
    float vertical_speed = fabsf(new_velocity.y);
    
    // Update statistics
    profile->move_distance_total += distance;
    profile->move_count++;
    profile->average_speed = profile->move_distance_total / (profile->move_count * (time_delta / 1000.0f));
    if (speed > profile->max_speed) {
        profile->max_speed = speed;
    }
    
    // Speed hack detection
    if (speed > g_anti_cheat.max_allowed_speed * SPEED_HACK_THRESHOLD) {
        profile->speed_violations++;
        profile->total_violations++;
        profile->last_violation_time = timestamp;
        profile->is_suspicious = true;
        
        g_anti_cheat.speed_hacks_detected++;
        LOG_WARN("Speed hack detected: %s (player_id=%llu) speed=%.2f", 
                 username, player_id, speed);
        
        if (profile->total_violations >= g_anti_cheat.violation_threshold) {
            anticheat_ban_player(player_id, "Speed hacking");
            return false;
        }
    }
    
    // Fly hack detection (sustained vertical movement)
    if (vertical_speed > g_anti_cheat.max_vertical_speed && time_delta < FLY_HACK_DETECTION_TIME) {
        profile->total_violations++;
        profile->last_violation_time = timestamp;
        profile->is_suspicious = true;
        
        g_anti_cheat.fly_hacks_detected++;
        LOG_WARN("Fly hack detected: %s (player_id=%llu) vertical_speed=%.2f", 
                 username, player_id, vertical_speed);
        
        if (profile->total_violations >= g_anti_cheat.violation_threshold) {
            anticheat_ban_player(player_id, "Fly hacking");
            return false;
        }
    }
    
    // Teleportation detection
    if (distance > g_anti_cheat.teleport_distance_threshold && time_delta < 1000) {
        profile->total_violations++;
        profile->last_violation_time = timestamp;
        profile->is_suspicious = true;
        
        LOG_WARN("Teleportation detected: %s (player_id=%llu) distance=%.2f in %llums", 
                 username, player_id, distance, time_delta);
        
        if (profile->total_violations >= g_anti_cheat.violation_threshold) {
            anticheat_ban_player(player_id, "Teleportation hacking");
            return false;
        }
    }
    
    // Update profile
    profile->last_position = new_position;
    profile->last_velocity = new_velocity;
    profile->last_move_time = timestamp;
    
    return true;
}

bool anticheat_validate_combat(uint64_t player_id, const char* username, 
                              uint64_t timestamp, bool hit_registered) {
    if (!g_anti_cheat.enabled || !hit_registered) {
        return true;
    }
    
    PlayerSecurityProfile* profile = find_or_create_profile(player_id, username);
    if (!profile || profile->is_banned) {
        return false;
    }
    
    uint64_t time_since_last_attack = timestamp - profile->last_attack_time;
    
    // Reset attack window if too much time has passed
    if (time_since_last_attack > COMBAT_HACK_WINDOW) {
        profile->attacks_in_window = 0;
    }
    
    profile->attacks_in_window++;
    profile->last_attack_time = timestamp;
    
    // Combat speed hack detection
    if (profile->attacks_in_window > g_anti_cheat.max_attacks_per_second) {
        profile->combat_violations++;
        profile->total_violations++;
        profile->last_violation_time = timestamp;
        profile->is_suspicious = true;
        
        g_anti_cheat.combat_hacks_detected++;
        LOG_WARN("Combat hack detected: %s (player_id=%llu) attacks=%u in window", 
                 username, player_id, profile->attacks_in_window);
        
        if (profile->total_violations >= g_anti_cheat.violation_threshold) {
            anticheat_ban_player(player_id, "Combat hacking");
            return false;
        }
    }
    
    return true;
}

bool anticheat_validate_client(uint64_t player_id, const char* username, uint32_t client_hash) {
    if (!g_anti_cheat.enabled) {
        return true;
    }
    
    PlayerSecurityProfile* profile = find_or_create_profile(player_id, username);
    if (!profile || profile->is_banned) {
        return false;
    }
    
    profile->client_hash = client_hash;
    
    // In a real implementation, this would validate against known good client hashes
    // For now, we'll just accept any hash but flag it for review
    if (profile->expected_client_hash != 0 && profile->client_hash != profile->expected_client_hash) {
        profile->total_violations++;
        profile->last_violation_time = timestamp;
        profile->is_suspicious = true;
        
        LOG_WARN("Client hash mismatch: %s (player_id=%llu) expected=%08x got=%08x", 
                 username, player_id, profile->expected_client_hash, profile->client_hash);
        
        return false;
    }
    
    profile->client_validated = true;
    return true;
}

void anticheat_ban_player(uint64_t player_id, const char* reason) {
    PlayerSecurityProfile* profile = NULL;
    
    for (uint32_t i = 0; i < g_anti_cheat.player_count; i++) {
        if (g_anti_cheat.players[i].player_id == player_id) {
            profile = &g_anti_cheat.players[i];
            break;
        }
    }
    
    if (!profile) {
        return;
    }
    
    profile->is_banned = true;
    profile->last_violation_time = get_current_time_ms();
    
    g_anti_cheat.total_bans++;
    
    LOG_ERROR("Player banned: %s (player_id=%llu) reason=%s violations=%u", 
              profile->username, player_id, reason, profile->total_violations);
    
    // TODO: Notify network system to disconnect player
    // TODO: Add to ban database
}

void anticheat_kick_player(uint64_t player_id, const char* reason) {
    PlayerSecurityProfile* profile = NULL;
    
    for (uint32_t i = 0; i < g_anti_cheat.player_count; i++) {
        if (g_anti_cheat.players[i].player_id == player_id) {
            profile = &g_anti_cheat.players[i];
            break;
        }
    }
    
    if (!profile) {
        return;
    }
    
    g_anti_cheat.total_kicks++;
    
    LOG_WARN("Player kicked: %s (player_id=%llu) reason=%s", profile->username, player_id, reason);
    
    // TODO: Notify network system to disconnect player
}

bool anticheat_is_player_banned(uint64_t player_id) {
    for (uint32_t i = 0; i < g_anti_cheat.player_count; i++) {
        if (g_anti_cheat.players[i].player_id == player_id) {
            return g_anti_cheat.players[i].is_banned;
        }
    }
    return false;
}

void anticheat_get_statistics(uint32_t* total_bans, uint32_t* total_kicks, 
                             uint32_t* speed_hacks, uint32_t* fly_hacks, uint32_t* combat_hacks) {
    if (total_bans) *total_bans = g_anti_cheat.total_bans;
    if (total_kicks) *total_kicks = g_anti_cheat.total_kicks;
    if (speed_hacks) *speed_hacks = g_anti_cheat.speed_hacks_detected;
    if (fly_hacks) *fly_hacks = g_anti_cheat.fly_hacks_detected;
    if (combat_hacks) *combat_hacks = g_anti_cheat.combat_hacks_detected;
}

void anticheat_update_settings(float max_speed, float max_vertical_speed, 
                              uint32_t max_attacks_per_second, uint32_t violation_threshold) {
    g_anti_cheat.max_allowed_speed = max_speed;
    g_anti_cheat.max_vertical_speed = max_vertical_speed;
    g_anti_cheat.max_attacks_per_second = max_attacks_per_second;
    g_anti_cheat.violation_threshold = violation_threshold;
    
    LOG_INFO("Anti-cheat settings updated: max_speed=%.2f, max_vertical_speed=%.2f, max_attacks=%u, threshold=%u",
             max_speed, max_vertical_speed, max_attacks_per_second, violation_threshold);
}
