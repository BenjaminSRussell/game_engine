#ifndef BLENDER_BRIDGE_H
#define BLENDER_BRIDGE_H

#include <common.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Blender Bridge Configuration
typedef struct {
    char blender_path[512];
    char scripts_directory[512];
    char temp_directory[512];
    bool initialized;
    u32 version_major;
    u32 version_minor;
} BlenderConfig;

// Blender Bridge Status
typedef enum {
    BLENDER_STATUS_SUCCESS = 0,
    BLENDER_STATUS_ERROR_NOT_FOUND,
    BLENDER_STATUS_ERROR_VERSION,
    BLENDER_STATUS_ERROR_SCRIPTS,
    BLENDER_STATUS_ERROR_EXECUTION,
    BLENDER_STATUS_ERROR_OUTPUT
} BlenderStatus;

// Image to Mesh Parameters
typedef struct {
    char input_image_path[512];
    char output_mesh_path[512];
    f32 thickness;              // 2.5D thickness in world units
    bool alpha_cutout;          // Use alpha channel for mesh cutting
    bool auto_uv;               // Generate UV coordinates
    f32 uv_scale;              // UV scaling factor
    bool shadeless;            // Use shadeless material
} ImageToMeshParams;

// Auto Rig Parameters
typedef struct {
    char input_mesh_path[512];
    char output_rigged_path[512];
    bool detect_humanoid;       // Auto-detect if character is humanoid
    f32 height_scale;          // Scale rig to match mesh height
    bool automatic_weights;     // Use automatic vertex weighting
    char rig_type[64];         // "human", "quadruped", "custom"
} AutoRigParams;

// ============================================================================
// PUBLIC API
// ============================================================================

// System initialization and management
BlenderStatus blender_initialize(BlenderConfig *config);
void blender_shutdown(void);
bool blender_is_available(void);
BlenderStatus blender_get_version_info(u32 *major, u32 *minor);

// ============================================================================
// IMAGE TO 3D CONVERSION (2.5D ASSETS)
// ============================================================================

BlenderStatus blender_convert_image_to_mesh(const ImageToMeshParams *params);
BlenderStatus blender_batch_convert_images(const char **image_paths, u32 count, 
                                           const char *output_directory);

// ============================================================================
// AUTO-RIGGING AND ANIMATION
// ============================================================================

BlenderStatus blender_auto_rig_character(const AutoRigParams *params);
BlenderStatus blender_generate_walk_cycle(const char *rigged_mesh_path, 
                                         const char *output_animation_path);
BlenderStatus blender_export_animations(const char *rigged_mesh_path, 
                                       const char *output_directory);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

BlenderStatus blender_execute_script(const char *script_path, const char **args, 
                                    u32 arg_count);
bool blender_verify_output_file(const char *file_path);
BlenderStatus blender_cleanup_temp_files(void);

// ============================================================================
// DEBUG AND DIAGNOSTICS
// ============================================================================

void blender_print_status(void);
BlenderStatus blender_test_connection(void);
bool blender_validate_scripts_directory(void);

#endif // BLENDER_BRIDGE_H
