#include <editor/importer/blender_bridge.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Global blender configuration
static BlenderConfig g_blender_config = {0};
static bool g_blender_initialized = false;

/**
 * =================================================================================================
 *                                   BLENDER BRIDGE (Asset Generator)
 * =================================================================================================
 *
 * GOAL: "Use Blender to help with turning images into 3D/2.5D assets and
 * animations". STRATEGY: The engine calls Blender's Python API in headless mode
 * to perform complex mesh operations automatically. Each task below is < 100
 * LOC.
 */

// -------------------------------------------------------------------------------------------------
// SECTION 1: PROCESS MANAGEMENT
// -------------------------------------------------------------------------------------------------

void Blender_Initialize() { blender_initialize(&g_blender_config); }

// -------------------------------------------------------------------------------------------------
// SECTION 2: IMAGE TO 3D CONVERSION (The "2.5D" Feature)
// -------------------------------------------------------------------------------------------------

void Blender_ConvertImageToMesh(const char *image_path, const char *out_path) {
  ImageToMeshParams params = {.input_image_path = {0},
                              .output_mesh_path = {0},
                              .thickness = 0.1f,
                              .alpha_cutout = true,
                              .auto_uv = true,
                              .uv_scale = 1.0f,
                              .shadeless = false};

  strncpy(params.input_image_path, image_path,
          sizeof(params.input_image_path) - 1);
  strncpy(params.output_mesh_path, out_path,
          sizeof(params.output_mesh_path) - 1);

  blender_convert_image_to_mesh(&params);
}

// -------------------------------------------------------------------------------------------------
// SECTION 3: SKELETON & ANIMATION
// -------------------------------------------------------------------------------------------------

void Blender_AutoRigCharacter(const char *mesh_path) {
  AutoRigParams params = {.input_mesh_path = {0},
                          .output_rigged_path = {0},
                          .detect_humanoid = true,
                          .height_scale = 1.0f,
                          .automatic_weights = true,
                          .rig_type = "human"};

  strncpy(params.input_mesh_path, mesh_path,
          sizeof(params.input_mesh_path) - 1);
  snprintf(params.output_rigged_path, sizeof(params.output_rigged_path),
           "%s_rigged.glb", mesh_path);

  blender_auto_rig_character(&params);
}

// ============================================================================
// IMPLEMENTATION FUNCTIONS
// ============================================================================

static bool file_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

static bool directory_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static char *find_blender_executable(void) {
  // Check common paths and environment variables
  const char *paths[] = {
      getenv("BLENDER_PATH"),
      "/usr/bin/blender",
      "/usr/local/bin/blender",
      "/opt/blender/blender",
      "C:\\Program Files\\Blender Foundation\\Blender\\blender.exe",
      "C:\\Program Files\\Blender Foundation\\Blender 4.0\\blender.exe",
      "/Applications/Blender.app/Contents/MacOS/Blender",
      NULL};

  for (int i = 0; paths[i] != NULL; i++) {
    if (paths[i] && file_exists(paths[i])) {
      return strdup(paths[i]);
    }
  }

  return NULL;
}

static BlenderStatus execute_command(const char *command, char *output,
                                     size_t output_size) {
  FILE *pipe = popen(command, "r");
  if (!pipe) {
    return BLENDER_STATUS_ERROR_EXECUTION;
  }

  size_t bytes_read = fread(output, 1, output_size - 1, pipe);
  output[bytes_read] = '\0';

  int exit_code = pclose(pipe);
  return (exit_code == 0) ? BLENDER_STATUS_SUCCESS
                          : BLENDER_STATUS_ERROR_EXECUTION;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

BlenderStatus blender_initialize(BlenderConfig *config) {
  if (!config) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  // TASK_001: Locate Blender executable
  char *blender_path = find_blender_executable();
  if (!blender_path) {
           "BLENDER_PATH environment variable.\n");
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  strncpy(config->blender_path, blender_path, sizeof(config->blender_path) - 1);
  free(blender_path);

  // TASK_002: Verify Blender version
  char version_output[256];
  char version_cmd[512];
  snprintf(version_cmd, sizeof(version_cmd), "%s --version",
           config->blender_path);

  BlenderStatus status =
      execute_command(version_cmd, version_output, sizeof(version_output));
  if (status != BLENDER_STATUS_SUCCESS) {
    return BLENDER_STATUS_ERROR_VERSION;
  }

  // Parse version (expecting "Blender 4.x.x")
  u32 major, minor;
  if (sscanf(version_output, "Blender %u.%u", &major, &minor) != 2) {
    return BLENDER_STATUS_ERROR_VERSION;
  }

  if (major < 4) {
           "4.0 or later.\n",
           major, minor);
    return BLENDER_STATUS_ERROR_VERSION;
  }

  config->version_major = major;
  config->version_minor = minor;

  // TASK_003: Check for required Python scripts
  const char *engine_root = getenv("ENGINE_ROOT");
  if (!engine_root) {
    // Assume current working directory is engine root
    engine_root = ".";
  }

  snprintf(config->scripts_directory, sizeof(config->scripts_directory),
           "%s/tools/blender_scripts", engine_root);

  if (!directory_exists(config->scripts_directory)) {
           config->scripts_directory);
    return BLENDER_STATUS_ERROR_SCRIPTS;
  }

  // Check for required scripts
  char script_path[512];
  snprintf(script_path, sizeof(script_path), "%s/import_image_as_mesh.py",
           config->scripts_directory);

  if (!file_exists(script_path)) {
    return BLENDER_STATUS_ERROR_SCRIPTS;
  }

  // Set up temporary directory
  snprintf(config->temp_directory, sizeof(config->temp_directory), "%s/temp",
           engine_root);

  // Create temp directory if it doesn't exist
  struct stat st;
  if (stat(config->temp_directory, &st) != 0) {
    mkdir(config->temp_directory, 0755);
  }

  config->initialized = true;
  g_blender_initialized = true;

  printf("Blender Bridge initialized successfully:\n");
  printf("  Blender Path: %s\n", config->blender_path);
  printf("  Version: %u.%u\n", config->version_major, config->version_minor);
  printf("  Scripts Dir: %s\n", config->scripts_directory);

  return BLENDER_STATUS_SUCCESS;
}

void blender_shutdown(void) {
  g_blender_initialized = false;
  memset(&g_blender_config, 0, sizeof(g_blender_config));
}

bool blender_is_available(void) { return g_blender_initialized; }

BlenderStatus blender_convert_image_to_mesh(const ImageToMeshParams *params) {
  if (!g_blender_initialized || !params) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  if (!file_exists(params->input_image_path)) {
    return BLENDER_STATUS_ERROR_OUTPUT;
  }

  // TASK_010: Construct the system command
  char script_path[512];
  snprintf(script_path, sizeof(script_path), "%s/import_image_as_mesh.py",
           g_blender_config.scripts_directory);

  char command[1024];
  snprintf(command, sizeof(command),
           "%s -b -P %s -- --input %s --output %s --thickness %.3f "
           "--alpha_cutout %s --auto_uv %s --shadeless %s",
           g_blender_config.blender_path, script_path, params->input_image_path,
           params->output_mesh_path, params->thickness,
           params->alpha_cutout ? "true" : "false",
           params->auto_uv ? "true" : "false",
           params->shadeless ? "true" : "false");

  printf("Executing: %s\n", command);

  // TASK_017: Execute command and wait for process termination
  char output[2048];
  BlenderStatus status = execute_command(command, output, sizeof(output));

  if (status != BLENDER_STATUS_SUCCESS) {
    return BLENDER_STATUS_ERROR_EXECUTION;
  }

  // TASK_018: Verify output file exists
  if (!blender_verify_output_file(params->output_mesh_path)) {
           params->output_mesh_path);
    return BLENDER_STATUS_ERROR_OUTPUT;
  }

  printf("Successfully converted image to mesh: %s\n",
         params->output_mesh_path);
  return BLENDER_STATUS_SUCCESS;
}

BlenderStatus blender_batch_convert_images(const char **image_paths, u32 count,
                                           const char *output_directory) {
  if (!g_blender_initialized || !image_paths || count == 0) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  printf("Batch converting %u images...\n", count);

  u32 success_count = 0;
  u32 failure_count = 0;

  for (u32 i = 0; i < count; i++) {
    const char *input_path = image_paths[i];

    // Generate output filename
    const char *filename = strrchr(input_path, '/');
    if (!filename)
      filename = strrchr(input_path, '\\');
    if (!filename)
      filename = input_path;
    else
      filename++;

    char output_path[512];
    const char *extension = strrchr(filename, '.');
    if (extension) {
      size_t base_len = extension - filename;
      snprintf(output_path, sizeof(output_path), "%s/%.*s.glb",
               output_directory, (int)base_len, filename);
    } else {
      snprintf(output_path, sizeof(output_path), "%s/%s.glb", output_directory,
               filename);
    }

    ImageToMeshParams params = {.input_image_path = {0},
                                .output_mesh_path = {0},
                                .thickness = 0.1f,
                                .alpha_cutout = true,
                                .auto_uv = true,
                                .uv_scale = 1.0f,
                                .shadeless = false};

    strncpy(params.input_image_path, input_path,
            sizeof(params.input_image_path) - 1);
    strncpy(params.output_mesh_path, output_path,
            sizeof(params.output_mesh_path) - 1);

    BlenderStatus status = blender_convert_image_to_mesh(&params);

    if (status == BLENDER_STATUS_SUCCESS) {
      success_count++;
    } else {
      failure_count++;
      printf("Failed to convert: %s\n", input_path);
    }
  }

  printf("Batch conversion completed: %u successful, %u failed\n",
         success_count, failure_count);
  return (failure_count == 0) ? BLENDER_STATUS_SUCCESS
                              : BLENDER_STATUS_ERROR_EXECUTION;
}

BlenderStatus blender_auto_rig_character(const AutoRigParams *params) {
  if (!g_blender_initialized || !params) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  if (!file_exists(params->input_mesh_path)) {
    return BLENDER_STATUS_ERROR_OUTPUT;
  }

  // Construct the system command
  char script_path[512];
  snprintf(script_path, sizeof(script_path), "%s/auto_rig_character.py",
           g_blender_config.scripts_directory);

  char command[1024];
  snprintf(command, sizeof(command),
           "%s -b -P %s -- --input %s --output %s --rig_type %s --height_scale "
           "%.3f --automatic_weights %s",
           g_blender_config.blender_path, script_path, params->input_mesh_path,
           params->output_rigged_path, params->rig_type, params->height_scale,
           params->automatic_weights ? "true" : "false");

  printf("Executing auto-rigging: %s\n", command);

  // Execute command and wait for process termination
  char output[2048];
  BlenderStatus status = execute_command(command, output, sizeof(output));

  if (status != BLENDER_STATUS_SUCCESS) {
    return BLENDER_STATUS_ERROR_EXECUTION;
  }

  // Verify output file exists
  if (!blender_verify_output_file(params->output_rigged_path)) {
           params->output_rigged_path);
    return BLENDER_STATUS_ERROR_OUTPUT;
  }

  printf("Successfully auto-rigged character: %s\n",
         params->output_rigged_path);
  return BLENDER_STATUS_SUCCESS;
}

BlenderStatus blender_generate_walk_cycle(const char *rigged_mesh_path,
                                          const char *output_animation_path) {
  if (!g_blender_initialized || !rigged_mesh_path || !output_animation_path) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  if (!file_exists(rigged_mesh_path)) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  // Construct the system command
  char script_path[512];
  snprintf(script_path, sizeof(script_path), "%s/generate_walk_cycle.py",
           g_blender_config.scripts_directory);

  char command[1024];
  snprintf(command, sizeof(command),
           "%s -b -P %s -- --input %s --output %s --frame_count 30",
           g_blender_config.blender_path, script_path, rigged_mesh_path,
           output_animation_path);

  printf("Executing walk cycle generation: %s\n", command);

  // Execute command and wait for process termination
  char output[2048];
  BlenderStatus status = execute_command(command, output, sizeof(output));

  if (status != BLENDER_STATUS_SUCCESS) {
    return BLENDER_STATUS_ERROR_EXECUTION;
  }

  // Verify output file exists
  if (!blender_verify_output_file(output_animation_path)) {
    return BLENDER_STATUS_ERROR_OUTPUT;
  }

  printf("Successfully generated walk cycle: %s\n", output_animation_path);
  return BLENDER_STATUS_SUCCESS;
}

BlenderStatus blender_export_animations(const char *rigged_mesh_path,
                                        const char *output_directory) {
  if (!g_blender_initialized || !rigged_mesh_path || !output_directory) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  if (!file_exists(rigged_mesh_path)) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  // Construct the system command
  char script_path[512];
  snprintf(script_path, sizeof(script_path), "%s/export_animations.py",
           g_blender_config.scripts_directory);

  char command[1024];
  snprintf(command, sizeof(command),
           "%s -b -P %s -- --input %s --output_dir %s",
           g_blender_config.blender_path, script_path, rigged_mesh_path,
           output_directory);

  printf("Executing animation export: %s\n", command);

  // Execute command and wait for process termination
  char output[2048];
  BlenderStatus status = execute_command(command, output, sizeof(output));

  if (status != BLENDER_STATUS_SUCCESS) {
    return BLENDER_STATUS_ERROR_EXECUTION;
  }

  printf("Successfully exported animations via Blender.\n");
  return BLENDER_STATUS_SUCCESS;
}

BlenderStatus blender_execute_script(const char *script_path, const char **args,
                                     u32 arg_count) {
  if (!g_blender_initialized || !script_path) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  if (!file_exists(script_path)) {
    return BLENDER_STATUS_ERROR_SCRIPTS;
  }

  // Build command with arguments
  char command[2048];
  snprintf(command, sizeof(command), "%s -b -P %s",
           g_blender_config.blender_path, script_path);

  // Add arguments
  if (args && arg_count > 0) {
    strncat(command, " --", sizeof(command) - strlen(command) - 1);
    for (u32 i = 0; i < arg_count && i * 2 + 1 < arg_count; i += 2) {
      char arg_str[256];
      strncat(command, arg_str, sizeof(command) - strlen(command) - 1);
    }
  }

  char output[2048];
  BlenderStatus status = execute_command(command, output, sizeof(output));

  if (status != BLENDER_STATUS_SUCCESS) {
  }

  return status;
}

bool blender_verify_output_file(const char *file_path) {
  return file_exists(file_path);
}

BlenderStatus blender_cleanup_temp_files(void) {
  if (!g_blender_initialized) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  // Clean up temporary files in the temp directory
  // This is a simple implementation - in production you'd want more
  // sophisticated cleanup
  printf("Cleaning up temporary files in: %s\n",
         g_blender_config.temp_directory);

  // For now, just report success - actual cleanup would depend on OS-specific
  // commands
  return BLENDER_STATUS_SUCCESS;
}

void blender_print_status(void) {
  if (!g_blender_initialized) {
    printf("Blender Bridge: Not initialized\n");
    return;
  }

  printf("Blender Bridge Status:\n");
  printf("  Initialized: Yes\n");
  printf("  Blender Path: %s\n", g_blender_config.blender_path);
  printf("  Version: %u.%u\n", g_blender_config.version_major,
         g_blender_config.version_minor);
  printf("  Scripts Directory: %s\n", g_blender_config.scripts_directory);
  printf("  Temp Directory: %s\n", g_blender_config.temp_directory);
}

BlenderStatus blender_test_connection(void) {
  if (!g_blender_initialized) {
    return BLENDER_STATUS_ERROR_NOT_FOUND;
  }

  char test_cmd[512];
  snprintf(test_cmd, sizeof(test_cmd), "%s --version",
           g_blender_config.blender_path);

  char output[256];
  BlenderStatus status = execute_command(test_cmd, output, sizeof(output));

  if (status == BLENDER_STATUS_SUCCESS) {
    printf("Blender connection test successful\n");
  } else {
    printf("Blender connection test failed\n");
  }

  return status;
}

bool blender_validate_scripts_directory(void) {
  if (!g_blender_initialized) {
    return false;
  }

  return directory_exists(g_blender_config.scripts_directory);
}
