/*
 * descriptor_pool.h
 * Descriptor pool management
 */

#ifndef CORE_DESCRIPTOR_POOL_H
#define CORE_DESCRIPTOR_POOL_H

#include "descriptor_set_layout.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t id;
} descriptor_pool_handle_t;

typedef struct {
    descriptor_type_t type;
    uint32_t count;
} descriptor_pool_size_t;

typedef struct {
    uint32_t max_sets;
    const descriptor_pool_size_t* pool_sizes;
    uint32_t pool_size_count;
} descriptor_pool_desc_t;

// Opaque handle for a descriptor set allocated from the pool
typedef struct {
    uint32_t id;
} descriptor_set_handle_t;

/* Initialization */
void descriptor_pool_init_system(void);
void descriptor_pool_shutdown_system(void);

/* Lifecycle */
descriptor_pool_handle_t descriptor_pool_create(const descriptor_pool_desc_t* desc);
void descriptor_pool_destroy(descriptor_pool_handle_t pool);

/* Operations */
void descriptor_pool_reset(descriptor_pool_handle_t pool);

// Allocate a descriptor set from the pool using the given layout
bool descriptor_pool_allocate(descriptor_pool_handle_t pool, 
                              descriptor_set_layout_handle_t layout, 
                              descriptor_set_handle_t* out_set);

// Statistics
uint32_t descriptor_pool_get_allocated_sets(descriptor_pool_handle_t pool);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DESCRIPTOR_POOL_H */
