#ifndef COMMON_POOLS_H
#define COMMON_POOLS_H

#include <stdbool.h>
#include <stddef.h>

// Initialize common memory pools
bool common_pools_init(void);

// Shutdown common memory pools
void common_pools_shutdown(void);

// Smart allocation from appropriate pool
void *common_pool_alloc(size_t size);

// Free to appropriate pool
void common_pool_free(void *ptr, size_t size);

// Print pool statistics
void common_pools_print_stats(void);

#endif // COMMON_POOLS_H
