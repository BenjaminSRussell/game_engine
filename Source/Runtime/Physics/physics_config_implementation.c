// physics_config_implementation.c
// Physics Configuration System Implementation

#include "physics.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// -----------------------------------------------------------------------------
// Configuration Presets
// -----------------------------------------------------------------------------

static PhysicsConfig g_current_config = {0};
static PhysicsPreset g_current_preset = PHYSICS_PRESET_DEFAULT;

// Material presets
static PhysicsMaterial g_material_presets[] = {
    {"default", 0.5f, 0.3f, 1000.0f},
    {"ice", 0.02f, 0.1f, 917.0f},
    {"rubber", 0.9f, 0.8f, 1500.0f},
    {"metal", 0.6f, 0.2f, 7850.0f},
    {"wood", 0.4f, 0.4f, 700.0f},
    {"stone", 0.7f, 0.3f, 2500.0f},
    {"glass", 0.1f, 0.9f, 2500.0f},
    {"plastic", 0.3f, 0.5f, 1200.0f}
};

static const u32 g_material_count = sizeof(g_material_presets) / sizeof(g_material_presets[0]);

// -----------------------------------------------------------------------------
// Configuration Management
// -----------------------------------------------------------------------------

void physics_config_init(void) {
    // Initialize with default preset
    g_current_config = physics_config_get_default();
    g_current_preset = PHYSICS_PRESET_DEFAULT;
    
    LOG_INFO("Physics configuration system initialized");
}

PhysicsConfig physics_config_get_default(void) {
    PhysicsConfig config = {
        .gravity = {0.0f, -9.81f, 0.0f},
        .fixed_timestep = 1.0f / 60.0f,
        .velocity_iterations = 8,
        .position_iterations = 3
    };
    
    LOG_DEBUG("Default physics config: gravity(0, -9.81, 0), timestep(1/60), vel_iter(8), pos_iter(3)");
    return config;
}

PhysicsConfig physics_config_get_block_game(void) {
    PhysicsConfig config = {
        .gravity = {0.0f, -16.0f, 0.0f},  // Stronger gravity for block game feel
        .fixed_timestep = 1.0f / 60.0f,
        .velocity_iterations = 6,
        .position_iterations = 2
    };
    
    LOG_DEBUG("Block game physics config: gravity(0, -16, 0), timestep(1/60), vel_iter(6), pos_iter(2)");
    return config;
}

PhysicsConfig physics_config_get_space(void) {
    PhysicsConfig config = {
        .gravity = {0.0f, 0.0f, 0.0f},  // No gravity in space
        .fixed_timestep = 1.0f / 60.0f,
        .velocity_iterations = 10,
        .position_iterations = 4
    };
    
    LOG_DEBUG("Space physics config: gravity(0, 0, 0), timestep(1/60), vel_iter(10), pos_iter(4)");
    return config;
}

PhysicsConfig physics_config_get_low_gravity(void) {
    PhysicsConfig config = {
        .gravity = {0.0f, -3.7f, 0.0f},  // Mars-like gravity
        .fixed_timestep = 1.0f / 60.0f,
        .velocity_iterations = 8,
        .position_iterations = 3
    };
    
    LOG_DEBUG("Low gravity physics config: gravity(0, -3.7, 0), timestep(1/60), vel_iter(8), pos_iter(3)");
    return config;
}

PhysicsConfig physics_config_get_current(void) {
    return g_current_config;
}

void physics_config_set_current(PhysicsConfig config) {
    g_current_config = config;
    g_current_preset = PHYSICS_PRESET_CUSTOM;
    
    LOG_INFO("Physics config set to custom: gravity(%.2f, %.2f, %.2f), timestep(%.4f), vel_iter(%d), pos_iter(%d)",
             config.gravity.x, config.gravity.y, config.gravity.z,
             config.fixed_timestep, config.velocity_iterations, config.position_iterations);
}

void physics_config_load_preset(PhysicsPreset preset) {
    switch (preset) {
        case PHYSICS_PRESET_DEFAULT:
            g_current_config = physics_config_get_default();
            break;
        case PHYSICS_PRESET_BLOCK_GAME:
            g_current_config = physics_config_get_block_game();
            break;
        case PHYSICS_PRESET_SPACE:
            g_current_config = physics_config_get_space();
            break;
        case PHYSICS_PRESET_LOW_GRAVITY:
            g_current_config = physics_config_get_low_gravity();
            break;
        case PHYSICS_PRESET_CUSTOM:
        default:
            LOG_WARN("Cannot load CUSTOM preset, use physics_config_set_current() instead");
            return;
    }
    
    g_current_preset = preset;
    
    LOG_INFO("Physics preset loaded: %d", preset);
}

void physics_config_load_preset_by_name(const char* name) {
    if (!name) {
        LOG_ERROR("Cannot load preset with NULL name");
        return;
    }
    
    if (strcmp(name, "default") == 0) {
        physics_config_load_preset(PHYSICS_PRESET_DEFAULT);
    } else if (strcmp(name, "block_game") == 0 || strcmp(name, "minecraft") == 0) {
        physics_config_load_preset(PHYSICS_PRESET_BLOCK_GAME);
    } else if (strcmp(name, "space") == 0) {
        physics_config_load_preset(PHYSICS_PRESET_SPACE);
    } else if (strcmp(name, "low_gravity") == 0 || strcmp(name, "mars") == 0) {
        physics_config_load_preset(PHYSICS_PRESET_LOW_GRAVITY);
    } else {
        LOG_ERROR("Unknown physics preset: %s", name);
    }
}

PhysicsPreset physics_config_get_current_preset(void) {
    return g_current_preset;
}

// -----------------------------------------------------------------------------
// Runtime Adjustments
// -----------------------------------------------------------------------------

void physics_config_set_gravity(Vec3 gravity) {
    g_current_config.gravity = gravity;
    g_current_preset = PHYSICS_PRESET_CUSTOM;
    
    LOG_INFO("Physics gravity set to: (%.2f, %.2f, %.2f)", gravity.x, gravity.y, gravity.z);
}

void physics_config_set_timestep(f32 timestep) {
    if (timestep <= 0.0f) {
        LOG_ERROR("Invalid timestep: %.4f, must be positive", timestep);
        return;
    }
    
    g_current_config.fixed_timestep = timestep;
    g_current_preset = PHYSICS_PRESET_CUSTOM;
    
    LOG_INFO("Physics timestep set to: %.4f", timestep);
}

void physics_config_set_iterations(u32 velocity_iterations, u32 position_iterations) {
    if (velocity_iterations == 0 || position_iterations == 0) {
        LOG_ERROR("Invalid iterations: vel=%d, pos=%d, must be positive", 
                  velocity_iterations, position_iterations);
        return;
    }
    
    g_current_config.velocity_iterations = velocity_iterations;
    g_current_config.position_iterations = position_iterations;
    g_current_preset = PHYSICS_PRESET_CUSTOM;
    
    LOG_INFO("Physics iterations set to: vel=%d, pos=%d", velocity_iterations, position_iterations);
}

// -----------------------------------------------------------------------------
// Configuration Validation
// -----------------------------------------------------------------------------

bool physics_config_validate(const PhysicsConfig* config) {
    if (!config) {
        LOG_ERROR("Cannot validate NULL config");
        return false;
    }
    
    bool valid = true;
    
    // Check timestep
    if (config->fixed_timestep <= 0.0f || config->fixed_timestep > 1.0f) {
        LOG_ERROR("Invalid timestep: %.4f, must be in range (0, 1]", config->fixed_timestep);
        valid = false;
    }
    
    // Check iterations
    if (config->velocity_iterations == 0 || config->velocity_iterations > 50) {
        LOG_ERROR("Invalid velocity iterations: %d, must be in range [1, 50]", 
                  config->velocity_iterations);
        valid = false;
    }
    
    if (config->position_iterations == 0 || config->position_iterations > 20) {
        LOG_ERROR("Invalid position iterations: %d, must be in range [1, 20]", 
                  config->position_iterations);
        valid = false;
    }
    
    // Check gravity magnitude (reasonable limits)
    f32 gravity_magnitude = sqrtf(config->gravity.x * config->gravity.x + 
                                  config->gravity.y * config->gravity.y + 
                                  config->gravity.z * config->gravity.z);
    if (gravity_magnitude > 100.0f) {
        LOG_WARN("High gravity magnitude: %.2f, may cause instability", gravity_magnitude);
    }
    
    if (valid) {
        LOG_DEBUG("Physics configuration is valid");
    } else {
        LOG_ERROR("Physics configuration validation failed");
    }
    
    return valid;
}

// -----------------------------------------------------------------------------
// Material Management
// -----------------------------------------------------------------------------

PhysicsMaterial physics_material_get_default(void) {
    PhysicsMaterial material = {
        .friction = 0.5f,
        .restitution = 0.3f,
        .density = 1000.0f
    };
    
    LOG_DEBUG("Default physics material: friction(0.5), restitution(0.3), density(1000.0)");
    return material;
}

PhysicsMaterial physics_material_get_by_name(const char* name) {
    if (!name) {
        LOG_ERROR("Cannot get material with NULL name");
        return physics_material_get_default();
    }
    
    for (u32 i = 0; i < g_material_count; i++) {
        if (strcmp(g_material_presets[i].name, name) == 0) {
            LOG_DEBUG("Found physics material: %s", name);
            return g_material_presets[i];
        }
    }
    
    LOG_WARN("Physics material not found: %s, using default", name);
    return physics_material_get_default();
}

bool physics_material_get_preset_by_name(const char* name, PhysicsMaterial* out_material) {
    if (!name || !out_material) {
        LOG_ERROR("Invalid parameters for material preset lookup");
        return false;
    }
    
    for (u32 i = 0; i < g_material_count; i++) {
        if (strcmp(g_material_presets[i].name, name) == 0) {
            *out_material = g_material_presets[i];
            LOG_DEBUG("Loaded physics material preset: %s", name);
            return true;
        }
    }
    
    LOG_WARN("Physics material preset not found: %s", name);
    return false;
}

void physics_material_list_presets(void) {
    LOG_INFO("Available physics material presets:");
    
    for (u32 i = 0; i < g_material_count; i++) {
        LOG_INFO("  %s: friction=%.2f, restitution=%.2f, density=%.1f",
                 g_material_presets[i].name,
                 g_material_presets[i].friction,
                 g_material_presets[i].restitution,
                 g_material_presets[i].density);
    }
}

// -----------------------------------------------------------------------------
// Configuration Serialization
// -----------------------------------------------------------------------------

typedef struct {
    u32 magic;
    u32 version;
    PhysicsConfig config;
    PhysicsPreset preset;
    f32 checksum;
} PhysicsConfigFile;

#define PHYSICS_CONFIG_MAGIC 0x50484346  // "PHCF"
#define PHYSICS_CONFIG_VERSION 1

bool physics_config_save(const char* filename) {
    if (!filename) {
        LOG_ERROR("Cannot save physics config to NULL filename");
        return false;
    }
    
    PhysicsConfigFile file_data = {
        .magic = PHYSICS_CONFIG_MAGIC,
        .version = PHYSICS_CONFIG_VERSION,
        .config = g_current_config,
        .preset = g_current_preset
    };
    
    // Calculate simple checksum
    file_data.checksum = file_data.config.gravity.x + file_data.config.gravity.y + 
                         file_data.config.gravity.z + file_data.config.fixed_timestep +
                         (f32)file_data.config.velocity_iterations + 
                         (f32)file_data.config.position_iterations;
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        LOG_ERROR("Failed to open physics config file for writing: %s", filename);
        return false;
    }
    
    size_t written = fwrite(&file_data, sizeof(PhysicsConfigFile), 1, file);
    fclose(file);
    
    if (written != 1) {
        LOG_ERROR("Failed to write physics config file: %s", filename);
        return false;
    }
    
    LOG_INFO("Physics configuration saved to: %s", filename);
    return true;
}

bool physics_config_load(const char* filename) {
    if (!filename) {
        LOG_ERROR("Cannot load physics config from NULL filename");
        return false;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        LOG_ERROR("Failed to open physics config file for reading: %s", filename);
        return false;
    }
    
    PhysicsConfigFile file_data;
    size_t read = fread(&file_data, sizeof(PhysicsConfigFile), 1, file);
    fclose(file);
    
    if (read != 1) {
        LOG_ERROR("Failed to read physics config file: %s", filename);
        return false;
    }
    
    // Validate magic and version
    if (file_data.magic != PHYSICS_CONFIG_MAGIC) {
        LOG_ERROR("Invalid physics config file magic: 0x%08X", file_data.magic);
        return false;
    }
    
    if (file_data.version != PHYSICS_CONFIG_VERSION) {
        LOG_ERROR("Unsupported physics config file version: %d", file_data.version);
        return false;
    }
    
    // Validate checksum
    f32 expected_checksum = file_data.config.gravity.x + file_data.config.gravity.y + 
                           file_data.config.gravity.z + file_data.config.fixed_timestep +
                           (f32)file_data.config.velocity_iterations + 
                           (f32)file_data.config.position_iterations;
    
    if (fabsf(file_data.checksum - expected_checksum) > 0.001f) {
        LOG_WARN("Physics config file checksum mismatch, data may be corrupted");
    }
    
    // Validate configuration
    if (!physics_config_validate(&file_data.config)) {
        LOG_ERROR("Loaded physics configuration is invalid");
        return false;
    }
    
    // Apply configuration
    g_current_config = file_data.config;
    g_current_preset = file_data.preset;
    
    LOG_INFO("Physics configuration loaded from: %s", filename);
    return true;
}

// -----------------------------------------------------------------------------
// Configuration Debugging
// -----------------------------------------------------------------------------

void physics_config_print_current(void) {
    LOG_INFO("Current Physics Configuration:");
    LOG_INFO("  Preset: %d", g_current_preset);
    LOG_INFO("  Gravity: (%.2f, %.2f, %.2f)", 
             g_current_config.gravity.x, g_current_config.gravity.y, g_current_config.gravity.z);
    LOG_INFO("  Fixed Timestep: %.4f seconds (%.1f FPS)", 
             g_current_config.fixed_timestep, 1.0f / g_current_config.fixed_timestep);
    LOG_INFO("  Velocity Iterations: %d", g_current_config.velocity_iterations);
    LOG_INFO("  Position Iterations: %d", g_current_config.position_iterations);
    
    // Calculate gravity magnitude
    f32 gravity_magnitude = sqrtf(g_current_config.gravity.x * g_current_config.gravity.x + 
                                  g_current_config.gravity.y * g_current_config.gravity.y + 
                                  g_current_config.gravity.z * g_current_config.gravity.z);
    LOG_INFO("  Gravity Magnitude: %.2f m/s²", gravity_magnitude);
}

void physics_config_print_presets(void) {
    LOG_INFO("Available Physics Presets:");
    LOG_INFO("  0: DEFAULT - Standard Earth gravity (9.81 m/s²)");
    LOG_INFO("  1: BLOCK_GAME - Stronger gravity for block games (16.0 m/s²)");
    LOG_INFO("  2: SPACE - Zero gravity for space simulations");
    LOG_INFO("  3: LOW_GRAVITY - Mars-like gravity (3.7 m/s²)");
    LOG_INFO("  4: CUSTOM - User-defined configuration");
}

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------

static f32 clamp_f32(f32 value, f32 min, f32 max) {
    return fmaxf(min, fminf(max, value));
}
