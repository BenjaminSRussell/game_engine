#ifndef CORE_MEMORY_BUDDY_ALLOCATOR_H
#define CORE_MEMORY_BUDDY_ALLOCATOR_H

#include "include/common.h"

typedef struct BuddyBlock {
  struct BuddyBlock *left;
  struct BuddyBlock *right;
  struct BuddyBlock *parent;
  u32 size;
  u32 level;
  bool is_free;
  bool is_split;
} BuddyBlock;

typedef struct BuddyAllocator {
  void *memory_pool;
  u32 pool_size;
  u32 min_block_size;
  u32 max_block_size;
  u32 max_level;
  BuddyBlock *root;
  BuddyBlock **free_lists;
  u32 total_blocks;
  u32 free_blocks;

  // Extended fields
  u8 *bit_tree;
  u32 bit_tree_size;
  u64 total_allocated;
  u64 peak_usage;
  u32 allocation_count;
} BuddyAllocator;

BuddyAllocator *buddy_allocator_create(u32 min_block_size, u32 max_block_size);
void buddy_allocator_destroy(BuddyAllocator *alloc);
void *buddy_allocator_alloc(BuddyAllocator *alloc, u32 size);
void buddy_allocator_free(BuddyAllocator *alloc, void *ptr);
void buddy_allocator_print_stats(BuddyAllocator *alloc);

// Extensions
void buddy_init_bit_tree(BuddyAllocator *alloc);
void buddy_set_block_status(BuddyAllocator *alloc, u32 index, bool used);
void buddy_track_allocation(BuddyAllocator *alloc, u32 size);
void buddy_track_deallocation(BuddyAllocator *alloc, u32 size);
void buddy_visual_debug(BuddyAllocator *alloc);

#endif
