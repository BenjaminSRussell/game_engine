#include "tools/asset_editor/editor_types.h"
#include "tools/asset_editor/editor_tools.h"
#include "tools/asset_editor/asset_editor.h"
#include <rendering/renderer.h>
#include "core/memory.h"
#include "include/platform/input/input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Sprite Editor State
typedef struct {
    Sprite2D* current_sprite;
    uint32_t* texture_data;
    int texture_width;
    int texture_height;
    
    // Editing state
    bool editing_pixels;
    bool editing_frames;
    bool editing_animation;
    
    // Drawing tools
    struct {
        vec4 brush_color;
        float brush_size;
        float brush_opacity;
        bool anti_aliasing;
        
        // Tool types
        enum {
            SPRITE_TOOL_BRUSH,
            SPRITE_TOOL_ERASER,
            SPRITE_TOOL_FILL,
            SPRITE_TOOL_LINE,
            SPRITE_TOOL_RECT,
            SPRITE_TOOL_CIRCLE,
            SPRITE_TOOL_PICKER
        } tool_type;
    } drawing_tool;
    
    // Selection
    struct {
        vec2 selection_start;
        vec2 selection_end;
        bool selecting;
        bool has_selection;
        uint32_t* selection_mask;
    } selection;
    
    // Animation editing
    struct {
        uint32_t current_frame;
        float frame_time;
        bool playing;
        bool looping;
        float playback_speed;
    } animation;
    
    // View settings
    vec2 pan_offset;
    float zoom;
    bool show_grid;
    bool show_onion_skin;
    bool show_transparency;
    int grid_size;
    
    // Onion skinning
    struct {
        float opacity;
        int frames_behind;
        int frames_ahead;
        bool show_behind;
        bool show_ahead;
    } onion_skin;
    
    // Undo/Redo
    struct {
        uint32_t** history;
        uint32_t history_count;
        uint32_t history_capacity;
        uint32_t history_index;
        size_t texture_size;
    } undo_system;
    
} SpriteEditor;

static SpriteEditor g_sprite_editor = {0};

// Forward declarations
static void sprite_editor_update_texture(SpriteEditor* editor);
static void sprite_editor_apply_brush(SpriteEditor* editor, const vec2 position);
static void sprite_editor_fill_area(SpriteEditor* editor, const vec2 start_pos, const vec4 fill_color);
static void sprite_editor_save_to_history(SpriteEditor* editor);
static uint32_t sprite_editor_blend_colors(uint32_t base, uint32_t overlay, float opacity);

bool sprite_editor_init(void) {
    memset(&g_sprite_editor, 0, sizeof(SpriteEditor));
    
    // Set default values
    g_sprite_editor.drawing_tool.brush_color = (vec4){1.0f, 1.0f, 1.0f, 1.0f};
    g_sprite_editor.drawing_tool.brush_size = 1.0f;
    g_sprite_editor.drawing_tool.brush_opacity = 1.0f;
    g_sprite_editor.drawing_tool.anti_aliasing = true;
    g_sprite_editor.drawing_tool.tool_type = SPRITE_TOOL_BRUSH;
    
    g_sprite_editor.zoom = 1.0f;
    g_sprite_editor.show_grid = true;
    g_sprite_editor.show_onion_skin = false;
    g_sprite_editor.show_transparency = true;
    g_sprite_editor.grid_size = 16;
    
    g_sprite_editor.onion_skin.opacity = 0.3f;
    g_sprite_editor.onion_skin.frames_behind = 1;
    g_sprite_editor.onion_skin.frames_ahead = 0;
    g_sprite_editor.onion_skin.show_behind = true;
    g_sprite_editor.onion_skin.show_ahead = false;
    
    g_sprite_editor.animation.playback_speed = 1.0f;
    g_sprite_editor.animation.looping = true;
    
    printf("Sprite Editor initialized\n");
    return true;
}

void sprite_editor_cleanup(void) {
    // Free texture data
    if (g_sprite_editor.texture_data) {
        core_free(g_sprite_editor.texture_data);
        g_sprite_editor.texture_data = NULL;
    }
    
    // Free selection mask
    if (g_sprite_editor.selection.selection_mask) {
        core_free(g_sprite_editor.selection.selection_mask);
        g_sprite_editor.selection.selection_mask = NULL;
    }
    
    // Free undo history
    if (g_sprite_editor.undo_system.history) {
        for (uint32_t i = 0; i < g_sprite_editor.undo_system.history_count; i++) {
            if (g_sprite_editor.undo_system.history[i]) {
                core_free(g_sprite_editor.undo_system.history[i]);
            }
        }
        core_free(g_sprite_editor.undo_system.history);
        g_sprite_editor.undo_system.history = NULL;
    }
    
    memset(&g_sprite_editor, 0, sizeof(SpriteEditor));
    printf("Sprite Editor cleaned up\n");
}

void sprite_editor_set_sprite(Sprite2D* sprite) {
    g_sprite_editor.current_sprite = sprite;
    
    if (sprite) {
        // Create or resize texture data
        int new_width = (int)sprite->size.x;
        int new_height = (int)sprite->size.y;
        
        if (g_sprite_editor.texture_width != new_width || g_sprite_editor.texture_height != new_height) {
            // Free old texture data
            if (g_sprite_editor.texture_data) {
                core_free(g_sprite_editor.texture_data);
            }
            
            // Allocate new texture data
            size_t texture_size = new_width * new_height * sizeof(uint32_t);
            g_sprite_editor.texture_data = core_alloc(texture_size);
            
            if (g_sprite_editor.texture_data) {
                g_sprite_editor.texture_width = new_width;
                g_sprite_editor.texture_height = new_height;
                
                // Initialize with transparent black
                memset(g_sprite_editor.texture_data, 0, texture_size);
                
                // Initialize undo system
                g_sprite_editor.undo_system.texture_size = texture_size;
                sprite_editor_save_to_history(&g_sprite_editor);
            }
        }
        
        // Load existing texture data if available
        if (sprite->texture_id != 0) {
            // Load texture from GPU
            renderer_get_texture_data(sprite->texture_id, g_sprite_editor.texture_data, 
                                     g_sprite_editor.texture_width, g_sprite_editor.texture_height);
        }
        
        // Update animation state
        g_sprite_editor.animation.current_frame = sprite->current_frame;
        g_sprite_editor.animation.frame_time = sprite->frame_time;
        
        // Allocate selection mask
        size_t mask_size = new_width * new_height * sizeof(uint32_t);
        if (g_sprite_editor.selection.selection_mask) {
            core_free(g_sprite_editor.selection.selection_mask);
        }
        g_sprite_editor.selection.selection_mask = core_alloc(mask_size);
        memset(g_sprite_editor.selection.selection_mask, 0, mask_size);
        
    } else {
        // Clear sprite
        if (g_sprite_editor.texture_data) {
            core_free(g_sprite_editor.texture_data);
            g_sprite_editor.texture_data = NULL;
        }
        g_sprite_editor.texture_width = 0;
        g_sprite_editor.texture_height = 0;
        
        if (g_sprite_editor.selection.selection_mask) {
            core_free(g_sprite_editor.selection.selection_mask);
            g_sprite_editor.selection.selection_mask = NULL;
        }
    }
    
    // Clear selection
    g_sprite_editor.selection.has_selection = false;
    g_sprite_editor.selection.selecting = false;
}

void sprite_editor_update(float dt) {
    if (!g_sprite_editor.current_sprite) {
        return;
    }
    
    // Update animation playback
    if (g_sprite_editor.animation.playing) {
        g_sprite_editor.animation.frame_time += dt * g_sprite_editor.animation.playback_speed;
        
        if (g_sprite_editor.animation.frame_time >= g_sprite_editor.current_sprite->frame_time) {
            g_sprite_editor.animation.frame_time = 0.0f;
            g_sprite_editor.animation.current_frame++;
            
            if (g_sprite_editor.animation.current_frame >= g_sprite_editor.current_sprite->frame_count) {
                if (g_sprite_editor.animation.looping) {
                    g_sprite_editor.animation.current_frame = 0;
                } else {
                    g_sprite_editor.animation.current_frame = g_sprite_editor.current_sprite->frame_count - 1;
                    g_sprite_editor.animation.playing = false;
                }
            }
            
            g_sprite_editor.current_sprite->current_frame = g_sprite_editor.animation.current_frame;
        }
    }
    
    // Update texture if pixels were edited
    if (g_sprite_editor.editing_pixels) {
        sprite_editor_update_texture(&g_sprite_editor);
        g_sprite_editor.editing_pixels = false;
    }
}

void sprite_editor_render(void) {
    if (!g_sprite_editor.current_sprite || !g_sprite_editor.texture_data) {
        return;
    }
    
    // Set up rendering for sprite editor
    renderer_set_2d_mode();
    
    // Render background
    renderer_clear_color((vec4){0.2f, 0.2f, 0.2f, 1.0f});
    
    // Render transparency checkerboard if enabled
    if (g_sprite_editor.show_transparency) {
        sprite_editor_render_transparency_background();
    }
    
    // Render onion skin frames if enabled
    if (g_sprite_editor.show_onion_skin) {
        sprite_editor_render_onion_skin();
    }
    
    // Render main sprite
    sprite_editor_render_sprite();
    
    // Render grid if enabled
    if (g_sprite_editor.show_grid) {
        sprite_editor_render_grid();
    }
    
    // Render selection if active
    if (g_sprite_editor.selection.has_selection) {
        sprite_editor_render_selection();
    }
    
    // Render current tool overlay
    sprite_editor_render_tool_overlay();
    
    // Render UI elements
    sprite_editor_render_ui();
}

void sprite_editor_handle_input(const AssetEditor* editor) {
    if (!g_sprite_editor.current_sprite || !g_sprite_editor.texture_data) {
        return;
    }
    
    vec2 mouse_pos = editor->mouse_position;
    vec2 world_pos = sprite_editor_screen_to_world(mouse_pos);
    
    // Handle mouse input based on current tool
    if (editor->mouse_left_pressed) {
        switch (g_sprite_editor.drawing_tool.tool_type) {
            case SPRITE_TOOL_BRUSH:
            case SPRITE_TOOL_ERASER:
                sprite_editor_apply_brush(&g_sprite_editor, world_pos);
                g_sprite_editor.editing_pixels = true;
                break;
            case SPRITE_TOOL_FILL:
                if (!g_sprite_editor.selection.selecting) {
                    sprite_editor_fill_area(&g_sprite_editor, world_pos, g_sprite_editor.drawing_tool.brush_color);
                    g_sprite_editor.editing_pixels = true;
                    sprite_editor_save_to_history(&g_sprite_editor);
                }
                break;
            case SPRITE_TOOL_PICKER:
                sprite_editor_pick_color(world_pos);
                break;
            case SPRITE_TOOL_LINE:
            case SPRITE_TOOL_RECT:
            case SPRITE_TOOL_CIRCLE:
                if (!g_sprite_editor.selection.selecting) {
                    g_sprite_editor.selection.selection_start = world_pos;
                    g_sprite_editor.selection.selecting = true;
                }
                break;
        }
    } else {
        // Mouse released
        if (g_sprite_editor.selection.selecting) {
            g_sprite_editor.selection.selection_end = world_pos;
            g_sprite_editor.selection.selecting = false;
            g_sprite_editor.selection.has_selection = true;
            
            // Apply shape tools
            switch (g_sprite_editor.drawing_tool.tool_type) {
                case SPRITE_TOOL_LINE:
                    sprite_editor_draw_line(g_sprite_editor.selection.selection_start, 
                                         g_sprite_editor.selection.selection_end);
                    break;
                case SPRITE_TOOL_RECT:
                    sprite_editor_draw_rectangle(g_sprite_editor.selection.selection_start, 
                                              g_sprite_editor.selection.selection_end);
                    break;
                case SPRITE_TOOL_CIRCLE:
                    sprite_editor_draw_circle(g_sprite_editor.selection.selection_start, 
                                           g_sprite_editor.selection.selection_end);
                    break;
            }
            
            g_sprite_editor.editing_pixels = true;
            sprite_editor_save_to_history(&g_sprite_editor);
        }
    }
    
    // Handle keyboard shortcuts
    if (editor->ctrl_pressed) {
        // Ctrl+Z: Undo
        if (editor->mouse_left_pressed) {
            sprite_editor_undo();
        }
        // Ctrl+Y: Redo
        if (editor->mouse_right_pressed) {
            sprite_editor_redo();
        }
        // Ctrl+S: Save
        // (This would be handled at a higher level)
    }
    
    // Handle zoom with mouse wheel
    if (editor->mouse_delta.y != 0.0f) {
        float zoom_speed = 0.1f;
        g_sprite_editor.zoom *= (1.0f + editor->mouse_delta.y * zoom_speed);
        g_sprite_editor.zoom = fmaxf(0.1f, fminf(32.0f, g_sprite_editor.zoom));
    }
    
    // Handle panning with middle mouse
    if (editor->mouse_middle_pressed) {
        float pan_speed = 1.0f / g_sprite_editor.zoom;
        g_sprite_editor.pan_offset.x += editor->mouse_delta.x * pan_speed;
        g_sprite_editor.pan_offset.y += editor->mouse_delta.y * pan_speed;
    }
}

void sprite_editor_set_brush_color(const vec4 color) {
    g_sprite_editor.drawing_tool.brush_color = color;
}

void sprite_editor_set_brush_size(float size) {
    g_sprite_editor.drawing_tool.brush_size = fmaxf(1.0f, size);
}

void sprite_editor_set_tool(int tool_type) {
    g_sprite_editor.drawing_tool.tool_type = tool_type;
}

void sprite_editor_undo(void) {
    if (g_sprite_editor.undo_system.history_index > 0) {
        g_sprite_editor.undo_system.history_index--;
        
        uint32_t* current_state = g_sprite_editor.undo_system.history[g_sprite_editor.undo_system.history_index];
        if (current_state) {
            memcpy(g_sprite_editor.texture_data, current_state, g_sprite_editor.undo_system.texture_size);
            g_sprite_editor.editing_pixels = true;
        }
    }
}

void sprite_editor_redo(void) {
    if (g_sprite_editor.undo_system.history_index < g_sprite_editor.undo_system.history_count - 1) {
        g_sprite_editor.undo_system.history_index++;
        
        uint32_t* current_state = g_sprite_editor.undo_system.history[g_sprite_editor.undo_system.history_index];
        if (current_state) {
            memcpy(g_sprite_editor.texture_data, current_state, g_sprite_editor.undo_system.texture_size);
            g_sprite_editor.editing_pixels = true;
        }
    }
}

void sprite_editor_play_animation(void) {
    g_sprite_editor.animation.playing = true;
    g_sprite_editor.animation.frame_time = 0.0f;
}

void sprite_editor_pause_animation(void) {
    g_sprite_editor.animation.playing = false;
}

void sprite_editor_stop_animation(void) {
    g_sprite_editor.animation.playing = false;
    g_sprite_editor.animation.current_frame = 0;
    g_sprite_editor.animation.frame_time = 0.0f;
    g_sprite_editor.current_sprite->current_frame = 0;
}

void sprite_editor_add_frame(void) {
    if (!g_sprite_editor.current_sprite) return;
    
    // Add new frame at current position
    uint32_t new_frame_count = g_sprite_editor.current_sprite->frame_count + 1;
    
    // Resize sprite sheet if needed
    // This is a simplified implementation
    g_sprite_editor.current_sprite->frame_count = new_frame_count;
    
    // Copy current frame data to new frame
    // (Implementation would depend on sprite sheet layout)
    
    // Switch to new frame
    g_sprite_editor.animation.current_frame = new_frame_count - 1;
    g_sprite_editor.current_sprite->current_frame = g_sprite_editor.animation.current_frame;
}

void sprite_editor_delete_frame(void) {
    if (!g_sprite_editor.current_sprite || g_sprite_editor.current_sprite->frame_count <= 1) {
        return;
    }
    
    // Delete current frame
    uint32_t new_frame_count = g_sprite_editor.current_sprite->frame_count - 1;
    
    // Shift frames after current one
    // (Implementation would depend on sprite sheet layout)
    
    g_sprite_editor.current_sprite->frame_count = new_frame_count;
    
    // Adjust current frame if necessary
    if (g_sprite_editor.animation.current_frame >= new_frame_count) {
        g_sprite_editor.animation.current_frame = new_frame_count - 1;
        g_sprite_editor.current_sprite->current_frame = g_sprite_editor.animation.current_frame;
    }
}

// Static helper functions
static void sprite_editor_update_texture(SpriteEditor* editor) {
    if (!editor->current_sprite || !editor->texture_data) {
        return;
    }
    
    // Update GPU texture with edited pixel data
    if (editor->current_sprite->texture_id == 0) {
        // Create new texture
        editor->current_sprite->texture_id = renderer_create_texture(
            editor->texture_data, editor->texture_width, editor->texture_height);
    } else {
        // Update existing texture
        renderer_update_texture(editor->current_sprite->texture_id, 
                               editor->texture_data, editor->texture_width, editor->texture_height);
    }
}

static void sprite_editor_apply_brush(SpriteEditor* editor, const vec2 position) {
    if (!editor->texture_data) return;
    
    int brush_radius = (int)ceilf(editor->drawing_tool.brush_size);
    vec4 brush_color = editor->drawing_tool.brush_color;
    float opacity = editor->drawing_tool.brush_opacity;
    
    if (editor->drawing_tool.tool_type == SPRITE_TOOL_ERASER) {
        brush_color = (vec4){0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    for (int y = -brush_radius; y <= brush_radius; y++) {
        for (int x = -brush_radius; x <= brush_radius; x++) {
            vec2 pixel_pos = vec2_add(position, (vec2){(float)x, (float)y});
            
            // Check if pixel is within brush circle
            float distance = sqrtf(x * x + y * y);
            if (distance <= editor->drawing_tool.brush_size) {
                // Calculate falloff
                float falloff = 1.0f;
                if (editor->drawing_tool.anti_aliasing) {
                    falloff = 1.0f - (distance / editor->drawing_tool.brush_size);
                    falloff = falloff * falloff; // Quadratic falloff
                }
                
                // Apply brush to pixel
                int px = (int)pixel_pos.x;
                int py = (int)pixel_pos.y;
                
                if (px >= 0 && px < editor->texture_width && py >= 0 && py < editor->texture_height) {
                    uint32_t index = py * editor->texture_width + px;
                    uint32_t base_color = editor->texture_data[index];
                    
                    // Convert colors to uint32 format
                    uint32_t brush_color_uint = ((uint32_t)(brush_color.a * 255.0f) << 24) |
                                               ((uint32_t)(brush_color.b * 255.0f) << 16) |
                                               ((uint32_t)(brush_color.g * 255.0f) << 8) |
                                               ((uint32_t)(brush_color.r * 255.0f));
                    
                    // Blend colors
                    float final_opacity = opacity * falloff;
                    editor->texture_data[index] = sprite_editor_blend_colors(
                        base_color, brush_color_uint, final_opacity);
                }
            }
        }
    }
}

static void sprite_editor_fill_area(SpriteEditor* editor, const vec2 start_pos, const vec4 fill_color) {
    if (!editor->texture_data) return;
    
    int start_x = (int)start_pos.x;
    int start_y = (int)start_pos.y;
    
    if (start_x < 0 || start_x >= editor->texture_width || 
        start_y < 0 || start_y >= editor->texture_height) {
        return;
    }
    
    // Get target color at start position
    uint32_t target_color = editor->texture_data[start_y * editor->texture_width + start_x];
    
    // Convert fill color to uint32
    uint32_t fill_color_uint = ((uint32_t)(fill_color.a * 255.0f) << 24) |
                               ((uint32_t)(fill_color.b * 255.0f) << 16) |
                               ((uint32_t)(fill_color.g * 255.0f) << 8) |
                               ((uint32_t)(fill_color.r * 255.0f));
    
    // If target color is same as fill color, nothing to do
    if (target_color == fill_color_uint) return;
    
    // Flood fill algorithm (queue-based)
    int* queue = core_alloc(editor->texture_width * editor->texture_height * 2 * sizeof(int));
    int queue_start = 0;
    int queue_end = 0;
    
    // Add starting position to queue
    queue[queue_end++] = start_x;
    queue[queue_end++] = start_y;
    
    // Mark as visited by setting to fill color
    editor->texture_data[start_y * editor->texture_width + start_x] = fill_color_uint;
    
    while (queue_start < queue_end) {
        int x = queue[queue_start++];
        int y = queue[queue_start++];
        
        // Check 4 neighboring pixels
        int neighbors[4][2] = {{x+1, y}, {x-1, y}, {x, y+1}, {x, y-1}};
        
        for (int i = 0; i < 4; i++) {
            int nx = neighbors[i][0];
            int ny = neighbors[i][1];
            
            if (nx >= 0 && nx < editor->texture_width && ny >= 0 && ny < editor->texture_height) {
                uint32_t neighbor_color = editor->texture_data[ny * editor->texture_width + nx];
                
                if (neighbor_color == target_color) {
                    // Fill this pixel and add to queue
                    editor->texture_data[ny * editor->texture_width + nx] = fill_color_uint;
                    queue[queue_end++] = nx;
                    queue[queue_end++] = ny;
                }
            }
        }
    }
    
    core_free(queue);
}

static void sprite_editor_save_to_history(SpriteEditor* editor) {
    if (!editor->texture_data) return;
    
    // Remove any states after current index
    for (uint32_t i = editor->undo_system.history_index + 1; i < editor->undo_system.history_count; i++) {
        if (editor->undo_system.history[i]) {
            core_free(editor->undo_system.history[i]);
        }
    }
    
    // Resize history array if needed
    if (editor->undo_system.history_index >= editor->undo_system.history_capacity) {
        uint32_t new_capacity = editor->undo_system.history_capacity * 2;
        if (new_capacity == 0) new_capacity = 16;
        
        uint32_t** new_history = core_realloc(editor->undo_system.history, 
                                              new_capacity * sizeof(uint32_t*));
        if (new_history) {
            editor->undo_system.history = new_history;
            editor->undo_system.history_capacity = new_capacity;
        }
    }
    
    // Add current state to history
    if (editor->undo_system.history_index < editor->undo_system.history_capacity) {
        uint32_t* state_copy = core_alloc(editor->undo_system.texture_size);
        if (state_copy) {
            memcpy(state_copy, editor->texture_data, editor->undo_system.texture_size);
            editor->undo_system.history[editor->undo_system.history_index] = state_copy;
            editor->undo_system.history_index++;
            editor->undo_system.history_count = editor->undo_system.history_index;
        }
    }
}

static uint32_t sprite_editor_blend_colors(uint32_t base, uint32_t overlay, float opacity) {
    // Extract color components
    float base_a = ((base >> 24) & 0xFF) / 255.0f;
    float base_r = ((base >> 0) & 0xFF) / 255.0f;
    float base_g = ((base >> 8) & 0xFF) / 255.0f;
    float base_b = ((base >> 16) & 0xFF) / 255.0f;
    
    float overlay_a = ((overlay >> 24) & 0xFF) / 255.0f;
    float overlay_r = ((overlay >> 0) & 0xFF) / 255.0f;
    float overlay_g = ((overlay >> 8) & 0xFF) / 255.0f;
    float overlay_b = ((overlay >> 16) & 0xFF) / 255.0f;
    
    // Alpha blending
    float final_a = overlay_a * opacity + base_a * (1.0f - overlay_a * opacity);
    float final_r = (overlay_r * overlay_a * opacity + base_r * base_a * (1.0f - overlay_a * opacity)) / final_a;
    float final_g = (overlay_g * overlay_a * opacity + base_g * base_a * (1.0f - overlay_a * opacity)) / final_a;
    float final_b = (overlay_b * overlay_a * opacity + base_b * base_a * (1.0f - overlay_a * opacity)) / final_a;
    
    // Convert back to uint32
    return ((uint32_t)(final_a * 255.0f) << 24) |
           ((uint32_t)(final_b * 255.0f) << 16) |
           ((uint32_t)(final_g * 255.0f) << 8) |
           ((uint32_t)(final_r * 255.0f));
}

// Render helper functions
void sprite_editor_render_transparency_background(void) {
    // Render checkerboard pattern for transparency
}

void sprite_editor_render_onion_skin(void) {
    // Render onion skin frames
}

void sprite_editor_render_sprite(void) {
    // Render the main sprite texture
}

void sprite_editor_render_grid(void) {
    // Render pixel grid
}

void sprite_editor_render_selection(void) {
    // Render selection outline
}

void sprite_editor_render_tool_overlay(void) {
    // Render current tool preview (brush cursor, etc.)
}

void sprite_editor_render_ui(void) {
    // Render UI elements (timeline, color picker, etc.)
}

// Utility functions
vec2 sprite_editor_screen_to_world(const vec2 screen_pos) {
    // Convert screen coordinates to world/pixel coordinates
    vec2 world_pos = vec2_sub(screen_pos, g_sprite_editor.pan_offset);
    world_pos = vec2_div(world_pos, (vec2){g_sprite_editor.zoom, g_sprite_editor.zoom});
    return world_pos;
}

vec2 sprite_editor_world_to_screen(const vec2 world_pos) {
    // Convert world/pixel coordinates to screen coordinates
    vec2 screen_pos = vec2_mul(world_pos, (vec2){g_sprite_editor.zoom, g_sprite_editor.zoom});
    screen_pos = vec2_add(screen_pos, g_sprite_editor.pan_offset);
    return screen_pos;
}

void sprite_editor_pick_color(const vec2 position) {
    if (!g_sprite_editor.texture_data) return;
    
    int px = (int)position.x;
    int py = (int)position.y;
    
    if (px >= 0 && px < g_sprite_editor.texture_width && py >= 0 && py < g_sprite_editor.texture_height) {
        uint32_t pixel = g_sprite_editor.texture_data[py * g_sprite_editor.texture_width + px];
        
        // Extract color components
        float a = ((pixel >> 24) & 0xFF) / 255.0f;
        float r = ((pixel >> 0) & 0xFF) / 255.0f;
        float g = ((pixel >> 8) & 0xFF) / 255.0f;
        float b = ((pixel >> 16) & 0xFF) / 255.0f;
        
        g_sprite_editor.drawing_tool.brush_color = (vec4){r, g, b, a};
    }
}

void sprite_editor_draw_line(const vec2 start, const vec2 end) {
    // Bresenham's line algorithm
    // Implementation would draw a line between start and end points
}

void sprite_editor_draw_rectangle(const vec2 start, const vec2 end) {
    // Draw filled rectangle
    // Implementation would fill rectangle between start and end points
}

void sprite_editor_draw_circle(const vec2 start, const vec2 end) {
    // Draw filled circle
    // Implementation would calculate radius and draw circle
}
