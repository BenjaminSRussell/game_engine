/*
 * pipeline_cache.h
 * Pipeline cache management
 */

#ifndef PIPELINE_CACHE_H
#define PIPELINE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct pipeline_cache pipeline_cache_t;

// Lifecycle
pipeline_cache_t* pipeline_cache_create(const void* initial_data, size_t size);
void pipeline_cache_destroy(pipeline_cache_t* cache);

// Data retrieval
size_t pipeline_cache_get_data(pipeline_cache_t* cache, void* buffer, size_t buffer_size);

void* pipeline_cache_get_handle(pipeline_cache_t* cache);

#endif // PIPELINE_CACHE_H
