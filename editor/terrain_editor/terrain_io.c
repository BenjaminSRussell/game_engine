#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                           TERRAIN I/O
 * =================================================================================================
 * 
 * Handles importing and exporting terrain heightmaps.
 * Formats: 
 *  - .RAW (Binary float array)
 *  - .PNG (Grayscale image - using stub or engine integration)
 */

bool terrain_export_raw(TerrainHeightmap* heightmap, const char* filepath) {
    if (!heightmap || !filepath) return false;
    
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        LOG_ERROR("Failed to open file for export: %s", filepath);
        return false;
    }
    
    // Header (Simple custom header)
    u32 header[2] = {heightmap->width, heightmap->height};
    fwrite(header, sizeof(u32), 2, file);
    
    // Data
    size_t data_size = heightmap->width * heightmap->height;
    size_t written = fwrite(heightmap->heights, sizeof(f32), data_size, file);
    
    fclose(file);
    
    if (written != data_size) {
        LOG_ERROR("Failed to write complete heightmap data");
        return false;
    }
    
    LOG_INFO("Exported terrain RAW to %s (%ux%u)", filepath, heightmap->width, heightmap->height);
    return true;
}

bool terrain_import_raw(TerrainHeightmap* heightmap, const char* filepath) {
    if (!heightmap || !filepath) return false;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        LOG_ERROR("Failed to open file for import: %s", filepath);
        return false;
    }
    
    // Header
    u32 header[2];
    if (fread(header, sizeof(u32), 2, file) != 2) {
        fclose(file);
        LOG_ERROR("Invalid raw terrain file header");
        return false;
    }
    
    u32 width = header[0];
    u32 height = header[1];
    
    // If dimensions match, just overwrite data
    // If not, we might need to verify if caller handles resize, or just error out for this simple impl
    if (width != heightmap->width || height != heightmap->height) {
        LOG_WARN("Import dimensions (%ux%u) differ from current (%ux%u). Creating new buffer...", 
            width, height, heightmap->width, heightmap->height);
            
        // Reallocate
        if (heightmap->heights) free(heightmap->heights);
        heightmap->heights = (f32*)malloc(sizeof(f32) * width * height);
        heightmap->width = width;
        heightmap->height = height;
    }
    
    size_t data_size = width * height;
    size_t read = fread(heightmap->heights, sizeof(f32), data_size, file);
    
    fclose(file);
    
    if (read != data_size) {
        LOG_ERROR("Incomplete read of heightmap data");
        return false;
    }
    
    LOG_INFO("Imported terrain RAW from %s", filepath);
    return true;
}

// Stub for PNG export (requires image lib)
bool terrain_export_png(TerrainHeightmap* heightmap, const char* filepath) {
    LOG_WARN("PNG export not yet implemented (requires image library)");
    return false;
}
