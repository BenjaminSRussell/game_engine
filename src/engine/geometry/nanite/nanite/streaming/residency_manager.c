/*
 * residency_manager.c
 * Implementation of cluster residency tracking
 */

#include "residency_manager.h"
#include <stdlib.h>
#include <string.h>

static uint8_t* g_residency_bitset = NULL;
static uint32_t g_max_clusters = 0;
static uint32_t g_resident_count = 0;

void residency_manager_init(uint32_t max_clusters) {
    g_max_clusters = max_clusters;
    g_residency_bitset = (uint8_t*)calloc((max_clusters + 7) / 8, 1);
    g_resident_count = 0;
}

void residency_manager_shutdown(void) {
    if (g_residency_bitset) free(g_residency_bitset);
}

bool residency_manager_is_resident(uint32_t cluster_id) {
    if (cluster_id >= g_max_clusters || !g_residency_bitset) return false;
    return (g_residency_bitset[cluster_id >> 3] & (1 << (cluster_id & 7))) != 0;
}

void residency_manager_mark_resident(uint32_t cluster_id, bool resident) {
    if (cluster_id >= g_max_clusters || !g_residency_bitset) return;

    bool current = residency_manager_is_resident(cluster_id);
    if (resident && !current) {
        g_residency_bitset[cluster_id >> 3] |= (1 << (cluster_id & 7));
        g_resident_count++;
    } else if (!resident && current) {
        g_residency_bitset[cluster_id >> 3] &= ~(1 << (cluster_id & 7));
        g_resident_count--;
    }
}

uint32_t residency_manager_get_count(void) {
    return g_resident_count;
}
