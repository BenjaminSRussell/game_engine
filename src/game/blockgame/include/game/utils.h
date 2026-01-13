#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "engine/include/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Scalable math utilities to replace inline MIN/MAX/CLAMP */
static inline i32 utils_min_i32(i32 a, i32 b) { return (a < b) ? a : b; }
static inline i32 utils_max_i32(i32 a, i32 b) { return (a > b) ? a : b; }
static inline i32 utils_clamp_i32(i32 v, i32 min_v, i32 max_v) {
    return (v < min_v) ? min_v : (v > max_v) ? max_v : v;
}

static inline f32 utils_min_f32(f32 a, f32 b) { return (a < b) ? a : b; }
static inline f32 utils_max_f32(f32 a, f32 b) { return (a > b) ? a : b; }
static inline f32 utils_clamp_f32(f32 v, f32 min_v, f32 max_v) {
    return (v < min_v) ? min_v : (v > max_v) ? max_v : v;
}

static inline u32 utils_min_u32(u32 a, u32 b) { return (a < b) ? a : b; }
static inline u32 utils_max_u32(u32 a, u32 b) { return (a > b) ? a : b; }

/* Adjacency checker: returns count of neighbors == adjacent_value around target position */
i32 utils_count_adjacent_blocks(void *chunk_getter, i32 x, i32 y, i32 z,
                                 void *getter_closure, u32 block_type);

#ifdef __cplusplus
}
#endif

#endif /* GAME_UTILS_H */
