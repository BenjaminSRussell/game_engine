#include "character_customization.h"
#include "core/common/memory/allocator.h"
#include "engine/include/core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_CUSTOMIZATION_OPTIONS 256
#define MAX_PRESET_OUTFITS 64
#define MAX_COLOR_PRESETS 128
#define MAX_CUSTOMIZATION_PARTS 32
#define OUTFIT_NAME_LENGTH 64
#define PRESET_NAME_LENGTH 32

typedef enum customization_part {
    CUSTOM_PART_HEAD = 0,
    CUSTOM_PART_HAIR,
    CUSTOM_PART_FACE,
    CUSTOM_PART_EYES,
    CUSTOM_PART_EYEBROWS,
    CUSTOM_PART_NOSE,
    CUSTOM_PART_MOUTH,
    CUSTOM_PART_BEARD,
    CUSTOM_PART_BODY,
    CUSTOM_PART_TORSO,
    CUSTOM_PART_ARMS,
    CUSTOM_PART_LEGS,
    CUSTOM_PART_HANDS,
    CUSTOM_PART_FEET,
    CUSTOM_PART_ACCESSORY_1,
    CUSTOM_PART_ACCESSORY_2,
    CUSTOM_PART_TATTOO_1,
    CUSTOM_PART_TATTOO_2,
    CUSTOM_PART_SCAR,
    CUSTOM_PART_COUNT
} customization_part_t;

typedef enum customization_type {
    CUSTOM_TYPE_COLOR = 0,
    CUSTOM_TYPE_MODEL,
    CUSTOM_TYPE_TEXTURE,
    CUSTOM_TYPE_MATERIAL,
    CUSTOM_TYPE_SIZE,
    CUSTOM_TYPE_POSITION,
    CUSTOM_TYPE_ROTATION,
    CUSTOM_TYPE_SCALE
} customization_type_t;

typedef struct customization_option {
    uint32_t option_id;
    customization_part_t part;
    customization_type_t type;
    char name[PRESET_NAME_LENGTH];
    
    union {
        struct {
            float r, g, b, a;
        } color;
        
        struct {
            uint32_t model_id;
            uint32_t variant_id;
        } model;
        
        struct {
            uint32_t texture_id;
            float u_offset, v_offset;
            float u_scale, v_scale;
        } texture;
        
        struct {
            uint32_t material_id;
            float roughness, metallic;
        } material;
        
        struct {
            float width, height, depth;
        } size;
        
        struct {
            float x, y, z;
        } position;
        
        struct {
            float pitch, yaw, roll;
        } rotation;
        
        struct {
            float x, y, z;
        } scale;
    } data;
    
    float min_value, max_value;
    bool is_enabled;
} customization_option_t;

typedef struct outfit_preset {
    uint32_t preset_id;
    char name[OUTFIT_NAME_LENGTH];
    
    customization_option_t options[MAX_CUSTOMIZATION_PARTS];
    uint32_t option_count;
    
    uint32_t required_level;
    bool is_premium;
    uint32_t cost;
    
    bool is_unlocked;
} outfit_preset_t;

typedef struct character_customization {
    uint32_t character_id;
    
    customization_option_t current_options[MAX_CUSTOMIZATION_PARTS];
    uint32_t current_option_count;
    
    outfit_preset_t saved_outfits[MAX_PRESET_OUTFITS];
    uint32_t saved_outfit_count;
    
    float body_scale;
    float muscle_tone;
    float height;
    
    bool is_dirty;
    uint32_t last_modified_time;
} character_customization_t;

typedef struct customization_system {
    customization_option_t available_options[MAX_CUSTOMIZATION_OPTIONS];
    uint32_t available_option_count;
    
    outfit_preset_t preset_outfits[MAX_PRESET_OUTFITS];
    uint32_t preset_outfit_count;
    
    character_customization_t characters[MAX_PLAYERS];
    uint32_t character_count;
    
    uint32_t next_option_id;
    uint32_t next_preset_id;
    
    bool initialized;
} customization_system_t;

static customization_system_t g_customization = {0};

// Forward declarations
static customization_option_t* get_option(uint32_t option_id);
static character_customization_t* get_character_customization(uint32_t character_id);
static bool apply_option_to_character(character_customization_t* character, const customization_option_t* option);
static void update_character_mesh(uint32_t character_id);

bool customization_init(void) {
    if (g_customization.initialized) {
        return true;
    }
    
    memset(&g_customization, 0, sizeof(g_customization));
    g_customization.next_option_id = 1;
    g_customization.next_preset_id = 1;
    
    g_customization.initialized = true;
    log_info("Character customization system initialized");
    return true;
}

void customization_shutdown(void) {
    if (!g_customization.initialized) {
        return;
    }
    
    memset(&g_customization, 0, sizeof(g_customization));
    log_info("Character customization system shutdown");
}

uint32_t customization_add_option(customization_part_t part, customization_type_t type, const char* name) {
    if (!g_customization.initialized || !name || g_customization.available_option_count >= MAX_CUSTOMIZATION_OPTIONS) {
        return 0;
    }
    
    customization_option_t* option = &g_customization.available_options[g_customization.available_option_count];
    memset(option, 0, sizeof(customization_option_t));
    
    option->option_id = g_customization.next_option_id++;
    option->part = part;
    option->type = type;
    strncpy(option->name, name, PRESET_NAME_LENGTH - 1);
    option->is_enabled = true;
    
    g_customization.available_option_count++;
    log_debug("Added customization option: %s (ID: %u)", name, option->option_id);
    
    return option->option_id;
}

bool customization_set_color_option(uint32_t option_id, float r, float g, float b, float a) {
    customization_option_t* option = get_option(option_id);
    if (!option || option->type != CUSTOM_TYPE_COLOR) {
        return false;
    }
    
    option->data.color.r = r;
    option->data.color.g = g;
    option->data.color.b = b;
    option->data.color.a = a;
    
    log_debug("Set color for option %u: (%.2f, %.2f, %.2f, %.2f)", 
             option_id, r, g, b, a);
    
    return true;
}

bool customization_set_model_option(uint32_t option_id, uint32_t model_id, uint32_t variant_id) {
    customization_option_t* option = get_option(option_id);
    if (!option || option->type != CUSTOM_TYPE_MODEL) {
        return false;
    }
    
    option->data.model.model_id = model_id;
    option->data.model.variant_id = variant_id;
    
    log_debug("Set model for option %u: model %u, variant %u", 
             option_id, model_id, variant_id);
    
    return true;
}

bool customization_set_texture_option(uint32_t option_id, uint32_t texture_id, 
                                   float u_offset, float v_offset, 
                                   float u_scale, float v_scale) {
    customization_option_t* option = get_option(option_id);
    if (!option || option->type != CUSTOM_TYPE_TEXTURE) {
        return false;
    }
    
    option->data.texture.texture_id = texture_id;
    option->data.texture.u_offset = u_offset;
    option->data.texture.v_offset = v_offset;
    option->data.texture.u_scale = u_scale;
    option->data.texture.v_scale = v_scale;
    
    log_debug("Set texture for option %u: texture %u, offset(%.2f,%.2f), scale(%.2f,%.2f)", 
             option_id, texture_id, u_offset, v_offset, u_scale, v_scale);
    
    return true;
}

bool customization_set_size_option(uint32_t option_id, float width, float height, float depth) {
    customization_option_t* option = get_option(option_id);
    if (!option || option->type != CUSTOM_TYPE_SIZE) {
        return false;
    }
    
    option->data.size.width = width;
    option->data.size.height = height;
    option->data.size.depth = depth;
    
    option->min_value = 0.1f;
    option->max_value = 3.0f;
    
    log_debug("Set size for option %u: (%.2f, %.2f, %.2f)", 
             option_id, width, height, depth);
    
    return true;
}

uint32_t customization_create_outfit_preset(const char* name, uint32_t required_level, 
                                          bool is_premium, uint32_t cost) {
    if (!g_customization.initialized || !name || g_customization.preset_outfit_count >= MAX_PRESET_OUTFITS) {
        return 0;
    }
    
    outfit_preset_t* preset = &g_customization.preset_outfits[g_customization.preset_outfit_count];
    memset(preset, 0, sizeof(outfit_preset_t));
    
    preset->preset_id = g_customization.next_preset_id++;
    strncpy(preset->name, name, OUTFIT_NAME_LENGTH - 1);
    preset->required_level = required_level;
    preset->is_premium = is_premium;
    preset->cost = cost;
    preset->is_unlocked = !is_premium;  // Free presets are unlocked by default
    
    g_customization.preset_outfit_count++;
    log_debug("Created outfit preset: %s (ID: %u)", name, preset->preset_id);
    
    return preset->preset_id;
}

bool customization_add_option_to_preset(uint32_t preset_id, uint32_t option_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    outfit_preset_t* preset = NULL;
    for (uint32_t i = 0; i < g_customization.preset_outfit_count; i++) {
        if (g_customization.preset_outfits[i].preset_id == preset_id) {
            preset = &g_customization.preset_outfits[i];
            break;
        }
    }
    
    if (!preset || preset->option_count >= MAX_CUSTOMIZATION_PARTS) {
        return false;
    }
    
    customization_option_t* option = get_option(option_id);
    if (!option) {
        return false;
    }
    
    // Copy option to preset
    preset->options[preset->option_count] = *option;
    preset->option_count++;
    
    log_debug("Added option %u to preset %u", option_id, preset_id);
    return true;
}

bool customization_apply_option_to_character(uint32_t character_id, uint32_t option_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    character_customization_t* character = get_character_customization(character_id);
    customization_option_t* option = get_option(option_id);
    
    if (!character || !option) {
        return false;
    }
    
    if (apply_option_to_character(character, option)) {
        character->is_dirty = true;
        character->last_modified_time = get_current_time_ms();
        update_character_mesh(character_id);
        
        log_debug("Applied option %u to character %u", option_id, character_id);
        return true;
    }
    
    return false;
}

bool customization_apply_preset_to_character(uint32_t character_id, uint32_t preset_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    character_customization_t* character = get_character_customization(character_id);
    if (!character) {
        return false;
    }
    
    // Find preset
    outfit_preset_t* preset = NULL;
    for (uint32_t i = 0; i < g_customization.preset_outfit_count; i++) {
        if (g_customization.preset_outfits[i].preset_id == preset_id) {
            preset = &g_customization.preset_outfits[i];
            break;
        }
    }
    
    if (!preset || !preset->is_unlocked) {
        return false;
    }
    
    // Apply all options from preset
    bool success = true;
    for (uint32_t i = 0; i < preset->option_count; i++) {
        if (!apply_option_to_character(character, &preset->options[i])) {
            success = false;
        }
    }
    
    if (success) {
        character->is_dirty = true;
        character->last_modified_time = get_current_time_ms();
        update_character_mesh(character_id);
        
        log_info("Applied preset %u to character %u", preset_id, character_id);
    }
    
    return success;
}

bool customization_save_character_outfit(uint32_t character_id, const char* outfit_name) {
    if (!g_customization.initialized || !outfit_name) {
        return false;
    }
    
    character_customization_t* character = get_character_customization(character_id);
    if (!character || character->saved_outfit_count >= MAX_PRESET_OUTFITS) {
        return false;
    }
    
    outfit_preset_t* saved_outfit = &character->saved_outfits[character->saved_outfit_count];
    memset(saved_outfit, 0, sizeof(outfit_preset_t));
    
    saved_outfit->preset_id = character->saved_outfit_count + 1;  // Local ID
    strncpy(saved_outfit->name, outfit_name, OUTFIT_NAME_LENGTH - 1);
    saved_outfit->is_unlocked = true;
    
    // Copy current options
    saved_outfit->option_count = character->current_option_count;
    for (uint32_t i = 0; i < character->current_option_count; i++) {
        saved_outfit->options[i] = character->current_options[i];
    }
    
    character->saved_outfit_count++;
    log_info("Saved outfit '%s' for character %u", outfit_name, character_id);
    
    return true;
}

bool customization_load_character_outfit(uint32_t character_id, uint32_t outfit_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    character_customization_t* character = get_character_customization(character_id);
    if (!character) {
        return false;
    }
    
    // Find saved outfit
    outfit_preset_t* outfit = NULL;
    for (uint32_t i = 0; i < character->saved_outfit_count; i++) {
        if (character->saved_outfits[i].preset_id == outfit_id) {
            outfit = &character->saved_outfits[i];
            break;
        }
    }
    
    if (!outfit) {
        return false;
    }
    
    // Apply all options from saved outfit
    bool success = true;
    for (uint32_t i = 0; i < outfit->option_count; i++) {
        if (!apply_option_to_character(character, &outfit->options[i])) {
            success = false;
        }
    }
    
    if (success) {
        character->is_dirty = true;
        character->last_modified_time = get_current_time_ms();
        update_character_mesh(character_id);
        
        log_info("Loaded saved outfit %u for character %u", outfit_id, character_id);
    }
    
    return success;
}

bool customization_randomize_character(uint32_t character_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    character_customization_t* character = get_character_customization(character_id);
    if (!character) {
        return false;
    }
    
    // Clear current options
    character->current_option_count = 0;
    
    // Randomly apply options for each part
    for (customization_part_t part = 0; part < CUSTOM_PART_COUNT; part++) {
        // Find available options for this part
        for (uint32_t i = 0; i < g_customization.available_option_count; i++) {
            customization_option_t* option = &g_customization.available_option[i];
            if (option->part == part && option->is_enabled) {
                // Random chance to apply this option
                if (rand() % 100 < 50) {  // 50% chance
                    apply_option_to_character(character, option);
                }
            }
        }
    }
    
    character->is_dirty = true;
    character->last_modified_time = get_current_time_ms();
    update_character_mesh(character_id);
    
    log_info("Randomized customization for character %u", character_id);
    return true;
}

void customization_set_body_parameters(uint32_t character_id, float height, float body_scale, float muscle_tone) {
    character_customization_t* character = get_character_customization(character_id);
    if (!character) {
        return;
    }
    
    character->height = height;
    character->body_scale = body_scale;
    character->muscle_tone = muscle_tone;
    character->is_dirty = true;
    character->last_modified_time = get_current_time_ms();
    
    update_character_mesh(character_id);
    
    log_debug("Set body parameters for character %u: height=%.2f, scale=%.2f, muscle=%.2f",
             character_id, height, body_scale, muscle_tone);
}

void customization_get_body_parameters(uint32_t character_id, float* height, float* body_scale, float* muscle_tone) {
    character_customization_t* character = get_character_customization(character_id);
    if (!character) {
        return;
    }
    
    if (height) *height = character->height;
    if (body_scale) *body_scale = character->body_scale;
    if (muscle_tone) *muscle_tone = character->muscle_tone;
}

void customization_get_current_options(uint32_t character_id, const customization_option_t** options, uint32_t* option_count) {
    character_customization_t* character = get_character_customization(character_id);
    if (!character || !options || !option_count) {
        return;
    }
    
    *options = character->current_options;
    *option_count = character->current_option_count;
}

void customization_get_available_presets(const outfit_preset_t** presets, uint32_t* preset_count) {
    if (!presets || !preset_count) {
        return;
    }
    
    *presets = g_customization.preset_outfits;
    *preset_count = g_customization.preset_outfit_count;
}

bool customization_unlock_preset(uint32_t preset_id) {
    if (!g_customization.initialized) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_customization.preset_outfit_count; i++) {
        if (g_customization.preset_outfits[i].preset_id == preset_id) {
            g_customization.preset_outfits[i].is_unlocked = true;
            log_info("Unlocked preset %u", preset_id);
            return true;
        }
    }
    
    return false;
}

// Static helper functions
static customization_option_t* get_option(uint32_t option_id) {
    for (uint32_t i = 0; i < g_customization.available_option_count; i++) {
        if (g_customization.available_option[i].option_id == option_id) {
            return &g_customization.available_option[i];
        }
    }
    return NULL;
}

static character_customization_t* get_character_customization(uint32_t character_id) {
    // Find existing character customization
    for (uint32_t i = 0; i < g_customization.character_count; i++) {
        if (g_customization.characters[i].character_id == character_id) {
            return &g_customization.characters[i];
        }
    }
    
    // Create new character customization if space allows
    if (g_customization.character_count < MAX_PLAYERS) {
        character_customization_t* character = &g_customization.characters[g_customization.character_count];
        memset(character, 0, sizeof(character_customization_t));
        character->character_id = character_id;
        character->body_scale = 1.0f;
        character->height = 1.0f;
        character->muscle_tone = 0.5f;
        g_customization.character_count++;
        return character;
    }
    
    return NULL;
}

static bool apply_option_to_character(character_customization_t* character, const customization_option_t* option) {
    if (character->current_option_count >= MAX_CUSTOMIZATION_PARTS) {
        return false;
    }
    
    // Check if character already has an option for this part
    for (uint32_t i = 0; i < character->current_option_count; i++) {
        if (character->current_options[i].part == option->part) {
            // Replace existing option
            character->current_options[i] = *option;
            return true;
        }
    }
    
    // Add new option
    character->current_options[character->current_option_count] = *option;
    character->current_option_count++;
    
    return true;
}

static void update_character_mesh(uint32_t character_id) {
    // This would interface with the rendering system to update the character's mesh
    // based on the current customization options
    log_debug("Updating mesh for character %u", character_id);
}

