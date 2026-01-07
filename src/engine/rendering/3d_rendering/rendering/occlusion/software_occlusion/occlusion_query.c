/*
 * occlusion_query.c
 * Software and Hardware Occlusion Queries
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#include "occlusion_query.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_QUERIES 4096

typedef struct occlusion_query_pool {
    uint32_t query_ids[MAX_QUERIES];
    bool in_use[MAX_QUERIES];
    bool initialized;
    void* gpu_pool_handle;
} occlusion_query_pool_t;

static occlusion_query_pool_t g_query_pool = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int occlusion_query_init(void) {
    if (g_query_pool.initialized) return 0;
    
    // RenderAPI_CreateQueryPool(QUERY_TYPE_OCCLUSION, MAX_QUERIES, &g_query_pool.gpu_pool_handle);
    
    for (int i = 0; i < MAX_QUERIES; i++) {
        g_query_pool.in_use[i] = false;
    }
    
    g_query_pool.initialized = true;
    return 0;
}

void occlusion_query_shutdown(void) {
    if (!g_query_pool.initialized) return;
    
    // RenderAPI_DestroyQueryPool(g_query_pool.gpu_pool_handle);
    g_query_pool.initialized = false;
}

uint32_t occlusion_query_begin(void) {
    // Find free query
    int id = -1;
    for (int i = 0; i < MAX_QUERIES; i++) {
        if (!g_query_pool.in_use[i]) {
            g_query_pool.in_use[i] = true;
            id = i;
            break;
        }
    }
    
    if (id == -1) return 0xFFFFFFFF; // Pool full
    
    // RenderAPI_CmdBeginQuery(g_query_pool.gpu_pool_handle, id);
    return (uint32_t)id;
}

void occlusion_query_end(uint32_t query_id) {
    if (query_id >= MAX_QUERIES) return;
    
    // RenderAPI_CmdEndQuery(g_query_pool.gpu_pool_handle, query_id);
}

bool occlusion_query_get_result(uint32_t query_id, uint64_t* samples_passed, bool wait) {
    if (query_id >= MAX_QUERIES) return false;
    
    // int flags = wait ? QUERY_RESULT_WAIT : 0;
    // return RenderAPI_GetQueryPoolResults(..., query_id, 1, samples_passed, ...);
    
    *samples_passed = 100; // Stub
    
    // Free query if done
    g_query_pool.in_use[query_id] = false;
    
    return true;
}
