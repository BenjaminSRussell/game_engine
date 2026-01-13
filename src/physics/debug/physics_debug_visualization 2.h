/**
 * @file physics_debug_visualization.h
 * @brief Physics debug visualization system
 *
 * Provides comprehensive debug visualization for physics objects,
 * collision detection, constraints, and performance metrics with
 * support for multiple rendering backends and customizable visual styles.
 */

#ifndef PHYSICS_DEBUG_VISUALIZATION_H
#define PHYSICS_DEBUG_VISUALIZATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsWorld PhysicsWorld;
typedef struct PhysicsObject PhysicsObject;
typedef struct PhysicsConstraint PhysicsConstraint;

// ========================================
// Debug Visualization Types
// ========================================

typedef enum DebugVisualizationType {
    DEBUG_VIS_NONE,                  // No visualization
    DEBUG_VIS_COLLISION_SHAPES,      // Show collision shapes
    DEBUG_VIS_BOUNDING_BOXES,        // Show bounding boxes
    DEBUG_VIS_BOUNDING_SPHERES,      // Show bounding spheres
    DEBUG_VIS_VELOCITY_VECTORS,      // Show velocity vectors
    DEBUG_VIS_FORCE_VECTORS,          // Show force vectors
    DEBUG_VIS_ANGULAR_VELOCITY,       // Show angular velocity
    DEBUG_VIS_CENTER_OF_MASS,         // Show center of mass
    DEBUG_VIS_CONTACT_POINTS,         // Show contact points
    DEBUG_VIS_COLLISION_NORMALS,      // Show collision normals
    DEBUG_VIS_CONSTRAINTS,            // Show constraints
    DEBUG_VIS_SLEEPING_OBJECTS,       // Show sleeping objects
    DEBUG_VIS_ACTIVE_OBJECTS,         // Show active objects
    DEBUG_VIS_TERRAIN,                // Show terrain
    DEBUG_VIS_PARTICLES,              // Show particles
    DEBUG_VIS_FLUID,                  // Show fluid
    DEBUG_VIS_CLOTH,                  // Show cloth mesh
    DEBUG_VIS_DESTRUCTION,            // Show destruction fragments
    DEBUG_VIS_PERFORMANCE_OVERLAY,   // Show performance overlay
    DEBUG_VIS_PROFILING_GRAPH,        // Show profiling graph
    DEBUG_VIS_MEMORY_USAGE,           // Show memory usage
    DEBUG_VIS_ALL                     // Show all visualizations
} DebugVisualizationType;

// ========================================
// Debug Visualization Flags
// ========================================

typedef enum DebugVisualizationFlags {
    DEBUG_FLAG_NONE = 0x00,
    DEBUG_FLAG_WIREFRAME = 0x01,      // Wireframe rendering
    DEBUG_FLAG_SOLID = 0x02,          // Solid rendering
    DEBUG_FLAG_TRANSPARENT = 0x04,     // Transparent rendering
    DEBUG_FLAG_DEPTH_TEST = 0x08,      // Enable depth testing
    DEBUG_FLAG_DEPTH_WRITE = 0x10,     // Enable depth writing
    DEBUG_FLAG_BLENDING = 0x20,        // Enable blending
    DEBUG_FLAG_CULL_BACK = 0x40,       // Backface culling
    DEBUG_FLAG_CULL_FRONT = 0x80,      // Frontface culling
    DEBUG_FLAG_ALL = 0xFF
} DebugVisualizationFlags;

// ========================================
// Color Scheme
// ========================================

typedef struct DebugColor {
    float r, g, b, a;                 // RGBA color components
} DebugColor;

typedef struct DebugColorScheme {
    DebugColor active_object;         // Active object color
    DebugColor sleeping_object;       // Sleeping object color
    DebugColor static_object;         // Static object color
    DebugColor kinematic_object;       // Kinematic object color
    DebugColor trigger_object;         // Trigger object color
    DebugColor sensor_object;          // Sensor object color
    DebugColor collision_shape;        // Collision shape color
    DebugColor bounding_box;          // Bounding box color
    DebugColor bounding_sphere;        // Bounding sphere color
    DebugColor velocity_vector;        // Velocity vector color
    DebugColor force_vector;          // Force vector color
    DebugColor angular_velocity;       // Angular velocity color
    DebugColor center_of_mass;         // Center of mass color
    DebugColor contact_point;          // Contact point color
    DebugColor collision_normal;       // Collision normal color
    DebugColor constraint;             // Constraint color
    DebugColor broken_constraint;      // Broken constraint color
    DebugColor terrain;                // Terrain color
    DebugColor particle;               // Particle color
    DebugColor fluid;                  // Fluid color
    DebugColor cloth;                  // Cloth color
    DebugColor destruction;            // Destruction fragment color
    DebugColor text;                   // Text color
    DebugColor background;            // Background color
    DebugColor grid;                   // Grid color
} DebugColorScheme;

// ========================================
// Debug Visualization Style
// ========================================

typedef struct DebugVisualStyle {
    DebugColorScheme color_scheme;    // Color scheme
    float line_width;                 // Line width for wireframe
    float point_size;                 // Point size for points
    float alpha;                       // Global alpha
    uint32_t flags;                   // Rendering flags
    bool show_labels;                 // Show object labels
    bool show_ids;                     // Show object IDs
    bool show_stats;                   // Show statistics
    bool show_grid;                    // Show coordinate grid
    bool show_axes;                    // Show coordinate axes
    float grid_size;                  // Grid size
    float grid_alpha;                 // Grid alpha
    float label_scale;                // Label scale
    int max_labels;                   // Maximum labels to show
} DebugVisualStyle;

// ========================================
// Debug Renderer Interface
// ========================================

typedef struct DebugRenderer {
    void *renderer_data;              // Renderer-specific data
    
    // Drawing functions
    void (*draw_line)(const float *start, const float *end, const DebugColor *color);
    void (*draw_point)(const float *position, const DebugColor *color, float size);
    void (*draw_sphere)(const float *center, float radius, const DebugColor *color);
    void (*draw_box)(const float *min_corner, const float *max_corner, const DebugColor *color);
    void (*draw_capsule)(const float *start, const float *end, float radius, const DebugColor *color);
    void (*draw_cylinder)(const float *center, float height, float radius, const DebugColor *color);
    void (*draw_cone)(const float *tip, const float *base, float radius, const DebugColor *color);
    void (*draw_triangle)(const float *v0, const float *v1, const float *v2, const DebugColor *color);
    void (*draw_quad)(const float *v0, const float *v1, const float *v2, const float *v3, const DebugColor *color);
    void (*draw_text)(const float *position, const char *text, const DebugColor *color, float scale);
    void (*draw_arrow)(const float *start, const float *end, const DebugColor *color, float head_size);
    void (*draw_circle)(const float *center, float radius, const DebugColor *color, int segments);
    void (*draw_arc)(const float *center, float radius, float start_angle, float end_angle, const DebugColor *color, int segments);
    
    // State management
    void (*set_transform)(const float *matrix);
    void (*set_color)(const DebugColor *color);
    void (*set_line_width)(float width);
    void (*set_point_size)(float size);
    void (*push_state)(void);
    void (*pop_state)(void);
    
    // Rendering control
    void (*begin_frame)(void);
    void (*end_frame)(void);
    void (*flush)(void);
    
} DebugRenderer;

// ========================================
// Debug Visualization Context
// ========================================

typedef struct DebugVisualizationContext {
    PhysicsWorld *physics_world;      // Physics world to visualize
    DebugRenderer *renderer;           // Debug renderer
    DebugVisualStyle style;            // Visual style
    uint32_t enabled_types;           // Enabled visualization types
    bool enabled;                      // Visualization enabled
    bool paused;                       // Visualization paused
    float time_scale;                  // Time scale for animations
    uint64_t frame_count;              // Frame counter
    float frame_time;                  // Frame time
    float total_time;                  // Total time
    
    // Performance metrics
    int objects_drawn;                 // Objects drawn this frame
    int lines_drawn;                   // Lines drawn this frame
    int triangles_drawn;               // Triangles drawn this frame
    int points_drawn;                  // Points drawn this frame
    int text_drawn;                    // Text elements drawn this frame
    float draw_time_ms;                // Time spent drawing
    
    // Culling and optimization
    bool enable_frustum_culling;       // Enable frustum culling
    bool enable_distance_culling;      // Enable distance culling
    float max_draw_distance;           // Maximum draw distance
    float min_object_size;             // Minimum object size to draw
    
    // Camera and viewport
    float camera_position[3];          // Camera position
    float camera_direction[3];         // Camera direction
    float camera_up[3];                // Camera up vector
    float fov;                         // Field of view
    float near_plane;                  // Near plane distance
    float far_plane;                   // Far plane distance
    int viewport_width;                // Viewport width
    int viewport_height;               // Viewport height
    
    // User data
    void *user_data;                   // User data pointer
    
} DebugVisualizationContext;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create debug visualization context
 * @param world Physics world to visualize
 * @param renderer Debug renderer interface
 * @return Visualization context or NULL on failure
 */
DebugVisualizationContext* debug_visualization_create(PhysicsWorld *world, DebugRenderer *renderer);

/**
 * Destroy debug visualization context
 * @param context Visualization context
 */
void debug_visualization_destroy(DebugVisualizationContext *context);

// ========================================
// Configuration Management
// ========================================

/**
 * Get default visual style
 * @return Default visual style
 */
DebugVisualStyle debug_visualization_get_default_style(void);

/**
 * Set visual style
 * @param context Visualization context
 * @param style Visual style
 */
void debug_visualization_set_style(DebugVisualizationContext *context, const DebugVisualStyle *style);

/**
 * Get current visual style
 * @param context Visualization context
 * @return Current visual style
 */
DebugVisualStyle debug_visualization_get_style(DebugVisualizationContext *context);

/**
 * Get default color scheme
 * @return Default color scheme
 */
DebugColorScheme debug_visualization_get_default_color_scheme(void);

/**
 * Set color scheme
 * @param context Visualization context
 * @param color_scheme Color scheme
 */
void debug_visualization_set_color_scheme(DebugVisualizationContext *context, const DebugColorScheme *color_scheme);

// ========================================
// Visualization Control
// ========================================

/**
 * Enable/disable visualization
 * @param context Visualization context
 * @param enabled Enable visualization
 */
void debug_visualization_set_enabled(DebugVisualizationContext *context, bool enabled);

/**
 * Check if visualization is enabled
 * @param context Visualization context
 * @return True if enabled
 */
bool debug_visualization_is_enabled(DebugVisualizationContext *context);

/**
 * Enable/disable specific visualization types
 * @param context Visualization context
 * @param types Visualization types to enable
 */
void debug_visualization_enable_types(DebugVisualizationContext *context, uint32_t types);

/**
 * Disable specific visualization types
 * @param context Visualization context
 * @param types Visualization types to disable
 */
void debug_visualization_disable_types(DebugVisualizationContext *context, uint32_t types);

/**
 * Check if visualization type is enabled
 * @param context Visualization context
 * @param type Visualization type
 * @return True if enabled
 */
bool debug_visualization_is_type_enabled(DebugVisualizationContext *context, DebugVisualizationType type);

// ========================================
// Rendering
// ========================================

/**
 * Begin debug visualization frame
 * @param context Visualization context
 */
void debug_visualization_begin_frame(DebugVisualizationContext *context);

/**
 * End debug visualization frame
 * @param context Visualization context
 */
void debug_visualization_end_frame(DebugVisualizationContext *context);

/**
 * Render all enabled visualizations
 * @param context Visualization context
 */
void debug_visualization_render(DebugVisualizationContext *context);

/**
 * Render specific visualization type
 * @param context Visualization context
 * @param type Visualization type
 */
void debug_visualization_render_type(DebugVisualizationContext *context, DebugVisualizationType type);

// ========================================
// Object Visualization
// ========================================

/**
 * Render physics object
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_object(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render collision shape
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_collision_shape(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render bounding box
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_bounding_box(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render bounding sphere
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_bounding_sphere(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render velocity vector
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_velocity_vector(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render force vector
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_force_vector(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render angular velocity
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_angular_velocity(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render center of mass
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_center_of_mass(DebugVisualizationContext *context, const PhysicsObject *object);

// ========================================
// Constraint Visualization
// ========================================

/**
 * Render physics constraint
 * @param context Visualization context
 * @param constraint Physics constraint
 */
void debug_visualization_render_constraint(DebugVisualizationContext *context, const PhysicsConstraint *constraint);

/**
 * Render constraint connection
 * @param context Visualization context
 * @param constraint Physics constraint
 */
void debug_visualization_render_constraint_connection(DebugVisualizationContext *context, const PhysicsConstraint *constraint);

/**
 * Render constraint limits
 * @param context Visualization context
 * @param constraint Physics constraint
 */
void debug_visualization_render_constraint_limits(DebugVisualizationContext *context, const PhysicsConstraint *constraint);

// ========================================
// Collision Visualization
// ========================================

/**
 * Render contact points
 * @param context Visualization context
 * @param contacts Contact points array
 * @param contact_count Number of contacts
 */
void debug_visualization_render_contact_points(DebugVisualizationContext *context, 
                                               const void *contacts, int contact_count);

/**
 * Render collision normals
 * @param context Visualization context
 * @param contacts Contact points array
 * @param contact_count Number of contacts
 */
void debug_visualization_render_collision_normals(DebugVisualizationContext *context, 
                                                   const void *contacts, int contact_count);

/**
 * Render penetration depth
 * @param context Visualization context
 * @param contacts Contact points array
 * @param contact_count Number of contacts
 */
void debug_visualization_render_penetration_depth(DebugVisualizationContext *context, 
                                                  const void *contacts, int contact_count);

// ========================================
// Performance Visualization
// ========================================

/**
 * Render performance overlay
 * @param context Visualization context
 */
void debug_visualization_render_performance_overlay(DebugVisualizationContext *context);

/**
 * Render profiling graph
 * @param context Visualization context
 * @param profile_data Profile data array
 * @param data_count Number of data points
 */
void debug_visualization_render_profiling_graph(DebugVisualizationContext *context, 
                                                const float *profile_data, int data_count);

/**
 * Render memory usage
 * @param context Visualization context
 */
void debug_visualization_render_memory_usage(DebugVisualizationContext *context);

/**
 * Render frame time graph
 * @param context Visualization context
 * @param frame_times Frame time array
 * @param frame_count Number of frames
 */
void debug_visualization_render_frame_time_graph(DebugVisualizationContext *context, 
                                                 const float *frame_times, int frame_count);

// ========================================
// Utility Visualization
// ========================================

/**
 * Render coordinate grid
 * @param context Visualization context
 */
void debug_visualization_render_grid(DebugVisualizationContext *context);

/**
 * Render coordinate axes
 * @param context Visualization context
 */
void debug_visualization_render_axes(DebugVisualizationContext *context);

/**
 * Render camera frustum
 * @param context Visualization context
 */
void debug_visualization_render_frustum(DebugVisualizationContext *context);

/**
 * Render object labels
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_object_label(DebugVisualizationContext *context, const PhysicsObject *object);

/**
 * Render object ID
 * @param context Visualization context
 * @param object Physics object
 */
void debug_visualization_render_object_id(DebugVisualizationContext *context, const PhysicsObject *object);

// ========================================
// Camera and Viewport
// ========================================

/**
 * Set camera position
 * @param context Visualization context
 * @param position Camera position
 */
void debug_visualization_set_camera_position(DebugVisualizationContext *context, const float *position);

/**
 * Set camera direction
 * @param context Visualization context
 * @param direction Camera direction
 */
void debug_visualization_set_camera_direction(DebugVisualizationContext *context, const float *direction);

/**
 * Set camera up vector
 * @param context Visualization context
 * @param up Camera up vector
 */
void debug_visualization_set_camera_up(DebugVisualizationContext *context, const float *up);

/**
 * Set camera parameters
 * @param context Visualization context
 * @param fov Field of view
 * @param near_plane Near plane distance
 * @param far_plane Far plane distance
 */
void debug_visualization_set_camera_params(DebugVisualizationContext *context, 
                                           float fov, float near_plane, float far_plane);

/**
 * Set viewport size
 * @param context Visualization context
 * @param width Viewport width
 * @param height Viewport height
 */
void debug_visualization_set_viewport(DebugVisualizationContext *context, int width, int height);

/**
 * Get camera position
 * @param context Visualization context
 * @param position Output camera position
 */
void debug_visualization_get_camera_position(DebugVisualizationContext *context, float *position);

/**
 * Get camera direction
 * @param context Visualization context
 * @param direction Output camera direction
 */
void debug_visualization_get_camera_direction(DebugVisualizationContext *context, float *direction);

// ========================================
// Culling and Optimization
// ========================================

/**
 * Enable/disable frustum culling
 * @param context Visualization context
 * @param enabled Enable frustum culling
 */
void debug_visualization_set_frustum_culling(DebugVisualizationContext *context, bool enabled);

/**
 * Enable/disable distance culling
 * @param context Visualization context
 * @param enabled Enable distance culling
 */
void debug_visualization_set_distance_culling(DebugVisualizationContext *context, bool enabled);

/**
 * Set maximum draw distance
 * @param context Visualization context
 * @param distance Maximum distance
 */
void debug_visualization_set_max_draw_distance(DebugVisualizationContext *context, float distance);

/**
 * Set minimum object size
 * @param context Visualization context
 * @param size Minimum size
 */
void debug_visualization_set_min_object_size(DebugVisualizationContext *context, float size);

/**
 * Check if object is visible
 * @param context Visualization context
 * @param object Physics object
 * @return True if object is visible
 */
bool debug_visualization_is_object_visible(DebugVisualizationContext *context, const PhysicsObject *object);

// ========================================
// Statistics and Metrics
// ========================================

/**
 * Get rendering statistics
 * @param context Visualization context
 * @param objects_drawn Objects drawn
 * @param lines_drawn Lines drawn
 * @param triangles_drawn Triangles drawn
 * @param points_drawn Points drawn
 * @param text_drawn Text elements drawn
 * @param draw_time_ms Draw time in milliseconds
 */
void debug_visualization_get_stats(DebugVisualizationContext *context, 
                                   int *objects_drawn, int *lines_drawn,
                                   int *triangles_drawn, int *points_drawn,
                                   int *text_drawn, float *draw_time_ms);

/**
 * Reset rendering statistics
 * @param context Visualization context
 */
void debug_visualization_reset_stats(DebugVisualizationContext *context);

/**
 * Get performance metrics
 * @param context Visualization context
 * @param frame_time Frame time
 * @param total_time Total time
 * @param frame_count Frame count
 */
void debug_visualization_get_performance_metrics(DebugVisualizationContext *context, 
                                                 float *frame_time, float *total_time, 
                                                 uint64_t *frame_count);

// ========================================
// Utility Functions
// ========================================

/**
 * Convert world coordinates to screen coordinates
 * @param context Visualization context
 * @param world_pos World position
 * @param screen_pos Output screen position
 * @return True if conversion successful
 */
bool debug_visualization_world_to_screen(DebugVisualizationContext *context, 
                                         const float *world_pos, float *screen_pos);

/**
 * Convert screen coordinates to world coordinates
 * @param context Visualization context
 * @param screen_pos Screen position
 * @param world_pos Output world position
 * @return True if conversion successful
 */
bool debug_visualization_screen_to_world(DebugVisualizationContext *context, 
                                         const float *screen_pos, float *world_pos);

/**
 * Pick object at screen position
 * @param context Visualization context
 * @param screen_x Screen X coordinate
 * @param screen_y Screen Y coordinate
 * @return Object ID or 0 if no object picked
 */
uint64_t debug_visualization_pick_object(DebugVisualizationContext *context, int screen_x, int screen_y);

/**
 * Validate visualization context
 * @param context Visualization context
 * @return True if context is valid
 */
bool debug_visualization_validate(DebugVisualizationContext *context);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_DEBUG_VISUALIZATION_H */
