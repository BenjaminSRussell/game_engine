/**
 * STACK & BUDDY ALLOCATORS
 * AGENT_CORE_1 - Stream 1
 * Additional memory allocators for completeness
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Stack Allocator
typedef struct {
  void *buffer;
  size_t size;
  size_t offset;
  size_t *markers;
  int marker_count;
} StackAllocator;

// Create stack allocator
StackAllocator *stack_allocator_create(size_t size) {
  StackAllocator *alloc = (StackAllocator *)malloc(sizeof(StackAllocator));
  alloc->buffer = malloc(size);
  alloc->size = size;
  alloc->offset = 0;
  alloc->markers = (size_t *)malloc(sizeof(size_t) * 100);
  alloc->marker_count = 0;
  return alloc;
}

// Allocate
void *stack_alloc(StackAllocator *alloc, size_t size, size_t alignment) {
  size_t aligned_offset = (alloc->offset + alignment - 1) & ~(alignment - 1);

  if (aligned_offset + size > alloc->size)
    return NULL;

  void *ptr = (char *)alloc->buffer + aligned_offset;
  alloc->offset = aligned_offset + size;

  return ptr;
}

// Push marker
void stack_push_marker(StackAllocator *alloc) {
  alloc->markers[alloc->marker_count++] = alloc->offset;
}

// Pop marker
void stack_pop_marker(StackAllocator *alloc) {
  if (alloc->marker_count > 0) {
    alloc->offset = alloc->markers[--alloc->marker_count];
  }
}

// Buddy Allocator
typedef struct BuddyBlock {
  size_t size;
  bool is_free;
  struct BuddyBlock *next;
} BuddyBlock;

typedef struct {
  void *buffer;
  size_t total_size;
  BuddyBlock *free_lists[32];
} BuddyAllocator;

static int buddy_level_for_size(size_t size) {
  int level = 0;
  size_t block_size = 1;
  while (block_size < size) {
    block_size <<= 1;
    level++;
  }
  return level;
}

static size_t buddy_block_size_for_level(int level) {
  return (size_t)1 << level;
}

static void buddy_list_remove(BuddyAllocator *alloc, int level,
                              BuddyBlock *block) {
  BuddyBlock **head = &alloc->free_lists[level];
  BuddyBlock *prev = NULL;
  BuddyBlock *cur = *head;
  while (cur) {
    if (cur == block) {
      if (prev) {
        prev->next = cur->next;
      } else {
        *head = cur->next;
      }
      return;
    }
    prev = cur;
    cur = cur->next;
  }
}

// Create buddy allocator
BuddyAllocator *buddy_allocator_create(size_t size) {
  BuddyAllocator *alloc = (BuddyAllocator *)calloc(1, sizeof(BuddyAllocator));
  if (!alloc) {
    return NULL;
  }
  size_t total = 1;
  int level = 0;
  while (total < size) {
    total <<= 1;
    level++;
  }

  alloc->buffer = malloc(total);
  if (!alloc->buffer) {
    free(alloc);
    return NULL;
  }
  alloc->total_size = total;

  BuddyBlock *block = (BuddyBlock *)alloc->buffer;
  block->size = total;
  block->is_free = true;
  block->next = NULL;
  alloc->total_size = total;
  alloc->free_lists[level] = block;

  return alloc;
}

// Allocate (buddy system)
void *buddy_alloc(BuddyAllocator *alloc, size_t size) {
  if (!alloc || size == 0) {
    return NULL;
  }

  size_t required = size + sizeof(BuddyBlock);
  int target_level = buddy_level_for_size(required);

  int level = target_level;
  while (level < 32 && !alloc->free_lists[level]) {
    level++;
  }
  if (level >= 32) {
    return NULL;
  }

  BuddyBlock *block = alloc->free_lists[level];
  alloc->free_lists[level] = block->next;
  block->next = NULL;

  while (level > target_level) {
    level--;
    size_t split_size = buddy_block_size_for_level(level);
    BuddyBlock *buddy = (BuddyBlock *)((char *)block + split_size);
    buddy->size = split_size;
    buddy->is_free = true;
    buddy->next = alloc->free_lists[level];
    alloc->free_lists[level] = buddy;

    block->size = split_size;
  }

  block->is_free = false;
  return (void *)(block + 1);
}

// Free (buddy system)
void buddy_free(BuddyAllocator *alloc, void *ptr) {
  if (!alloc || !ptr) {
    return;
  }

  BuddyBlock *block = ((BuddyBlock *)ptr) - 1;
  block->is_free = true;

  uintptr_t base = (uintptr_t)alloc->buffer;
  uintptr_t offset = (uintptr_t)block - base;
  size_t size = block->size;
  int level = buddy_level_for_size(size);

  while (level < 32) {
    uintptr_t buddy_offset = offset ^ size;
    BuddyBlock *buddy = (BuddyBlock *)(base + buddy_offset);

    if (buddy_offset >= alloc->total_size || !buddy->is_free ||
        buddy->size != size) {
      break;
    }

    buddy_list_remove(alloc, level, buddy);

    if (buddy < block) {
      block = buddy;
      offset = buddy_offset;
    }

    size <<= 1;
    level++;
    block->size = size;
  }

  block->is_free = true;
  block->next = alloc->free_lists[level];
  alloc->free_lists[level] = block;
}
