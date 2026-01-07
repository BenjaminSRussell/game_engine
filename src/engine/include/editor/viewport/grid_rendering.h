// include/editor/viewport/grid_rendering.h
//
// Purpose: Infinite grid rendering system with distance-based fading
// Provides visual reference grid for 3D viewport
//
#ifndef EDITOR_VIEWPORT_GRID_RENDERING_H
#define EDITOR_VIEWPORT_GRID_RENDERING_H

#include "include/common.h"
#include "include/math/math.h"

// Forward declarations
typedef struct Camera Camera;
typedef struct IRenderer IRenderer;

// Grid rendering modes
typedef enum {
    GRID_MODE_PERSPECTIVE,      // Perspective grid with perspective correction
    GRID_MODE_ORTHOGRAPHIC,     // Orthographic grid (2D)
    GRID_MODE_COUNT
} GridMode;

// Grid plane orientations
typedef enum {
    GRID_PLANE_XY,              // Horizontal grid (floor)
    GRID_PLANE_XZ,              // Vertical grid (wall)
    GRID_PLANE_YZ,              // Vertical grid (wall)
    GRID_PLANE_COUNT
} GridPlane;

// Grid configuration
typedef struct {
    GridMode mode;
    GridPlane plane;
    Vec3 origin;                // Grid origin point
    f32 spacing;                // Grid spacing between lines
    f32 major_spacing;          // Spacing for major grid lines (multiples of spacing)
    u32 subdivision;            // Number of subdivisions per major line
    
    // Visual properties
    Vec3 color_major;           // Color for major grid lines
    Vec3 color_minor;           // Color for minor grid lines
    Vec3 color_origin;          // Color for origin lines
    f32 line_width_major;       // Line width for major lines
    f32 line_width_minor;       // Line width for minor lines
    f32 line_width_origin;      // Line width for origin lines
    
    // Fading
    bool enable_fading;         // Enable distance-based fading
    f32 fade_start_distance;    // Distance at which fading starts
    f32 fade_end_distance;      // Distance at which grid completely fades
    f32 fade_power;             // Fading curve power (1.0 = linear)
    
    // Rendering
    bool render_above_objects;  // Render grid on top of objects
    bool snap_to_grid;          // Enable grid snapping
    bool show_origin;           // Show origin lines
    bool show_subdivisions;     // Show subdivision lines
} GridConfig;

// Grid rendering system
typedef struct {
    GridConfig config;
    
    // Rendering resources
    u32 grid_vertex_buffer;     // Vertex buffer for grid lines
    u32 grid_index_buffer;      // Index buffer for grid lines
    u32 vertex_count;           // Number of vertices
    u32 index_count;            // Number of indices
    
    // Dynamic grid generation
    bool grid_dirty;            // Whether grid needs regeneration
    f32 last_camera_distance;   // Last camera distance for LOD calculation
    u32 current_lod;            // Current LOD level
    
    // Performance
    bool use_instanced_rendering; // Use instanced rendering for efficiency
    u32 max_grid_lines;        // Maximum number of grid lines to render
} GridRendering;

// Public API
GridRendering* grid_rendering_create(void);
void grid_rendering_destroy(GridRendering *grid);

// Configuration
void grid_set_mode(GridRendering *grid, GridMode mode);
void grid_set_plane(GridRendering *grid, GridPlane plane);
void grid_set_origin(GridRendering *grid, Vec3 origin);
void grid_set_spacing(GridRendering *grid, f32 spacing);
void grid_set_colors(GridRendering *grid, Vec3 major, Vec3 minor, Vec3 origin);
void grid_set_line_widths(GridRendering *grid, f32 major, f32 minor, f32 origin);
void grid_set_fading(GridRendering *grid, bool enable, f32 start_dist, f32 end_dist, f32 power);

// Grid snapping
Vec3 grid_snap_position(const GridRendering *grid, Vec3 position, bool snap_x, bool snap_y, bool snap_z);
bool grid_is_snap_enabled(const GridRendering *grid);
void grid_set_snap_enabled(GridRendering *grid, bool enabled);

// Rendering
void grid_render(GridRendering *grid, IRenderer *renderer, const Camera *camera);
void grid_render_with_fade(GridRendering *grid, IRenderer *renderer, const Camera *camera, f32 fade_factor);

// Utility
bool grid_is_visible(const GridRendering *grid, const Camera *camera);
f32 grid_get_fade_factor(const GridRendering *grid, const Camera *camera, Vec3 position);
u32 grid_get_lod_level(const GridRendering *grid, f32 camera_distance);

// Grid generation
void grid_regenerate(GridRendering *grid);
void grid_mark_dirty(GridRendering *grid);

#endif // EDITOR_VIEWPORT_GRID_RENDERING_H
