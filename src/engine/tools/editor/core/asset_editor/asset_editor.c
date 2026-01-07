#include "tools/asset_editor/asset_editor.h"
#include "tools/asset_editor/editor_tools.h"
#include "tools/asset_editor/editor_ui.h"
#include "include/platform/input/input.h"
#include <rendering/renderer.h>
#include "audio/audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global asset editor instance
static AssetEditor g_editor = {0};

// Forward declarations
static void asset_editor_update_input(AssetEditor* editor);
static void asset_editor_update_viewport(AssetEditor* editor);
static void asset_editor_render_grid(AssetEditor* editor);
static void asset_editor_render_gizmos(AssetEditor* editor);

bool asset_editor_init(AssetEditor* editor) {
    if (!editor) return false;
    
    // Initialize editor state
    memset(editor, 0, sizeof(AssetEditor));
    editor->state = EDITOR_STATE_INACTIVE;
    editor->active_tool = TOOL_SELECT;
    editor->current_asset_type = ASSET_TYPE_3D_MODEL;
    
    // Initialize viewport
    editor->viewport.position = (vec3){0.0f, 0.0f, 5.0f};
    editor->viewport.zoom = 1.0f;
    editor->viewport.orthographic = false;
    editor->viewport.wireframe = false;
    editor->viewport.show_grid = true;
    editor->viewport.show_gizmos = true;
    
    // Initialize UI state
    editor->show_properties_panel = true;
    editor->show_asset_browser = true;
    editor->show_timeline = false;
    editor->show_toolbar = true;
    
    // Initialize grid settings
    editor->grid_visible = true;
    editor->grid_size = 1.0f;
    editor->grid_subdivisions = 10;
    editor->grid_color = (vec3){0.5f, 0.5f, 0.5f};
    
    // Initialize gizmo settings
    editor->gizmo_visible = true;
    editor->gizmo_size = 1.0f;
    editor->gizmo_local_space = false;
    
    // Initialize asset management
    editor->asset_modified = false;
    editor->auto_save = true;
    memset(editor->current_asset_path, 0, sizeof(editor->current_asset_path));
    
    // Initialize tools
    if (!editor_tools_init()) {
        printf("Failed to initialize editor tools\n");
        return false;
    }
    
    // Initialize UI
    UILayout ui_layout;
    if (!editor_ui_init(&ui_layout)) {
        printf("Failed to initialize editor UI\n");
        editor_tools_cleanup();
        return false;
    }
    
    printf("Asset Editor initialized successfully\n");
    return true;
}

void asset_editor_cleanup(AssetEditor* editor) {
    if (!editor) return;
    
    // Cleanup tools
    editor_tools_cleanup();
    
    // Cleanup UI
    editor_ui_cleanup(NULL);
    
    // Clear any allocated resources
    memset(editor, 0, sizeof(AssetEditor));
    
    printf("Asset Editor cleaned up\n");
}

void asset_editor_update(AssetEditor* editor, float dt) {
    if (!editor) return;
    
    // Update input state
    asset_editor_update_input(editor);
    
    // Update viewport
    asset_editor_update_viewport(editor);
    
    // Update tools
    editor_tools_update(editor, dt);
    
    // Update UI
    UILayout ui_layout = {0}; // This should be persistent
    editor_ui_update(&ui_layout, editor, dt);
    
    // Auto-save if enabled and asset is modified
    if (editor->auto_save && editor->asset_modified) {
        // Auto-save logic here
        editor->asset_modified = false;
    }
}

void asset_editor_render(AssetEditor* editor) {
    if (!editor) return;
    
    // Set up viewport camera
    renderer_set_camera(&editor->viewport.camera);
    
    // Render grid if visible
    if (editor->grid_visible) {
        asset_editor_render_grid(editor);
    }
    
    // Render current asset
    // This would depend on the current asset type
    
    // Render gizmos if visible
    if (editor->gizmo_visible) {
        asset_editor_render_gizmos(editor);
    }
    
    // Render tools
    editor_tools_render(editor);
    
    // Render UI
    UILayout ui_layout = {0}; // This should be persistent
    editor_ui_render(&ui_layout, editor);
}

void asset_editor_set_state(AssetEditor* editor, EditorState state) {
    if (!editor) return;
    
    if (editor->state != state) {
        editor->state = state;
        
        // Handle state transitions
        switch (state) {
            case EDITOR_STATE_CREATE:
                editor->active_tool = TOOL_NONE;
                break;
            case EDITOR_STATE_EDIT:
                editor->active_tool = TOOL_SELECT;
                break;
            case EDITOR_STATE_PREVIEW:
                editor->active_tool = TOOL_NONE;
                break;
            case EDITOR_STATE_LIBRARY:
                editor->active_tool = TOOL_NONE;
                break;
            default:
                break;
        }
    }
}

void asset_editor_set_tool(AssetEditor* editor, EditorTool tool) {
    if (!editor) return;
    
    if (editor->active_tool != tool) {
        editor->active_tool = tool;
        editor_tools_set_active_tool(tool);
    }
}

void asset_editor_set_asset_type(AssetEditor* editor, AssetType type) {
    if (!editor) return;
    
    if (editor->current_asset_type != type) {
        editor->current_asset_type = type;
        
        // Reset tool based on asset type
        switch (type) {
            case ASSET_TYPE_3D_MODEL:
            case ASSET_TYPE_ANIMATION:
                editor->active_tool = TOOL_SELECT;
                break;
            case ASSET_TYPE_2D_SPRITE:
            case ASSET_TYPE_TEXTURE:
            case ASSET_TYPE_UI_ELEMENT:
                editor->active_tool = TOOL_PAINT;
                break;
            case ASSET_TYPE_PARTICLE:
                editor->active_tool = TOOL_SELECT;
                break;
            default:
                editor->active_tool = TOOL_SELECT;
                break;
        }
    }
}

bool asset_editor_new_asset(AssetEditor* editor, AssetType type, const char* name) {
    if (!editor || !name) return false;
    
    // Set asset type and state
    asset_editor_set_asset_type(editor, type);
    asset_editor_set_state(editor, EDITOR_STATE_CREATE);
    
    // Create new asset based on type
    switch (type) {
        case ASSET_TYPE_3D_MODEL:
            // Create new 3D model
            break;
        case ASSET_TYPE_2D_SPRITE:
            // Create new sprite
            break;
        case ASSET_TYPE_TEXTURE:
            // Create new texture
            break;
        case ASSET_TYPE_MATERIAL:
            // Create new material
            break;
        case ASSET_TYPE_PARTICLE:
            // Create new particle system
            break;
        case ASSET_TYPE_UI_ELEMENT:
            // Create new UI element
            break;
        case ASSET_TYPE_ANIMATION:
            // Create new animation
            break;
        case ASSET_TYPE_SOUND:
            // Create new sound asset
            break;
        default:
            return false;
    }
    
    // Set current asset path
    snprintf(editor->current_asset_path, sizeof(editor->current_asset_path), "assets/%s", name);
    
    editor->asset_modified = false;
    return true;
}

bool asset_editor_load_asset(AssetEditor* editor, const char* path) {
    if (!editor || !path) return false;
    
    // Load asset from file
    // This would implement file loading based on file extension
    
    // Update current asset path
    strncpy(editor->current_asset_path, path, sizeof(editor->current_asset_path) - 1);
    
    // Set editor state to edit
    asset_editor_set_state(editor, EDITOR_STATE_EDIT);
    
    editor->asset_modified = false;
    return true;
}

bool asset_editor_save_asset(AssetEditor* editor, const char* path) {
    if (!editor || !path) return false;
    
    // Save current asset to file
    // This would implement file saving based on asset type
    
    // Update current asset path
    strncpy(editor->current_asset_path, path, sizeof(editor->current_asset_path) - 1);
    
    editor->asset_modified = false;
    return true;
}

bool asset_editor_export_asset(AssetEditor* editor, const char* path, const char* format) {
    if (!editor || !path || !format) return false;
    
    // Export asset in specified format
    // This would implement format-specific export
    
    return true;
}

void asset_editor_handle_mouse_input(AssetEditor* editor, int button, int action, int mods) {
    if (!editor) return;
    
    // Update mouse button state
    switch (button) {
        case 0: // Left button
            editor->mouse_left_pressed = (action == 1);
            break;
        case 1: // Right button
            editor->mouse_right_pressed = (action == 1);
            break;
        case 2: // Middle button
            editor->mouse_middle_pressed = (action == 1);
            break;
    }
    
    // Update keyboard modifiers
    editor->shift_pressed = (mods & 0x0001) != 0;
    editor->ctrl_pressed = (mods & 0x0002) != 0;
    editor->alt_pressed = (mods & 0x0004) != 0;
    
    // Handle UI input first
    if (editor_ui_handle_mouse_input(editor, button, action, mods)) {
        return; // UI handled the input
    }
    
    // Handle tool input
    editor_tools_handle_input(editor);
}

void asset_editor_handle_cursor_pos(AssetEditor* editor, double xpos, double ypos) {
    if (!editor) return;
    
    // Calculate mouse delta
    vec2 new_pos = (vec2){(float)xpos, (float)ypos};
    editor->mouse_delta = vec2_sub(new_pos, editor->mouse_position);
    editor->mouse_position = new_pos;
    
    // Handle UI cursor
    if (editor_ui_handle_mouse_input(editor, -1, -1, -1)) {
        return; // UI handled the cursor
    }
    
    // Handle tool cursor
    editor_tools_handle_input(editor);
}

void asset_editor_handle_scroll(AssetEditor* editor, double xoffset, double yoffset) {
    if (!editor) return;
    
    // Handle zoom in viewport
    if (!editor_ui_handle_mouse_input(editor, -1, -1, -1)) {
        // Zoom viewport
        float zoom_speed = 0.1f;
        editor->viewport.zoom *= (1.0f + (float)yoffset * zoom_speed);
        editor->viewport.zoom = fmaxf(0.1f, fminf(10.0f, editor->viewport.zoom));
    }
}

void asset_editor_handle_key_input(AssetEditor* editor, int key, int scancode, int action, int mods) {
    if (!editor) return;
    
    // Update keyboard modifiers
    editor->shift_pressed = (mods & 0x0001) != 0;
    editor->ctrl_pressed = (mods & 0x0002) != 0;
    editor->alt_pressed = (mods & 0x0004) != 0;
    
    // Handle UI input first
    if (editor_ui_handle_key_input(editor, key, scancode, action, mods)) {
        return; // UI handled the input
    }
    
    // Handle keyboard shortcuts
    if (action == 1) { // Key press
        switch (key) {
            case 71: // G key - Toggle grid
                editor->grid_visible = !editor->grid_visible;
                break;
            case 82: // R key - Toggle wireframe
                editor->viewport.wireframe = !editor->viewport.wireframe;
                break;
            case 84: // T key - Toggle gizmos
                editor->gizmo_visible = !editor->gizmo_visible;
                break;
            case 256: // Escape key - Deselect
                editor_tools_clear_selection();
                break;
            case 90: // Z key - Undo
                if (editor->ctrl_pressed) {
                    // Undo action
                }
                break;
            case 89: // Y key - Redo
                if (editor->ctrl_pressed) {
                    // Redo action
                }
                break;
            case 83: // S key - Save
                if (editor->ctrl_pressed) {
                    asset_editor_save_asset(editor, editor->current_asset_path);
                }
                break;
        }
    }
}

void asset_editor_reset_camera(AssetEditor* editor) {
    if (!editor) return;
    
    editor->viewport.position = (vec3){0.0f, 0.0f, 5.0f};
    editor->viewport.rotation = (vec3){0.0f, 0.0f, 0.0f};
    editor->viewport.zoom = 1.0f;
}

void asset_editor_frame_selected(AssetEditor* editor) {
    if (!editor) return;
    
    // Frame selected objects in viewport
    // This would calculate bounds of selected objects and adjust camera
}

void asset_editor_set_viewport_mode(AssetEditor* editor, bool orthographic) {
    if (!editor) return;
    
    editor->viewport.orthographic = orthographic;
}

bool asset_editor_is_modified(const AssetEditor* editor) {
    return editor ? editor->asset_modified : false;
}

const char* asset_editor_get_current_asset(const AssetEditor* editor) {
    return editor ? editor->current_asset_path : NULL;
}

void asset_editor_set_auto_save(AssetEditor* editor, bool enabled) {
    if (editor) {
        editor->auto_save = enabled;
    }
}

// Static helper functions
static void asset_editor_update_input(AssetEditor* editor) {
    // Update camera controls based on mouse input
    if (editor->mouse_right_pressed) {
        // Rotate camera
        editor->viewport.rotation.x += editor->mouse_delta.y * 0.01f;
        editor->viewport.rotation.y += editor->mouse_delta.x * 0.01f;
    }
    
    if (editor->mouse_middle_pressed) {
        // Pan camera
        float pan_speed = 0.01f / editor->viewport.zoom;
        editor->viewport.position.x -= editor->mouse_delta.x * pan_speed;
        editor->viewport.position.y += editor->mouse_delta.y * pan_speed;
    }
    
    // Reset mouse delta
    editor->mouse_delta = (vec2){0.0f, 0.0f};
}

static void asset_editor_update_viewport(AssetEditor* editor) {
    // Update camera matrices based on position, rotation, and zoom
    // This would update the camera's view and projection matrices
    
    // For now, just set basic camera properties
    editor->viewport.camera.position = editor->viewport.position;
    editor->viewport.camera.rotation = editor->viewport.rotation;
    editor->viewport.camera.fov = 45.0f / editor->viewport.zoom;
}

static void asset_editor_render_grid(AssetEditor* editor) {
    // Render grid lines
    // This would use the renderer to draw grid lines
    
    // For now, this is a placeholder
}

static void asset_editor_render_gizmos(AssetEditor* editor) {
    // Render gizmos for selected objects
    // This would render transform gizmos, selection outlines, etc.
    
    // For now, this is a placeholder
}

// Global accessor functions
AssetEditor* get_asset_editor(void) {
    return &g_editor;
}

bool init_asset_editor_system(void) {
    return asset_editor_init(&g_editor);
}

void cleanup_asset_editor_system(void) {
    asset_editor_cleanup(&g_editor);
}
