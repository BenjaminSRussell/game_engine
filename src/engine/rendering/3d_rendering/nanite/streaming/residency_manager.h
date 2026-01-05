/*
 * residency_manager.h
 * Tracks which clusters are currently resident on GPU
 */

#ifndef RESIDENCY_MANAGER_H
#define RESIDENCY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Residency Tracking
void residency_manager_init(uint32_t max_clusters);
void residency_manager_shutdown(void);

// Residency Status
bool residency_manager_is_resident(uint32_t cluster_id);
void residency_manager_mark_resident(uint32_t cluster_id, bool resident);

// Get current resident count
uint32_t residency_manager_get_count(void);

#endif // RESIDENCY_MANAGER_H
