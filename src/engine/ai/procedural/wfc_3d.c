#include "ai/ai_types.h"
#include "core/core.h"
#include <stdlib.h>
#include <string.h>

#define MAX_MODULES 32
#define GRID_SIZE_3D 16

typedef struct {
    uint32_t allowed_modules; // Bitmask
    int collapsed;
    int module_id;
} WFCSlot;

typedef struct {
    WFCSlot grid[GRID_SIZE_3D][GRID_SIZE_3D][GRID_SIZE_3D];
    uint32_t adjacency_rules[MAX_MODULES][6]; // 6 directions, bitmask of allowed neighbors
    int size;
} WFC3D;

static WFC3D g_wfc = {0};

void wfc_init(int size) {
    g_wfc.size = size;
    uint32_t all = (1 << MAX_MODULES) - 1;
    for (int z = 0; z < size; z++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                g_wfc.grid[z][y][x].allowed_modules = all;
                g_wfc.grid[z][y][x].collapsed = 0;
            }
        }
    }
}

int wfc_entropy(WFCSlot *slot) {
    int count = 0;
    for (int i = 0; i < MAX_MODULES; i++) {
        if (slot->allowed_modules & (1 << i)) count++;
    }
    return count;
}

void wfc_collapse_slot(int x, int y, int z) {
    WFCSlot *slot = &g_wfc.grid[z][y][x];
    if (slot->collapsed) return;
    
    // Pick random allowed module
    int candidates[MAX_MODULES], count = 0;
    for (int i = 0; i < MAX_MODULES; i++) {
        if (slot->allowed_modules & (1 << i)) candidates[count++] = i;
    }
    
    if (count > 0) {
        slot->module_id = candidates[rand() % count];
        slot->collapsed = 1;
        slot->allowed_modules = 1 << slot->module_id;
    }
}

void wfc_propagate(int x, int y, int z) {
    // Update neighbors based on adjacency rules
    // Simplified - would need full constraint propagation
    (void)x; (void)y; (void)z;
}

void wfc_generate() {
    while(1) {
        // Find min entropy slot
        int min_entropy = MAX_MODULES + 1;
        int mx = 0, my = 0, mz = 0;
        int all_collapsed = 1;
        
        for (int z = 0; z < g_wfc.size; z++) {
            for (int y = 0; y < g_wfc.size; y++) {
                for (int x = 0; x < g_wfc.size; x++) {
                    WFCSlot *s = &g_wfc.grid[z][y][x];
                    if (!s->collapsed) {
                        all_collapsed = 0;
                        int e = wfc_entropy(s);
                        if (e < min_entropy) {
                            min_entropy = e;
                            mx = x; my = y; mz = z;
                        }
                    }
                }
            }
        }
        
        if (all_collapsed || min_entropy == 0) break;
        
        wfc_collapse_slot(mx, my, mz);
        wfc_propagate(mx, my, mz);
    }
}
