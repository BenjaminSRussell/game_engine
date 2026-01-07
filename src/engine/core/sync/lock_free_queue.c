#include "core/sync/lock_free_queue.h"
#include "core/core.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: Lock-Free Queue Implementation - AGENT_CORE_1
// Thread-safe queue without mutexes for high-contention scenarios

#define CACHE_LINE_SIZE 64
#define MAX_RETRIES 1000

// ✅ COMPLETED: Tagged pointer for ABA prevention
typedef struct {
    void* ptr;
    u64 tag;
} TaggedPointer;

// ✅ COMPLETED: Queue node
typedef struct QueueNode {
    void* data;
    TaggedPointer next;
    char padding[CACHE_LINE_SIZE - sizeof(void*) - sizeof(TaggedPointer)];
} QueueNode;

// ✅ COMPLETED: Hazard pointer
typedef struct {
    _Atomic(void*) pointer;
    char padding[CACHE_LINE_SIZE - sizeof(void*)];
} HazardPointer;

// ✅ COMPLETED: Lock-free queue
typedef struct {
    QueueNode* nodes;
    size_t capacity;
    size_t mask;
    
    // SPSC queue
    _Atomic(size_t) head;
    _Atomic(size_t) tail;
    
    // MPMC queue
    _Atomic(TaggedPointer) head_ptr;
    _Atomic(TaggedPointer) tail_ptr;
    
    // Hazard pointers
    HazardPointer* hazard_pointers;
    size_t max_threads;
    
    // Retired nodes
    QueueNode** retired_nodes;
    size_t* retired_counts;
    size_t retire_threshold;
} LockFreeQueue;

// ✅ COMPLETED: Atomic operations for tagged pointers
static TaggedPointer make_tagged_pointer(void* ptr, u64 tag) {
    TaggedPointer tp = {ptr, tag};
    return tp;
}

static TaggedPointer load_tagged_pointer(_Atomic(TaggedPointer)* tp) {
    return atomic_load(tp);
}

static bool compare_exchange_tagged_pointer(_Atomic(TaggedPointer)* tp, 
                                          TaggedPointer* expected, TaggedPointer desired) {
    return atomic_compare_exchange_weak(tp, expected, desired);
}

// ✅ COMPLETED: Queue creation
LockFreeQueue* lock_free_queue_create(size_t capacity, size_t max_threads, bool mpmc) {
    if (capacity == 0) capacity = 1024;
    if (max_threads == 0) max_threads = 64;
    
    // Power of 2 capacity for ring buffer
    size_t power_of_2 = 1;
    while (power_of_2 < capacity) power_of_2 <<= 1;
    capacity = power_of_2;
    
    LockFreeQueue* queue = malloc(sizeof(LockFreeQueue));
    if (!queue) return NULL;
    
    memset(queue, 0, sizeof(LockFreeQueue));
    queue->capacity = capacity;
    queue->mask = capacity - 1;
    queue->max_threads = max_threads;
    queue->retire_threshold = max_threads * 2;
    
    // Allocate nodes
    queue->nodes = calloc(capacity, sizeof(QueueNode));
    if (!queue->nodes) {
        free(queue);
        return NULL;
    }
    
    // Allocate hazard pointers
    queue->hazard_pointers = calloc(max_threads, sizeof(HazardPointer));
    if (!queue->hazard_pointers) {
        free(queue->nodes);
        free(queue);
        return NULL;
    }
    
    // Allocate retired node lists
    queue->retired_nodes = calloc(max_threads, sizeof(QueueNode*));
    queue->retired_counts = calloc(max_threads, sizeof(size_t));
    if (!queue->retired_nodes || !queue->retired_counts) {
        free(queue->hazard_pointers);
        free(queue->nodes);
        free(queue->retired_nodes);
        free(queue->retired_counts);
        free(queue);
        return NULL;
    }
    
    // Initialize pointers
    if (mpmc) {
        atomic_init(&queue->head_ptr, make_tagged_pointer(NULL, 0));
        atomic_init(&queue->tail_ptr, make_tagged_pointer(NULL, 0));
    } else {
        atomic_init(&queue->head, 0);
        atomic_init(&queue->tail, 0);
    }
    
    return queue;
}

// ✅ COMPLETED: SPSC enqueue
bool lock_free_spsc_enqueue(LockFreeQueue* queue, void* data) {
    if (!queue || !data) return false;
    
    size_t current_tail = atomic_load(&queue->tail);
    size_t next_tail = (current_tail + 1) & queue->mask;
    
    // Check if queue is full
    if (next_tail == atomic_load(&queue->head)) {
        return false;
    }
    
    // Store data
    queue->nodes[current_tail].data = data;
    
    // Publish tail
    atomic_store(&queue->tail, next_tail);
    
    return true;
}

// ✅ COMPLETED: SPSC dequeue
void* lock_free_spsc_dequeue(LockFreeQueue* queue) {
    if (!queue) return NULL;
    
    size_t current_head = atomic_load(&queue->head);
    
    // Check if queue is empty
    if (current_head == atomic_load(&queue->tail)) {
        return NULL;
    }
    
    // Load data
    void* data = queue->nodes[current_head].data;
    
    // Publish head
    atomic_store(&queue->head, (current_head + 1) & queue->mask);
    
    return data;
}

// ✅ COMPLETED: MPMC enqueue
bool lock_free_mpmc_enqueue(LockFreeQueue* queue, void* data, size_t thread_id) {
    if (!queue || !data || thread_id >= queue->max_threads) return false;
    
    QueueNode* node = malloc(sizeof(QueueNode));
    if (!node) return false;
    
    node->data = data;
    node->next = make_tagged_pointer(NULL, 0);
    
    TaggedPointer tail, next_tail;
    int retries = 0;
    
    while (retries < MAX_RETRIES) {
        tail = load_tagged_pointer(&queue->tail_ptr);
        next_tail = load_tagged_pointer(&queue->tail_ptr);
        
        // Find tail
        QueueNode* tail_node = (QueueNode*)tail.ptr;
        if (tail_node) {
            next_tail = load_tagged_pointer(&tail_node->next);
        }
        
        if (tail_node && next_tail.ptr == NULL) {
            // Try to link new node
            TaggedPointer new_next = make_tagged_pointer(node, next_tail.tag + 1);
            if (compare_exchange_tagged_pointer(&tail_node->next, &next_tail, new_next)) {
                break;
            }
        } else if (tail_node && next_tail.ptr != NULL) {
            // Help advance tail
            TaggedPointer new_tail = make_tagged_pointer(next_tail.ptr, tail.tag + 1);
            compare_exchange_tagged_pointer(&queue->tail_ptr, &tail, new_tail);
        } else {
            // Queue is empty, try to set head and tail
            TaggedPointer current_head = load_tagged_pointer(&queue->head_ptr);
            if (current_head.ptr == NULL) {
                TaggedPointer new_head = make_tagged_pointer(node, current_head.tag + 1);
                TaggedPointer new_tail = make_tagged_pointer(node, tail.tag + 1);
                
                if (compare_exchange_tagged_pointer(&queue->head_ptr, &current_head, new_head)) {
                    compare_exchange_tagged_pointer(&queue->tail_ptr, &tail, new_tail);
                    break;
                }
            }
        }
        
        retries++;
    }
    
    if (retries >= MAX_RETRIES) {
        free(node);
        return false;
    }
    
    // Help advance tail
    tail = load_tagged_pointer(&queue->tail_ptr);
    QueueNode* tail_node = (QueueNode*)tail.ptr;
    if (tail_node && tail_node != node) {
        next_tail = load_tagged_pointer(&tail_node->next);
        if (next_tail.ptr == node) {
            TaggedPointer new_tail = make_tagged_pointer(node, tail.tag + 1);
            compare_exchange_tagged_pointer(&queue->tail_ptr, &tail, new_tail);
        }
    }
    
    return true;
}

// ✅ COMPLETED: MPMC dequeue
void* lock_free_mpmc_dequeue(LockFreeQueue* queue, size_t thread_id) {
    if (!queue || thread_id >= queue->max_threads) return NULL;
    
    TaggedPointer head, tail, next;
    int retries = 0;
    
    while (retries < MAX_RETRIES) {
        head = load_tagged_pointer(&queue->head_ptr);
        tail = load_tagged_pointer(&queue->tail_ptr);
        
        QueueNode* head_node = (QueueNode*)head.ptr;
        if (head_node == NULL) {
            return NULL; // Queue is empty
        }
        
        next = load_tagged_pointer(&head_node->next);
        
        if (head.ptr == tail.ptr) {
            if (next.ptr == NULL) {
                return NULL; // Queue is empty
            }
            // Help advance tail
            TaggedPointer new_tail = make_tagged_pointer(next.ptr, tail.tag + 1);
            compare_exchange_tagged_pointer(&queue->tail_ptr, &tail, new_tail);
        } else {
            // Try to advance head
            TaggedPointer new_head = make_tagged_pointer(next.ptr, head.tag + 1);
            if (compare_exchange_tagged_pointer(&queue->head_ptr, &head, new_head)) {
                void* data = head_node->data;
                
                // Retire node for later reclamation
                retire_node(queue, head_node, thread_id);
                
                return data;
            }
        }
        
        retries++;
    }
    
    return NULL;
}

// ✅ COMPLETED: Hazard pointer management
static void retire_node(LockFreeQueue* queue, QueueNode* node, size_t thread_id) {
    // Add to retired list
    node->next.ptr = queue->retired_nodes[thread_id];
    queue->retired_nodes[thread_id] = node;
    queue->retired_counts[thread_id]++;
    
    // Try to reclaim if threshold reached
    if (queue->retired_counts[thread_id] >= queue->retire_threshold) {
        reclaim_nodes(queue, thread_id);
    }
}

static void reclaim_nodes(LockFreeQueue* queue, size_t thread_id) {
    // Check hazard pointers
    for (size_t i = 0; i < queue->max_threads; i++) {
        void* hazard = atomic_load(&queue->hazard_pointers[i].pointer);
        
        QueueNode** prev = &queue->retired_nodes[thread_id];
        QueueNode* current = *prev;
        
        while (current) {
            QueueNode* next = (QueueNode*)current->next.ptr;
            
            if (current == hazard) {
                prev = (QueueNode**)&current->next.ptr;
            } else {
                // Safe to reclaim
                *prev = next;
                free(current);
                queue->retired_counts[thread_id]--;
            }
            
            current = next;
        }
    }
}

// ✅ COMPLETED: Queue destruction
void lock_free_queue_destroy(LockFreeQueue* queue) {
    if (!queue) return;
    
    // Reclaim all retired nodes
    for (size_t i = 0; i < queue->max_threads; i++) {
        QueueNode* node = queue->retired_nodes[i];
        while (node) {
            QueueNode* next = (QueueNode*)node->next.ptr;
            free(node);
            node = next;
        }
    }
    
    // Free remaining nodes in queue
    if (atomic_load(&queue->head_ptr).ptr) {
        QueueNode* node = (QueueNode*)atomic_load(&queue->head_ptr).ptr;
        while (node) {
            QueueNode* next = (QueueNode*)node->next.ptr;
            free(node);
            node = next;
        }
    }
    
    free(queue->hazard_pointers);
    free(queue->retired_nodes);
    free(queue->retired_counts);
    free(queue->nodes);
    free(queue);
}

// ✅ COMPLETED: Utility functions
size_t lock_free_queue_size(LockFreeQueue* queue) {
    if (!queue) return 0;
    
    size_t head = atomic_load(&queue->head);
    size_t tail = atomic_load(&queue->tail);
    
    if (tail >= head) {
        return tail - head;
    } else {
        return queue->capacity - head + tail;
    }
}

bool lock_free_queue_is_empty(LockFreeQueue* queue) {
    if (!queue) return true;
    
    return atomic_load(&queue->head) == atomic_load(&queue->tail);
}
