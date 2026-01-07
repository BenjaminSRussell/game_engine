#ifndef EDITOR_TOOLS_H
#define EDITOR_TOOLS_H

#include <stdbool.h>
#include "include/tools/asset_editor/asset_editor.h"
#include "include/tools/asset_editor/editor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tool Interface
typedef struct {
    EditorTool type;
    const char* name;
    const char* description;
    bool (*init)(void* tool_data);
    void (*cleanup)(void* tool_data);
    void (*update)(void* tool_data, AssetEditor* editor, float dt);
    void (*render)(void* tool_data, AssetEditor* editor);
    void (*handle_input)(void* tool_data, AssetEditor* editor);
    void (*on_activate)(void* tool_data, AssetEditor* editor);
    void (*on_deactivate)(void* tool_data, AssetEditor* editor);
    void* tool_data;
} EditorToolInterface;

// Selection Tool
typedef struct {
    Selection selection;
    SelectionType selection_mode;
    bool box_selecting;
    vec2 box_start;
    vec2 box_end;
    bool additive_selection;
    bool subtractive_selection;
} SelectionTool;

// Transform Tool
typedef struct {
    TransformOperation operation;
    bool transforming;
    vec3 transform_start;
    vec3 transform_current;
    bool local_space;
    bool snap_enabled;
    float snap_value;
    
    // Gizmo state
    int gizmo_axis; // 0=X, 1=Y, 2=Z, 3=XYZ
    bool gizmo_hovered;
    bool gizmo_dragging;
} TransformTool;

// Sculpt Tool
typedef struct {
    enum {
        SCULPT_BRUSH,
        SCULPT_INFLATE,
        SCULPT_SMOOTH,
        SCULPT_PINCH,
        SCULPT_GRAB
    } sculpt_mode;
    
    float brush_size;
    float brush_strength;
    float brush_falloff;
    bool flood_fill;
    
    // Sculpting state
    bool sculpting;
    vec3 sculpt_position;
    vec3 sculpt_normal;
    float sculpt_delta;
} SculptTool;

// Paint Tool
typedef struct {
    enum {
        PAINT_COLOR,
        PAINT_TEXTURE,
        PAINT_NORMAL,
        PAINT_ROUGHNESS,
        PAINT_METALLIC,
        PAINT_EMISSION
    } paint_mode;
    
    vec4 paint_color;
    float brush_size;
    float brush_strength;
    float brush_opacity;
    uint32_t texture_id;
    
    // Painting state
    bool painting;
    vec2 paint_position;
    vec3 paint_world_position;
    vec3 paint_normal;
} PaintTool;

// Measurement Tool
typedef struct {
    enum {
        MEASURE_DISTANCE,
        MEASURE_ANGLE,
        MEASURE_AREA,
        MEASURE_VOLUME
    } measure_mode;
    
    // Measurement points
    vec3 points[4];
    uint32_t point_count;
    
    // Results
    float distance;
    float angle;
    float area;
    float volume;
    
    bool measuring;
    bool show_measurements;
} MeasurementTool;

// Tool Management Functions
bool editor_tools_init(void);
void editor_tools_cleanup(void);
void editor_tools_update(AssetEditor* editor, float dt);
void editor_tools_render(AssetEditor* editor);

// Tool Registration
bool editor_tools_register_tool(EditorToolInterface* tool);
EditorToolInterface* editor_tools_get_tool(EditorTool type);
void editor_tools_set_active_tool(EditorTool type);

// Individual Tool Functions
bool selection_tool_init(SelectionTool* tool);
void selection_tool_update(SelectionTool* tool, AssetEditor* editor, float dt);
void selection_tool_render(SelectionTool* tool, AssetEditor* editor);
void selection_tool_handle_input(SelectionTool* tool, AssetEditor* editor);

bool transform_tool_init(TransformTool* tool);
void transform_tool_update(TransformTool* tool, AssetEditor* editor, float dt);
void transform_tool_render(TransformTool* tool, AssetEditor* editor);
void transform_tool_handle_input(TransformTool* tool, AssetEditor* editor);

bool sculpt_tool_init(SculptTool* tool);
void sculpt_tool_update(SculptTool* tool, AssetEditor* editor, float dt);
void sculpt_tool_render(SculptTool* tool, AssetEditor* editor);
void sculpt_tool_handle_input(SculptTool* tool, AssetEditor* editor);

bool paint_tool_init(PaintTool* tool);
void paint_tool_update(PaintTool* tool, AssetEditor* editor, float dt);
void paint_tool_render(PaintTool* tool, AssetEditor* editor);
void paint_tool_handle_input(PaintTool* tool, AssetEditor* editor);

bool measurement_tool_init(MeasurementTool* tool);
void measurement_tool_update(MeasurementTool* tool, AssetEditor* editor, float dt);
void measurement_tool_render(MeasurementTool* tool, AssetEditor* editor);
void measurement_tool_handle_input(MeasurementTool* tool, AssetEditor* editor);

// Utility Functions
Selection* editor_tools_get_selection(void);
void editor_tools_clear_selection(void);
void editor_tools_select_all(void);
void editor_tools_invert_selection(void);
bool editor_tools_has_selection(void);

// Gizmo Functions
bool editor_tools_gizmo_hovered(const AssetEditor* editor, const vec2 mouse_pos);
int editor_tools_gizmo_get_axis(const AssetEditor* editor, const vec2 mouse_pos);
void editor_tools_gizmo_render(const AssetEditor* editor, const TransformTool* tool);

// Grid Snapping
vec3 editor_tools_snap_to_grid(const vec3 position, float grid_size);
bool editor_tools_should_snap(const AssetEditor* editor);
float editor_tools_get_snap_value(const AssetEditor* editor);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_TOOLS_H
