/*
 * material_editor.h
 * Material editor interface
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_MATERIAL_EDITOR_H
#define MATERIALS_MATERIAL_EDITOR_H

#include "materials/material_system.h"
#include "materials/material_instance.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// TYPES AND CONSTANTS
// ============================================================================

typedef struct material_editor_handle {
    uint32_t id;
} material_editor_handle_t;

typedef enum material_editor_mode_e {
    MATERIAL_EDITOR_MODE_MASTER = 0,
    MATERIAL_EDITOR_MODE_INSTANCE = 1,
    MATERIAL_EDITOR_MODE_PREVIEW = 2
} material_editor_mode_e;

typedef enum material_editor_panel_e {
    MATERIAL_EDITOR_PANEL_PROPERTIES = 0,
    MATERIAL_EDITOR_PANEL_PREVIEW = 1,
    MATERIAL_EDITOR_PANEL_SHADER = 2,
    MATERIAL_EDITOR_PANEL_TEXTURES = 3,
    MATERIAL_EDITOR_PANEL_EXPORT = 4
} material_editor_panel_e;

typedef struct material_editor_desc_t {
    material_editor_mode_e mode;
    char initial_material[128];
    bool show_preview;
    bool auto_save;
    void* user_data;
} material_editor_desc_t;

typedef struct material_editor_info_t {
    uint32_t id;
    material_editor_mode_e mode;
    bool initialized;
    char current_material[128];
    bool has_unsaved_changes;
} material_editor_info_t;

// ============================================================================
// EDITOR LIFECYCLE
// ============================================================================

// Initialize material editor system
bool material_editor_init(void);

// Shutdown material editor system
void material_editor_shutdown(void);

// Create new material editor instance
material_editor_handle_t material_editor_create(const material_editor_desc_t* desc);

// Destroy material editor instance
void material_editor_destroy(material_editor_handle_t handle);

// Get editor information
bool material_editor_get_info(material_editor_handle_t handle, material_editor_info_t* info);

// ============================================================================
// EDITOR OPERATIONS
// ============================================================================

// Set current material for editing
bool material_editor_set_material(material_editor_handle_t handle, const char* material_name);

// Create new master material
bool material_editor_create_master(material_editor_handle_t handle, const char* name, const char* shader_path);

// Create new material instance
bool material_editor_create_instance(material_editor_handle_t handle, const char* master_name, const char* instance_name);

// Duplicate material (master or instance)
bool material_editor_duplicate_material(material_editor_handle_t handle, const char* source_name, const char* new_name);

// Delete material (master or instance)
bool material_editor_delete_material(material_editor_handle_t handle, const char* material_name);

// Save current material
bool material_editor_save_material(material_editor_handle_t handle);

// Save all materials
bool material_editor_save_all(material_editor_handle_t handle);

// ============================================================================
// PARAMETER EDITING
// ============================================================================

// Set parameter value
bool material_editor_set_parameter(material_editor_handle_t handle, const char* param_name, 
                                  MaterialParameterType type, const void* value);

// Get parameter value
bool material_editor_get_parameter(material_editor_handle_t handle, const char* param_name, 
                                  MaterialParameterType* type, void* value);

// Reset parameter to default
bool material_editor_reset_parameter(material_editor_handle_t handle, const char* param_name);

// Add new parameter to master material
bool material_editor_add_parameter(material_editor_handle_t handle, const char* param_name, 
                                   MaterialParameterType type, const void* default_value);

// Remove parameter from master material
bool material_editor_remove_parameter(material_editor_handle_t handle, const char* param_name);

// ============================================================================
// TEXTURE MANAGEMENT
// ============================================================================

// Set texture parameter
bool material_editor_set_texture(material_editor_handle_t handle, const char* param_name, uint32_t texture_id);

// Get texture parameter
bool material_editor_get_texture(material_editor_handle_t handle, const char* param_name, uint32_t* texture_id);

// Browse and assign texture
bool material_editor_browse_texture(material_editor_handle_t handle, const char* param_name);

// Import texture file
uint32_t material_editor_import_texture(material_editor_handle_t handle, const char* file_path);

// ============================================================================
// PREVIEW AND RENDERING
// ============================================================================

// Update preview mesh
bool material_editor_set_preview_mesh(material_editor_handle_t handle, const char* mesh_name);

// Set preview environment
bool material_editor_set_preview_environment(material_editor_handle_t handle, const char* environment_name);

// Update preview lighting
bool material_editor_set_preview_lighting(material_editor_handle_t handle, float intensity, const float* direction);

// Render preview to texture
bool material_editor_render_preview(material_editor_handle_t handle, uint32_t* output_texture);

// ============================================================================
// IMPORT/EXPORT
// ============================================================================

// Import material from file
bool material_editor_import_material(material_editor_handle_t handle, const char* file_path);

// Export material to file
bool material_editor_export_material(material_editor_handle_t handle, const char* material_name, const char* file_path);

// Export material library
bool material_editor_export_library(material_editor_handle_t handle, const char* file_path);

// Import material library
bool material_editor_import_library(material_editor_handle_t handle, const char* file_path);

// ============================================================================
// UNDO/REDO SYSTEM
// ============================================================================

// Begin undo group
void material_editor_begin_undo_group(material_editor_handle_t handle, const char* description);

// End undo group
void material_editor_end_undo_group(material_editor_handle_t handle);

// Undo last operation
bool material_editor_undo(material_editor_handle_t handle);

// Redo last undone operation
bool material_editor_redo(material_editor_handle_t handle);

// Get undo/redo stack sizes
void material_editor_get_undo_redo_info(material_editor_handle_t handle, uint32_t* undo_count, uint32_t* redo_count);

// Clear undo/redo history
void material_editor_clear_history(material_editor_handle_t handle);

// ============================================================================
// SEARCH AND FILTERING
// ============================================================================

// Search materials by name
bool material_editor_search_materials(material_editor_handle_t handle, const char* search_term, 
                                       char** results, uint32_t* result_count);

// Filter materials by type
bool material_editor_filter_by_type(material_editor_handle_t handle, material_editor_mode_e mode, 
                                    char** results, uint32_t* result_count);

// Filter materials by shader
bool material_editor_filter_by_shader(material_editor_handle_t handle, const char* shader_path, 
                                       char** results, uint32_t* result_count);

// ============================================================================
// VALIDATION AND DIAGNOSTICS
// ============================================================================

// Validate current material
bool material_editor_validate_material(material_editor_handle_t handle);

// Get material diagnostics
typedef struct material_diagnostic_t {
    char message[256];
    bool is_error;
    uint32_t line_number;
    char parameter_name[64];
} material_diagnostic_t;

bool material_editor_get_diagnostics(material_editor_handle_t handle, material_diagnostic_t* diagnostics, 
                                    uint32_t* diagnostic_count);

// Fix common issues
bool material_editor_auto_fix_issues(material_editor_handle_t handle);

// ============================================================================
// UI INTEGRATION
// ============================================================================

// Update editor UI (call this each frame)
void material_editor_update_ui(material_editor_handle_t handle);

// Handle input events
bool material_editor_handle_input(material_editor_handle_t handle, const void* input_event);

// Set UI callback for parameter changes
typedef void (*material_editor_param_change_callback_t)(material_editor_handle_t handle, 
                                                        const char* param_name, 
                                                        MaterialParameterType type, 
                                                        const void* old_value, 
                                                        const void* new_value, 
                                                        void* user_data);

void material_editor_set_param_change_callback(material_editor_handle_t handle, 
                                               material_editor_param_change_callback_t callback, 
                                               void* user_data);

// ============================================================================
// UTILITIES
// ============================================================================

// Get available shaders
bool material_editor_get_available_shaders(char** shaders, uint32_t* shader_count);

// Get available textures
bool material_editor_get_available_textures(char** textures, uint32_t* texture_count);

// Get material usage statistics
typedef struct material_usage_stats_t {
    uint32_t total_masters;
    uint32_t total_instances;
    uint32_t total_parameters;
    uint32_t total_textures;
    uint32_t memory_usage_bytes;
} material_usage_stats_t;

bool material_editor_get_usage_stats(material_editor_handle_t handle, material_usage_stats_t* stats);

// Reload shaders
bool material_editor_reload_shaders(material_editor_handle_t handle);

// Hot reload enabled
bool material_editor_is_hot_reload_enabled(material_editor_handle_t handle);

// Enable/disable hot reload
void material_editor_set_hot_reload(material_editor_handle_t handle, bool enabled);

#ifdef __cplusplus
}
#endif

#endif // MATERIALS_MATERIAL_EDITOR_H
