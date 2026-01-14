/*
 * feedback_analysis.c
 * Virtual texture feedback analysis implementation
 */

#include "assets/textures/virtual_texturing/feedback_analysis.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static feedback_stats_t g_stats = {0};
static bool g_initialized = false;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int feedback_analysis_init(void) {
    if (g_initialized) return 0;
    memset(&g_stats, 0, sizeof(feedback_stats_t));
    g_initialized = true;
    return 0;
}

void feedback_analysis_shutdown(void) {
    g_initialized = false;
}

int feedback_analysis_process(const void* feedback_data, uint32_t count) {
    if (!g_initialized || !feedback_data) return -1;
    
    // Process feedback requests to trigger streaming
    // (Simplified: update stats)
    g_stats.requested_pages += count;
    g_stats.unique_pages = count / 2; // Simulated uniqueness
    
    return 0;
}

int feedback_analysis_get_stats(feedback_stats_t* out_stats) {
    if (!g_initialized || !out_stats) return -1;
    *out_stats = g_stats;
    return 0;
}

/* Original stub compatibility */
int texture_feedback_analysis_init(void) {
    return feedback_analysis_init();
}

void texture_feedback_analysis_shutdown(void) {
    feedback_analysis_shutdown();
}

