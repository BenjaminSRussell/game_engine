#ifndef GAME_QUEUE_H
#define GAME_QUEUE_H

#include "engine/include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generic FIFO queue (reusable for crafting, processing, events, etc.) */
typedef struct {
    void *data;
    u32 capacity;
    u32 element_size;
    u32 head;
    u32 tail;
} GameQueue;

void game_queue_init(GameQueue *q, u32 capacity, u32 element_size);
void game_queue_free(GameQueue *q);

bool game_queue_enqueue(GameQueue *q, const void *item);
bool game_queue_dequeue(GameQueue *q, void *out_item);
bool game_queue_peek(GameQueue *q, void *out_item);

u32 game_queue_count(GameQueue *q);
bool game_queue_is_full(GameQueue *q);
bool game_queue_is_empty(GameQueue *q);
void game_queue_clear(GameQueue *q);

#ifdef __cplusplus
}
#endif

#endif /* GAME_QUEUE_H */
