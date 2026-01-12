#include "geometry/lod/lod_generator.h"
#include "geometry/lod/lod_selector.h"
#include "geometry/mesh.h"
#include "geometry/geometry_types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// ============================================================================
// CONSTANTS AND DEFAULTS
// ============================================================================

#define LOD_SYSTEM_MAX_MESHES 10000
#define LOD_SYSTEM_DEFAULT_HYSTERESIS 0.1f
#define LOD_SYSTEM_MIN_SCREEN_SIZE 0.001f
#define LOD_SYSTEM_MAX_SCREEN_SIZE 1.0f

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct lod_mesh_entry_t {
    mesh_t* original_mesh;
    lod_chain_t* lod_chain;
    u32 mesh_id;
    char name[64];
    bool auto_generated;
    f32 last_switch_distance;
    u32 current_lod;
    bool dirty;
} lod_mesh_entry_t;

typedef struct lod_system_state_t {
    bool initialized;
    lod_mesh_entry_t* mesh_entries;
    u32 mesh_count;
    u32 mesh_capacity;
    u32 next_mesh_id;
    
    // Global settings
    f32 global_hysteresis;
    f32 global_bias;
    bool force_lod;
    u32 forced_lod_level;
    
    // Statistics
    u32 total_lod_switches;
    f32 average_lod_level;
    u64 last_frame_time;
} lod_system_state_t;

static lod_system_state_t g_lod_system = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static u32 find_mesh_entry_by_id(u32 mesh_id) {
    for (u32 i = 0; i < g_lod_system.mesh_count; i++) {
        if (g_lod_system.mesh_entries[i].mesh_id == mesh_id) {
            return i;
        }
    }
    return UINT32_MAX;
}

static u32 find_mesh_entry_by_name(const char* name) {
    if (!name) return UINT32_MAX;
    
    for (u32 i = 0; i < g_lod_system.mesh_count; i++) {
        if (strcmp(g_lod_system.mesh_entries[i].name, name) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

static bool add_mesh_entry(const mesh_t* mesh, const char* name, bool auto_generate) {
    if (g_lod_system.mesh_count >= g_lod_system.mesh_capacity) {
        // Resize array
        u32 new_capacity = g_lod_system.mesh_capacity * 2;
        lod_mesh_entry_t* new_entries = realloc(g_lod_system.mesh_entries, new_capacity * sizeof(lod_mesh_entry_t));
        if (!new_entries) return false;
        
        g_lod_system.mesh_entries = new_entries;
        g_lod_system.mesh_capacity = new_capacity;
    }
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[g_lod_system.mesh_count];
    memset(entry, 0, sizeof(lod_mesh_entry_t));
    
    entry->original_mesh = (mesh_t*)mesh; // Store reference
    entry->mesh_id = g_lod_system.next_mesh_id++;
    entry->auto_generated = auto_generate;
    entry->current_lod = 0;
    entry->last_switch_distance = 1.0f;
    
    if (name) {
        strncpy(entry->name, name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
    } else {
        snprintf(entry->name, sizeof(entry->name), "mesh_%u", entry->mesh_id);
    }
    
    g_lod_system.mesh_count++;
    return true;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

int lod_system_init(void) {
    if (g_lod_system.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_lod_system, 0, sizeof(lod_system_state_t));
    
    g_lod_system.mesh_capacity = 1024;
    g_lod_system.mesh_entries = malloc(g_lod_system.mesh_capacity * sizeof(lod_mesh_entry_t));
    if (!g_lod_system.mesh_entries) {
        return -1;
    }
    
    g_lod_system.global_hysteresis = LOD_SYSTEM_DEFAULT_HYSTERESIS;
    g_lod_system.global_bias = 0.0f;
    g_lod_system.force_lod = false;
    g_lod_system.forced_lod_level = 0;
    
    g_lod_system.initialized = true;
    
    printf("LOD System initialized with capacity %u\n", g_lod_system.mesh_capacity);
    return 0;
}

void lod_system_shutdown(void) {
    if (!g_lod_system.initialized) return;
    
    // Destroy all LOD chains
    for (u32 i = 0; i < g_lod_system.mesh_count; i++) {
        lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[i];
        if (entry->lod_chain) {
            lod_chain_destroy(entry->lod_chain);
        }
    }
    
    free(g_lod_system.mesh_entries);
    memset(&g_lod_system, 0, sizeof(lod_system_state_t));
    
    printf("LOD System shutdown\n");
}

u32 lod_system_register_mesh(const mesh_t* mesh, const char* name) {
    if (!mesh || !g_lod_system.initialized) return UINT32_MAX;
    
    if (!add_mesh_entry(mesh, name, false)) {
        return UINT32_MAX;
    }
    
    return g_lod_system.mesh_entries[g_lod_system.mesh_count - 1].mesh_id;
}

bool lod_system_unregister_mesh(u32 mesh_id) {
    if (!g_lod_system.initialized) return false;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return false;
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    // Destroy LOD chain if it exists
    if (entry->lod_chain) {
        lod_chain_destroy(entry->lod_chain);
        entry->lod_chain = NULL;
    }
    
    // Remove entry by shifting remaining entries
    for (u32 i = index; i < g_lod_system.mesh_count - 1; i++) {
        g_lod_system.mesh_entries[i] = g_lod_system.mesh_entries[i + 1];
    }
    
    g_lod_system.mesh_count--;
    return true;
}

bool lod_system_generate_lods(u32 mesh_id, const lod_generation_config_t* config) {
    if (!g_lod_system.initialized) return false;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return false;
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    // Generate LOD chain
    entry->lod_chain = lod_generate_chain_advanced(entry->original_mesh, config);
    if (!entry->lod_chain) {
        printf("Failed to generate LODs for mesh %u\n", mesh_id);
        return false;
    }
    
    entry->auto_generated = true;
    printf("Generated %u LOD levels for mesh %u (%s)\n", 
           entry->lod_chain->lod_count, mesh_id, entry->name);
    
    return true;
}

bool lod_system_generate_lods_simple(u32 mesh_id, const f32* ratios, u32 count) {
    if (!g_lod_system.initialized) return false;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return false;
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    // Generate LOD chain
    entry->lod_chain = lod_generate_chain(entry->original_mesh, ratios, count);
    if (!entry->lod_chain) {
        printf("Failed to generate LODs for mesh %u\n", mesh_id);
        return false;
    }
    
    entry->auto_generated = true;
    printf("Generated %u LOD levels for mesh %u (%s)\n", 
           entry->lod_chain->lod_count, mesh_id, entry->name);
    
    return true;
}

mesh_t* lod_system_select_lod(u32 mesh_id, f32 screen_size) {
    if (!g_lod_system.initialized) return NULL;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return NULL;
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    // If no LOD chain exists, return original mesh
    if (!entry->lod_chain) {
        return entry->original_mesh;
    }
    
    // Apply global bias
    f32 adjusted_screen_size = screen_size + g_lod_system.global_bias;
    
    // Force specific LOD level if requested
    if (g_lod_system.force_lod) {
        u32 lod_level = fminf(g_lod_system.forced_lod_level, entry->lod_chain->lod_count - 1);
        return entry->lod_chain->lods[lod_level];
    }
    
    // Find appropriate LOD level
    u32 selected_lod = 0;
    f32 hysteresis = g_lod_system.global_hysteresis;
    
    // Apply hysteresis to prevent rapid switching
    f32 effective_threshold = entry->last_switch_distance * (1.0f - hysteresis);
    
    for (u32 i = entry->lod_chain->lod_count - 1; i > 0; i--) {
        f32 threshold = entry->lod_chain->switch_distances[i];
        
        if (adjusted_screen_size < threshold) {
            selected_lod = i;
            break;
        }
    }
    
    // Update current LOD if changed
    if (selected_lod != entry->current_lod) {
        entry->current_lod = selected_lod;
        entry->last_switch_distance = entry->lod_chain->switch_distances[selected_lod];
        g_lod_system.total_lod_switches++;
    }
    
    return entry->lod_chain->lods[selected_lod];
}

u32 lod_system_get_current_lod(u32 mesh_id) {
    if (!g_lod_system.initialized) return 0;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return 0;
    
    return g_lod_system.mesh_entries[index].current_lod;
}

bool lod_system_set_lod_chain(u32 mesh_id, lod_chain_t* chain) {
    if (!g_lod_system.initialized || !chain) return false;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return false;
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    // Destroy existing chain if present
    if (entry->lod_chain) {
        lod_chain_destroy(entry->lod_chain);
    }
    
    entry->lod_chain = chain;
    entry->auto_generated = false;
    
    return true;
}

lod_chain_t* lod_system_get_lod_chain(u32 mesh_id) {
    if (!g_lod_system.initialized) return NULL;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) return NULL;
    
    return g_lod_system.mesh_entries[index].lod_chain;
}

// ============================================================================
// GLOBAL SETTINGS
// ============================================================================

void lod_system_set_global_hysteresis(f32 hysteresis) {
    if (hysteresis >= 0.0f && hysteresis <= 1.0f) {
        g_lod_system.global_hysteresis = hysteresis;
    }
}

void lod_system_set_global_bias(f32 bias) {
    g_lod_system.global_bias = bias;
}

void lod_system_force_lod_level(u32 level) {
    g_lod_system.force_lod = true;
    g_lod_system.forced_lod_level = level;
}

void lod_system_disable_force_lod(void) {
    g_lod_system.force_lod = false;
}

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

void lod_system_get_statistics(u32* total_meshes, u32* meshes_with_lods, u32* total_lod_switches) {
    if (!g_lod_system.initialized) return;
    
    if (total_meshes) *total_meshes = g_lod_system.mesh_count;
    
    if (meshes_with_lods) {
        u32 count = 0;
        for (u32 i = 0; i < g_lod_system.mesh_count; i++) {
            if (g_lod_system.mesh_entries[i].lod_chain) {
                count++;
            }
        }
        *meshes_with_lods = count;
    }
    
    if (total_lod_switches) *total_lod_switches = g_lod_system.total_lod_switches;
}

void lod_system_print_statistics(void) {
    if (!g_lod_system.initialized) {
        printf("LOD System not initialized\n");
        return;
    }
    
    u32 meshes_with_lods = 0;
    for (u32 i = 0; i < g_lod_system.mesh_count; i++) {
        if (g_lod_system.mesh_entries[i].lod_chain) {
            meshes_with_lods++;
        }
    }
    
    printf("LOD System Statistics:\n");
    printf("  Total registered meshes: %u\n", g_lod_system.mesh_count);
    printf("  Meshes with LODs: %u\n", meshes_with_lods);
    printf("  Total LOD switches: %u\n", g_lod_system.total_lod_switches);
    printf("  Global hysteresis: %.3f\n", g_lod_system.global_hysteresis);
    printf("  Global bias: %.3f\n", g_lod_system.global_bias);
    printf("  Force LOD: %s\n", g_lod_system.force_lod ? "Yes" : "No");
    if (g_lod_system.force_lod) {
        printf("  Forced LOD level: %u\n", g_lod_system.forced_lod_level);
    }
}

void lod_system_print_mesh_info(u32 mesh_id) {
    if (!g_lod_system.initialized) return;
    
    u32 index = find_mesh_entry_by_id(mesh_id);
    if (index == UINT32_MAX) {
        printf("Mesh %u not found\n", mesh_id);
        return;
    }
    
    lod_mesh_entry_t* entry = &g_lod_system.mesh_entries[index];
    
    printf("Mesh Info for ID %u (%s):\n", mesh_id, entry->name);
    printf("  Mesh ID: %u\n", entry->mesh_id);
    printf("  Current LOD: %u\n", entry->current_lod);
    printf("  Auto-generated: %s\n", entry->auto_generated ? "Yes" : "No");
    
    if (entry->lod_chain) {
        printf("  LOD Count: %u\n", entry->lod_chain->lod_count);
        printf("  LOD Levels:\n");
        for (u32 i = 0; i < entry->lod_chain->lod_count; i++) {
            mesh_t* lod_mesh = entry->lod_chain->lods[i];
            printf("    LOD %u: %u vertices, %u triangles, switch at %.3f\n",
                   i, lod_mesh->vertex_count, lod_mesh->index_count / 3,
                   entry->lod_chain->switch_distances[i]);
        }
    } else {
        printf("  No LOD chain generated\n");
    }
}
