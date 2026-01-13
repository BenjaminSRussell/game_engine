// include/editor/viewport/selection.h
//
// Purpose: GPU-based object selection and picking system
// Provides ray-casting and ID-based selection for editor viewport
//
#ifndef EDITOR_VIEWPORT_SELECTION_H
#define EDITOR_VIEWPORT_SELECTION_H

#include "include/common.h"
#include "engine/include/math/math.h"

// Forward declarations
typedef struct Camera Camera;
typedef struct IRenderer IRenderer;

// Selection modes
typedef enum {
    SELECTION_MODE_REPLACE,      // Replace current selection
    SELECTION_MODE_ADD,          // Add to selection
    SELECTION_MODE_REMOVE,       // Remove from selection
    SELECTION_MODE_TOGGLE,       // Toggle selection state
    SELECTION_MODE_COUNT
} SelectionMode;

// Selection types
typedef enum {
    SELECTION_TYPE_OBJECT,       // Object-level selection
    SELECTION_TYPE_FACE,         // Face/polygon selection
    SELECTION_TYPE_EDGE,         // Edge selection
    SELECTION_TYPE_VERTEX,       // Vertex selection
    SELECTION_TYPE_COUNT
} SelectionType;

// Selection hit result
typedef struct {
    u32 object_id;               // Object ID (from ID buffer)
    u32 primitive_id;            // Primitive/face ID
    f32 depth;                   // Depth value
    Vec3 world_position;         // Hit position in world space
    Vec3 world_normal;           // Hit normal in world space
    Vec2 screen_position;        // Hit position in screen space
    bool valid;                  // Whether hit is valid
} SelectionHit;

// Selection box (for marquee selection)
typedef struct {
    Vec2 min;                    // Minimum screen coordinate
    Vec2 max;                    // Maximum screen coordinate
    bool active;                 // Whether box selection is active
    bool started;                // Whether box selection has started
} SelectionBox;

// Selection system
typedef struct {
    // GPU picking resources
    u32 id_framebuffer;         // Framebuffer for ID rendering
    u32 id_color_texture;       // Color texture for object IDs
    u32 id_depth_texture;       // Depth texture for depth testing
    
    // Selection state
    u32 *selected_objects;      // Array of selected object IDs
    u32 selected_count;          // Number of selected objects
    u32 max_selections;         // Maximum number of selections
    SelectionMode mode;         // Current selection mode
    SelectionType type;         // Current selection type
    
    // Box selection
    SelectionBox box;
    
    // Last hit result
    SelectionHit last_hit;
    
    // Configuration
    bool enable_depth_testing;  // Enable depth testing for selection
    bool enable_box_selection;   // Enable marquee box selection
    f32 selection_tolerance;     // Pixel tolerance for selection
    
    // Performance
    bool id_texture_dirty;       // Whether ID texture needs update
} SelectionSystem;

// Public API
SelectionSystem* selection_system_create(u32 max_selections);
void selection_system_destroy(SelectionSystem *selection);

// Selection operations
bool selection_pick_object(SelectionSystem *selection, Vec2 screen_pos, const Camera *camera, IRenderer *renderer);
bool selection_pick_objects_in_box(SelectionSystem *selection, const SelectionBox *box, const Camera *camera, IRenderer *renderer);
void selection_clear(SelectionSystem *selection);
void selection_select_object(SelectionSystem *selection, u32 object_id);
void selection_deselect_object(SelectionSystem *selection, u32 object_id);
void selection_toggle_object(SelectionSystem *selection, u32 object_id);

// Selection queries
bool selection_is_object_selected(const SelectionSystem *selection, u32 object_id);
u32 selection_get_count(const SelectionSystem *selection);
const u32* selection_get_objects(const SelectionSystem *selection);
SelectionHit selection_get_last_hit(const SelectionSystem *selection);

// Selection mode
void selection_set_mode(SelectionSystem *selection, SelectionMode mode);
void selection_set_type(SelectionSystem *selection, SelectionType type);
SelectionMode selection_get_mode(const SelectionSystem *selection);
SelectionType selection_get_type(const SelectionSystem *selection);

// Box selection
void selection_box_start(SelectionSystem *selection, Vec2 start_pos);
void selection_box_update(SelectionSystem *selection, Vec2 current_pos);
void selection_box_end(SelectionSystem *selection);
bool selection_box_is_active(const SelectionSystem *selection);
SelectionBox selection_get_box(const SelectionSystem *selection);

// Rendering (for ID buffer)
void selection_render_id_buffer(SelectionSystem *selection, IRenderer *renderer, const Camera *camera);
void selection_mark_id_texture_dirty(SelectionSystem *selection);

// Configuration
void selection_set_tolerance(SelectionSystem *selection, f32 tolerance);
void selection_enable_depth_testing(SelectionSystem *selection, bool enable);
void selection_enable_box_selection(SelectionSystem *selection, bool enable);

// Utility
Vec3 selection_get_center(const SelectionSystem *selection, const Vec3 *object_positions);
void selection_get_bounds(const SelectionSystem *selection, const Vec3 *object_positions, Vec3 *min_bounds, Vec3 *max_bounds);

#endif // EDITOR_VIEWPORT_SELECTION_H
