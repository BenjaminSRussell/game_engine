#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include <stdbool.h>
#include "asset_editor.h"
#include "editor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// UI Panel Types
typedef enum {
    UI_PANEL_TOOLBAR,
    UI_PANEL_PROPERTIES,
    UI_PANEL_ASSET_BROWSER,
    UI_PANEL_TIMELINE,
    UI_PANEL_OUTLINER,
    UI_PANEL_CONSOLE,
    UI_PANEL_SETTINGS,
    UI_PANEL_COUNT
} UIPanelType;

// UI Layout Structure
typedef struct {
    bool panel_visible[UI_PANEL_COUNT];
    float panel_width[UI_PANEL_COUNT];
    float panel_height[UI_PANEL_COUNT];
    vec2 panel_position[UI_PANEL_COUNT];
    
    // Main viewport area
    vec2 viewport_position;
    vec2 viewport_size;
    
    // UI Scaling
    float ui_scale;
    bool compact_mode;
    
    // Theme
    bool dark_theme;
    float alpha;
} UILayout;

// Asset Browser Item
typedef struct {
    char name[64];
    char path[256];
    AssetType type;
    uint32_t preview_texture;
    bool is_folder;
    bool selected;
    
    // Metadata
    uint64_t file_size;
    uint64_t modified_time;
    char tags[4][32];
    uint32_t tag_count;
} AssetBrowserItem;

// Asset Browser State
typedef struct {
    AssetBrowserItem* items;
    uint32_t item_count;
    uint32_t selected_index;
    
    char current_path[256];
    char search_query[128];
    
    // View mode
    enum {
        BROWSER_VIEW_LIST,
        BROWSER_VIEW_GRID,
        BROWSER_VIEW_TILES
    } view_mode;
    
    // Sorting
    enum {
        SORT_BY_NAME,
        SORT_BY_TYPE,
        SORT_BY_DATE,
        SORT_BY_SIZE
    } sort_by;
    
    bool ascending;
    bool show_hidden;
} AssetBrowserState;

// Properties Panel State
typedef struct {
    // Current asset being edited
    void* current_asset;
    AssetType asset_type;
    
    // Property categories
    bool show_transform;
    bool show_material;
    bool show_geometry;
    bool show_animation;
    bool show_physics;
    
    // Editing state
    bool editing_property;
    char property_buffer[128];
} PropertiesPanelState;

// Timeline State
typedef struct {
    bool playing;
    bool looping;
    float current_time;
    float start_time;
    float end_time;
    float zoom;
    
    // Playback controls
    float playback_speed;
    bool show_keyframes;
    bool show_timeline;
    
    // Animation data
    uint32_t current_animation;
    uint32_t keyframe_count;
} TimelineState;

// UI Initialization
bool editor_ui_init(UILayout* layout);
void editor_ui_cleanup(UILayout* layout);
void editor_ui_update(UILayout* layout, AssetEditor* editor, float dt);
void editor_ui_render(UILayout* layout, AssetEditor* editor);

// Panel Management
void editor_ui_show_panel(UIPanelType panel, bool show);
bool editor_ui_is_panel_visible(UIPanelType panel);
void editor_ui_toggle_panel(UIPanelType panel);

// Toolbar Functions
void editor_ui_render_toolbar(AssetEditor* editor);
void editor_ui_toolbar_set_tool(EditorTool tool);
EditorTool editor_ui_toolbar_get_active_tool(void);

// Properties Panel Functions
void editor_ui_render_properties_panel(AssetEditor* editor);
void editor_ui_properties_set_asset(void* asset, AssetType type);
void editor_ui_properties_update_transform(const TransformOperation* transform);
void editor_ui_properties_update_material(const Material* material);

// Asset Browser Functions
void editor_ui_render_asset_browser(AssetEditor* editor);
void editor_ui_browser_refresh(void);
void editor_ui_browser_set_path(const char* path);
void editor_ui_browser_set_search(const char* query);
void editor_ui_browser_select_item(uint32_t index);

// Timeline Functions
void editor_ui_render_timeline(AssetEditor* editor);
void editor_ui_timeline_play(void);
void editor_ui_timeline_pause(void);
void editor_ui_timeline_stop(void);
void editor_ui_timeline_set_time(float time);
void editor_ui_timeline_set_range(float start, float end);

// Outliner Functions
void editor_ui_render_outliner(AssetEditor* editor);
void editor_ui_outliner_refresh(void);
void editor_ui_outliner_select_item(uint32_t index);

// Console Functions
void editor_ui_render_console(void);
void editor_ui_console_log(const char* message);
void editor_ui_console_warning(const char* message);
void editor_ui_console_error(const char* message);
void editor_ui_console_clear(void);

// Settings Functions
void editor_ui_render_settings(void);
void editor_ui_settings_load(void);
void editor_ui_settings_save(void);
void editor_ui_settings_reset(void);

// Utility Functions
void editor_ui_set_scale(float scale);
float editor_ui_get_scale(void);
void editor_ui_set_theme(bool dark);
bool editor_ui_is_dark_theme(void);

// Input Handling
bool editor_ui_handle_mouse_input(AssetEditor* editor, int button, int action, int mods);
bool editor_ui_handle_key_input(AssetEditor* editor, int key, int scancode, int action, int mods);
bool editor_ui_handle_char_input(AssetEditor* editor, unsigned int codepoint);

// Context Menus
void editor_ui_render_context_menu(AssetEditor* editor);
void editor_ui_show_context_menu(const vec2 position);
void editor_ui_hide_context_menu(void);

// Dialog Boxes
bool editor_ui_show_save_dialog(void);
bool editor_ui_show_load_dialog(void);
bool editor_ui_show_export_dialog(void);
bool editor_ui_show_settings_dialog(void);
bool editor_ui_show_about_dialog(void);

// File Operations
const char* editor_ui_get_selected_file(void);
const char* editor_ui_get_save_path(void);
const char* editor_ui_get_load_path(void);

// Notification System
void editor_ui_show_notification(const char* message, float duration);
void editor_ui_show_error(const char* message);
void editor_ui_show_warning(const char* message);
void editor_ui_show_success(const char* message);

// Progress Indicators
void editor_ui_show_progress(const char* operation, float progress);
void editor_ui_hide_progress(void);
bool editor_ui_is_progress_visible(void);

// Hotkeys and Shortcuts
void editor_ui_register_hotkey(int key, int mods, void (*callback)(void));
void editor_ui_unregister_hotkey(int key, int mods);
bool editor_ui_handle_hotkey(int key, int mods);

// Tooltip System
void editor_ui_set_tooltip(const char* text);
void editor_ui_clear_tooltip(void);
const char* editor_ui_get_tooltip(void);

// Drag and Drop
bool editor_ui_is_dragging(void);
void editor_ui_start_drag(void* data, const char* type);
void editor_ui_end_drag(void);
void* editor_ui_get_drag_data(const char* type);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_UI_H
