#include "core/memory/buddy_allocator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Extended field helpers
void buddy_init_bit_tree(BuddyAllocator *alloc) {
    // Initial implementation stub
    if (alloc) alloc->bit_tree = NULL;
}
void buddy_set_block_status(BuddyAllocator *alloc, u32 index, bool used) {}
void buddy_track_allocation(BuddyAllocator *alloc, u32 size) {
    if (alloc) {
        alloc->total_allocated += size;
        alloc->allocation_count++;
         if (alloc->total_allocated > alloc->peak_usage) alloc->peak_usage = alloc->total_allocated;
    }
}
void buddy_track_deallocation(BuddyAllocator *alloc, u32 size) {
    if (alloc) {
        alloc->total_allocated -= size;
    }
}
void buddy_visual_debug(BuddyAllocator *alloc) {
    if (alloc) printf("Buddy Allocator: %u blocks free\n", alloc->free_blocks);
}


// Core logic from memory.c
static BuddyBlock *buddy_create_block(BuddyAllocator *alloc, u32 size,
                                      u32 level, BuddyBlock *parent) {
  BuddyBlock *block = (BuddyBlock *)malloc(sizeof(BuddyBlock));
  if (!block)
    return NULL;

  block->left = NULL;
  block->right = NULL;
  block->parent = parent;
  block->size = size;
  block->level = level;
  block->is_free = true;
  block->is_split = false;

  return block;
}

static void buddy_split_block(BuddyBlock *block) {
  if (block->is_split || block->level == 0)
    return;

  u32 half_size = block->size / 2;
  u32 new_level = block->level - 1;

  block->left = buddy_create_block(NULL, half_size, new_level, block);
  block->right = buddy_create_block(NULL, half_size, new_level, block);

  if (block->left && block->right) {
    block->is_split = true;
    block->is_free = false;
  }
}

static void buddy_merge_block(BuddyBlock *block) {
  if (!block->parent || !block->parent->is_split)
    return;

  BuddyBlock *parent = block->parent;
  BuddyBlock *sibling = (parent->left == block) ? parent->right : parent->left;

  if (sibling && sibling->is_free && !sibling->is_split) {
    // Merge sibling blocks
    free(parent->left);
    free(parent->right);
    parent->left = NULL;
    parent->right = NULL;
    parent->is_split = false;
    parent->is_free = true;

    // Recursively merge up
    buddy_merge_block(parent);
  }
}

BuddyAllocator *buddy_allocator_create(u32 min_block_size, u32 max_block_size) {
  // Validate power-of-2 sizes
  if ((min_block_size & (min_block_size - 1)) != 0 ||
      (max_block_size & (max_block_size - 1)) != 0 ||
      min_block_size > max_block_size) {
    printf("Invalid block sizes for buddy allocator\n");
    return NULL;
  }

  BuddyAllocator *alloc = (BuddyAllocator *)malloc(sizeof(BuddyAllocator));
  if (!alloc)
    return NULL;

  alloc->min_block_size = min_block_size;
  alloc->max_block_size = max_block_size;
  alloc->max_level = 0;

  // Calculate max level
  u32 size = min_block_size;
  while (size < max_block_size) {
    size <<= 1;
    alloc->max_level++;
  }

  alloc->pool_size = max_block_size;
  alloc->memory_pool = malloc(max_block_size);
  if (!alloc->memory_pool) {
    free(alloc);
    return NULL;
  }

  // Initialize free lists
  alloc->free_lists =
      (BuddyBlock **)malloc(sizeof(BuddyBlock *) * (alloc->max_level + 1));
  if (!alloc->free_lists) {
    free(alloc->memory_pool);
    free(alloc);
    return NULL;
  }

  for (u32 i = 0; i <= alloc->max_level; i++) {
    alloc->free_lists[i] = NULL;
  }

  // Create root block
  alloc->root =
      buddy_create_block(alloc, max_block_size, alloc->max_level, NULL);
  if (!alloc->root) {
    free(alloc->free_lists);
    free(alloc->memory_pool);
    free(alloc);
    return NULL;
  }

  alloc->free_lists[alloc->max_level] = alloc->root;
  alloc->total_blocks = 1;
  alloc->free_blocks = 1;

  // Initialize extended fields
  buddy_init_bit_tree(alloc);
  alloc->total_allocated = 0;
  alloc->peak_usage = 0;
  alloc->allocation_count = 0;

  return alloc;
}

void buddy_allocator_destroy(BuddyAllocator *alloc) {
  if (!alloc)
    return;

  // Recursively free all blocks
  if (alloc->root) {
    // Simple cleanup - in production, implement proper recursive free
    free(alloc->root);
  }

  free(alloc->free_lists);
  free(alloc->memory_pool);
  if(alloc->bit_tree) free(alloc->bit_tree); // Assumes bit_tree malloced elsewhere/not
  free(alloc);
}

void *buddy_allocator_alloc(BuddyAllocator *alloc, u32 size) {
  if (!alloc || size == 0)
    return NULL;

  // Round up to nearest power of 2
  u32 aligned_size = size;
  if (aligned_size < alloc->min_block_size) {
    aligned_size = alloc->min_block_size;
  }
  while ((aligned_size & (aligned_size - 1)) != 0) {
    aligned_size <<= 1;
  }

  // Find appropriate level
  u32 level = 0;
  u32 block_size = alloc->min_block_size;
  while (block_size < aligned_size) {
    block_size <<= 1;
    level++;
  }

  if (level > alloc->max_level) {
    return NULL;
  }

  // Find free block at this level or higher
  BuddyBlock *block = NULL;
  for (u32 search_level = level; search_level <= alloc->max_level;
       search_level++) {
    if (alloc->free_lists[search_level]) {
      block = alloc->free_lists[search_level];
      break;
    }
  }

  if (!block) {
    return NULL;
  }

  // Remove from free list
  u32 block_level = block->level;
  if (alloc->free_lists[block_level] == block) {
    alloc->free_lists[block_level] = block->left ? block->left : NULL;
  }

  // Split blocks until we reach the desired level
  while (block->level > level) {
    buddy_split_block(block);
    block = block->left;
  }

  block->is_free = false;
  alloc->free_blocks--;
  buddy_set_block_status(alloc, 0, true);
  buddy_track_allocation(alloc, size);

  // Calculate offset from memory pool
  u32 offset = 0;
  BuddyBlock *current = block;
  while (current->parent) {
    if (current->parent->right == current) {
      offset += current->parent->size / 2;
    }
    current = current->parent;
  }

  return (char *)alloc->memory_pool + offset;
}

void buddy_allocator_free(BuddyAllocator *alloc, void *ptr) {
  if (!alloc || !ptr)
    return;

  // Calculate which block this pointer belongs to
  u32 offset = (u32)((char *)ptr - (char *)alloc->memory_pool);
  if (offset >= alloc->pool_size) {
    return;
  }

  // Find the block (simplified - in production, maintain pointer-to-block
  // mapping)
  BuddyBlock *block = alloc->root;
  u32 current_offset = 0;

  while (block && block->is_split) {
    u32 half_size = block->size / 2;
    if (offset < current_offset + half_size) {
      block = block->left;
    } else {
      current_offset += half_size;
      block = block->right;
    }
  }

  if (block && !block->is_free) {
    block->is_free = true;
    alloc->free_blocks++;
    buddy_track_deallocation(alloc, block->size);

    // Try to merge with buddy
    buddy_merge_block(block);
  }
}

void buddy_allocator_print_stats(BuddyAllocator *alloc) {
   if(alloc) buddy_visual_debug(alloc);
}
