/*
 * lod_memory.c
 * LOD memory management and budgeting
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

#define MAX_LOD_CHAINS 1024

static struct {
    bool initialized;
    lod_memory_budget_t budget;
    lod_memory_stats_t stats;
    lod_memory_entry_t entries[MAX_LOD_CHAINS];
    uint32_t entry_count;
} g_lod_memory = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int lod_memory_init(const lod_memory_budget_t* budget) {
    if (g_lod_memory.initialized) {
        return -1; // Already initialized
    }
    
    if (budget) {
        g_lod_memory.budget = *budget;
    } else {
        // Default budget: 512MB total
        g_lod_memory.budget.total_budget_bytes = 512 * 1024 * 1024;
        g_lod_memory.budget.cpu_budget_bytes = 256 * 1024 * 1024;
        g_lod_memory.budget.gpu_budget_bytes = 256 * 1024 * 1024;
        g_lod_memory.budget.eviction_threshold = 0.9f;
        g_lod_memory.budget.allow_over_budget = true;
    }
    
    memset(&g_lod_memory.stats, 0, sizeof(lod_memory_stats_t));
    g_lod_memory.entry_count = 0;
    g_lod_memory.initialized = true;
    
    return 0;
}

void lod_memory_shutdown(void) {
    if (!g_lod_memory.initialized) return;
    
    // Free all tracked memory
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        // In production, would free GPU/CPU memory here
    }
    
    memset(&g_lod_memory, 0, sizeof(g_lod_memory));
}

/* ============================================================================
 * BUDGET MANAGEMENT
 * ============================================================================ */

void lod_memory_set_budget(const lod_memory_budget_t* budget) {
    if (!budget) return;
    g_lod_memory.budget = *budget;
}

lod_memory_budget_t lod_memory_get_budget(void) {
    return g_lod_memory.budget;
}

lod_memory_stats_t lod_memory_get_stats(void) {
    // Update usage percentage
    g_lod_memory.stats.budget_usage_percent = 
        (float)g_lod_memory.stats.total_allocated / 
        (float)g_lod_memory.budget.total_budget_bytes * 100.0f;
    
    return g_lod_memory.stats;
}

/* ============================================================================
 * MEMORY ESTIMATION
 * ============================================================================ */

size_t lod_memory_estimate_mesh_size(const mesh_data_t* mesh) {
    if (!mesh) return 0;
    
    size_t total = 0;
    total += mesh->vertex_data_size;
    total += mesh->index_data_size;
    total += mesh->submesh_count * sizeof(submesh_t);
    total += sizeof(mesh_data_t);
    
    return total;
}

size_t lod_memory_estimate_chain_size(const lod_chain_t* chain) {
    if (!chain) return 0;
    
    size_t total = sizeof(lod_chain_t);
    
    for (uint32_t i = 0; i < chain->lod_count; i++) {
        if (chain->lods[i]) {
            // Estimate based on GPU data structure
            total += sizeof(mesh_gpu_data_t);
            total += chain->lods[i]->vertex_count * chain->lods[i]->vertex_stride;
            total += chain->lods[i]->index_count * 
                     (chain->lods[i]->index_size > 0 ? chain->lods[i]->index_size : 4);
        }
    }
    
    return total;
}

/* ============================================================================
 * LOD CHAIN TRACKING
 * ============================================================================ */

int lod_memory_register_chain(uint32_t chain_id, const lod_chain_t* chain) {
    if (!g_lod_memory.initialized || !chain) return -1;
    if (g_lod_memory.entry_count >= MAX_LOD_CHAINS) return -1;
    
    size_t chain_size = lod_memory_estimate_chain_size(chain);
    
    // Add entry
    lod_memory_entry_t* entry = &g_lod_memory.entries[g_lod_memory.entry_count++];
    entry->chain_id = chain_id;
    entry->cpu_size = 0; // Assume GPU-only for now
    entry->gpu_size = chain_size;
    entry->last_access_frame = 0;
    entry->priority = 1.0f;
    entry->pinned = false;
    
    // Update stats
    g_lod_memory.stats.gpu_allocated += chain_size;
    g_lod_memory.stats.total_allocated += chain_size;
    g_lod_memory.stats.lod_chain_count++;
    g_lod_memory.stats.total_lod_meshes += chain->lod_count;
    
    return 0;
}

void lod_memory_unregister_chain(uint32_t chain_id) {
    if (!g_lod_memory.initialized) return;
    
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        if (g_lod_memory.entries[i].chain_id == chain_id) {
            // Update stats
            g_lod_memory.stats.gpu_allocated -= g_lod_memory.entries[i].gpu_size;
            g_lod_memory.stats.cpu_allocated -= g_lod_memory.entries[i].cpu_size;
            g_lod_memory.stats.total_allocated -= 
                (g_lod_memory.entries[i].gpu_size + g_lod_memory.entries[i].cpu_size);
            g_lod_memory.stats.lod_chain_count--;
            
            // Remove entry (swap with last)
            g_lod_memory.entries[i] = g_lod_memory.entries[g_lod_memory.entry_count - 1];
            g_lod_memory.entry_count--;
            break;
        }
    }
}

void lod_memory_update_access(uint32_t chain_id, uint64_t frame_number) {
    if (!g_lod_memory.initialized) return;
    
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        if (g_lod_memory.entries[i].chain_id == chain_id) {
            g_lod_memory.entries[i].last_access_frame = frame_number;
            break;
        }
    }
}

/* ============================================================================
 * EVICTION
 * ============================================================================ */

bool lod_memory_is_over_budget(void) {
    if (!g_lod_memory.initialized) return false;
    
    float usage = (float)g_lod_memory.stats.total_allocated / 
                  (float)g_lod_memory.budget.total_budget_bytes;
    
    return usage > g_lod_memory.budget.eviction_threshold;
}

int lod_memory_evict_lru(size_t bytes_needed) {
    if (!g_lod_memory.initialized) return -1;
    
    size_t freed = 0;
    uint32_t evicted_count = 0;
    
    // Find least recently used, non-pinned entries
    while (freed < bytes_needed && g_lod_memory.entry_count > 0) {
        uint32_t lru_index = UINT32_MAX;
        uint64_t oldest_frame = UINT64_MAX;
        
        for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
            if (!g_lod_memory.entries[i].pinned && 
                g_lod_memory.entries[i].last_access_frame < oldest_frame) {
                oldest_frame = g_lod_memory.entries[i].last_access_frame;
                lru_index = i;
            }
        }
        
        if (lru_index == UINT32_MAX) {
            // No evictable entries
            break;
        }
        
        // Evict this entry
        lod_memory_entry_t* entry = &g_lod_memory.entries[lru_index];
        freed += entry->gpu_size + entry->cpu_size;
        evicted_count++;
        
        // In production: actually free the GPU/CPU memory
        // For now, just remove from tracking
        lod_memory_unregister_chain(entry->chain_id);
    }
    
    g_lod_memory.stats.eviction_count += evicted_count;
    
    return (freed >= bytes_needed) ? 0 : -1;
}

void lod_memory_set_chain_priority(uint32_t chain_id, float priority) {
    if (!g_lod_memory.initialized) return;
    
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        if (g_lod_memory.entries[i].chain_id == chain_id) {
            g_lod_memory.entries[i].priority = priority;
            break;
        }
    }
}

void lod_memory_pin_chain(uint32_t chain_id, bool pinned) {
    if (!g_lod_memory.initialized) return;
    
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        if (g_lod_memory.entries[i].chain_id == chain_id) {
            g_lod_memory.entries[i].pinned = pinned;
            break;
        }
    }
}

/* ============================================================================
 * MEMORY ALLOCATION (Simple wrapper)
 * ============================================================================ */

void* lod_memory_alloc(size_t size, bool is_gpu) {
    if (!g_lod_memory.initialized) return NULL;
    
    // Check if over budget
    if (!g_lod_memory.budget.allow_over_budget && lod_memory_is_over_budget()) {
        // Try to evict
        lod_memory_evict_lru(size);
    }
    
    void* ptr = malloc(size);
    
    if (ptr) {
        if (is_gpu) {
            g_lod_memory.stats.gpu_allocated += size;
        } else {
            g_lod_memory.stats.cpu_allocated += size;
        }
        g_lod_memory.stats.total_allocated += size;
    }
    
    return ptr;
}

void lod_memory_free(void* ptr, bool is_gpu) {
    if (!ptr || !g_lod_memory.initialized) return;
    
    // Note: In production, would track allocation sizes
    // For now, just free
    free(ptr);
}

/* ============================================================================
 * DEBUG
 * ============================================================================ */

void lod_memory_print_stats(void) {
    lod_memory_stats_t stats = lod_memory_get_stats();
    
    printf("=== LOD Memory Statistics ===\n");
    printf("Total Allocated: %.2f MB / %.2f MB (%.1f%%)\n",
           stats.total_allocated / (1024.0f * 1024.0f),
           g_lod_memory.budget.total_budget_bytes / (1024.0f * 1024.0f),
           stats.budget_usage_percent);
    printf("CPU Allocated: %.2f MB\n", stats.cpu_allocated / (1024.0f * 1024.0f));
    printf("GPU Allocated: %.2f MB\n", stats.gpu_allocated / (1024.0f * 1024.0f));
    printf("LOD Chains: %zu\n", stats.lod_chain_count);
    printf("Total LOD Meshes: %zu\n", stats.total_lod_meshes);
    printf("Evictions: %u\n", stats.eviction_count);
    printf("=============================\n");
}

void lod_memory_validate(void) {
    if (!g_lod_memory.initialized) return;
    
    // Validate that tracked memory matches stats
    size_t calculated_cpu = 0, calculated_gpu = 0;
    
    for (uint32_t i = 0; i < g_lod_memory.entry_count; i++) {
        calculated_cpu += g_lod_memory.entries[i].cpu_size;
        calculated_gpu += g_lod_memory.entries[i].gpu_size;
    }
    
    if (calculated_cpu != g_lod_memory.stats.cpu_allocated ||
        calculated_gpu != g_lod_memory.stats.gpu_allocated) {
        printf("WARNING: LOD memory tracking mismatch!\n");
    }
}

/* End of lod_memory.c */
