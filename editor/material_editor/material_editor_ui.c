#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>
#include <math.h>

/**
 * =================================================================================================
 *                           MATERIAL EDITOR UI
 * =================================================================================================
 * 
 * Implements the visual representation and interaction logic for the Material Node Graph.
 * 
 * DESIGN:
 * Since we don't have a direct ImGui dependency here, we generate "Draw Commands" 
 * that the engine's renderer can interpret.
 */

// UI Constants
#define NODE_WIDTH 150.0f
#define NODE_HEADER_HEIGHT 30.0f
#define SOCKET_RADIUS 6.0f
#define SOCKET_SPACING 20.0f
#define GRID_SIZE 20.0f

// Interaction State
static struct {
    bool is_dragging_node;
    u32 dragged_node_id;
    Vec2 drag_offset;
    
    bool is_dragging_connection;
    u32 connection_source_node;
    u32 connection_source_output;
    Vec2 current_mouse_pos;
    
    Vec2 pan_offset;
    f32 zoom_level;
} ui_state = {0, 0, {0,0}, 0, 0, 0, {0,0}, {0,0}, 1.0f};

// Forward declaration of internal rendering helpers
// These would populate a render list in a real implementation
static void draw_rect(Vec2 pos, Vec2 size, Vec4 color);
static void draw_text(Vec2 pos, const char* text, Vec4 color);
static void draw_bezier(Vec2 start, Vec2 end, Vec4 color, f32 thickness);
static void draw_circle(Vec2 center, f32 radius, Vec4 color);

void material_editor_ui_init(void) {
    ui_state.zoom_level = 1.0f;
    ui_state.pan_offset = (Vec2){0, 0};
    LOG_INFO("Material Editor UI Initialized");
}

void material_editor_ui_update_interaction(EditorContext* ctx, MaterialGraph* graph, 
                                         i32 mouse_x, i32 mouse_y, 
                                         bool mouse_down, bool mouse_right_down) {
    // Convert mouse to graph space
    Vec2 mouse_world = {
        ((f32)mouse_x - ui_state.pan_offset.x) / ui_state.zoom_level,
        ((f32)mouse_y - ui_state.pan_offset.y) / ui_state.zoom_level
    };
    
    ui_state.current_mouse_pos = mouse_world;

    // Handle Node Dragging
    if (mouse_down) {
        if (!ui_state.is_dragging_node) {
            // Hit test nodes
            for (u32 i = 0; i < graph->node_count; i++) {
                MaterialNode* node = &graph->nodes[i];
                // Simple box collision
                if (mouse_world.x >= node->ui_position.x && 
                    mouse_world.x <= node->ui_position.x + NODE_WIDTH &&
                    mouse_world.y >= node->ui_position.y &&
                    mouse_world.y <= node->ui_position.y + NODE_HEADER_HEIGHT) { // Drag header only
                    
                    ui_state.is_dragging_node = true;
                    ui_state.dragged_node_id = node->node_id;
                    ui_state.drag_offset = (Vec2){
                        mouse_world.x - node->ui_position.x,
                        mouse_world.y - node->ui_position.y
                    };
                    break;
                }
            }
        } else {
            // Update node position
            for (u32 i = 0; i < graph->node_count; i++) {
                if (graph->nodes[i].node_id == ui_state.dragged_node_id) {
                    graph->nodes[i].ui_position.x = mouse_world.x - ui_state.drag_offset.x;
                    graph->nodes[i].ui_position.y = mouse_world.y - ui_state.drag_offset.y;
                    break;
                }
            }
        }
    } else {
        ui_state.is_dragging_node = false;
        ui_state.is_dragging_connection = false;
    }

    // Panning (Right Mouse)
    static Vec2 last_mouse = {0,0};
    if (mouse_right_down) {
        // ui_state.pan_offset.x += mouse_x - last_mouse.x; // Basic logic, needs frame delta or state
    }
    last_mouse = (Vec2){(f32)mouse_x, (f32)mouse_y};
}

void material_editor_ui_render(EditorContext* ctx, MaterialGraph* graph) {
    if (!graph) return;

    // 1. Draw Grid Background
    // In a real APP we'd loop specific visible range
    // draw_rect((Vec2){0,0}, (Vec2){1920, 1080}, (Vec4){0.2f, 0.2f, 0.2f, 1.0f});

    // 2. Draw Connections (Wires)
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* dest_node = &graph->nodes[i];
        for (u32 j = 0; j < dest_node->input_count; j++) {
            if (dest_node->inputs[j].connected_node) {
                MaterialNode* src_node = dest_node->inputs[j].connected_node;
                u32 src_idx = dest_node->inputs[j].connected_output_index;

                // Calculate socket positions
                Vec2 start_pos = {
                    src_node->ui_position.x + NODE_WIDTH,
                    src_node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (src_idx * SOCKET_SPACING)
                };
                
                Vec2 end_pos = {
                    dest_node->ui_position.x,
                    dest_node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)
                };

                draw_bezier(start_pos, end_pos, (Vec4){0.8f, 0.8f, 0.8f, 1.0f}, 2.0f);
            }
        }
    }

    // 3. Draw Nodes
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* node = &graph->nodes[i];
        
        // Calculate height
        f32 height = NODE_HEADER_HEIGHT + 
                    (fmaxf(node->input_count, node->output_count) * SOCKET_SPACING) + 
                    10.0f;

        // Node Body
        draw_rect(node->ui_position, (Vec2){NODE_WIDTH, height}, (Vec4){0.3f, 0.3f, 0.3f, 1.0f});
        
        // Header
        draw_rect(node->ui_position, (Vec2){NODE_WIDTH, NODE_HEADER_HEIGHT}, (Vec4){0.4f, 0.4f, 0.5f, 1.0f});
        draw_text((Vec2){node->ui_position.x + 5, node->ui_position.y + 5}, node->name, (Vec4){1,1,1,1});

        // Inputs
        for (u32 j = 0; j < node->input_count; j++) {
            Vec2 socket_pos = {
                node->ui_position.x,
                node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)
            };
            draw_circle(socket_pos, SOCKET_RADIUS, (Vec4){0.8f, 0.2f, 0.2f, 1.0f});
            draw_text((Vec2){socket_pos.x + 10, socket_pos.y - 5}, node->inputs[j].name, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        }

        // Outputs
        for (u32 j = 0; j < node->output_count; j++) {
            Vec2 socket_pos = {
                node->ui_position.x + NODE_WIDTH,
                node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)
            };
            draw_circle(socket_pos, SOCKET_RADIUS, (Vec4){0.2f, 0.8f, 0.2f, 1.0f});
            // Align text right would be better, using simpler offset for now
            draw_text((Vec2){socket_pos.x - 40, socket_pos.y - 5}, node->outputs[j].name, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Internal Mock Rendering Functions
// In a real implementation, these would submit vertices to the renderer or ImGui draw list
// -------------------------------------------------------------------------------------------------

static void draw_rect(Vec2 pos, Vec2 size, Vec4 color) {
    // Placeholder: log rendering command
    // printf("DRAW_RECT: [%f, %f] size [%f, %f]\n", pos.x, pos.y, size.x, size.y);
}

static void draw_text(Vec2 pos, const char* text, Vec4 color) {
    // Placeholder
}

static void draw_bezier(Vec2 start, Vec2 end, Vec4 color, f32 thickness) {
    // Placeholder
}

static void draw_circle(Vec2 center, f32 radius, Vec4 color) {
    // Placeholder
}
