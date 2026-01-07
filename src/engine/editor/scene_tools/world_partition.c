#include "editor/scene_tools/world_partition.h"
#include "core/memory.h"
#include "core/logger.h"
#include <stdlib.h>
#include <include/math/math.h>

void world_partition_init(WorldPartitionContext* ctx, int cell_size, int count_x, int count_y) {
    if (!ctx) return;
    
    ctx->grid_size = cell_size;
    ctx->cell_count_x = count_x;
    ctx->cell_count_y = count_y;
    ctx->loading_range = 2; // Default 2 cells radius
    ctx->origin = (Vec3){0,0,0};
    
    int total_cells = count_x * count_y;
    ctx->cells = (PartitionCell*)calloc(total_cells, sizeof(PartitionCell));
    
    for (int y = 0; y < count_y; y++) {
        for (int x = 0; x < count_x; x++) {
            int idx = y * count_x + x;
            ctx->cells[idx].x = x;
            ctx->cells[idx].y = y;
            ctx->cells[idx].is_loaded = false;
        }
    }
    LOG_INFO("World Partition Initialized: %dx%d grid (Size: %d)", count_x, count_y, cell_size);
}

void world_partition_destroy(WorldPartitionContext* ctx) {
    if (!ctx) return;
    if (ctx->cells) {
        free(ctx->cells);
        ctx->cells = NULL;
    }
}

void world_partition_get_cell_coords(WorldPartitionContext* ctx, Vec3 pos, int* x, int* y) {
    if (!ctx) return;
    // Simple 2D grid on XZ plane
    float local_x = pos.x - ctx->origin.x;
    float local_z = pos.z - ctx->origin.z;
    
    *x = (int)floorf(local_x / ctx->grid_size);
    *y = (int)floorf(local_z / ctx->grid_size); // Mapping Z to Y index
}

void world_partition_update(WorldPartitionContext* ctx, Vec3 viewer_pos) {
    if (!ctx) return;
    
    int center_x, center_y;
    world_partition_get_cell_coords(ctx, viewer_pos, &center_x, &center_y);
    
    int range = ctx->loading_range;
    
    // Bounds check iteration
    int start_x = center_x - range;
    int end_x = center_x + range;
    int start_y = center_y - range;
    int end_y = center_y + range;
    
    // Unload far cells
    for (int i = 0; i < ctx->cell_count_x * ctx->cell_count_y; i++) {
        PartitionCell* cell = &ctx->cells[i];
        if (cell->is_loaded) {
            // Check distance
            int dx = abs(cell->x - center_x);
            int dy = abs(cell->y - center_y);
            if (dx > range || dy > range) {
                world_partition_unload_cell(ctx, cell->x, cell->y);
            }
        }
    }
    
    // Load near cells
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            if (x >= 0 && x < ctx->cell_count_x && y >= 0 && y < ctx->cell_count_y) {
                int idx = y * ctx->cell_count_x + x;
                if (!ctx->cells[idx].is_loaded) {
                    world_partition_load_cell(ctx, x, y);
                }
            }
        }
    }
}

void world_partition_load_cell(WorldPartitionContext* ctx, int x, int y) {
    int idx = y * ctx->cell_count_x + x;
    if (idx < 0 || idx >= ctx->cell_count_x * ctx->cell_count_y) return;
    
    ctx->cells[idx].is_loaded = true;
    // Signal game to load cell content
    LOG_INFO("World Partition: Loaded Cell [%d, %d]", x, y);
}

void world_partition_unload_cell(WorldPartitionContext* ctx, int x, int y) {
    int idx = y * ctx->cell_count_x + x;
    if (idx < 0 || idx >= ctx->cell_count_x * ctx->cell_count_y) return;
    
    ctx->cells[idx].is_loaded = false;
    LOG_INFO("World Partition: Unloaded Cell [%d, %d]", x, y);
}

// Kept for compatibility if needed by old calls, but routed to new API
void world_partition_create_grid(int grid_size) {
    // Legacy stub wrapper, shouldn't be used
}
