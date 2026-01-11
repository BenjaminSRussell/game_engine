#include "../editor_common.h"
#include "core/logger.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "rendering/vulkan.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265359f
#endif

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
    
    // Connection dragging state
    Vec2 connection_start_pos;
    u32 hovered_node;
    u32 hovered_input;
    u32 hovered_output;
} ui_state = {0, 0, {0,0}, 0, 0, 0, {0,0}, {0,0}, 1.0f, {0,0}, 0, 0, 0};

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
                                         bool mouse_down, bool mouse_right_down,
                                         f32 mouse_wheel_delta) {
    // Convert mouse to graph space
    Vec2 mouse_world = {
        ((f32)mouse_x - ui_state.pan_offset.x) / ui_state.zoom_level,
        ((f32)mouse_y - ui_state.pan_offset.y) / ui_state.zoom_level
    };
    
    ui_state.current_mouse_pos = mouse_world;
    
    // Handle Zoom with Mouse Wheel
    if (fabsf(mouse_wheel_delta) > 0.01f) {
        f32 zoom_speed = 0.1f;
        f32 old_zoom = ui_state.zoom_level;
        
        // Apply zoom
        ui_state.zoom_level *= (1.0f + mouse_wheel_delta * zoom_speed);
        
        // Clamp zoom to reasonable limits
        if (ui_state.zoom_level < 0.1f) ui_state.zoom_level = 0.1f;
        if (ui_state.zoom_level > 5.0f) ui_state.zoom_level = 5.0f;
        
        // Adjust pan offset to zoom towards mouse position
        Vec2 zoom_center = {
            (f32)mouse_x,
            (f32)mouse_y
        };
        
        Vec2 old_world_pos = {
            (zoom_center.x - ui_state.pan_offset.x) / old_zoom,
            (zoom_center.y - ui_state.pan_offset.y) / old_zoom
        };
        
        Vec2 new_world_pos = {
            (zoom_center.x - ui_state.pan_offset.x) / ui_state.zoom_level,
            (zoom_center.y - ui_state.pan_offset.y) / ui_state.zoom_level
        };
        
        Vec2 world_delta = {
            new_world_pos.x - old_world_pos.x,
            new_world_pos.y - old_world_pos.y
        };
        
        ui_state.pan_offset.x += world_delta.x * ui_state.zoom_level;
        ui_state.pan_offset.y += world_delta.y * ui_state.zoom_level;
        
        LOG_DEBUG("Zoom changed: %.2f -> %.2f", old_zoom, ui_state.zoom_level);
    }

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
        ui_state.hovered_node = 0;
        ui_state.hovered_input = 0;
        ui_state.hovered_output = 0;
    }
    
    // Handle connection dragging
    if (mouse_down && !ui_state.is_dragging_node) {
        // Check for socket hit testing
        bool hit_socket = false;
        
        for (u32 i = 0; i < graph->node_count; i++) {
            MaterialNode* node = &graph->nodes[i];
            
            // Check output sockets
            for (u32 j = 0; j < node->output_count; j++) {
                Vec2 socket_pos = {
                    node->ui_position.x + NODE_WIDTH,
                    node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)
                };
                
                f32 dist = sqrtf(powf(mouse_world.x - socket_pos.x, 2.0f) + 
                                powf(mouse_world.y - socket_pos.y, 2.0f));
                
                if (dist <= SOCKET_RADIUS * 2.0f) { // Hit test with larger radius
                    if (!ui_state.is_dragging_connection) {
                        // Start dragging connection from this output
                        ui_state.is_dragging_connection = true;
                        ui_state.connection_source_node = node->node_id;
                        ui_state.connection_source_output = j;
                        ui_state.connection_start_pos = socket_pos;
                        hit_socket = true;
                        
                        // Clear any existing connection from this output
                        for (u32 k = 0; k < graph->node_count; k++) {
                            for (u32 l = 0; l < graph->nodes[k].input_count; l++) {
                                if (graph->nodes[k].inputs[l].connected_node == node &&
                                    graph->nodes[k].inputs[l].connected_output_index == j) {
                                    graph->nodes[k].inputs[l].connected_node = NULL;
                                    graph->nodes[k].inputs[l].connected_output_index = 0;
                                }
                            }
                        }
                    }
                    break;
                }
            }
            
            if (hit_socket) break;
            
            // Check input sockets (only when dragging a connection)
            if (ui_state.is_dragging_connection) {
                for (u32 j = 0; j < node->input_count; j++) {
                    Vec2 socket_pos = {
                        node->ui_position.x,
                        node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)
                    };
                    
                    f32 dist = sqrtf(powf(mouse_world.x - socket_pos.x, 2.0f) + 
                                    powf(mouse_world.y - socket_pos.y, 2.0f));
                    
                    if (dist <= SOCKET_RADIUS * 2.0f) {
                        // Find source node
                        MaterialNode* source_node = NULL;
                        for (u32 k = 0; k < graph->node_count; k++) {
                            if (graph->nodes[k].node_id == ui_state.connection_source_node) {
                                source_node = &graph->nodes[k];
                                break;
                            }
                        }
                        
                        if (source_node) {
                            // Create connection
                            node->inputs[j].connected_node = source_node;
                            node->inputs[j].connected_output_index = ui_state.connection_source_output;
                            
                            // Mark graph for recompilation
                            graph->needs_recompile = true;
                            
                            LOG_INFO("Connected node %u output %u to node %u input %u", 
                                     ui_state.connection_source_node, ui_state.connection_source_output,
                                     node->node_id, j);
                        }
                        
                        // End connection dragging
                        ui_state.is_dragging_connection = false;
                        hit_socket = true;
                        break;
                    }
                }
            }
            
            if (hit_socket) break;
        }
    }

    // Panning (Right Mouse)
    static Vec2 last_mouse = {0,0};
    static bool was_panning = false;
    
    if (mouse_right_down) {
        if (!was_panning) {
            // Start panning - store initial mouse position
            last_mouse = (Vec2){(f32)mouse_x, (f32)mouse_y};
            was_panning = true;
        } else {
            // Continue panning - update pan offset based on mouse movement
            Vec2 current_mouse = {(f32)mouse_x, (f32)mouse_y};
            Vec2 mouse_delta = {
                current_mouse.x - last_mouse.x,
                current_mouse.y - last_mouse.y
            };
            
            // Apply panning with zoom consideration
            ui_state.pan_offset.x += mouse_delta.x;
            ui_state.pan_offset.y += mouse_delta.y;
            
            last_mouse = current_mouse;
        }
    } else {
        was_panning = false;
    }
}

void material_editor_ui_render(EditorContext* ctx, MaterialGraph* graph) {
    if (!graph) return;

    // Apply zoom and pan transformations
    // In a real implementation, this would modify the rendering transform
    // For now, we'll apply transformations to the drawing coordinates
    
    // 1. Draw Grid Background (with zoom and pan applied)
    // In a real APP we'd loop specific visible range
    // draw_rect_transformed((Vec2){0,0}, (Vec2){1920, 1080}, (Vec4){0.2f, 0.2f, 0.2f, 1.0f}, 
    //                      ui_state.pan_offset, ui_state.zoom_level);

    // 2. Draw Connections (Wires) with transformations
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* dest_node = &graph->nodes[i];
        for (u32 j = 0; j < dest_node->input_count; j++) {
            if (dest_node->inputs[j].connected_node) {
                MaterialNode* src_node = dest_node->inputs[j].connected_node;
                u32 src_idx = dest_node->inputs[j].connected_output_index;

                // Calculate socket positions with zoom and pan
                Vec2 start_pos = {
                    (src_node->ui_position.x + NODE_WIDTH) * ui_state.zoom_level + ui_state.pan_offset.x,
                    (src_node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (src_idx * SOCKET_SPACING)) * ui_state.zoom_level + ui_state.pan_offset.y
                };
                
                Vec2 end_pos = {
                    dest_node->ui_position.x * ui_state.zoom_level + ui_state.pan_offset.x,
                    (dest_node->ui_position.y + NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)) * ui_state.zoom_level + ui_state.pan_offset.y
                };

                draw_bezier(start_pos, end_pos, (Vec4){0.8f, 0.8f, 0.8f, 1.0f}, 2.0f * ui_state.zoom_level);
            }
        }
    }
    
    // 2.5. Draw active connection being dragged (with transformations)
    if (ui_state.is_dragging_connection) {
        Vec2 transformed_start = {
            ui_state.connection_start_pos.x * ui_state.zoom_level + ui_state.pan_offset.x,
            ui_state.connection_start_pos.y * ui_state.zoom_level + ui_state.pan_offset.y
        };
        
        Vec2 transformed_mouse = {
            ui_state.current_mouse_pos.x * ui_state.zoom_level + ui_state.pan_offset.x,
            ui_state.current_mouse_pos.y * ui_state.zoom_level + ui_state.pan_offset.y
        };
        
        draw_bezier(transformed_start, transformed_mouse, 
                   (Vec4){1.0f, 1.0f, 0.0f, 1.0f}, 3.0f * ui_state.zoom_level);
    }

    // 3. Draw Nodes (with zoom and pan)
    for (u32 i = 0; i < graph->node_count; i++) {
        MaterialNode* node = &graph->nodes[i];
        
        // Calculate height
        f32 height = NODE_HEADER_HEIGHT + 
                    (fmaxf(node->input_count, node->output_count) * SOCKET_SPACING) + 
                    10.0f;

        // Apply transformations to node position
        Vec2 node_pos = {
            node->ui_position.x * ui_state.zoom_level + ui_state.pan_offset.x,
            node->ui_position.y * ui_state.zoom_level + ui_state.pan_offset.y
        };
        
        Vec2 node_size = {
            NODE_WIDTH * ui_state.zoom_level,
            height * ui_state.zoom_level
        };

        // Node Body
        draw_rect(node_pos, node_size, (Vec4){0.3f, 0.3f, 0.3f, 1.0f});
        
        // Header
        Vec2 header_size = {NODE_WIDTH * ui_state.zoom_level, NODE_HEADER_HEIGHT * ui_state.zoom_level};
        draw_rect(node_pos, header_size, (Vec4){0.4f, 0.4f, 0.5f, 1.0f});
        draw_text((Vec2){node_pos.x + 5.0f * ui_state.zoom_level, node_pos.y + 5.0f * ui_state.zoom_level}, 
                 node->name, (Vec4){1,1,1,1});

        // Inputs
        for (u32 j = 0; j < node->input_count; j++) {
            Vec2 socket_pos = {
                node_pos.x,
                node_pos.y + (NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)) * ui_state.zoom_level
            };
            draw_circle(socket_pos, SOCKET_RADIUS * ui_state.zoom_level, (Vec4){0.8f, 0.2f, 0.2f, 1.0f});
            draw_text((Vec2){socket_pos.x + 10.0f * ui_state.zoom_level, socket_pos.y - 5.0f * ui_state.zoom_level}, 
                     node->inputs[j].name, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        }

        // Outputs
        for (u32 j = 0; j < node->output_count; j++) {
            Vec2 socket_pos = {
                node_pos.x + NODE_WIDTH * ui_state.zoom_level,
                node_pos.y + (NODE_HEADER_HEIGHT + SOCKET_SPACING + (j * SOCKET_SPACING)) * ui_state.zoom_level
            };
            draw_circle(socket_pos, SOCKET_RADIUS * ui_state.zoom_level, (Vec4){0.2f, 0.8f, 0.2f, 1.0f});
            // Align text right would be better, using simpler offset for now
            draw_text((Vec2){socket_pos.x - 40.0f * ui_state.zoom_level, socket_pos.y - 5.0f * ui_state.zoom_level}, 
                     node->outputs[j].name, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Internal Rendering Functions
// These functions submit drawing commands to the renderer
// -------------------------------------------------------------------------------------------------

static void draw_rect(Vec2 pos, Vec2 size, Vec4 color) {
    // In a real implementation, this would submit a rectangle to the renderer
    // For now, we'll log the drawing command and could implement a simple vertex buffer
    
    /*
    // Create rectangle vertices
    f32 vertices[] = {
        // Position        // Color
        pos.x, pos.y,      color.r, color.g, color.b, color.a,
        pos.x + size.x, pos.y, color.r, color.g, color.b, color.a,
        pos.x + size.x, pos.y + size.y, color.r, color.g, color.b, color.a,
        pos.x, pos.y + size.y, color.r, color.g, color.b, color.a
    };
    
    u32 indices[] = {0, 1, 2, 2, 3, 0};
    
    // Submit to renderer
    VulkanRenderer* vk_renderer = (VulkanRenderer*)get_global_renderer();
    if (vk_renderer) {
        // Create temporary vertex buffer
        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_memory;
        vulkan_create_buffer(vk_renderer, sizeof(vertices), 
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &vertex_buffer, &vertex_memory);
        
        // Map and copy vertex data
        void* data;
        vkMapMemory(vk_renderer->device, vertex_memory, 0, sizeof(vertices), 0, &data);
        memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(vk_renderer->device, vertex_memory);
        
        // Create index buffer
        VkBuffer index_buffer;
        VkDeviceMemory index_memory;
        vulkan_create_buffer(vk_renderer, sizeof(indices),
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &index_buffer, &index_memory);
        
        // Map and copy index data
        vkMapMemory(vk_renderer->device, index_memory, 0, sizeof(indices), 0, &data);
        memcpy(data, indices, sizeof(indices));
        vkUnmapMemory(vk_renderer->device, index_memory);
        
        // Record drawing commands
        // This would be done within a command buffer recording scope
        // vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer, &offset);
        // vkCmdBindIndexBuffer(cmd, index_buffer, 0, VK_INDEX_TYPE_UINT32);
        // vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
        
        // Clean up temporary buffers (or cache them)
        vulkan_destroy_buffer(vk_renderer, vertex_buffer, vertex_memory);
        vulkan_destroy_buffer(vk_renderer, index_buffer, index_memory);
    }
    */
    
    // For debugging purposes, log the drawing command
    LOG_DEBUG("DRAW_RECT: pos=(%.1f,%.1f) size=(%.1f,%.1f) color=(%.2f,%.2f,%.2f,%.2f)", 
             pos.x, pos.y, size.x, size.y, color.r, color.g, color.b, color.a);
}

static void draw_text(Vec2 pos, const char* text, Vec4 color) {
    // In a real implementation, this would use a font rendering system
    // This could be implemented with:
    // 1. SDF font rendering in a shader
    // 2. Bitmap font atlas
    // 3. System font rendering (stb_truetype)
    
    /*
    // Example with bitmap font atlas:
    FontAtlas* font = get_editor_font();
    if (!font || !text) return;
    
    f32 cursor_x = pos.x;
    f32 cursor_y = pos.y;
    
    for (const char* c = text; *c; ++c) {
        Glyph* glyph = font_get_glyph(font, *c);
        if (!glyph) continue;
        
        Vec2 glyph_pos = {cursor_x + glyph->bearing_x, cursor_y - glyph->bearing_y};
        Vec2 glyph_size = {glyph->width, glyph->height};
        
        // Draw glyph as textured rectangle
        draw_textured_rect(glyph_pos, glyph_size, glyph->uv_offset, glyph->uv_size, color);
        
        cursor_x += glyph->advance_x;
    }
    */
    
    // For debugging, just log the text
    LOG_DEBUG("DRAW_TEXT: pos=(%.1f,%.1f) text='%s' color=(%.2f,%.2f,%.2f,%.2f)", 
             pos.x, pos.y, text, color.r, color.g, color.b, color.a);
}

static void draw_bezier(Vec2 start, Vec2 end, Vec4 color, f32 thickness) {
    // In a real implementation, this would draw a bezier curve between nodes
    // This could be implemented by:
    // 1. Sampling the bezier curve at multiple points
    // 2. Creating line segments between samples
    // 3. Rendering as thick lines or triangles
    
    /*
    const u32 segments = 20;
    f32 vertices[segments * 4]; // pos + color for each segment endpoint
    u32 indices[(segments - 1) * 2]; // line indices
    
    // Sample bezier curve
    for (u32 i = 0; i < segments; ++i) {
        f32 t = (f32)i / (f32)(segments - 1);
        
        // Cubic bezier with control points
        f32 one_minus_t = 1.0f - t;
        f32 one_minus_t_sq = one_minus_t * one_minus_t;
        f32 one_minus_t_cu = one_minus_t_sq * one_minus_t;
        f32 t_sq = t * t;
        f32 t_cu = t_sq * t;
        
        // Control points for nice curve
        Vec2 cp1 = {start.x + 100.0f, start.y};
        Vec2 cp2 = {end.x - 100.0f, end.y};
        
        Vec2 point = {
            one_minus_t_cu * start.x + 3.0f * one_minus_t_sq * t * cp1.x + 
            3.0f * one_minus_t * t_sq * cp2.x + t_cu * end.x,
            one_minus_t_cu * start.y + 3.0f * one_minus_t_sq * t * cp1.y + 
            3.0f * one_minus_t * t_sq * cp2.y + t_cu * end.y
        };
        
        u32 vertex_index = i * 4;
        vertices[vertex_index + 0] = point.x;
        vertices[vertex_index + 1] = point.y;
        vertices[vertex_index + 2] = thickness; // Use as thickness data
        vertices[vertex_index + 3] = 0.0f; // Padding
        
        if (i < segments - 1) {
            u32 index_index = i * 2;
            indices[index_index + 0] = i;
            indices[index_index + 1] = i + 1;
        }
    }
    
    // Submit as line rendering to GPU
    */
    
    LOG_DEBUG("DRAW_BEZIER: start=(%.1f,%.1f) end=(%.1f,%.1f) thickness=%.1f color=(%.2f,%.2f,%.2f,%.2f)", 
             start.x, start.y, end.x, end.y, thickness, color.r, color.g, color.b, color.a);
}

static void draw_circle(Vec2 center, f32 radius, Vec4 color) {
    // In a real implementation, this would draw a filled or outlined circle
    // This could be implemented by:
    // 1. Creating a triangle fan for filled circle
    // 2. Creating line segments for outlined circle
    // 3. Using a shader that renders circles procedurally
    
    /*
    const u32 segments = 32;
    const u32 vertex_count = segments + 2; // Center + segments + closing point
    const u32 index_count = segments * 3; // Triangles for triangle fan
    
    f32 vertices[vertex_count * 6]; // pos + color for each vertex
    u32 indices[index_count];
    
    // Center vertex
    vertices[0] = center.x;
    vertices[1] = center.y;
    vertices[2] = color.r;
    vertices[3] = color.g;
    vertices[4] = color.b;
    vertices[5] = color.a;
    
    // Circle vertices
    for (u32 i = 0; i <= segments; ++i) {
        f32 angle = (f32)i / (f32)segments * 2.0f * PI;
        Vec2 point = {
            center.x + cosf(angle) * radius,
            center.y + sinf(angle) * radius
        };
        
        u32 vertex_index = (i + 1) * 6;
        vertices[vertex_index + 0] = point.x;
        vertices[vertex_index + 1] = point.y;
        vertices[vertex_index + 2] = color.r;
        vertices[vertex_index + 3] = color.g;
        vertices[vertex_index + 4] = color.b;
        vertices[vertex_index + 5] = color.a;
        
        if (i < segments) {
            u32 index_index = i * 3;
            indices[index_index + 0] = 0; // Center
            indices[index_index + 1] = i + 1; // Current point
            indices[index_index + 2] = i + 2; // Next point
        }
    }
    
    // Submit to renderer as triangle fan
    */
    
    LOG_DEBUG("DRAW_CIRCLE: center=(%.1f,%.1f) radius=%.1f color=(%.2f,%.2f,%.2f,%.2f)", 
             center.x, center.y, radius, color.r, color.g, color.b, color.a);
}
