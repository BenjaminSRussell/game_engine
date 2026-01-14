/*
 * material_properties.h
 * Material properties and parameters system
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_MATERIAL_PROPERTIES_H
#define MATERIALS_MATERIAL_PROPERTIES_H

#include "materials/material_instance.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES AND CONSTANTS
// ============================================================================

typedef struct material_properties_handle {
    uint32_t id;
} material_properties_handle_t;

typedef enum material_property_category_e {
    MATERIAL_PROPERTY_CATEGORY_BASE = 0,
    MATERIAL_PROPERTY_CATEGORY_PBR = 1,
    MATERIAL_PROPERTY_CATEGORY_SURFACE = 2,
    MATERIAL_PROPERTY_CATEGORY_GEOMETRIC = 3,
    MATERIAL_PROPERTY_CATEGORY_ANIMATION = 4,
    MATERIAL_PROPERTY_CATEGORY_SPECIAL = 5,
    MATERIAL_PROPERTY_CATEGORY_CUSTOM = 6,
    MATERIAL_PROPERTY_CATEGORY_COUNT = 7
} material_property_category_e;

typedef enum material_property_type_e {
    MATERIAL_PROPERTY_TYPE_SCALAR = 0,
    MATERIAL_PROPERTY_TYPE_VECTOR2 = 1,
    MATERIAL_PROPERTY_TYPE_VECTOR3 = 2,
    MATERIAL_PROPERTY_TYPE_VECTOR4 = 3,
    MATERIAL_PROPERTY_TYPE_COLOR = 4,
    MATERIAL_PROPERTY_TYPE_TEXTURE = 5,
    MATERIAL_PROPERTY_TYPE_BOOLEAN = 6,
    MATERIAL_PROPERTY_TYPE_INTEGER = 7,
    MATERIAL_PROPERTY_TYPE_ENUM = 8,
    MATERIAL_PROPERTY_TYPE_RANGE = 9,
    MATERIAL_PROPERTY_TYPE_CURVE = 10,
    MATERIAL_PROPERTY_TYPE_GRADIENT = 11,
    MATERIAL_PROPERTY_TYPE_COUNT = 12
} material_property_type_e;

typedef enum material_property_flags_e {
    MATERIAL_PROPERTY_FLAG_NONE = 0,
    MATERIAL_PROPERTY_FLAG_EDITABLE = 1 << 0,
    MATERIAL_PROPERTY_FLAG_ANIMATABLE = 1 << 1,
    MATERIAL_PROPERTY_FLAG_INSTANCED = 1 << 2,
    MATERIAL_PROPERTY_FLAG_REQUIRED = 1 << 3,
    MATERIAL_PROPERTY_FLAG_ADVANCED = 1 << 4,
    MATERIAL_PROPERTY_FLAG_HIDDEN = 1 << 5,
    MATERIAL_PROPERTY_FLAG_READ_ONLY = 1 << 6
} material_property_flags_e;

typedef struct material_property_range_t {
    float min_value;
    float max_value;
    float default_value;
    bool is logarithmic;
} material_property_range_t;

typedef struct material_property_enum_t {
    char options[16][64];
    uint32_t option_count;
    uint32_t default_index;
} material_property_enum_t;

typedef struct material_property_curve_point_t {
    float time;
    float value;
    float in_tangent;
    float out_tangent;
} material_property_curve_point_t;

typedef struct material_property_curve_t {
    material_property_curve_point_t points[16];
    uint32_t point_count;
    bool is_looping;
    uint32_t resolution;
} material_property_curve_t;

typedef struct material_property_gradient_key_t {
    float position;
    float value[4]; // RGBA
} material_property_gradient_key_t;

typedef struct material_property_gradient_t {
    material_property_gradient_key_t keys[8];
    uint32_t key_count;
    bool is_alpha_gradient;
} material_property_gradient_t;

typedef struct material_property_metadata_t {
    char name[64];
    char display_name[64];
    char description[256];
    char group[64];
    material_property_category_e category;
    material_property_type_e type;
    material_property_flags_e flags;
    uint32_t version;
    
    union {
        material_property_range_t range;
        material_property_enum_t enum_data;
        material_property_curve_t curve;
        material_property_gradient_t gradient;
    } type_data;
    
    char unit[32];
    float precision;
    bool is_normalized;
} material_property_metadata_t;

typedef struct material_property_value_t {
    material_property_type_e type;
    union {
        float scalar;
        float vector2[2];
        float vector3[3];
        float vector4[4];
        uint32_t texture_id;
        bool boolean;
        int32_t integer;
        uint32_t enum_index;
        material_property_curve_t curve;
        material_property_gradient_t gradient;
    } data;
} material_property_value_t;

typedef struct material_property_instance_t {
    material_property_metadata_t metadata;
    material_property_value_t value;
    bool is_overridden;
    bool is_dirty;
    uint64_t last_modified;
} material_property_instance_t;

typedef struct material_property_set_t {
    material_property_instance_t* properties;
    uint32_t property_count;
    uint32_t property_capacity;
    char name[128];
    uint32_t version;
    bool is_dirty;
} material_property_set_t;

typedef struct material_properties_desc_t {
    uint32_t flags;
    void* user_data;
} material_properties_desc_t;

typedef struct material_properties_info_t {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t total_properties;
    uint32_t registered_sets;
} material_properties_info_t;

// ============================================================================
// PROPERTY REGISTRATION
// ============================================================================

// Initialize material properties system
bool material_properties_init(void);

// Shutdown material properties system
void material_properties_shutdown(void);

// Create property system instance
material_properties_handle_t material_properties_create(const material_properties_desc_t* desc);

// Destroy property system instance
void material_properties_destroy(material_properties_handle_t handle);

// Get property system information
bool material_properties_get_info(material_properties_handle_t handle, material_properties_info_t* info);

// Register property metadata
bool material_properties_register_property(material_properties_handle_t handle,
                                           const material_property_metadata_t* metadata);

// Unregister property
bool material_properties_unregister_property(material_properties_handle_t handle, const char* name);

// Get property metadata
bool material_properties_get_property_metadata(material_properties_handle_t handle,
                                              const char* name,
                                              material_property_metadata_t* metadata);

// List all registered properties
bool material_properties_list_properties(material_properties_handle_t handle,
                                         material_property_metadata_t* properties,
                                         uint32_t* property_count);

// ============================================================================
// PROPERTY SETS
// ============================================================================

// Create property set
material_property_set_t* material_properties_create_set(material_properties_handle_t handle,
                                                          const char* name);

// Destroy property set
void material_properties_destroy_set(material_property_set_t* set);

// Add property to set
bool material_properties_set_add_property(material_property_set_t* set,
                                          const material_property_metadata_t* metadata,
                                          const material_property_value_t* default_value);

// Remove property from set
bool material_properties_set_remove_property(material_property_set_t* set, const char* name);

// Get property from set
material_property_instance_t* material_properties_set_get_property(material_property_set_t* set,
                                                                   const char* name);

// Set property value
bool material_properties_set_set_value(material_property_set_t* set,
                                       const char* name,
                                       const material_property_value_t* value);

// Get property value
bool material_properties_set_get_value(material_property_set_t* set,
                                       const char* name,
                                       material_property_value_t* value);

// Reset property to default
bool material_properties_set_reset_property(material_property_set_t* set, const char* name);

// Reset all properties to defaults
void material_properties_set_reset_all(material_property_set_t* set);

// Check if set is dirty
bool material_properties_set_is_dirty(material_property_set_t* set);

// Mark set as clean
void material_properties_set_mark_clean(material_property_set_t* set);

// ============================================================================
// PROPERTY VALUE OPERATIONS
// ============================================================================

// Create property value
material_property_value_t material_property_value_create_scalar(float value);
material_property_value_t material_property_value_create_vector2(float x, float y);
material_property_value_t material_property_value_create_vector3(float x, float y, float z);
material_property_value_t material_property_value_create_vector4(float x, float y, float z, float w);
material_property_value_t material_property_value_create_color(float r, float g, float b, float a);
material_property_value_t material_property_value_create_texture(uint32_t texture_id);
material_property_value_t material_property_value_create_boolean(bool value);
material_property_value_t material_property_value_create_integer(int32_t value);
material_property_value_t material_property_value_create_enum(uint32_t index);
material_property_value_t material_property_value_create_curve(const material_property_curve_t* curve);
material_property_value_t material_property_value_create_gradient(const material_property_gradient_t* gradient);

// Copy property value
bool material_property_value_copy(material_property_value_t* dest, const material_property_value_t* src);

// Compare property values
bool material_property_value_equals(const material_property_value_t* a, const material_property_value_t* b);

// Interpolate property values
bool material_property_value_interpolate(material_property_value_t* result,
                                          const material_property_value_t* a,
                                          const material_property_value_t* b,
                                          float t,
                                          material_property_type_e type);

// Validate property value against metadata
bool material_property_value_validate(const material_property_value_t* value,
                                       const material_property_metadata_t* metadata);

// ============================================================================
// SERIALIZATION
// ============================================================================

// Serialize property set to JSON
bool material_properties_serialize_to_json(const material_property_set_t* set,
                                            char* json_output,
                                            size_t output_size);

// Deserialize property set from JSON
bool material_properties_deserialize_from_json(const char* json_input,
                                                material_property_set_t* set,
                                                material_properties_handle_t handle);

// Serialize property set to binary
bool material_properties_serialize_to_binary(const material_property_set_t* set,
                                              uint8_t* binary_output,
                                              size_t* output_size);

// Deserialize property set from binary
bool material_properties_deserialize_from_binary(const uint8_t* binary_input,
                                                  size_t input_size,
                                                  material_property_set_t* set,
                                                  material_properties_handle_t handle);

// ============================================================================
// ANIMATION AND TIME
// ============================================================================

// Evaluate animated property value
bool material_property_evaluate_at_time(material_property_instance_t* property,
                                         float time,
                                         material_property_value_t* result);

// Sample curve at time
float material_property_curve_sample(const material_property_curve_t* curve, float time);

// Sample gradient at position
bool material_property_gradient_sample(const material_property_gradient_t* gradient,
                                        float position,
                                        float* result_rgba);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get property type name
const char* material_property_get_type_name(material_property_type_e type);

// Get property category name
const char* material_property_get_category_name(material_property_category_e category);

// Get property flag name
const char* material_property_get_flag_name(material_property_flags_e flag);

// Check if property type is animatable
bool material_property_type_is_animatable(material_property_type_e type);

// Check if property type supports interpolation
bool material_property_type_supports_interpolation(material_property_type_e type);

// Get default value for property type
material_property_value_t material_property_get_default_value(material_property_type_e type);

// Validate property metadata
bool material_property_validate_metadata(const material_property_metadata_t* metadata);

// ============================================================================
// PREDEFINED PROPERTIES
// ============================================================================

// PBR properties
extern const material_property_metadata_t MATERIAL_PROPERTY_ALBEDO;
extern const material_property_metadata_t MATERIAL_PROPERTY_NORMAL;
extern const material_property_metadata_t MATERIAL_PROPERTY_METALLIC;
extern const material_property_metadata_t MATERIAL_PROPERTY_ROUGHNESS;
extern const material_property_metadata_t MATERIAL_PROPERTY_AO;
extern const material_property_metadata_t MATERIAL_PROPERTY_EMISSIVE;

// Surface properties
extern const material_property_metadata_t MATERIAL_PROPERTY_SUBSURFACE_COLOR;
extern const material_property_metadata_t MATERIAL_PROPERTY_SUBSURFACE_RADIUS;
extern const material_property_metadata_t MATERIAL_PROPERTY_COAT_COLOR;
extern const material_property_metadata_t MATERIAL_PROPERTY_COAT_ROUGHNESS;

// Geometric properties
extern const material_property_metadata_t MATERIAL_PROPERTY_DISPLACEMENT;
extern const material_property_metadata_t MATERIAL_PROPERTY_DISPLACEMENT_SCALE;
extern const material_property_metadata_t MATERIAL_PROPERTY_TESSELLATION_FACTOR;

// Animation properties
extern const material_property_metadata_t MATERIAL_PROPERTY_UV_SCROLL_SPEED;
extern const material_property_metadata_t MATERIAL_PROPERTY_UV_ROTATION_SPEED;
extern const material_property_metadata_t MATERIAL_PROPERTY_OPACITY;

// Register all predefined properties
bool material_properties_register_predefined(material_properties_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // MATERIALS_MATERIAL_PROPERTIES_H
