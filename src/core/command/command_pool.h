/*
 * command_pool.h
 * Thread-local command pool management
 */

#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct command_pool command_pool_t;

typedef struct command_pool_desc {
    uint32_t queue_family_index;
    uint32_t flags;
} command_pool_desc_t;

// Creates a new command pool for the current thread
command_pool_t* command_pool_create(const command_pool_desc_t* desc);

// Destroys a command pool
void command_pool_destroy(command_pool_t* pool);

// Resets a command pool, releasing all resources allocated from it
void command_pool_reset(command_pool_t* pool);

// Gets the native backend handle (e.g., VkCommandPool)
void* command_pool_get_handle(command_pool_t* pool);

#endif // COMMAND_POOL_H
