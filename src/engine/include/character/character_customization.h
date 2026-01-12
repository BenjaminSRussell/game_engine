#ifndef CHARACTER_CUSTOMIZATION_H
#define CHARACTER_CUSTOMIZATION_H

#include <stdint.h>
#include <stdbool.h>

// Customization parts for character body
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

// Types of customization options
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

// Forward declarations for opaque types
typedef struct customization_option customization_option_t;
typedef struct outfit_preset outfit_preset_t;
typedef struct character_customization character_customization_t;

// Initialize the character customization system
bool customization_init(void);

// Shutdown the character customization system
void customization_shutdown(void);

// Add a new customization option
// part: Body part to customize
// type: Type of customization
// name: Option name
// Returns: Option ID or 0 on failure
uint32_t customization_add_option(customization_part_t part, customization_type_t type, const char* name);

// Set color values for a color option
// option_id: Option to modify
// r, g, b, a: RGBA color values (0.0 to 1.0)
bool customization_set_color_option(uint32_t option_id, float r, float g, float b, float a);

// Set model values for a model option
// option_id: Option to modify
// model_id: Model asset ID
// variant_id: Model variant ID
bool customization_set_model_option(uint32_t option_id, uint32_t model_id, uint32_t variant_id);

// Set texture values for a texture option
// option_id: Option to modify
// texture_id: Texture asset ID
// u_offset, v_offset: UV offset values
// u_scale, v_scale: UV scale values
bool customization_set_texture_option(uint32_t option_id, uint32_t texture_id, 
                                   float u_offset, float v_offset, 
                                   float u_scale, float v_scale);

// Set size values for a size option
// option_id: Option to modify
// width, height, depth: Size dimensions
bool customization_set_size_option(uint32_t option_id, float width, float height, float depth);

// Create a new outfit preset
// name: Preset name
// required_level: Minimum level required
// is_premium: Whether preset requires premium access
// cost: Cost to unlock
// Returns: Preset ID or 0 on failure
uint32_t customization_create_outfit_preset(const char* name, uint32_t required_level, 
                                          bool is_premium, uint32_t cost);

// Add an option to an outfit preset
// preset_id: Preset to modify
// option_id: Option to add
bool customization_add_option_to_preset(uint32_t preset_id, uint32_t option_id);

// Apply a single customization option to a character
// character_id: Character to modify
// option_id: Option to apply
bool customization_apply_option_to_character(uint32_t character_id, uint32_t option_id);

// Apply an entire outfit preset to a character
// character_id: Character to modify
// preset_id: Preset to apply
bool customization_apply_preset_to_character(uint32_t character_id, uint32_t preset_id);

// Save current character customization as an outfit
// character_id: Character to save
// outfit_name: Name for the saved outfit
bool customization_save_character_outfit(uint32_t character_id, const char* outfit_name);

// Load a saved character outfit
// character_id: Character to modify
// outfit_id: Saved outfit to load
bool customization_load_character_outfit(uint32_t character_id, uint32_t outfit_id);

// Randomize character customization
// character_id: Character to randomize
bool customization_randomize_character(uint32_t character_id);

// Set body parameters for a character
// character_id: Character to modify
// height: Character height multiplier
// body_scale: Overall body scale
// muscle_tone: Muscle tone (0.0 to 1.0)
void customization_set_body_parameters(uint32_t character_id, float height, float body_scale, float muscle_tone);

// Get body parameters for a character
// character_id: Character to query
// height: Output height multiplier
// body_scale: Output body scale
// muscle_tone: Output muscle tone
void customization_get_body_parameters(uint32_t character_id, float* height, float* body_scale, float* muscle_tone);

// Get current customization options for a character
// character_id: Character to query
// options: Output array of options
// option_count: Output number of options
void customization_get_current_options(uint32_t character_id, const customization_option_t** options, uint32_t* option_count);

// Get all available outfit presets
// presets: Output array of presets
// preset_count: Output number of presets
void customization_get_available_presets(const outfit_preset_t** presets, uint32_t* preset_count);

// Unlock a premium preset
// preset_id: Preset to unlock
bool customization_unlock_preset(uint32_t preset_id);

// Utility function to get current time (placeholder)
uint32_t get_current_time_ms(void);

#endif // CHARACTER_CUSTOMIZATION_H
