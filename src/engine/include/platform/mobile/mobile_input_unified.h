// include/platform/mobile/mobile_input_unified.h
//
// Purpose: Unified mobile input system consolidating touch, accelerometer, and gesture input
// This replaces multiple disparate mobile input APIs with a single interface

#ifndef MOBILE_INPUT_UNIFIED_H
#define MOBILE_INPUT_UNIFIED_H

#include "engine/include/common.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED MOBILE INPUT CONFIGURATION
// ============================================================================

typedef struct {
    // Touch configuration
    bool enable_touch_input;
    bool enable_multi_touch;
    uint32_t max_touch_points;
    float tap_timeout_seconds;
    float double_tap_timeout_seconds;
    float long_press_timeout_seconds;
    float drag_threshold;
    float pinch_threshold;
    float rotation_threshold;
    
    // Accelerometer configuration
    bool enable_accelerometer;
    float accelerometer_update_rate;
    Vec3 accelerometer_gravity;
    float accelerometer_low_pass_filter;
    
    // Gyroscope configuration
    bool enable_gyroscope;
    float gyroscope_update_rate;
    Vec3 gyroscope_bias;
    
    // Haptic feedback configuration
    bool enable_haptics;
    float haptic_intensity;
    uint32_t haptic_duration_ms;
    
    // Virtual controls configuration
    bool enable_virtual_joystick;
    bool enable_virtual_buttons;
    float virtual_joystick_size;
    float virtual_button_size;
    Vec2 virtual_joystick_position;
    Vec2 virtual_button_position;
    
    // Gesture configuration
    bool enable_gestures;
    bool enable_swipe_gestures;
    bool enable_pinch_gestures;
    bool enable_rotation_gestures;
    float swipe_min_distance;
    float swipe_min_velocity;
} MobileInputConfig;

// ============================================================================
// TOUCH INPUT TYPES
// ============================================================================

typedef enum {
    TOUCH_STATE_PRESSED = 0,
    TOUCH_STATE_MOVED,
    TOUCH_STATE_RELEASED,
    TOUCH_STATE_CANCELLED,
    TOUCH_STATE_STATIONARY
} TouchState;

typedef enum {
    GESTURE_NONE = 0,
    GESTURE_TAP,
    GESTURE_DOUBLE_TAP,
    GESTURE_LONG_PRESS,
    GESTURE_SWIPE_LEFT,
    GESTURE_SWIPE_RIGHT,
    GESTURE_SWIPE_UP,
    GESTURE_SWIPE_DOWN,
    GESTURE_PINCH_IN,
    GESTURE_PINCH_OUT,
    GESTURE_ROTATE_CLOCKWISE,
    GESTURE_ROTATE_COUNTER_CLOCKWISE
} GestureType;

typedef struct {
    uint32_t touch_id;
    TouchState state;
    Vec2 position;
    Vec2 previous_position;
    Vec2 start_position;
    float pressure;
    float radius;
    double timestamp;
    double start_timestamp;
    bool is_primary;
} TouchPoint;

typedef struct {
    GestureType type;
    Vec2 start_position;
    Vec2 end_position;
    Vec2 center_position;
    float distance;
    float angle;
    float velocity;
    double timestamp;
    uint32_t touch_count;
} GestureEvent;

// ============================================================================
// SENSOR INPUT TYPES
// ============================================================================

typedef struct {
    Vec3 acceleration;
    Vec3 gravity;
    Vec3 user_acceleration;
    double timestamp;
    bool is_available;
} AccelerometerData;

typedef struct {
    Vec3 rotation_rate;
    Vec3 orientation;
    Vec3 calibrated_rotation;
    double timestamp;
    bool is_available;
} GyroscopeData;

typedef struct {
    Vec3 magnetic_field;
    float heading;
    double timestamp;
    bool is_available;
} MagnetometerData;

// ============================================================================
// VIRTUAL CONTROLS
// ============================================================================

typedef enum {
    VIRTUAL_BUTTON_NONE = 0,
    VIRTUAL_BUTTON_JUMP,
    VIRTUAL_BUTTON_ATTACK,
    VIRTUAL_BUTTON_INTERACT,
    VIRTUAL_BUTTON_SPRINT,
    VIRTUAL_BUTTON_SNEAK,
    VIRTUAL_BUTTON_INVENTORY,
    VIRTUAL_BUTTON_PAUSE,
    VIRTUAL_BUTTON_COUNT
} VirtualButtonType;

typedef struct {
    Vec2 center;
    float radius;
    Vec2 current_position;
    Vec2 deadzone;
    bool is_active;
    bool is_visible;
    float alpha;
} VirtualJoystick;

typedef struct {
    Vec2 position;
    Vec2 size;
    bool is_pressed;
    bool is_visible;
    float alpha;
    VirtualButtonType button_type;
    const char* label;
} VirtualButton;

// ============================================================================
// HAPTIC FEEDBACK
// ============================================================================

typedef enum {
    HAPTIC_TYPE_NONE = 0,
    HAPTIC_TYPE_LIGHT_TAP,
    HAPTIC_TYPE_MEDIUM_TAP,
    HAPTIC_TYPE_HEAVY_TAP,
    HAPTIC_TYPE_SUCCESS,
    HAPTIC_TYPE_WARNING,
    HAPTIC_TYPE_ERROR,
    HAPTIC_TYPE_SELECTION_CHANGED,
    HAPTIC_TYPE_IMPACT,
    HAPTIC_TYPE_RUMBLE
} HapticType;

typedef struct {
    HapticType type;
    float intensity;
    uint32_t duration_ms;
    float sharpness;
    bool is_continuous;
} HapticFeedback;

// ============================================================================
// UNIFIED MOBILE INPUT SYSTEM
// ============================================================================

typedef struct MobileInputSystem {
    // Configuration and state
    MobileInputConfig config;
    bool is_initialized;
    
    // Touch input
    TouchPoint* touch_points;
    uint32_t max_touch_points;
    uint32_t active_touch_count;
    GestureEvent current_gesture;
    bool gesture_in_progress;
    
    // Sensor input
    AccelerometerData accelerometer;
    GyroscopeData gyroscope;
    MagnetometerData magnetometer;
    
    // Virtual controls
    VirtualJoystick virtual_joystick;
    VirtualButton virtual_buttons[VIRTUAL_BUTTON_COUNT];
    Vec2 virtual_movement;
    Vec2 virtual_look;
    
    // Haptic feedback
    HapticFeedback current_haptic;
    bool haptic_active;
    
    // Input mapping
    uint32_t touch_to_action_map[32];
    uint32_t gesture_to_action_map[16];
    
    // Platform-specific data
    void* platform_data;
    
    // Callbacks
    void (*touch_callback)(const TouchPoint* touches, uint32_t count);
    void (*gesture_callback)(const GestureEvent* gesture);
    void (*sensor_callback)(const AccelerometerData* accel, const GyroscopeData* gyro);
    void (*virtual_control_callback)(VirtualButtonType button_type, bool pressed);
} MobileInputSystem;

// ============================================================================
// MAIN MOBILE INPUT API
// ============================================================================

// System management
MobileInputSystem* mobile_input_create(const MobileInputConfig* config);
void mobile_input_destroy(MobileInputSystem* input);
bool mobile_input_is_initialized(MobileInputSystem* input);
void mobile_input_update(MobileInputSystem* input, float delta_time);

// ============================================================================
// TOUCH INPUT API
// ============================================================================

void mobile_input_touch_init(MobileInputSystem* input);
void mobile_input_touch_shutdown(MobileInputSystem* input);
uint32_t mobile_input_get_touch_count(MobileInputSystem* input);
const TouchPoint* mobile_input_get_touch_points(MobileInputSystem* input);
bool mobile_input_is_touch_active(MobileInputSystem* input, uint32_t touch_id);
TouchPoint* mobile_input_get_touch_point(MobileInputSystem* input, uint32_t touch_id);
Vec2 mobile_input_get_touch_position(MobileInputSystem* input, uint32_t touch_id);
TouchState mobile_input_get_touch_state(MobileInputSystem* input, uint32_t touch_id);

// ============================================================================
// GESTURE RECOGNITION API
// ============================================================================

void mobile_input_gestures_init(MobileInputSystem* input);
void mobile_input_gestures_shutdown(MobileInputSystem* input);
bool mobile_input_is_gesture_enabled(MobileInputSystem* input);
GestureType mobile_input_get_current_gesture(MobileInputSystem* input);
const GestureEvent* mobile_input_get_last_gesture(MobileInputSystem* input);
void mobile_input_enable_gesture_type(MobileInputSystem* input, GestureType gesture_type, bool enable);

// ============================================================================
// SENSOR INPUT API
// ============================================================================

void mobile_input_sensors_init(MobileInputSystem* input);
void mobile_input_sensors_shutdown(MobileInputSystem* input);
bool mobile_input_is_accelerometer_available(MobileInputSystem* input);
bool mobile_input_is_gyroscope_available(MobileInputSystem* input);
bool mobile_input_is_magnetometer_available(MobileInputSystem* input);
const AccelerometerData* mobile_input_get_accelerometer_data(MobileInputSystem* input);
const GyroscopeData* mobile_input_get_gyroscope_data(MobileInputSystem* input);
const MagnetometerData* mobile_input_get_magnetometer_data(MobileInputSystem* input);

// ============================================================================
// VIRTUAL CONTROLS API
// ============================================================================

void mobile_input_virtual_controls_init(MobileInputSystem* input);
void mobile_input_virtual_controls_shutdown(MobileInputSystem* input);
void mobile_input_set_virtual_joystick_position(MobileInputSystem* input, Vec2 position);
void mobile_input_set_virtual_joystick_size(MobileInputSystem* input, float size);
void mobile_input_set_virtual_button_position(MobileInputSystem* input, VirtualButtonType button_type, Vec2 position);
void mobile_input_set_virtual_button_size(MobileInputSystem* input, float size);
bool mobile_input_is_virtual_button_pressed(MobileInputSystem* input, VirtualButtonType button_type);
Vec2 mobile_input_get_virtual_movement(MobileInputSystem* input);
Vec2 mobile_input_get_virtual_look(MobileInputSystem* input);

// ============================================================================
// HAPTIC FEEDBACK API
// ============================================================================

void mobile_input_haptics_init(MobileInputSystem* input);
void mobile_input_haptics_shutdown(MobileInputSystem* input);
bool mobile_input_is_haptics_available(MobileInputSystem* input);
void mobile_input_play_haptic(MobileInputSystem* input, const HapticFeedback* haptic);
void mobile_input_play_haptic_type(MobileInputSystem* input, HapticType type);
void mobile_input_stop_haptic(MobileInputSystem* input);
void mobile_input_set_haptic_intensity(MobileInputSystem* input, float intensity);

// ============================================================================
// INPUT MAPPING API
// ============================================================================

void mobile_input_set_touch_mapping(MobileInputSystem* input, uint32_t touch_id, uint32_t action);
void mobile_input_set_gesture_mapping(MobileInputSystem* input, GestureType gesture, uint32_t action);
uint32_t mobile_input_get_touch_action(MobileInputSystem* input, uint32_t touch_id);
uint32_t mobile_input_get_gesture_action(MobileInputSystem* input, GestureType gesture);
void mobile_input_clear_mappings(MobileInputSystem* input);

// ============================================================================
// CALLBACK REGISTRATION API
// ============================================================================

void mobile_input_set_touch_callback(MobileInputSystem* input, 
                                void (*callback)(const TouchPoint* touches, uint32_t count));
void mobile_input_set_gesture_callback(MobileInputSystem* input, 
                                  void (*callback)(const GestureEvent* gesture));
void mobile_input_set_sensor_callback(MobileInputSystem* input, 
                                 void (*callback)(const AccelerometerData* accel, const GyroscopeData* gyro));
void mobile_input_set_virtual_control_callback(MobileInputSystem* input, 
                                        void (*callback)(VirtualButtonType button_type, bool pressed));

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration
MobileInputConfig mobile_input_create_default_config(void);
bool mobile_input_validate_config(const MobileInputConfig* config);

// Touch utilities
bool mobile_input_is_tap_gesture(const TouchPoint* touch);
bool mobile_input_is_double_tap_gesture(const TouchPoint* touch1, const TouchPoint* touch2);
bool mobile_input_is_long_press_gesture(const TouchPoint* touch);
bool mobile_input_is_swipe_gesture(const TouchPoint* start_touch, const TouchPoint* end_touch);
float mobile_input_calculate_pinch_distance(const TouchPoint* touch1, const TouchPoint* touch2);
float mobile_input_calculate_rotation_angle(const TouchPoint* touch1, const TouchPoint* touch2, const TouchPoint* center);

// Sensor utilities
Vec3 mobile_input_filter_accelerometer(const Vec3* raw_accel, float filter_alpha);
Vec3 mobile_input_calculate_tilt_angles(const Vec3* acceleration);
float mobile_input_calculate_device_orientation(const Vec3* acceleration, const Vec3* magnetic_field);

// Platform detection
typedef enum {
    PLATFORM_MOBILE_IOS = 0,
    PLATFORM_MOBILE_ANDROID,
    PLATFORM_MOBILE_UNKNOWN
} MobilePlatform;

MobilePlatform mobile_input_get_platform(void);
bool mobile_input_is_tablet_device(void);
float mobile_input_get_screen_density(void);
Vec2 mobile_input_get_screen_size(void);

#ifdef __cplusplus
}
#endif

#endif // MOBILE_INPUT_UNIFIED_H
