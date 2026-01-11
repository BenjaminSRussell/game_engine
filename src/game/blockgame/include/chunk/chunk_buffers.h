// include/chunk/chunk_buffers.h
//
// Purpose: Declares functions for managing the graphical buffers (e.g., Vulkan
// buffers) associated with a chunk's mesh data. These functions facilitate the
// creation, updating, and destruction of GPU-side resources needed to render
// chunks.
//
// Public APIs:
// - `chunk_create_vulkan_buffers`: Allocates and initializes Vulkan buffers for
// a given chunk's mesh.
// - `chunk_update_vulkan_buffers`: Updates the content of existing Vulkan
// buffers when a chunk's mesh
//   data has changed (e.g., after mesh regeneration).
// - `chunk_destroy_vulkan_buffers`: Frees the resources held by a chunk's
// Vulkan buffers.
//
// Ownership: These functions manage GPU-side resources that are conceptually
// owned by the `Chunk` they are associated with. The `VulkanRenderer` instance
// provides the necessary device context and command queues for these
// operations.
//
// Invariants:
// - A valid `Chunk` pointer and an initialized `VulkanRenderer` instance must
// be provided.
// - Buffers must be created before they can be updated or destroyed.
// - The `Chunk`'s mesh data (`chunk->mesh.vertices`, `chunk->mesh.indices`)
// must be valid
//   when calling create or update functions.
//
#ifndef CHUNK_BUFFERS_H
#define CHUNK_BUFFERS_H

#include "../game_common.h"
#include "chunk.h"
#include <rendering/vulkan.h>

// Forward declarations
// VulkanRenderer is defined in render/vulkan.h

// Create Vulkan buffers for chunk
bool chunk_create_vulkan_buffers(Chunk *chunk, VulkanRenderer *renderer);

// Update chunk buffers (when mesh changes)
bool chunk_update_vulkan_buffers(Chunk *chunk, VulkanRenderer *renderer);

// Destroy Vulkan buffers for chunk
void chunk_destroy_vulkan_buffers(Chunk *chunk, VulkanRenderer *renderer);

#endif // CHUNK_BUFFERS_H
