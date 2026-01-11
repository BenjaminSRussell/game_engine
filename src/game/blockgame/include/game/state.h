#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdint.h>

struct GameState {
    int running;        /* 1 if running, 0 otherwise */
    uint64_t tick_count; /* number of ticks processed */
};

#endif /* GAME_STATE_H */
