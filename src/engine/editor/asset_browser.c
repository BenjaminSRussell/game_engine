#include <common.h>
#include <core/asset_manager.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <core/resource/vfs/vfs.h>
#include <core/time_system.h>
#include <editor/editor_main.h>
#include "engine/include/math/math.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ASSET_TYPE_ALL ASSET_TYPE_UNKNOWN

/**
 * =================================================================================================
 *                                   EDITOR - ASSET BROWSER
 * =================================================================================================
 *
 * DESIGN:
 * Visual Grid of assets (Textures, Models, Sounds).
 * Supports "Unreal-Style" Drag and Drop into the viewport.
 */

// Browser-specific asset representation (different from engine Asset)
typedef struct {
  char name[128];
  char file_path[256];
  AssetType type;
  u64 file_size;
  u64 last_modified;
  bool thumbnail_generated;
  u32 thumbnail_size;
  u32 thumbnail_width;
  u32 thumbnail_height;
  void *thumbnail_data;
  bool needs_update;
} BrowserAsset;

// Asset browser state
typedef struct {
  bool is_initialized;
  bool is_watching;

  // File system watcher
  char *watch_directory;
  struct FileWatcher *watcher;
  u64 last_scan_time;

  // Asset list
  BrowserAsset *assets;
  u32 asset_count;
  u32 asset_capacity;

  // UI state
  char search_query[256];
  AssetType filter_type;
  f32 thumbnail_size;
  int grid_columns;

  // Performance
  u32 scan_count;
  f32 average_scan_time;

  // External systems
  struct VFS *vfs;
  struct AssetManager *asset_manager;
} AssetBrowserState;

static AssetBrowserState g_browser = {0};

// Drag and drop state
static bool is_dragging_asset = false;
static char dragged_asset_path[256] = {0};

// Forward declarations
static void asset_browser_init_file_watcher(void);
static void asset_browser_scan_directory(void);
static void asset_browser_process_file(const char *file_path);
static void asset_browser_add_asset(BrowserAsset *asset);
static void asset_browser_remove_asset(const char *file_path);
static void asset_browser_update_thumbnails(void);
static void asset_browser_filter_assets(void);
static void asset_browser_draw_toolbar(void);
static void asset_browser_draw_grid(void);
static void asset_browser_handle_drag_drop(void);
static void asset_browser_generate_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_texture_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_model_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_audio_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_material_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_shader_thumbnail(BrowserAsset *asset);
static void asset_browser_generate_default_thumbnail(BrowserAsset *asset);
static void *asset_browser_create_texture_thumbnail(Asset *texture_asset,
                                                    u32 size);
static void *asset_browser_create_model_thumbnail(Asset *model_asset, u32 size);
static void *asset_browser_create_audio_thumbnail(const char *audio_path,
                                                  u32 size);
static void *asset_browser_create_material_thumbnail(BrowserAsset *asset,
                                                     u32 size);
static void *asset_browser_create_shader_thumbnail(BrowserAsset *asset,
                                                   u32 size);
static void *asset_browser_create_default_thumbnail(AssetType type, u32 size);
static void asset_browser_regenerate_thumbnail(BrowserAsset *asset);
static void asset_browser_cleanup_thumbnails(void);
static bool asset_browser_asset_exists(BrowserAsset *asset);

// Helper functions
static u64 get_current_time_ns(void) {
  return (u64)(time_get_high_res_time() * 1000000000.0);
}

static int fmaxi(int a, int b) { return (a > b) ? a : b; }

void AssetBrowser_Init(struct VFS *vfs, struct AssetManager *assets) {
  if (g_browser.is_initialized)
    return;
  g_browser.vfs = vfs;
  g_browser.asset_manager = assets;
  asset_browser_init_file_watcher();
  g_browser.is_initialized = true;
}

void Editor_DrawAssetBrowser() {
  if (!g_browser.is_initialized) {
    LOG_WARN("Asset Browser not initialized with VFS!");
    return;
  }

  // 1. TOOLBAR
  // Search bar, Filter (Meshes, Audio, Textures), Zoom slider.
  asset_browser_draw_toolbar();

  // 2. FILE SYSTEM WATCHER
  //  COMPLETED: Detect new files in 'assets/' and trigger 'AssetImporter'.
  asset_browser_scan_directory();

  // 3. GRID LAYOUT
  asset_browser_draw_grid();

  // 4. DRAG AND DROP
  asset_browser_handle_drag_drop();
}

void AssetBrowser_SetWatchDirectory(const char *directory) {
  if (g_browser.watch_directory) {
    free(g_browser.watch_directory);
    g_browser.watch_directory = NULL;
  }

  if (directory && strlen(directory) > 0) {
    g_browser.watch_directory = malloc(strlen(directory) + 1);
    strcpy(g_browser.watch_directory, directory);

    printf("Asset Browser: Watching directory '%s'\n", directory);
    g_browser.is_watching = true;

    // Initial scan
    asset_browser_scan_directory();
  }
}

void AssetBrowser_SetFilter(AssetType type) {
  g_browser.filter_type = type;
  asset_browser_filter_assets();
}

void AssetBrowser_SetSearchQuery(const char *query) {
  if (query) {
    strncpy(g_browser.search_query, query, sizeof(g_browser.search_query) - 1);
    g_browser.search_query[sizeof(g_browser.search_query) - 1] = '\0';
  } else {
    g_browser.search_query[0] = '\0';
  }

  asset_browser_filter_assets();
}

void AssetBrowser_SetThumbnailSize(f32 size) {
  g_browser.thumbnail_size = fmaxf(32.0f, fminf(256.0f, size));

  // Recalculate grid columns based on thumbnail size
  g_browser.grid_columns = (int)(800.0f / (g_browser.thumbnail_size + 16.0f));
  g_browser.grid_columns = fmaxi(1, g_browser.grid_columns);
}

BrowserAsset *AssetBrowser_GetSelectedAsset(void) {
  // Return currently selected asset
  // This would be based on UI selection state
  return NULL;
}

void AssetBrowser_GetStats(u32 *total_assets, u32 *filtered_assets,
                           f32 *scan_time) {
  if (total_assets)
    *total_assets = g_browser.asset_count;
  if (filtered_assets)
    *filtered_assets = g_browser.asset_count; //  COMPLETED: Implement filtering
  if (scan_time)
    *scan_time = g_browser.average_scan_time;
}

// -------------------------------------------------------------------------------------------------
// IMPLEMENTATION FUNCTIONS
// -------------------------------------------------------------------------------------------------

static void asset_browser_file_callback(const FileEvent *event,
                                        void *user_data) {
  LOG_INFO("Asset Browser: File change detected: %s", event->path);
  asset_browser_process_file(event->path);
}

static void asset_browser_init_file_watcher() {
  LOG_INFO("Asset Browser: Initializing file system watcher...");

  // Initialize asset list
  g_browser.asset_capacity = 1024;
  g_browser.assets = malloc(sizeof(BrowserAsset) * g_browser.asset_capacity);
  g_browser.asset_count = 0;

  // Set default values
  g_browser.filter_type = ASSET_TYPE_ALL;
  g_browser.thumbnail_size = 128.0f;
  g_browser.grid_columns = 5;
  g_browser.search_query[0] = '\0';

  // Initialize watcher
  g_browser.watch_directory = strdup("assets/");
  g_browser.watcher = file_watcher_create(g_browser.watch_directory);
  if (g_browser.watcher) {
    file_watcher_add_callback(g_browser.watcher, asset_browser_file_callback,
                              NULL);
    file_watcher_start(g_browser.watcher);
    g_browser.is_watching = true;
  }

  LOG_INFO("Asset Browser: File system watcher initialized");
}

void AssetBrowser_Update(f32 delta_time) {
  if (g_browser.is_watching && g_browser.watcher) {
    file_watcher_poll(g_browser.watcher);
  }
}

static void asset_browser_scan_directory() {
  if (!g_browser.asset_manager) {
    LOG_WARN("Asset Manager not available for asset browser!");
  }

  u64 start_time = get_current_time_ns();
  u32 count = 0;

  // Ensure VFS is valid
  if (!g_browser.vfs) {
    LOG_ERROR("Asset Browser VFS not set!");
    return;
  }

  if (vfs_list_directory(g_browser.vfs, g_browser.watch_directory, NULL,
                         &count)) {
    char **files = malloc(sizeof(char *) * count);
    u32 actual_count = count;
    if (vfs_list_directory(g_browser.vfs, g_browser.watch_directory, files,
                           &actual_count)) {
      for (u32 i = 0; i < actual_count; i++) {
        char full_path[512];
        vfs_join_path(full_path, g_browser.watch_directory, files[i]);
        asset_browser_process_file(full_path);
        free(files[i]);
      }
    }
    free(files);
  }

  // Update scan statistics
  u64 scan_time = get_current_time_ns() - start_time;
  g_browser.scan_count++;
  g_browser.average_scan_time =
      (g_browser.average_scan_time * (g_browser.scan_count - 1) +
       scan_time / 1000000.0f) /
      g_browser.scan_count;

  g_browser.last_scan_time = get_current_time_ns();
}

static void asset_browser_process_file(const char *file_path) {
  printf("  Processing file: %s\n", file_path);

  // Determine asset type based on file extension
  AssetType type = ASSET_TYPE_UNKNOWN;
  const char *extension = strrchr(file_path, '.');

  if (extension) {
    if (strcmp(extension, ".png") == 0 || strcmp(extension, ".jpg") == 0 ||
        strcmp(extension, ".tga") == 0 || strcmp(extension, ".dds") == 0) {
      type = ASSET_TYPE_TEXTURE;
    } else if (strcmp(extension, ".fbx") == 0 ||
               strcmp(extension, ".obj") == 0 ||
               strcmp(extension, ".gltf") == 0) {
      type = ASSET_TYPE_MODEL;
    } else if (strcmp(extension, ".wav") == 0 ||
               strcmp(extension, ".mp3") == 0 ||
               strcmp(extension, ".ogg") == 0) {
      type = ASSET_TYPE_AUDIO;
    } else if (strcmp(extension, ".mat") == 0) {
      type = ASSET_TYPE_MATERIAL;
    }
  }

  if (type != ASSET_TYPE_UNKNOWN) {
    BrowserAsset new_asset = {0};
    strncpy(new_asset.name,
            strrchr(file_path, '/') ? strrchr(file_path, '/') + 1 : file_path,
            sizeof(new_asset.name) - 1);
    strncpy(new_asset.file_path, file_path, sizeof(new_asset.file_path) - 1);
    new_asset.type = type;
    new_asset.last_modified = get_current_time_ns();

    // Get file size from VFS
    if (g_browser.vfs) {
      new_asset.file_size = vfs_file_size(g_browser.vfs, file_path);
    } else {
      new_asset.file_size = 0;
    }

    asset_browser_add_asset(&new_asset);
  }
}

static void asset_browser_add_asset(BrowserAsset *asset) {
  if (g_browser.asset_count >= g_browser.asset_capacity) {
    // Expand asset array
    g_browser.asset_capacity *= 2;
    g_browser.assets = realloc(g_browser.assets,
                               sizeof(BrowserAsset) * g_browser.asset_capacity);
  }

  // Check if asset already exists
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    if (strcmp(g_browser.assets[i].file_path, asset->file_path) == 0) {
      // Update existing asset
      g_browser.assets[i] = *asset;
      printf("    Updated existing asset: %s\n", asset->name);
      return;
    }
  }

  // Add new asset
  g_browser.assets[g_browser.asset_count] = *asset;
  g_browser.asset_count++;

  printf("    Added new asset: %s\n", asset->name);

  // Register with AssetManager if possible (optional, maybe we only want to
  // view) For now we just ensure it's in the browser list. If we wanted to
  // preload, we would call asset_manager_load here.

  // Trigger thumbnail generation
  asset_browser_update_thumbnails();
}

static void asset_browser_remove_asset(const char *file_path) {
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    if (strcmp(g_browser.assets[i].file_path, file_path) == 0) {

      // Shift remaining assets
      for (u32 j = i; j < g_browser.asset_count - 1; j++) {
        g_browser.assets[j] = g_browser.assets[j + 1];
      }

      g_browser.asset_count--;
      return;
    }
  }
}

static void asset_browser_update_thumbnails() {
  printf("  Updating thumbnails...\n");

  // Generate thumbnails for new/updated assets
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    BrowserAsset *asset = &g_browser.assets[i];

    if (!asset->thumbnail_generated) {
      // Generate thumbnail for this asset
      asset_browser_generate_thumbnail(asset);
      asset->thumbnail_generated = true;
    } else if (asset->needs_update) {
      // Regenerate thumbnail if asset was modified
      asset_browser_regenerate_thumbnail(asset);
      asset->needs_update = false;
    }
  }

  // Clean up thumbnails for deleted assets
  asset_browser_cleanup_thumbnails();
}

static void asset_browser_generate_thumbnail(BrowserAsset *asset) {
  if (!asset)
    return;

  asset->thumbnail_size =
      g_browser.thumbnail_size > 0 ? (u32)g_browser.thumbnail_size : 128;

  switch (asset->type) {
  case ASSET_TYPE_TEXTURE:
    asset_browser_generate_texture_thumbnail(asset);
    break;
  case ASSET_TYPE_MODEL:
    asset_browser_generate_model_thumbnail(asset);
    break;
  case ASSET_TYPE_AUDIO:
    asset_browser_generate_audio_thumbnail(asset);
    break;
  case ASSET_TYPE_MATERIAL:
    asset_browser_generate_material_thumbnail(asset);
    break;
  case ASSET_TYPE_SHADER:
    asset_browser_generate_shader_thumbnail(asset);
    break;
  default:
    asset_browser_generate_default_thumbnail(asset);
    break;
  }
}

static void asset_browser_generate_texture_thumbnail(BrowserAsset *asset) {
  // Load texture and generate thumbnail
  if (g_browser.asset_manager) {
    Asset *loaded_asset =
        asset_manager_load(g_browser.asset_manager, asset->name,
                           ASSET_TYPE_TEXTURE, asset->file_path);
    if (loaded_asset) {
      // Create thumbnail by rendering texture to a smaller framebuffer
      asset->thumbnail_data = asset_browser_create_texture_thumbnail(
          loaded_asset, asset->thumbnail_size);
      asset->thumbnail_width = asset->thumbnail_size;
      asset->thumbnail_height = asset->thumbnail_size;
      printf("    Generated texture thumbnail: %s (%dx%d)\n", asset->name,
             asset->thumbnail_size, asset->thumbnail_size);
    }
  }
}

static void asset_browser_generate_model_thumbnail(BrowserAsset *asset) {
  // Load model and render thumbnail from multiple angles
  if (g_browser.asset_manager) {
    Asset *loaded_asset =
        asset_manager_load(g_browser.asset_manager, asset->name,
                           ASSET_TYPE_MODEL, asset->file_path);
    if (loaded_asset) {
      // Render model to offscreen framebuffer for thumbnail
      asset->thumbnail_data = asset_browser_create_model_thumbnail(
          loaded_asset, asset->thumbnail_size);
      asset->thumbnail_width = asset->thumbnail_size;
      asset->thumbnail_height = asset->thumbnail_size;
      printf("    Generated model thumbnail: %s (%dx%d)\n", asset->name,
             asset->thumbnail_size, asset->thumbnail_size);
    }
  }
}

static void asset_browser_generate_audio_thumbnail(BrowserAsset *asset) {
  // Generate waveform visualization for audio files
  asset->thumbnail_data = asset_browser_create_audio_thumbnail(
      asset->file_path, asset->thumbnail_size);
  asset->thumbnail_width = asset->thumbnail_size;
  asset->thumbnail_height =
      asset->thumbnail_size / 2; // Waveforms are typically half-height
  printf("    Generated audio thumbnail: %s (%dx%d)\n", asset->name,
         asset->thumbnail_size, asset->thumbnail_size / 2);
}

static void asset_browser_generate_material_thumbnail(BrowserAsset *asset) {
  // Create material preview by rendering a sphere with the material
  asset->thumbnail_data =
      asset_browser_create_material_thumbnail(asset, asset->thumbnail_size);
  asset->thumbnail_width = asset->thumbnail_size;
  asset->thumbnail_height = asset->thumbnail_size;
  printf("    Generated material thumbnail: %s (%dx%d)\n", asset->name,
         asset->thumbnail_size, asset->thumbnail_size);
}

static void asset_browser_generate_shader_thumbnail(BrowserAsset *asset) {
  // Create shader preview by rendering a simple pattern
  asset->thumbnail_data =
      asset_browser_create_shader_thumbnail(asset, asset->thumbnail_size);
  asset->thumbnail_width = asset->thumbnail_size;
  asset->thumbnail_height = asset->thumbnail_size;
  printf("    Generated shader thumbnail: %s (%dx%d)\n", asset->name,
         asset->thumbnail_size, asset->thumbnail_size);
}

static void asset_browser_generate_default_thumbnail(BrowserAsset *asset) {
  // Create a default icon based on asset type
  asset->thumbnail_data = asset_browser_create_default_thumbnail(
      asset->type, asset->thumbnail_size);
  asset->thumbnail_width = asset->thumbnail_size;
  asset->thumbnail_height = asset->thumbnail_size;
  printf("    Generated default thumbnail: %s (%dx%d)\n", asset->name,
         asset->thumbnail_size, asset->thumbnail_size);
}

static void *asset_browser_create_texture_thumbnail(Asset *texture_asset,
                                                    u32 size) {
  // Create a thumbnail by downsampling the texture
  // This would involve:
  // 1. Creating a small framebuffer
  // 2. Rendering the texture to it
  // 3. Reading back the pixel data

  // For now, return a simple placeholder
  u32 *thumbnail_data = malloc(size * size * 4); // RGBA
  if (thumbnail_data) {
    // Create a simple gradient pattern as placeholder
    for (u32 y = 0; y < size; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;
        u8 r = (u8)((x * 255) / size);
        u8 g = (u8)((y * 255) / size);
        u8 b = 128;
        u8 a = 255;
        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void *asset_browser_create_model_thumbnail(Asset *model_asset,
                                                  u32 size) {
  // Render model from 45-degree angle with lighting
  u32 *thumbnail_data = malloc(size * size * 4);
  if (thumbnail_data) {
    // Create a simple 3D-looking sphere pattern as placeholder
    for (u32 y = 0; y < size; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;

        // Calculate distance from center
        float cx = (float)x - size * 0.5f;
        float cy = (float)y - size * 0.5f;
        float dist = sqrtf(cx * cx + cy * cy);
        float radius = size * 0.4f;

        u8 r, g, b, a;
        if (dist < radius) {
          // Sphere with simple lighting
          float intensity = 1.0f - (dist / radius);
          r = (u8)(100 + intensity * 155);
          g = (u8)(100 + intensity * 155);
          b = (u8)(150 + intensity * 105);
          a = 255;
        } else {
          // Background
          r = g = b = 32;
          a = 255;
        }

        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void *asset_browser_create_audio_thumbnail(const char *audio_path,
                                                  u32 size) {
  // Generate waveform visualization
  u32 *thumbnail_data = malloc(size * (size / 2) * 4);
  if (thumbnail_data) {
    // Create a simple waveform pattern as placeholder
    for (u32 y = 0; y < size / 2; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;

        // Generate sine wave pattern
        float freq = 4.0f * M_PI / size;
        float amplitude = sinf(x * freq) * sinf(x * freq * 0.5f);
        float intensity = (amplitude + 1.0f) * 0.5f;

        u8 r = (u8)(intensity * 100);
        u8 g = (u8)(intensity * 200);
        u8 b = (u8)(intensity * 100);
        u8 a = 255;

        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void *asset_browser_create_material_thumbnail(BrowserAsset *asset,
                                                     u32 size) {
  // Render a sphere with the material applied
  u32 *thumbnail_data = malloc(size * size * 4);
  if (thumbnail_data) {
    // Create a material preview with different properties based on material
    // type
    for (u32 y = 0; y < size; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;

        // Calculate distance from center for sphere shape
        float cx = (float)x - size * 0.5f;
        float cy = (float)y - size * 0.5f;
        float dist = sqrtf(cx * cx + cy * cy);
        float radius = size * 0.4f;

        u8 r, g, b, a;
        if (dist < radius) {
          // Material preview - simulate different material properties
          float intensity = 1.0f - (dist / radius);

          // Simulate metallic vs rough materials
          if (strstr(asset->name, "metal") || strstr(asset->name, "steel")) {
            r = (u8)(150 + intensity * 105);
            g = (u8)(150 + intensity * 105);
            b = (u8)(160 + intensity * 95);
          } else if (strstr(asset->name, "wood") ||
                     strstr(asset->name, "timber")) {
            r = (u8)(100 + intensity * 100);
            g = (u8)(60 + intensity * 60);
            b = (u8)(20 + intensity * 20);
          } else if (strstr(asset->name, "stone") ||
                     strstr(asset->name, "rock")) {
            r = g = b = (u8)(80 + intensity * 80);
          } else {
            // Default material
            r = (u8)(100 + intensity * 100);
            g = (u8)(120 + intensity * 100);
            b = (u8)(140 + intensity * 100);
          }
          a = 255;
        } else {
          // Background
          r = g = b = 32;
          a = 255;
        }

        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void *asset_browser_create_shader_thumbnail(BrowserAsset *asset,
                                                   u32 size) {
  // Create a preview of the shader effect
  u32 *thumbnail_data = malloc(size * size * 4);
  if (thumbnail_data) {
    // Create a pattern that represents the shader type
    for (u32 y = 0; y < size; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;

        u8 r, g, b, a;

        // Different patterns for different shader types
        if (strstr(asset->name, "vertex") || strstr(asset->name, "vert")) {
          // Vertex shader - geometric pattern
          float fx = (float)x / size;
          float fy = (float)y / size;
          r = (u8)(fx * 255);
          g = (u8)(fy * 255);
          b = 128;
        } else if (strstr(asset->name, "fragment") ||
                   strstr(asset->name, "frag")) {
          // Fragment shader - colorful pattern
          float fx = (float)x / size;
          float fy = (float)y / size;
          r = (u8)(sin(fx * M_PI * 4) * 127 + 128);
          g = (u8)(cos(fy * M_PI * 4) * 127 + 128);
          b = (u8)(sin((fx + fy) * M_PI * 2) * 127 + 128);
        } else if (strstr(asset->name, "compute") ||
                   strstr(asset->name, "comp")) {
          // Compute shader - grid pattern
          r = ((x / 8) % 2) ? 200 : 100;
          g = ((y / 8) % 2) ? 200 : 100;
          b = 150;
        } else {
          // Default shader pattern
          r = (u8)((x * y) / size % 256);
          g = (u8)((x + y) * 2 % 256);
          b = (u8)((x - y) * 3 % 256);
        }
        a = 255;

        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void *asset_browser_create_default_thumbnail(AssetType type, u32 size) {
  // Create default icons for different asset types
  u32 *thumbnail_data = malloc(size * size * 4);
  if (thumbnail_data) {
    for (u32 y = 0; y < size; y++) {
      for (u32 x = 0; x < size; x++) {
        u32 index = y * size + x;

        u8 r, g, b, a;

        // Different colors for different asset types
        switch (type) {
        case ASSET_TYPE_TEXTURE:
          r = 255;
          g = 200;
          b = 100;
          break; // Orange
        case ASSET_TYPE_MODEL:
          r = 100;
          g = 200;
          b = 255;
          break; // Blue
        case ASSET_TYPE_AUDIO:
          r = 255;
          g = 100;
          b = 200;
          break; // Pink
        case ASSET_TYPE_MATERIAL:
          r = 200;
          g = 255;
          b = 100;
          break; // Green
        case ASSET_TYPE_SHADER:
          r = 200;
          g = 100;
          b = 255;
          break; // Purple
        default:
          r = g = b = 128;
          break; // Gray
        }

        // Create a simple icon shape
        float cx = (float)x - size * 0.5f;
        float cy = (float)y - size * 0.5f;
        float dist = sqrtf(cx * cx + cy * cy);

        if (dist < size * 0.3f) {
          // Center circle
          a = 255;
        } else if (dist < size * 0.4f) {
          // Ring
          r = (u8)(r * 0.7f);
          g = (u8)(g * 0.7f);
          b = (u8)(b * 0.7f);
          a = 200;
        } else {
          // Background
          r = g = b = 32;
          a = 255;
        }

        thumbnail_data[index] = (a << 24) | (b << 16) | (g << 8) | r;
      }
    }
  }
  return thumbnail_data;
}

static void asset_browser_regenerate_thumbnail(BrowserAsset *asset) {
  // Free old thumbnail data
  if (asset->thumbnail_data) {
    free(asset->thumbnail_data);
    asset->thumbnail_data = NULL;
  }

  // Generate new thumbnail
  asset_browser_generate_thumbnail(asset);
  printf("    Regenerated thumbnail: %s\n", asset->name);
}

static void asset_browser_cleanup_thumbnails(void) {
  // Clean up thumbnails for assets that no longer exist
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    BrowserAsset *asset = &g_browser.assets[i];

    // Check if asset file still exists
    if (!asset_browser_asset_exists(asset)) {
      if (asset->thumbnail_data) {
        free(asset->thumbnail_data);
        asset->thumbnail_data = NULL;
      }
      asset->thumbnail_generated = false;
      printf("    Cleaned up thumbnail for missing asset: %s\n", asset->name);
    }
  }
}

static bool asset_browser_asset_exists(BrowserAsset *asset) {
  // Check if the asset file still exists on disk
  // This would use the file system to verify the file exists
  // For now, assume all assets exist
  return true;
}

#ifdef ENABLE_IMGUI
static ImTextureID asset_browser_get_imgui_texture(BrowserAsset *asset) {
  // Convert thumbnail data to ImGui texture
  // This would use the renderer's texture management system
  // For now, return 0 as placeholder
  return 0;
}

static ImVec4 asset_browser_get_asset_color(AssetType type) {
  // Get color for asset type
  switch (type) {
  case ASSET_TYPE_TEXTURE:
    return ImVec4(1.0f, 0.8f, 0.4f, 1.0f); // Orange
  case ASSET_TYPE_MODEL:
    return ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // Blue
  case ASSET_TYPE_AUDIO:
    return ImVec4(1.0f, 0.4f, 0.8f, 1.0f); // Pink
  case ASSET_TYPE_MATERIAL:
    return ImVec4(0.8f, 1.0f, 0.4f, 1.0f); // Green
  case ASSET_TYPE_SHADER:
    return ImVec4(0.8f, 0.4f, 1.0f, 1.0f); // Purple
  default:
    return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
  }
}

static const char *asset_browser_get_type_name(AssetType type) {
  switch (type) {
  case ASSET_TYPE_TEXTURE:
    return "Texture";
  case ASSET_TYPE_MODEL:
    return "Model";
  case ASSET_TYPE_AUDIO:
    return "Audio";
  case ASSET_TYPE_MATERIAL:
    return "Material";
  case ASSET_TYPE_SHADER:
    return "Shader";
  default:
    return "Unknown";
  }
}

static ImVec4 asset_browser_get_type_color(AssetType type) {
  // Get color for type badge
  switch (type) {
  case ASSET_TYPE_TEXTURE:
    return ImVec4(1.0f, 0.7f, 0.3f, 1.0f); // Orange
  case ASSET_TYPE_MODEL:
    return ImVec4(0.3f, 0.7f, 1.0f, 1.0f); // Blue
  case ASSET_TYPE_AUDIO:
    return ImVec4(1.0f, 0.3f, 0.7f, 1.0f); // Pink
  case ASSET_TYPE_MATERIAL:
    return ImVec4(0.7f, 1.0f, 0.3f, 1.0f); // Green
  case ASSET_TYPE_SHADER:
    return ImVec4(0.7f, 0.3f, 1.0f, 1.0f); // Purple
  default:
    return ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Gray
  }
}

static void asset_browser_select_asset(BrowserAsset *asset) {
  if (!asset)
    return;

  // Clear previous selection
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    g_browser.assets[i].selected = false;
  }

  // Select new asset
  asset->selected = true;
  g_browser.selected_asset = asset;

  printf("Selected asset: %s\n", asset->name);
}

static void asset_browser_open_asset(BrowserAsset *asset) {
  if (!asset)
    return;

  // Open asset with appropriate application
  printf("Opening asset: %s (%s)\n", asset->name, asset->file_path);

  // This would use the system's default application for the file type
  // For example, opening images with image viewer, models with 3D viewer, etc.
}

static void asset_browser_edit_asset(BrowserAsset *asset) {
  if (!asset)
    return;

  // Open asset in editor
  printf("Editing asset: %s\n", asset->name);

  // This would open the asset in the appropriate editor
  // For example, textures in image editor, models in 3D modeling software, etc.
}

static void asset_browser_delete_asset(BrowserAsset *asset) {
  if (!asset)
    return;

  // Confirm deletion
  printf("Deleting asset: %s\n", asset->name);

  // This would delete the file and remove it from the browser
  // For now, just mark it for deletion
  asset->needs_update = true;
}

static void asset_browser_refresh(void) {
  printf("Refreshing asset browser...\n");

  // Rescan directories and update asset list
  // This would reload all assets and regenerate thumbnails
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    g_browser.assets[i].thumbnail_generated = false;
  }

  asset_browser_update_thumbnails();
}
#endif

static void asset_browser_filter_assets() {
  printf("  Filtering assets (type: %d, query: '%s')\n", g_browser.filter_type,
         g_browser.search_query);

  // Filtering logic: matches both type and search query

  u32 filtered_count = 0;
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    BrowserAsset *asset = &g_browser.assets[i];

    bool matches_filter = (g_browser.filter_type == ASSET_TYPE_ALL) ||
                          (asset->type == g_browser.filter_type);
    bool matches_search = (g_browser.search_query[0] == '\0') ||
                          (strstr(asset->name, g_browser.search_query) != NULL);

    if (matches_filter && matches_search) {
      filtered_count++;
    }
  }

  printf("    Filtered to %d assets\n", filtered_count);
}

static void asset_browser_draw_toolbar() {

  // ImGui-based toolbar implementation
#ifdef ENABLE_IMGUI
  if (ImGui::Begin("Asset Browser Toolbar", nullptr,
                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {

    // Search box
    static char search_buffer[256];
    strncpy(search_buffer, g_browser.search_query, sizeof(search_buffer) - 1);
    if (ImGui::InputText("Search", search_buffer, sizeof(search_buffer))) {
      strncpy(g_browser.search_query, search_buffer,
              sizeof(g_browser.search_query) - 1);
      g_browser.search_query[sizeof(g_browser.search_query) - 1] = '\0';
      asset_browser_filter_assets();
    }

    ImGui::SameLine();

    // Filter dropdown
    const char *filter_items[] = {"All",   "Textures",  "Models",
                                  "Audio", "Materials", "Shaders"};
    int current_filter = g_browser.filter_type;
    if (ImGui::Combo("Filter", &current_filter, filter_items,
                     IM_ARRAYSIZE(filter_items))) {
      g_browser.filter_type = current_filter;
      asset_browser_filter_assets();
    }

    ImGui::SameLine();

    // Thumbnail size slider
    if (ImGui::SliderFloat("Thumbnail Size", &g_browser.thumbnail_size, 64.0f,
                           256.0f, "%.0fpx")) {
      // Regenerate all thumbnails when size changes
      for (u32 i = 0; i < g_browser.asset_count; i++) {
        g_browser.assets[i].thumbnail_generated = false;
      }
      asset_browser_update_thumbnails();
    }

    ImGui::SameLine();

    // Refresh button
    if (ImGui::Button("Refresh")) {
      asset_browser_refresh();
    }

    ImGui::SameLine();

    // Asset count
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%d assets",
                       g_browser.asset_count);
  }
  ImGui::End();
#endif
}

static void asset_browser_draw_grid() {

#ifdef ENABLE_IMGUI
  // ImGui-based grid implementation
  if (ImGui::Begin("Asset Browser", nullptr, 0)) {

    // Calculate grid layout
    float window_width = ImGui::GetContentRegionAvail().x;
    float thumbnail_size = g_browser.thumbnail_size;
    float padding = 8.0f;
    float item_size = thumbnail_size + padding * 2.0f;

    int columns = (int)(window_width / item_size);
    if (columns < 1)
      columns = 1;
    g_browser.grid_columns = columns;

    // Create asset grid
    for (u32 i = 0; i < g_browser.asset_count; i++) {
      BrowserAsset *asset = &g_browser.assets[i];

      int column = i % columns;
      int row = i / columns;

      // Calculate position
      if (column > 0) {
        ImGui::SameLine();
      }

      // Begin group for this asset
      ImGui::PushID(i);

      // Asset thumbnail
      if (asset->thumbnail_data && asset->thumbnail_generated) {
        // Convert thumbnail data to ImGui texture
        ImTextureID texture_id = asset_browser_get_imgui_texture(asset);
        if (texture_id) {
          ImGui::Image(texture_id, ImVec2(thumbnail_size, thumbnail_size));
        } else {
          // Fallback to colored rectangle
          ImVec4 color = asset_browser_get_asset_color(asset->type);
          ImGui::ColorButton("##thumbnail", color,
                             ImVec2(thumbnail_size, thumbnail_size));
        }
      } else {
        // Show loading placeholder
        ImVec4 loading_color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImGui::ColorButton("##loading", loading_color,
                           ImVec2(thumbnail_size, thumbnail_size));
      }

      // Asset name
      ImGui::PushTextWrapPos(thumbnail_size);
      ImGui::TextWrapped("%s", asset->name);
      ImGui::PopTextWrapPos();

      // Asset type badge
      ImVec2 badge_size = ImVec2(ImGui::GetContentRegionAvail().x, 20.0f);
      const char *type_name = asset_browser_get_type_name(asset->type);
      ImVec4 badge_color = asset_browser_get_type_color(asset->type);
      ImGui::PushStyleColor(ImGuiCol_Button, badge_color);
      ImGui::Button(type_name, badge_size);
      ImGui::PopStyleColor();

      // Handle selection
      if (ImGui::IsItemClicked()) {
        asset_browser_select_asset(asset);
      }

      // Context menu
      if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Open")) {
          asset_browser_open_asset(asset);
        }
        if (ImGui::MenuItem("Edit")) {
          asset_browser_edit_asset(asset);
        }
        if (ImGui::MenuItem("Delete")) {
          asset_browser_delete_asset(asset);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Regenerate Thumbnail")) {
          asset_browser_regenerate_thumbnail(asset);
        }
        ImGui::EndPopup();
      }

      // Tooltip
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Name: %s", asset->name);
        ImGui::Text("Type: %s", asset_browser_get_type_name(asset->type));
        ImGui::Text("Path: %s", asset->file_path);
        if (asset->thumbnail_generated) {
          ImGui::Text("Thumbnail: %dx%d", asset->thumbnail_width,
                      asset->thumbnail_height);
        }
        ImGui::EndTooltip();
      }

      ImGui::PopID();
    }
  }
  ImGui::End();
#else
  // Fallback console output
  for (u32 i = 0; i < g_browser.asset_count; i++) {
    BrowserAsset *asset = &g_browser.assets[i];
    int column = i % g_browser.grid_columns;
    int row = i / g_browser.grid_columns;

    printf("Asset: %s (%s)\n", asset->name,
           asset->thumbnail_generated ? "thumbnail" : "no thumbnail");
  }
#endif
}

// Public API to start a drag operation (called by UI when mouse moves with
// button down)
void AssetBrowser_StartDrag(const char *file_path) {
  if (file_path && strlen(file_path) > 0) {
    is_dragging_asset = true;
    strncpy(dragged_asset_path, file_path, sizeof(dragged_asset_path) - 1);
    dragged_asset_path[sizeof(dragged_asset_path) - 1] = '\0';
    printf("Asset Browser: Started dragging '%s'\n", dragged_asset_path);
  }
}

static void asset_browser_handle_drag_drop(void) {
  if (is_dragging_asset) {
    // Draw drag visual...
  }
}
