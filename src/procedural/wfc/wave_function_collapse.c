/*
 * wave_function_collapse.c
 * Wave Function Collapse algorithm implementation
 *
 * Part of the Procedural Generation subsystem
 * Advanced 3D Rendering Engine
 */

#include "wave_function_collapse.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

WFCGenerator* wfc_create_generator(uint32_t width, uint32_t height, uint32_t depth, bool is_3d) {
    WFCGenerator* gen = malloc(sizeof(WFCGenerator));
    if (!gen) return NULL;
    
    gen->width = width;
    gen->height = height;
    gen->depth = depth;
    gen->is_3d = is_3d;
    gen->total_cells = width * height * (is_3d ? depth : 1);
    gen->periodic_boundary = false;
    gen->seed = 12345;
    gen->is_complete = false;
    gen->collapsed_count = 0;
    
    gen->cells = malloc(gen->total_cells * sizeof(WFCCell));
    if (!gen->cells) {
        free(gen);
        return NULL;
    }
    
    gen->tileset = NULL;
    return gen;
}

bool wfc_set_tileset(WFCGenerator* gen, const WFCTileset* tileset) {
    if (!gen || !tileset) return false;
    
    gen->tileset = malloc(sizeof(WFCTileset));
    if (!gen->tileset) return false;
    
    *gen->tileset = *tileset;
    return wfc_initialize(gen);
}

bool wfc_initialize(WFCGenerator* gen) {
    if (!gen || !gen->tileset) return false;
    
    for (uint32_t i = 0; i < gen->total_cells; i++) {
        WFCCell* cell = &gen->cells[i];
        cell->possible_count = gen->tileset->tile_count;
        cell->possible_tiles = malloc(cell->possible_count * sizeof(uint32_t));
        
        for (uint32_t j = 0; j < cell->possible_count; j++) {
            cell->possible_tiles[j] = j;
        }
        
        cell->is_collapsed = false;
        cell->collapsed_tile = 0;
        cell->entropy = logf(cell->possible_count);
        
        uint32_t x, y, z;
        wfc_get_cell_coords(gen, i, &x, &y, &z);
        cell->x = x;
        cell->y = y;
        cell->z = z;
    }
    
    return true;
}

uint32_t wfc_find_min_entropy_cell(const WFCGenerator* gen) {
    if (!gen) return gen->total_cells;
    
    uint32_t min_cell = gen->total_cells;
    float min_entropy = INFINITY;
    
    for (uint32_t i = 0; i < gen->total_cells; i++) {
        const WFCCell* cell = &gen->cells[i];
        if (!cell->is_collapsed && cell->entropy < min_entropy) {
            min_entropy = cell->entropy;
            min_cell = i;
        }
    }
    
    return min_cell;
}

bool wfc_collapse_cell(WFCGenerator* gen, uint32_t cell_index, uint32_t tile_id) {
    if (!gen || cell_index >= gen->total_cells) return false;
    
    WFCCell* cell = &gen->cells[cell_index];
    if (cell->is_collapsed) return false;
    
    cell->is_collapsed = true;
    cell->collapsed_tile = tile_id;
    cell->possible_count = 1;
    cell->entropy = 0.0f;
    gen->collapsed_count++;
    
    return wfc_propagate_constraints(gen, cell_index);
}

bool wfc_propagate_constraints(WFCGenerator* gen, uint32_t changed_cell) {
    if (!gen) return false;
    
    // Simplified propagation - would be more sophisticated in production
    for (uint32_t dir = 0; dir < 6; dir++) {
        uint32_t neighbor = wfc_get_neighbor_cell(gen, changed_cell, dir);
        if (neighbor < gen->total_cells && !gen->cells[neighbor].is_collapsed) {
            wfc_update_cell_constraints(gen, neighbor);
        }
    }
    
    return true;
}

bool wfc_step(WFCGenerator* gen) {
    if (!gen || gen->is_complete) return false;
    
    uint32_t cell_index = wfc_find_min_entropy_cell(gen);
    if (cell_index >= gen->total_cells) {
        gen->is_complete = true;
        return true;
    }
    
    WFCCell* cell = &gen->cells[cell_index];
    if (cell->possible_count == 0) return false; // Contradiction
    
    // Random selection based on weights
    float total_weight = 0.0f;
    for (uint32_t i = 0; i < cell->possible_count; i++) {
        uint32_t tile_id = cell->possible_tiles[i];
        total_weight += gen->tileset->tiles[tile_id].weight;
    }
    
    float random = (float)(rand() % 1000) / 1000.0f * total_weight;
    float current_weight = 0.0f;
    uint32_t selected_tile = cell->possible_tiles[0];
    
    for (uint32_t i = 0; i < cell->possible_count; i++) {
        uint32_t tile_id = cell->possible_tiles[i];
        current_weight += gen->tileset->tiles[tile_id].weight;
        if (current_weight >= random) {
            selected_tile = tile_id;
            break;
        }
    }
    
    return wfc_collapse_cell(gen, cell_index, selected_tile);
}

bool wfc_collapse_to_completion(WFCGenerator* gen) {
    if (!gen) return false;
    
    while (!gen->is_complete) {
        if (!wfc_step(gen)) {
            return false; // Failed to collapse
        }
    }
    
    return true;
}

uint32_t wfc_get_cell_index(const WFCGenerator* gen, uint32_t x, uint32_t y, uint32_t z) {
    if (!gen) return 0;
    return x + y * gen->width + z * gen->width * gen->height;
}

bool wfc_get_cell_coords(const WFCGenerator* gen, uint32_t index, uint32_t* x, uint32_t* y, uint32_t* z) {
    if (!gen || !x || !y || !z || index >= gen->total_cells) return false;
    
    *x = index % gen->width;
    *y = (index / gen->width) % gen->height;
    *z = index / (gen->width * gen->height);
    return true;
}

uint32_t wfc_get_neighbor_cell(const WFCGenerator* gen, uint32_t cell_index, uint32_t direction) {
    if (!gen || cell_index >= gen->total_cells || direction > 5) {
        return gen->total_cells;
    }
    
    uint32_t x, y, z;
    wfc_get_cell_coords(gen, cell_index, &x, &y, &z);
    
    switch (direction) {
        case 0: x++; break; // +X
        case 1: x--; break; // -X
        case 2: y++; break; // +Y
        case 3: y--; break; // -Y
        case 4: z++; break; // +Z
        case 5: z--; break; // -Z
    }
    
    if (gen->periodic_boundary) {
        x = x % gen->width;
        y = y % gen->height;
        z = z % gen->depth;
    } else if (x >= gen->width || y >= gen->height || z >= gen->depth) {
        return gen->total_cells;
    }
    
    return wfc_get_cell_index(gen, x, y, z);
}

void wfc_destroy_generator(WFCGenerator* gen) {
    if (!gen) return;
    
    if (gen->cells) {
        for (uint32_t i = 0; i < gen->total_cells; i++) {
            free(gen->cells[i].possible_tiles);
        }
        free(gen->cells);
    }
    
    free(gen->tileset);
    free(gen);
}
