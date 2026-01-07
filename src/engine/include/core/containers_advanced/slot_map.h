#pragma once

#include "include/core/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct SlotMap SlotMap;
typedef struct SlotMapIterator SlotMapIterator;

// Slot handle - combines index and generation for safe access
typedef struct {
    u32 index;
    u32 generation;
} SlotHandle;

// Slot state enumeration
typedef enum {
    SLOT_STATE_VACANT = 0,
    SLOT_STATE_OCCUPIED = 1,
    SLOT_STATE_TOMBSTONE = 2
} SlotState;

// Slot entry with generation and data
typedef struct {
    u32 generation;
    SlotState state;
    u32 next_vacant;  // For free-list
    void *data;
} SlotEntry;

// Slot callback types
typedef void (*SlotMapIteratorFn)(SlotHandle handle, void *data, void *user_data);
typedef void (*SlotMapRemovalCallback)(SlotHandle handle, void *data, void *user_data);

/**
 * SlotMap: Stable handle-based storage with generation counters
 *
 * Properties:
 *   - O(1) insert, remove, lookup
 *   - Stable handles (don't change on removal)
 *   - Generation counters prevent use-after-free
 *   - Iteration over active elements only
 *   - Cache-efficient storage
 *
 * Advantages over arrays:
 *   - Handles remain valid after other removals
 *   - No dangling pointer issues
 *   - Gap handling without costly shifting
 *
 * Use cases:
 *   - Game object management
 *   - Scene graphs
 *   - Asset handles
 *   - Resource pools
 */

// ============================================================================
// Creation and Destruction
// ============================================================================

/**
 * Create a new slot map
 *
 * Args:
 *   capacity: Initial capacity
 *   element_size: Size of each element in bytes
 *
 * Returns:
 *   New SlotMap (must be freed with slot_map_destroy)
 */
SlotMap *slot_map_create(u32 capacity, u32 element_size);

/**
 * Destroy slot map and free all memory
 *
 * Args:
 *   map: SlotMap to destroy (NULL-safe)
 */
void slot_map_destroy(SlotMap *map);

/**
 * Clear all elements (resets generations)
 *
 * Args:
 *   map: SlotMap
 */
void slot_map_clear(SlotMap *map);

/**
 * Clear with element removal callbacks
 *
 * Args:
 *   map: SlotMap
 *   callback: Called for each removed element
 *   user_data: Passed to callback
 */
void slot_map_clear_with_callback(SlotMap *map, SlotMapRemovalCallback callback, void *user_data);

// ============================================================================
// Core Operations
// ============================================================================

/**
 * Insert element and get stable handle
 *
 * Args:
 *   map: SlotMap
 *   element: Pointer to element data
 *
 * Returns:
 *   Valid SlotHandle for this element
 */
SlotHandle slot_map_insert(SlotMap *map, const void *element);

/**
 * Remove element by handle
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle from slot_map_insert
 *
 * Returns:
 *   true if element was removed, false if handle invalid
 */
bool slot_map_remove(SlotMap *map, SlotHandle handle);

/**
 * Check if handle is valid and element exists
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle to validate
 *
 * Returns:
 *   true if handle is current and valid
 */
bool slot_map_contains(SlotMap *map, SlotHandle handle);

/**
 * Get element by handle
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle to lookup
 *
 * Returns:
 *   Pointer to element if valid, NULL if handle invalid
 */
void *slot_map_get(SlotMap *map, SlotHandle handle);

/**
 * Get mutable element by handle
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle to lookup
 *
 * Returns:
 *   Mutable pointer to element or NULL
 */
void *slot_map_get_mut(SlotMap *map, SlotHandle handle);

/**
 * Update element data
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle to update
 *   element: New element data
 *
 * Returns:
 *   true if updated successfully
 */
bool slot_map_update(SlotMap *map, SlotHandle handle, const void *element);

// ============================================================================
// Generation Management
// ============================================================================

/**
 * Get generation counter for handle
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle
 *
 * Returns:
 *   Current generation value
 */
u32 slot_map_generation(SlotMap *map, SlotHandle handle);

/**
 * Check if slot is occupied
 *
 * Args:
 *   map: SlotMap
 *   handle: Handle to check
 *
 * Returns:
 *   true if slot has valid data
 */
bool slot_map_is_occupied(SlotMap *map, SlotHandle handle);

/**
 * Get generation at index without handle validation
 *
 * Args:
 *   map: SlotMap
 *   index: Slot index
 *
 * Returns:
 *   Generation counter at index
 */
u32 slot_map_get_generation_at_index(SlotMap *map, u32 index);

// ============================================================================
// Iteration
// ============================================================================

/**
 * Get number of elements in map
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Number of occupied slots
 */
u32 slot_map_count(SlotMap *map);

/**
 * Get capacity
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Current capacity
 */
u32 slot_map_capacity(SlotMap *map);

/**
 * Create iterator for occupied slots
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Iterator positioned at first occupied slot
 */
SlotMapIterator slot_map_iterator_create(SlotMap *map);

/**
 * Get next occupied slot in iteration
 *
 * Args:
 *   iter: Iterator
 *   handle_out: Output for slot handle
 *   data_out: Output for element pointer
 *
 * Returns:
 *   true if element returned, false at end
 */
bool slot_map_iterator_next(SlotMapIterator *iter, SlotHandle *handle_out, void **data_out);

/**
 * Get current handle without advancing
 *
 * Args:
 *   iter: Iterator
 *
 * Returns:
 *   Current handle or invalid handle if at end
 */
SlotHandle slot_map_iterator_current_handle(SlotMapIterator *iter);

/**
 * Reset iterator to beginning
 *
 * Args:
 *   iter: Iterator
 */
void slot_map_iterator_reset(SlotMapIterator *iter);

/**
 * Iterate with callback
 *
 * Args:
 *   map: SlotMap
 *   callback: Called for each occupied slot
 *   user_data: Passed to callback
 */
void slot_map_foreach(SlotMap *map, SlotMapIteratorFn callback, void *user_data);

/**
 * Get handles of all occupied slots
 *
 * Args:
 *   map: SlotMap
 *   handles_out: Output array
 *   max_handles: Maximum handles to retrieve
 *
 * Returns:
 *   Number of handles retrieved
 */
u32 slot_map_get_all_handles(SlotMap *map, SlotHandle *handles_out, u32 max_handles);

// ============================================================================
// Free List Management
// ============================================================================

/**
 * Get head of free list
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Index of first vacant slot
 */
u32 slot_map_free_list_head(SlotMap *map);

/**
 * Get number of vacant slots
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Number of available slots
 */
u32 slot_map_vacant_count(SlotMap *map);

/**
 * Reserve multiple slots at once
 *
 * Args:
 *   map: SlotMap
 *   count: Number of slots to allocate
 *   handles_out: Output array for handles
 *
 * Returns:
 *   true if all slots allocated (atomic)
 */
bool slot_map_reserve(SlotMap *map, u32 count, SlotHandle *handles_out);

/**
 * Check if map needs expansion
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   true if load factor is high
 */
bool slot_map_needs_expansion(SlotMap *map);

// ============================================================================
// Compaction and Defragmentation
// ============================================================================

/**
 * Compact map by removing tombstones
 *
 * Args:
 *   map: SlotMap
 *
 * Note: Preserves handles and generations
 */
void slot_map_compact(SlotMap *map);

/**
 * Get fragmentation ratio
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Ratio of wasted slots (0.0 to 1.0)
 */
f32 slot_map_fragmentation_ratio(SlotMap *map);

/**
 * Check if compaction would improve efficiency
 *
 * Args:
 *   map: SlotMap
 *   threshold: Fragmentation threshold (e.g., 0.25 for 25%)
 *
 * Returns:
 *   true if fragmentation exceeds threshold
 */
bool slot_map_should_compact(SlotMap *map, f32 threshold);

// ============================================================================
// Serialization
// ============================================================================

/**
 * Serialize slot map to buffer
 *
 * Args:
 *   map: SlotMap
 *   buffer: Output buffer
 *   buffer_size: Buffer size
 *
 * Returns:
 *   Number of bytes written, 0 on error
 */
u32 slot_map_serialize(SlotMap *map, void *buffer, u32 buffer_size);

/**
 * Deserialize slot map from buffer
 *
 * Args:
 *   buffer: Input buffer
 *   buffer_size: Buffer size
 *   element_size: Size of elements
 *
 * Returns:
 *   New SlotMap or NULL on error
 */
SlotMap *slot_map_deserialize(void *buffer, u32 buffer_size, u32 element_size);

/**
 * Save slot map to file
 *
 * Args:
 *   map: SlotMap to save
 *   file_path: Output file path
 *
 * Returns:
 *   true if saved successfully
 */
bool slot_map_save_file(SlotMap *map, const char *file_path);

/**
 * Load slot map from file
 *
 * Args:
 *   file_path: Input file path
 *   element_size: Size of elements
 *
 * Returns:
 *   New SlotMap or NULL on error
 */
SlotMap *slot_map_load_file(const char *file_path, u32 element_size);

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

/**
 * Get memory usage
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Total bytes allocated
 */
u64 slot_map_memory_usage(SlotMap *map);

/**
 * Get load factor (occupied / capacity)
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   Load factor from 0.0 to 1.0
 */
f32 slot_map_load_factor(SlotMap *map);

/**
 * Print diagnostic information
 *
 * Args:
 *   map: SlotMap
 */
void slot_map_print_diagnostics(SlotMap *map);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Validate map integrity
 *
 * Args:
 *   map: SlotMap
 *
 * Returns:
 *   true if all invariants hold
 */
bool slot_map_validate(SlotMap *map);

/**
 * Run comprehensive tests
 *
 * Returns:
 *   Number of tests passed (0 = all passed)
 */
u32 slot_map_run_tests(void);

// ============================================================================
// Advanced: Stable Handle Guarantees
// ============================================================================

/**
 * Get original handle from index and generation
 *
 * Args:
 *   index: Slot index
 *   generation: Generation counter
 *
 * Returns:
 *   SlotHandle combining both values
 */
INLINE SlotHandle slot_map_make_handle(u32 index, u32 generation) {
    return (SlotHandle){index, generation};
}

/**
 * Extract index from handle
 *
 * Args:
 *   handle: SlotHandle
 *
 * Returns:
 *   Index component
 */
INLINE u32 slot_map_handle_index(SlotHandle handle) {
    return handle.index;
}

/**
 * Extract generation from handle
 *
 * Args:
 *   handle: SlotHandle
 *
 * Returns:
 *   Generation component
 */
INLINE u32 slot_map_handle_generation(SlotHandle handle) {
    return handle.generation;
}

#ifdef __cplusplus
}
#endif

#endif // SLOT_MAP_H
