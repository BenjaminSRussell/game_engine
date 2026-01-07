#include "tools/asset_editor/editor_types.h"
#include "tools/asset_editor/editor_tools.h"
#include "tools/asset_editor/asset_editor.h"
#include <rendering/renderer.h>
#include "core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Model Editor State
typedef struct {
    Model3D* current_model;
    Mesh* editing_mesh;
    Vertex* vertex_buffer;
    uint32_t* index_buffer;
    
    // Editing state
    bool editing_vertices;
    bool editing_edges;
    bool editing_faces;
    
    // Selection
    Selection vertex_selection;
    Selection edge_selection;
    Selection face_selection;
    
    // Tool-specific data
    struct {
        vec3 extrude_direction;
        float extrude_amount;
        bool extrude_individual;
    } extrude_tool;
    
    struct {
        vec3 sculpt_center;
        float sculpt_radius;
        float sculpt_strength;
        bool sculpting;
    } sculpt_tool;
    
    struct {
        vec3 paint_position;
        vec4 paint_color;
        float brush_size;
        bool painting;
    } paint_tool;
    
    // Preview settings
    bool show_wireframe;
    bool show_normals;
    bool show_bounding_box;
    bool show_uv_overlay;
    
    // Grid snapping
    bool snap_to_grid;
    float grid_size;
    
} ModelEditor;

static ModelEditor g_model_editor = {0};

// Forward declarations
static void model_editor_update_selection(ModelEditor* editor);
static void model_editor_update_vertex_positions(ModelEditor* editor);
static void model_editor_calculate_normals(ModelEditor* editor);
static void model_editor_calculate_bounds(ModelEditor* editor);

bool model_editor_init(void) {
    memset(&g_model_editor, 0, sizeof(ModelEditor));
    
    // Initialize selection systems
    g_model_editor.vertex_selection.type = SELECTION_TYPE_VERTEX;
    g_model_editor.edge_selection.type = SELECTION_TYPE_EDGE;
    g_model_editor.face_selection.type = SELECTION_TYPE_FACE;
    
    // Set default values
    g_model_editor.show_wireframe = false;
    g_model_editor.show_normals = false;
    g_model_editor.show_bounding_box = true;
    g_model_editor.show_uv_overlay = false;
    
    g_model_editor.snap_to_grid = true;
    g_model_editor.grid_size = 0.125f; // 1/8 unit grid
    
    printf("Model Editor initialized\n");
    return true;
}

void model_editor_cleanup(void) {
    // Free allocated memory
    if (g_model_editor.vertex_buffer) {
        core_free(g_model_editor.vertex_buffer);
        g_model_editor.vertex_buffer = NULL;
    }
    
    if (g_model_editor.index_buffer) {
        core_free(g_model_editor.index_buffer);
        g_model_editor.index_buffer = NULL;
    }
    
    // Clear selections
    if (g_model_editor.vertex_selection.indices) {
        core_free(g_model_editor.vertex_selection.indices);
        g_model_editor.vertex_selection.indices = NULL;
    }
    
    if (g_model_editor.edge_selection.indices) {
        core_free(g_model_editor.edge_selection.indices);
        g_model_editor.edge_selection.indices = NULL;
    }
    
    if (g_model_editor.face_selection.indices) {
        core_free(g_model_editor.face_selection.indices);
        g_model_editor.face_selection.indices = NULL;
    }
    
    memset(&g_model_editor, 0, sizeof(ModelEditor));
    printf("Model Editor cleaned up\n");
}

void model_editor_set_model(Model3D* model) {
    g_model_editor.current_model = model;
    
    if (model && model->mesh_count > 0) {
        g_model_editor.editing_mesh = &model->meshes[0];
        
        // Allocate buffers for editing
        size_t vertex_size = model->meshes[0].vertex_count * sizeof(Vertex);
        size_t index_size = model->meshes[0].index_count * sizeof(uint32_t);
        
        if (g_model_editor.vertex_buffer) {
            core_free(g_model_editor.vertex_buffer);
        }
        g_model_editor.vertex_buffer = core_alloc(vertex_size);
        memcpy(g_model_editor.vertex_buffer, model->meshes[0].vertices, vertex_size);
        
        if (g_model_editor.index_buffer) {
            core_free(g_model_editor.index_buffer);
        }
        g_model_editor.index_buffer = core_alloc(index_size);
        memcpy(g_model_editor.index_buffer, model->meshes[0].indices, index_size);
    } else {
        g_model_editor.editing_mesh = NULL;
        if (g_model_editor.vertex_buffer) {
            core_free(g_model_editor.vertex_buffer);
            g_model_editor.vertex_buffer = NULL;
        }
        if (g_model_editor.index_buffer) {
            core_free(g_model_editor.index_buffer);
            g_model_editor.index_buffer = NULL;
        }
    }
    
    // Clear selections
    model_editor_clear_selection();
}

void model_editor_update(float dt) {
    if (!g_model_editor.current_model || !g_model_editor.editing_mesh) {
        return;
    }
    
    // Update selection based on current tool
    model_editor_update_selection(&g_model_editor);
    
    // Update vertex positions if sculpting
    if (g_model_editor.sculpt_tool.sculpting) {
        model_editor_update_vertex_positions(&g_model_editor);
    }
    
    // Update normals if geometry changed
    if (g_model_editor.editing_vertices) {
        model_editor_calculate_normals(&g_model_editor);
        model_editor_calculate_bounds(&g_model_editor);
        g_model_editor.editing_vertices = false;
    }
}

void model_editor_render(void) {
    if (!g_model_editor.current_model || !g_model_editor.editing_mesh) {
        return;
    }
    
    // Render the model with current settings
    renderer_set_wireframe(g_model_editor.show_wireframe);
    
    // Render mesh
    renderer_render_mesh(g_model_editor.editing_mesh);
    
    // Render overlays
    if (g_model_editor.show_normals) {
        model_editor_render_normals();
    }
    
    if (g_model_editor.show_bounding_box) {
        model_editor_render_bounding_box();
    }
    
    if (g_model_editor.show_uv_overlay) {
        model_editor_render_uv_overlay();
    }
    
    // Render selection
    model_editor_render_selection();
    
    // Render tool-specific overlays
    if (g_model_editor.sculpt_tool.sculpting) {
        model_editor_render_sculpt_overlay();
    }
    
    if (g_model_editor.paint_tool.painting) {
        model_editor_render_paint_overlay();
    }
}

void model_editor_handle_input(const AssetEditor* editor) {
    if (!g_model_editor.current_model || !g_model_editor.editing_mesh) {
        return;
    }
    
    EditorTool active_tool = editor->active_tool;
    
    switch (active_tool) {
        case TOOL_SELECT:
            model_editor_handle_select_input(editor);
            break;
        case TOOL_MOVE:
            model_editor_handle_move_input(editor);
            break;
        case TOOL_ROTATE:
            model_editor_handle_rotate_input(editor);
            break;
        case TOOL_SCALE:
            model_editor_handle_scale_input(editor);
            break;
        case TOOL_EXTRUDE:
            model_editor_handle_extrude_input(editor);
            break;
        case TOOL_SCULPT:
            model_editor_handle_sculpt_input(editor);
            break;
        case TOOL_PAINT:
            model_editor_handle_paint_input(editor);
            break;
        default:
            break;
    }
}

void model_editor_extrude_selection(const vec3 direction, float amount) {
    if (!g_model_editor.editing_mesh || !g_model_editor.vertex_selection.count) {
        return;
    }
    
    // Create new vertices for extruded faces
    uint32_t original_vertex_count = g_model_editor.editing_mesh->vertex_count;
    uint32_t selected_vertex_count = g_model_editor.vertex_selection.count;
    
    // Resize vertex buffer
    size_t new_vertex_size = (original_vertex_count + selected_vertex_count) * sizeof(Vertex);
    Vertex* new_vertices = core_realloc(g_model_editor.vertex_buffer, new_vertex_size);
    
    if (!new_vertices) {
        printf("Failed to allocate memory for extrusion\n");
        return;
    }
    
    g_model_editor.vertex_buffer = new_vertices;
    
    // Copy selected vertices and extrude them
    for (uint32_t i = 0; i < selected_vertex_count; i++) {
        uint32_t src_index = g_model_editor.vertex_selection.indices[i];
        uint32_t dst_index = original_vertex_count + i;
        
        // Copy vertex data
        g_model_editor.vertex_buffer[dst_index] = g_model_editor.vertex_buffer[src_index];
        
        // Extrude position
        vec3 extrude_vec = vec3_mul_scalar(direction, amount);
        g_model_editor.vertex_buffer[dst_index].position = vec3_add(
            g_model_editor.vertex_buffer[src_index].position,
            extrude_vec
        );
    }
    
    // Create new faces for extrusion
    // This is a simplified version - proper implementation would need to handle edge cases
    uint32_t original_index_count = g_model_editor.editing_mesh->index_count;
    uint32_t new_index_count = original_index_count + (selected_vertex_count * 6); // 6 indices per quad
    
    uint32_t* new_indices = core_realloc(g_model_editor.index_buffer, new_index_count * sizeof(uint32_t));
    
    if (!new_indices) {
        printf("Failed to allocate memory for extrusion indices\n");
        return;
    }
    
    g_model_editor.index_buffer = new_indices;
    
    // Add side faces (simplified - assumes selection forms a closed loop)
    for (uint32_t i = 0; i < selected_vertex_count; i++) {
        uint32_t current = g_model_editor.vertex_selection.indices[i];
        uint32_t next = g_model_editor.vertex_selection.indices[(i + 1) % selected_vertex_count];
        uint32_t current_extruded = original_vertex_count + i;
        uint32_t next_extruded = original_vertex_count + ((i + 1) % selected_vertex_count);
        
        uint32_t index_offset = original_index_count + (i * 6);
        
        // Create quad face
        g_model_editor.index_buffer[index_offset + 0] = current;
        g_model_editor.index_buffer[index_offset + 1] = next;
        g_model_editor.index_buffer[index_offset + 2] = next_extruded;
        g_model_editor.index_buffer[index_offset + 3] = current;
        g_model_editor.index_buffer[index_offset + 4] = next_extruded;
        g_model_editor.index_buffer[index_offset + 5] = current_extruded;
    }
    
    // Update mesh data
    g_model_editor.editing_mesh->vertices = g_model_editor.vertex_buffer;
    g_model_editor.editing_mesh->vertex_count = original_vertex_count + selected_vertex_count;
    g_model_editor.editing_mesh->indices = g_model_editor.index_buffer;
    g_model_editor.editing_mesh->index_count = new_index_count;
    
    // Mark as modified
    g_model_editor.editing_vertices = true;
}

void model_editor_sculpt_vertices(const vec3 center, float radius, float strength, bool add) {
    if (!g_model_editor.editing_mesh) {
        return;
    }
    
    float radius_sq = radius * radius;
    
    for (uint32_t i = 0; i < g_model_editor.editing_mesh->vertex_count; i++) {
        Vertex* vertex = &g_model_editor.vertex_buffer[i];
        
        // Calculate distance from sculpt center
        vec3 to_vertex = vec3_sub(vertex->position, center);
        float distance_sq = vec3_dot(to_vertex, to_vertex);
        
        if (distance_sq < radius_sq) {
            // Calculate falloff
            float falloff = 1.0f - (distance_sq / radius_sq);
            falloff = falloff * falloff; // Quadratic falloff
            
            // Calculate displacement
            vec3 displacement = vec3_normalize(to_vertex);
            displacement = vec3_mul_scalar(displacement, strength * falloff);
            
            if (!add) {
                displacement = vec3_mul_scalar(displacement, -1.0f);
            }
            
            // Apply displacement
            vertex->position = vec3_add(vertex->position, displacement);
        }
    }
    
    // Mark as modified
    g_model_editor.editing_vertices = true;
}

void model_editor_paint_vertices(const vec2 uv_pos, const vec4 color, float brush_size) {
    if (!g_model_editor.editing_mesh) {
        return;
    }
    
    float brush_size_sq = brush_size * brush_size;
    
    for (uint32_t i = 0; i < g_model_editor.editing_mesh->vertex_count; i++) {
        Vertex* vertex = &g_model_editor.vertex_buffer[i];
        
        // Calculate UV distance
        vec2 to_uv = vec2_sub(vertex->texcoord, uv_pos);
        float distance_sq = vec2_dot(to_uv, to_uv);
        
        if (distance_sq < brush_size_sq) {
            // Calculate falloff
            float falloff = 1.0f - (distance_sq / brush_size_sq);
            falloff = falloff * falloff;
            
            // Blend color
            vec4 current_color = vertex->color;
            vec4 blended_color = vec4_lerp(current_color, color, falloff);
            vertex->color = blended_color;
        }
    }
    
    // Mark as modified
    g_model_editor.editing_vertices = true;
}

void model_editor_select_vertices_in_rect(const vec2 min, const vec2 max) {
    if (!g_model_editor.editing_mesh) {
        return;
    }
    
    // Clear current selection
    model_editor_clear_selection();
    
    // Project vertices to screen space and select those in rectangle
    // This is a simplified version - proper implementation would need camera projection
    for (uint32_t i = 0; i < g_model_editor.editing_mesh->vertex_count; i++) {
        Vertex* vertex = &g_model_editor.vertex_buffer[i];
        
        // Simple screen space projection (placeholder)
        vec2 screen_pos = (vec2){vertex->position.x, vertex->position.y};
        
        if (screen_pos.x >= min.x && screen_pos.x <= max.x &&
            screen_pos.y >= min.y && screen_pos.y <= max.y) {
            
            // Add to selection
            if (g_model_editor.vertex_selection.count >= g_model_editor.vertex_selection.capacity) {
                // Resize selection buffer
                uint32_t new_capacity = g_model_editor.vertex_selection.capacity * 2;
                if (new_capacity == 0) new_capacity = 64;
                
                uint32_t* new_indices = core_realloc(g_model_editor.vertex_selection.indices, 
                                                   new_capacity * sizeof(uint32_t));
                if (new_indices) {
                    g_model_editor.vertex_selection.indices = new_indices;
                    g_model_editor.vertex_selection.capacity = new_capacity;
                }
            }
            
            if (g_model_editor.vertex_selection.count < g_model_editor.vertex_selection.capacity) {
                g_model_editor.vertex_selection.indices[g_model_editor.vertex_selection.count] = i;
                g_model_editor.vertex_selection.count++;
            }
        }
    }
}

void model_editor_clear_selection(void) {
    g_model_editor.vertex_selection.count = 0;
    g_model_editor.edge_selection.count = 0;
    g_model_editor.face_selection.count = 0;
}

void model_editor_apply_changes(void) {
    if (!g_model_editor.current_model || !g_model_editor.editing_mesh) {
        return;
    }
    
    // Copy edited data back to original mesh
    size_t vertex_size = g_model_editor.editing_mesh->vertex_count * sizeof(Vertex);
    size_t index_size = g_model_editor.editing_mesh->index_count * sizeof(uint32_t);
    
    memcpy(g_model_editor.editing_mesh->vertices, g_model_editor.vertex_buffer, vertex_size);
    memcpy(g_model_editor.editing_mesh->indices, g_model_editor.index_buffer, index_size);
    
    // Recalculate model bounds
    model_editor_calculate_bounds(&g_model_editor);
}

void model_editor_discard_changes(void) {
    if (!g_model_editor.current_model || !g_model_editor.editing_mesh) {
        return;
    }
    
    // Reload original data
    size_t vertex_size = g_model_editor.editing_mesh->vertex_count * sizeof(Vertex);
    size_t index_size = g_model_editor.editing_mesh->index_count * sizeof(uint32_t);
    
    memcpy(g_model_editor.vertex_buffer, g_model_editor.editing_mesh->vertices, vertex_size);
    memcpy(g_model_editor.index_buffer, g_model_editor.editing_mesh->indices, index_size);
}

// Static helper functions
static void model_editor_update_selection(ModelEditor* editor) {
    // Update selection based on current tool and input
    // This would handle selection logic for different tools
}

static void model_editor_update_vertex_positions(ModelEditor* editor) {
    // Update vertex positions based on sculpting or other operations
}

static void model_editor_calculate_normals(ModelEditor* editor) {
    if (!editor->editing_mesh) return;
    
    // Calculate face normals and vertex normals
    for (uint32_t i = 0; i < editor->editing_mesh->index_count; i += 3) {
        uint32_t i0 = editor->index_buffer[i];
        uint32_t i1 = editor->index_buffer[i + 1];
        uint32_t i2 = editor->index_buffer[i + 2];
        
        if (i0 < editor->editing_mesh->vertex_count &&
            i1 < editor->editing_mesh->vertex_count &&
            i2 < editor->editing_mesh->vertex_count) {
            
            Vertex* v0 = &editor->vertex_buffer[i0];
            Vertex* v1 = &editor->vertex_buffer[i1];
            Vertex* v2 = &editor->vertex_buffer[i2];
            
            // Calculate face normal
            vec3 edge1 = vec3_sub(v1->position, v0->position);
            vec3 edge2 = vec3_sub(v2->position, v0->position);
            vec3 normal = vec3_cross(edge1, edge2);
            normal = vec3_normalize(normal);
            
            // Add to vertex normals (will be normalized later)
            v0->normal = vec3_add(v0->normal, normal);
            v1->normal = vec3_add(v1->normal, normal);
            v2->normal = vec3_add(v2->normal, normal);
        }
    }
    
    // Normalize vertex normals
    for (uint32_t i = 0; i < editor->editing_mesh->vertex_count; i++) {
        editor->vertex_buffer[i].normal = vec3_normalize(editor->vertex_buffer[i].normal);
    }
}

static void model_editor_calculate_bounds(ModelEditor* editor) {
    if (!editor->editing_mesh || !editor->current_model) return;
    
    // Initialize bounds
    vec3 min_bounds = {FLT_MAX, FLT_MAX, FLT_MAX};
    vec3 max_bounds = {FLT_MIN, FLT_MIN, FLT_MIN};
    
    // Calculate bounds from vertices
    for (uint32_t i = 0; i < editor->editing_mesh->vertex_count; i++) {
        Vertex* vertex = &editor->vertex_buffer[i];
        
        min_bounds.x = fminf(min_bounds.x, vertex->position.x);
        min_bounds.y = fminf(min_bounds.y, vertex->position.y);
        min_bounds.z = fminf(min_bounds.z, vertex->position.z);
        
        max_bounds.x = fmaxf(max_bounds.x, vertex->position.x);
        max_bounds.y = fmaxf(max_bounds.y, vertex->position.y);
        max_bounds.z = fmaxf(max_bounds.z, vertex->position.z);
    }
    
    // Update model bounds
    editor->current_model->min_bounds = min_bounds;
    editor->current_model->max_bounds = max_bounds;
    editor->current_model->center = vec3_mul_scalar(vec3_add(min_bounds, max_bounds), 0.5f);
    
    // Calculate radius
    vec3 extent = vec3_sub(max_bounds, min_bounds);
    editor->current_model->radius = vec3_length(extent) * 0.5f;
}

// Render helper functions (simplified)
void model_editor_render_normals(void) {
    // Render vertex normals as lines
}

void model_editor_render_bounding_box(void) {
    // Render bounding box
}

void model_editor_render_uv_overlay(void) {
    // Render UV coordinates overlay
}

void model_editor_render_selection(void) {
    // Render selection highlights
}

void model_editor_render_sculpt_overlay(void) {
    // Render sculpt brush overlay
}

void model_editor_render_paint_overlay(void) {
    // Render paint brush overlay
}

// Input handling functions
void model_editor_handle_select_input(const AssetEditor* editor) {
    // Handle selection tool input
}

void model_editor_handle_move_input(const AssetEditor* editor) {
    // Handle move tool input
}

void model_editor_handle_rotate_input(const AssetEditor* editor) {
    // Handle rotate tool input
}

void model_editor_handle_scale_input(const AssetEditor* editor) {
    // Handle scale tool input
}

void model_editor_handle_extrude_input(const AssetEditor* editor) {
    // Handle extrude tool input
}

void model_editor_handle_sculpt_input(const AssetEditor* editor) {
    // Handle sculpt tool input
}

void model_editor_handle_paint_input(const AssetEditor* editor) {
    // Handle paint tool input
}
