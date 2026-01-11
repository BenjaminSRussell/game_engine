#include <game/queue.h>
#include <stdlib.h>
#include <string.h>

void game_queue_init(GameQueue *q, u32 capacity, u32 element_size) {
  if (!q)
    return;
  q->data = malloc(capacity * element_size);
  q->capacity = capacity;
  q->element_size = element_size;
  q->head = 0;
  q->tail = 0;
}

void game_queue_free(GameQueue *q) {
  if (!q)
    return;
  if (q->data) {
    free(q->data);
    q->data = NULL;
  }
  q->capacity = 0;
  q->head = 0;
  q->tail = 0;
}

bool game_queue_enqueue(GameQueue *q, const void *item) {
  if (!q || !item)
    return false;
  u32 next = (q->tail + 1) % q->capacity;
  if (next == q->head)
    return false; /* queue full */
  memcpy((u8 *)q->data + q->tail * q->element_size, item, q->element_size);
  q->tail = next;
  return true;
}

bool game_queue_dequeue(GameQueue *q, void *out_item) {
  if (!q || !out_item)
    return false;
  if (q->head == q->tail)
    return false; /* queue empty */
  memcpy(out_item, (u8 *)q->data + q->head * q->element_size, q->element_size);
  q->head = (q->head + 1) % q->capacity;
  return true;
}

bool game_queue_peek(GameQueue *q, void *out_item) {
  if (!q || !out_item)
    return false;
  if (q->head == q->tail)
    return false; /* queue empty */
  memcpy(out_item, (u8 *)q->data + q->head * q->element_size, q->element_size);
  return true;
}

u32 game_queue_count(GameQueue *q) {
  if (!q)
    return 0;
  if (q->tail >= q->head)
    return q->tail - q->head;
  return q->capacity - q->head + q->tail;
}

bool game_queue_is_full(GameQueue *q) {
  if (!q)
    return true;
  return ((q->tail + 1) % q->capacity) == q->head;
}

bool game_queue_is_empty(GameQueue *q) {
  if (!q)
    return true;
  return q->head == q->tail;
}

void game_queue_clear(GameQueue *q) {
  if (!q)
    return;
  q->head = 0;
  q->tail = 0;
}
