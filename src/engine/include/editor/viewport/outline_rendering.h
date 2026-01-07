// include/editor/viewport/outline_rendering.h
//
// Purpose: Selection outline rendering using jump-flood algorithm
// Provides high-quality object selection outlines with configurable thickness
//
#ifndef EDITOR_VIEWPORT_OUTLINE_RENDERING_H
#define EDITOR_VIEWPORT_OUTLINE_RENDERING_H

#include "../../common.h"
#include "../../math/math.h"

// Forward declarations
typedef struct Camera Camera;
typedef struct IRenderer IRenderer;

// Outline rendering modes
typedef enum {
    OUTLINE_MODE_JUMP_FLOOD,    // GPU-based jump-flood algorithm
    OUTLINE_MODE_SOBEL,         // CPU-based Sobel edge detection
    OUTLINE_MODE_DISTANCE_FIELD,// Distance field-based outlines
    OUTLINE_MODE_COUNT
} OutlineMode;

// Outline styles
typedef enum {
    OUTLINE_STYLE_SOLID,        // Solid color outline
    OUTLINE_STYLE_GLOW,         // Glowing outline with falloff
    OUTLINE_STYLE_PULSE,        // Pulsing outline animation
    OUTLINE_STYLE_RAINBOW,      // Rainbow-colored outline
    OUTLINE_STYLE_COUNT
} OutlineStyle;

// Outline configuration
typedef struct {
    OutlineMode mode;
    OutlineStyle style;
    
    // Visual properties
    Vec3 color;                 // Outline color
    f32 thickness;              // Outline thickness in pixels
    f32 intensity;              // Outline intensity/brightness
    f32 glow_radius;            // Glow radius for glow style
    f32 pulse_speed;            // Pulse animation speed
    f32 pulse_amplitude;        // Pulse animation amplitude
    
    // Jump-flood specific
    u32 jfa_iterations;         // Number of jump-flood iterations
    f32 jfa_threshold;          // Distance threshold for edge detection
    
    // Rendering
    bool render_behind_objects; // Render outline behind objects
    bool render_in_front;       // Render outline in front of objects
    bool enable_anti_aliasing;  // Enable anti-aliasing for smooth edges
    f32 mipmap_bias;           // Mipmap bias for texture sampling
    
    // Performance
    bool use_half_resolution;   // Use half-resolution for performance
    u32 max_outline_objects;   // Maximum number of objects to outline
} OutlineConfig;

// Outline object data
typedef struct {
    u32 object_id;              // Object ID for selection
    Vec3 bounds_min;            // Object bounding box minimum
    Vec3 bounds_max;            // Object bounding box maximum
    Mat4 transform;             // Object transform matrix
    bool enabled;               // Whether outline is enabled for this object
} OutlineObject;

// Outline rendering system
typedef struct {
    OutlineConfig config;
    
    // GPU resources for jump-flood
    u32 id_framebuffer;         // Framebuffer for object ID rendering
    u32 id_color_texture;       // Color texture for object IDs
    u32 id_depth_texture;       // Depth texture for depth testing
    
    // Jump-flood textures
    u32 jfa_texture_0;         // Jump-flood texture ping-pong
    u32 jfa_texture_1;         // Jump-flood texture ping-pong
    u32 jfa_framebuffer;        // Jump-flood framebuffer
    
    // Outline rendering
    u32 outline_framebuffer;    // Final outline framebuffer
    u32 outline_color_texture;  // Final outline color texture
    
    // Objects to outline
    OutlineObject *objects;     // Array of objects to outline
    u32 object_count;           // Number of objects
    u32 max_objects;            // Maximum number of objects
    
    // Animation state
    f32 pulse_time;             // Current pulse animation time
    
    // Performance
    bool textures_dirty;        // Whether textures need regeneration
    u32 texture_width;          // Texture width
    u32 texture_height;         // Texture height
} OutlineRendering;

// Public API
OutlineRendering* outline_rendering_create(u32 max_objects, u32 texture_width, u32 texture_height);
void outline_rendering_destroy(OutlineRendering *outline);

// Configuration
void outline_set_mode(OutlineRendering *outline, OutlineMode mode);
void outline_set_style(OutlineRendering *outline, OutlineStyle style);
void outline_set_color(OutlineRendering *outline, Vec3 color);
void outline_set_thickness(OutlineRendering *outline, f32 thickness);
void outline_set_intensity(OutlineRendering *outline, f32 intensity);
void outline_set_glow_radius(OutlineRendering *outline, f32 radius);
void outline_set_pulse_animation(OutlineRendering *outline, f32 speed, f32 amplitude);
void outline_set_jfa_settings(OutlineRendering *outline, u32 iterations, f32 threshold);

// Object management
u32 outline_add_object(OutlineRendering *outline, u32 object_id, Vec3 bounds_min, Vec3 bounds_max, Mat4 transform);
void outline_remove_object(OutlineRendering *outline, u32 object_id);
void outline_update_object(OutlineRendering *outline, u32 object_id, Vec3 bounds_min, Vec3 bounds_max, Mat4 transform);
void outline_clear_objects(OutlineRendering *outline);
void outline_set_object_enabled(OutlineRendering *outline, u32 object_id, bool enabled);

// Rendering
void outline_render(OutlineRendering *outline, IRenderer *renderer, const Camera *camera, f32 dt);
void outline_render_id_buffer(OutlineRendering *outline, IRenderer *renderer, const Camera *camera);
void outline_render_jump_flood(OutlineRendering *outline);
void outline_render_final(OutlineRendering *outline, IRenderer *renderer, const Camera *camera);

// Animation
void outline_update_animation(OutlineRendering *outline, f32 dt);
Vec3 outline_get_animated_color(const OutlineRendering *outline, const OutlineConfig *config, f32 time);

// Utility
bool outline_is_object_outlined(const OutlineRendering *outline, u32 object_id);
u32 outline_get_object_count(const OutlineRendering *outline);
void outline_mark_textures_dirty(OutlineRendering *outline);

// Performance
void outline_set_resolution(OutlineRendering *outline, u32 width, u32 height);
void outline_enable_half_resolution(OutlineRendering *outline, bool enable);

#endif // EDITOR_VIEWPORT_OUTLINE_RENDERING_H
