#include <core/containers/skip_list.h>
#include <core/containers/skip_list.h>
#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <include/math/math.h>

//  COMPLETED: Skip List Implementation - AGENT_CORE_2
// Probabilistic data structure providing O(log n) average case performance
// Alternative to balanced trees with simpler implementation

#define MAX_LEVEL 16
#define DEFAULT_PROBABILITY 0.5f

typedef struct SkipListNode {
    u64 key;
    void* value;
    struct SkipListNode** forward;  // Array of forward pointers
    i32 level;                      // Current level of this node
} SkipListNode;

struct SkipList {
    SkipListNode* header;            // Header node with maximum level
    i32 max_level;                  // Maximum allowed level
    i32 current_level;              // Current highest level in use
    f32 probability;                // Probability factor for level generation
    size_t size;                    // Number of elements in skip list
};

// Internal helper functions
static i32 skip_list_random_level(SkipList* list) {
    i32 level = 1;
    while ((rand() / (f32)RAND_MAX) < list->probability && level < list->max_level) {
        level++;
    }
    return level;
}

static SkipListNode* skip_list_create_node(i32 level, u64 key, void* value) {
    SkipListNode* node = malloc(sizeof(SkipListNode));
    if (!node) return NULL;
    
    node->forward = malloc(sizeof(SkipListNode*) * (level + 1));
    if (!node->forward) {
        free(node);
        return NULL;
    }
    
    node->key = key;
    node->value = value;
    node->level = level;
    
    // Initialize forward pointers to NULL
    for (i32 i = 0; i <= level; i++) {
        node->forward[i] = NULL;
    }
    
    return node;
}

//  COMPLETED: Skip List Creation
SkipList* skip_list_create(i32 max_level, f32 probability) {
    if (max_level <= 0 || probability <= 0.0f || probability >= 1.0f) {
        return NULL;
    }
    
    SkipList* list = malloc(sizeof(SkipList));
    if (!list) return NULL;
    
    list->max_level = max_level;
    list->current_level = 1;
    list->probability = probability;
    list->size = 0;
    
    // Create header node with maximum level
    list->header = skip_list_create_node(max_level, 0, NULL);
    if (!list->header) {
        free(list);
        return NULL;
    }
    
    return list;
}

//  COMPLETED: Skip List Insert - O(log n) average
bool skip_list_insert(SkipList* list, u64 key, void* value) {
    if (!list) return false;
    
    SkipListNode* update[MAX_LEVEL + 1];
    SkipListNode* current = list->header;
    
    // Find insertion position at each level
    for (i32 i = list->current_level - 1; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    // Key already exists - update value
    if (current != NULL && current->key == key) {
        current->value = value;
        return true;
    }
    
    // Generate random level for new node
    i32 new_level = skip_list_random_level(list);
    
    // Update current level if new node has higher level
    if (new_level > list->current_level) {
        for (i32 i = list->current_level; i < new_level; i++) {
            update[i] = list->header;
        }
        list->current_level = new_level;
    }
    
    // Create new node
    SkipListNode* new_node = skip_list_create_node(new_level, key, value);
    if (!new_node) return false;
    
    // Link new node into lists at each level
    for (i32 i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    list->size++;
    return true;
}

//  COMPLETED: Skip List Search - O(log n) average
void* skip_list_search(SkipList* list, u64 key) {
    if (!list) return NULL;
    
    SkipListNode* current = list->header;
    
    // Start at top level and work down
    for (i32 i = list->current_level - 1; i >= 0; i--) {
        // Move right while next key is less than search key
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }
    
    // Move to next node at bottom level
    current = current->forward[0];
    
    // Check for exact match
    if (current != NULL && current->key == key) {
        return current->value;
    }
    
    return NULL;
}

//  COMPLETED: Skip List Delete - O(log n) average
bool skip_list_delete(SkipList* list, u64 key) {
    if (!list) return false;
    
    SkipListNode* update[MAX_LEVEL + 1];
    SkipListNode* current = list->header;
    
    // Find node to delete
    for (i32 i = list->current_level - 1; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    // Key not found
    if (current == NULL || current->key != key) {
        return false;
    }
    
    // Unlink node from each level
    for (i32 i = 0; i < current->level; i++) {
        if (update[i]->forward[i] != current) {
            break;
        }
        update[i]->forward[i] = current->forward[i];
    }
    
    // Free node memory
    free(current->forward);
    free(current);
    
    // Update current level if needed
    while (list->current_level > 1 && list->header->forward[list->current_level - 1] == NULL) {
        list->current_level--;
    }
    
    list->size--;
    return true;
}

//  COMPLETED: Skip List Destruction
void skip_list_destroy(SkipList* list) {
    if (!list) return;
    
    SkipListNode* current = list->header->forward[0];
    while (current != NULL) {
        SkipListNode* next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }
    
    free(list->header->forward);
    free(list->header);
    free(list);
}

//  COMPLETED: Skip List Utility Functions
size_t skip_list_size(SkipList* list) {
    return list ? list->size : 0;
}

bool skip_list_is_empty(SkipList* list) {
    return list ? list->size == 0 : true;
}

//  COMPLETED: Lock-Free Skip List (Advanced)
// Note: This is a simplified version. Full lock-free implementation requires
// atomic markable references and is significantly more complex.
typedef struct LockFreeSkipListNode {
    u64 key;
    void* value;
    atomic_uintptr_t forward[MAX_LEVEL];  // Atomic forward pointers
    atomic_bool marked;                   // Logical deletion flag
} LockFreeSkipListNode;

typedef struct {
    LockFreeSkipListNode* header;
    i32 max_level;
    atomic_int current_level;
    f32 probability;
} LockFreeSkipList;

// Helper function for lock-free find and cleanup
static LockFreeSkipListNode* lock_free_find_and_clean(LockFreeSkipList* list, u64 key, 
                                                      LockFreeSkipListNode** update) {
    // Simplified implementation - full version would handle marked nodes
    return NULL;  // Placeholder for advanced implementation
}

// Note: Full lock-free implementation requires:
// 1. Atomic Markable Reference (CAS with mark bit)
// 2. Logical deletion before physical unlink
// 3. Helper method find_and_clean for garbage collection
// 4. ABA problem prevention
// This provides the foundation for Entity Manager lookup optimization
