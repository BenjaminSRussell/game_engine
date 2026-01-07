#include "core/containers_advanced/priority_queue.h"

#include <stdlib.h>
#include <string.h>

struct PriorityQueue {
  void **items;
  double *priorities;
  size_t size;
  size_t capacity;
  uint32_t arity;
};

static void pqueue_swap(PriorityQueue *queue, size_t a, size_t b) {
  void *tmp_item = queue->items[a];
  queue->items[a] = queue->items[b];
  queue->items[b] = tmp_item;

  double tmp_pri = queue->priorities[a];
  queue->priorities[a] = queue->priorities[b];
  queue->priorities[b] = tmp_pri;
}

static void pqueue_sift_up(PriorityQueue *queue, size_t index) {
  while (index > 0) {
    size_t parent = (index - 1) / queue->arity;
    if (queue->priorities[index] >= queue->priorities[parent]) {
      break;
    }
    pqueue_swap(queue, index, parent);
    index = parent;
  }
}

static void pqueue_sift_down(PriorityQueue *queue, size_t index) {
  for (;;) {
    size_t best = index;
    size_t first_child = index * queue->arity + 1;
    for (uint32_t i = 0; i < queue->arity; ++i) {
      size_t child = first_child + i;
      if (child >= queue->size) {
        break;
      }
      if (queue->priorities[child] < queue->priorities[best]) {
        best = child;
      }
    }
    if (best == index) {
      break;
    }
    pqueue_swap(queue, index, best);
    index = best;
  }
}

static bool pqueue_reserve(PriorityQueue *queue, size_t new_capacity) {
  if (new_capacity <= queue->capacity) {
    return true;
  }

  void **items = (void **)realloc(queue->items, new_capacity * sizeof(void *));
  double *priorities =
      (double *)realloc(queue->priorities, new_capacity * sizeof(double));
  if (!items || !priorities) {
    free(items);
    free(priorities);
    return false;
  }

  queue->items = items;
  queue->priorities = priorities;
  queue->capacity = new_capacity;
  return true;
}

PriorityQueue *pqueue_create(size_t capacity, uint32_t arity) {
  if (capacity == 0) {
    capacity = 16;
  }
  if (arity < 2) {
    arity = 2;
  }

  PriorityQueue *queue = (PriorityQueue *)calloc(1, sizeof(PriorityQueue));
  if (!queue) {
    return NULL;
  }

  queue->items = (void **)calloc(capacity, sizeof(void *));
  queue->priorities = (double *)calloc(capacity, sizeof(double));
  if (!queue->items || !queue->priorities) {
    free(queue->items);
    free(queue->priorities);
    free(queue);
    return NULL;
  }

  queue->size = 0;
  queue->capacity = capacity;
  queue->arity = arity;
  return queue;
}

PriorityQueue *pqueue_create_fibonacci(size_t capacity) {
  return pqueue_create(capacity, 2);
}

void pqueue_destroy(PriorityQueue *queue) {
  if (!queue) {
    return;
  }
  free(queue->items);
  free(queue->priorities);
  free(queue);
}

bool pqueue_push(PriorityQueue *queue, void *item, double priority) {
  if (!queue) {
    return false;
  }

  if (queue->size == queue->capacity) {
    if (!pqueue_reserve(queue, queue->capacity * 2)) {
      return false;
    }
  }

  queue->items[queue->size] = item;
  queue->priorities[queue->size] = priority;
  pqueue_sift_up(queue, queue->size);
  queue->size++;
  return true;
}

void *pqueue_pop(PriorityQueue *queue, double *out_priority) {
  if (!queue || queue->size == 0) {
    return NULL;
  }

  void *item = queue->items[0];
  if (out_priority) {
    *out_priority = queue->priorities[0];
  }

  queue->size--;
  if (queue->size > 0) {
    queue->items[0] = queue->items[queue->size];
    queue->priorities[0] = queue->priorities[queue->size];
    pqueue_sift_down(queue, 0);
  }

  return item;
}

bool pqueue_update_priority(PriorityQueue *queue, void *item, double priority) {
  if (!queue) {
    return false;
  }

  for (size_t i = 0; i < queue->size; ++i) {
    if (queue->items[i] == item) {
      double old = queue->priorities[i];
      queue->priorities[i] = priority;
      if (priority < old) {
        pqueue_sift_up(queue, i);
      } else if (priority > old) {
        pqueue_sift_down(queue, i);
      }
      return true;
    }
  }

  return false;
}

size_t pqueue_size(const PriorityQueue *queue) {
  return queue ? queue->size : 0;
}

bool pqueue_self_test(void) {
  PriorityQueue *queue = pqueue_create(4, 2);
  if (!queue) {
    return false;
  }

  int a = 1, b = 2, c = 3;
  pqueue_push(queue, &a, 3.0);
  pqueue_push(queue, &b, 1.0);
  pqueue_push(queue, &c, 2.0);

  double pri = 0.0;
  int *item = (int *)pqueue_pop(queue, &pri);
  bool ok = (item == &b) && (pri == 1.0);

  pqueue_destroy(queue);
  return ok;
}
