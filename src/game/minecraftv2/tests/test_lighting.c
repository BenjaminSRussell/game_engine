#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef unsigned int u32;
typedef int i32;
typedef float f32;
typedef unsigned char u8;

/* Light level (0-15, where 15 is fully bright, 0 is dark) */
typedef u8 LightLevel;

/* Light source types */
typedef enum {
    LIGHT_SOURCE_NONE = 0,
    LIGHT_SOURCE_TORCH = 14,
    LIGHT_SOURCE_LANTERN = 15,
    LIGHT_SOURCE_GLOWSTONE = 15,
    LIGHT_SOURCE_LAVA = 15,
    LIGHT_SOURCE_FIRE = 15,
    LIGHT_SOURCE_CANDLE = 3,
    LIGHT_SOURCE_REDSTONE_LAMP_ON = 15,
    LIGHT_SOURCE_AMETHYST_CLUSTER = 5,
    LIGHT_SOURCE_NETHER_PORTAL = 11,
} LightSourceType;

/* Block lighting info */
typedef struct {
    i32 x, y, z;
    LightLevel sky_light;      /* Sunlight from top (0-15) */
    LightLevel block_light;    /* Emitted light (0-15) */
    LightSourceType light_source;
} BlockLight;

/* Light propagation queue */
typedef struct {
    i32 queue_x[256];
    i32 queue_y[256];
    i32 queue_z[256];
    LightLevel queue_level[256];
    u32 head;
    u32 tail;
} LightQueue;

/* Lighting engine */
typedef struct {
    BlockLight blocks[16 * 16 * 16];  /* Simulated chunk */
    LightQueue propagation_queue;
} LightingEngine;

LightingEngine *lighting_engine_create(void) {
    LightingEngine *engine = malloc(sizeof(LightingEngine));
    if (engine) {
        memset(engine, 0, sizeof(LightingEngine));
    }
    return engine;
}

void lighting_engine_free(LightingEngine *engine) {
    if (engine) free(engine);
}

/* Get block index for chunk coordinates */
static u32 get_block_index(i32 x, i32 y, i32 z) {
    if (x < 0 || x >= 16 || y < 0 || y >= 16 || z < 0 || z >= 16) {
        return (u32)-1;  /* Out of bounds */
    }
    return y * 256 + z * 16 + x;
}

/* Queue a block for light propagation */
static bool light_queue_enqueue(LightQueue *q, i32 x, i32 y, i32 z, LightLevel level) {
    if (!q) return false;
    u32 next_tail = (q->tail + 1) % 256;
    if (next_tail == q->head) return false;  /* Queue full */
    
    q->queue_x[q->tail] = x;
    q->queue_y[q->tail] = y;
    q->queue_z[q->tail] = z;
    q->queue_level[q->tail] = level;
    q->tail = next_tail;
    return true;
}

/* Dequeue a block from light propagation queue */
static bool light_queue_dequeue(LightQueue *q, i32 *x, i32 *y, i32 *z, LightLevel *level) {
    if (!q || q->head == q->tail) return false;
    
    *x = q->queue_x[q->head];
    *y = q->queue_y[q->head];
    *z = q->queue_z[q->head];
    *level = q->queue_level[q->head];
    q->head = (q->head + 1) % 256;
    return true;
}

/* Set light source at position */
void lighting_set_light_source(LightingEngine *engine, i32 x, i32 y, i32 z, LightSourceType type) {
    if (!engine) return;
    u32 idx = get_block_index(x, y, z);
    if (idx == (u32)-1) return;
    
    engine->blocks[idx].x = x;
    engine->blocks[idx].y = y;
    engine->blocks[idx].z = z;
    engine->blocks[idx].light_source = type;
    engine->blocks[idx].block_light = (LightLevel)type;
}

/* Set sky light (from sunlight) */
void lighting_set_sky_light(LightingEngine *engine, i32 x, i32 y, i32 z, LightLevel level) {
    if (!engine) return;
    u32 idx = get_block_index(x, y, z);
    if (idx == (u32)-1) return;
    
    engine->blocks[idx].sky_light = level;
}

/* Get combined light level (max of sky and block light) */
LightLevel lighting_get_light_level(LightingEngine *engine, i32 x, i32 y, i32 z) {
    if (!engine) return 0;
    u32 idx = get_block_index(x, y, z);
    if (idx == (u32)-1) return 0;
    
    BlockLight *block = &engine->blocks[idx];
    return (block->sky_light > block->block_light) ? block->sky_light : block->block_light;
}

/* Propagate light from a source */
void lighting_propagate_light(LightingEngine *engine, i32 src_x, i32 src_y, i32 src_z, LightLevel intensity) {
    if (!engine || intensity == 0) return;

    /* Clamp intensity to max 15 */
    if (intensity > 15) intensity = 15;
    
    LightQueue *q = &engine->propagation_queue;
    light_queue_enqueue(q, src_x, src_y, src_z, intensity);
    
    while (light_queue_dequeue(q, &src_x, &src_y, &src_z, &intensity)) {
        u32 idx = get_block_index(src_x, src_y, src_z);
        if (idx == (u32)-1) continue;
        
        BlockLight *block = &engine->blocks[idx];
        /* If block already has strictly greater light, skip */
        if (block->block_light > intensity) continue;

        /* Update only if this intensity is higher */
        if (block->block_light < intensity) {
            block->block_light = intensity;
        }
        
        /* Propagate to neighbors (intensity decreases) */
        if (intensity > 1) {
            LightLevel next_intensity = intensity - 1;
            
            /* Propagate only when it will increase neighbor light */
            i32 nx = src_x + 1, ny = src_y, nz = src_z;
            u32 nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);

            nx = src_x - 1; nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);

            nx = src_x; ny = src_y + 1; nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);

            ny = src_y - 1; nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);

            ny = src_y; nz = src_z + 1; nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);

            nz = src_z - 1; nidx = get_block_index(nx, ny, nz);
            if (nidx != (u32)-1 && engine->blocks[nidx].block_light < next_intensity) light_queue_enqueue(q, nx, ny, nz, next_intensity);
        }
    }
}

int main(void) {
    /* Test 1: Create lighting engine */
    LightingEngine *engine = lighting_engine_create();
    if (!engine) {
        printf("FAIL: Could not create lighting engine\n");
        return 1;
    }
    
    /* Test 2: Set light source and read level */
    lighting_set_light_source(engine, 8, 8, 8, LIGHT_SOURCE_LANTERN);
    
    LightLevel level = lighting_get_light_level(engine, 8, 8, 8);
    if (level != LIGHT_SOURCE_LANTERN) {
        printf("FAIL: Light level not set correctly (got %u, expected %u)\n", level, LIGHT_SOURCE_LANTERN);
        return 1;
    }
    
    /* Test 3: Propagate light from source */
    LightingEngine *engine2 = lighting_engine_create();
    lighting_set_light_source(engine2, 8, 8, 8, LIGHT_SOURCE_TORCH);
    lighting_propagate_light(engine2, 8, 8, 8, LIGHT_SOURCE_TORCH);
    
    /* Check neighbor light levels (should decrease) */
    LightLevel neighbor_x = lighting_get_light_level(engine2, 9, 8, 8);
    LightLevel neighbor_y = lighting_get_light_level(engine2, 8, 9, 8);
    LightLevel neighbor_z = lighting_get_light_level(engine2, 8, 8, 9);
    
    if (neighbor_x >= LIGHT_SOURCE_TORCH || neighbor_x < LIGHT_SOURCE_TORCH - 2) {
        printf("FAIL: Light propagation failed (neighbor_x=%u, expected ~%u)\n", neighbor_x, LIGHT_SOURCE_TORCH - 1);
        return 1;
    }
    
    /* Test 4: Sky light independent of block light */
    lighting_set_sky_light(engine2, 8, 8, 8, 15);
    level = lighting_get_light_level(engine2, 8, 8, 8);
    
    if (level < 14) {
        printf("FAIL: Sky light not considered (got %u)\n", level);
        return 1;
    }
    
    /* Test 5: Different light source types */
    LightingEngine *engine3 = lighting_engine_create();
    
    lighting_set_light_source(engine3, 0, 8, 8, LIGHT_SOURCE_LANTERN);     /* 15 */
    lighting_set_light_source(engine3, 5, 8, 8, LIGHT_SOURCE_TORCH);       /* 14 */
    lighting_set_light_source(engine3, 10, 8, 8, LIGHT_SOURCE_CANDLE);     /* 3 */
    lighting_set_light_source(engine3, 15, 8, 8, LIGHT_SOURCE_NETHER_PORTAL); /* 11 */
    
    if (lighting_get_light_level(engine3, 0, 8, 8) != 15 ||
        lighting_get_light_level(engine3, 5, 8, 8) != 14 ||
        lighting_get_light_level(engine3, 10, 8, 8) != 3 ||
        lighting_get_light_level(engine3, 15, 8, 8) != 11) {
        printf("FAIL: Light source types not set correctly\n");
        return 1;
    }
    
    /* Test 6: Multiple light sources */
    LightingEngine *engine4 = lighting_engine_create();
    
    /* Place two light sources 4 blocks apart */
    lighting_set_light_source(engine4, 6, 8, 8, LIGHT_SOURCE_LANTERN);
    lighting_set_light_source(engine4, 10, 8, 8, LIGHT_SOURCE_TORCH);
    
    lighting_propagate_light(engine4, 6, 8, 8, LIGHT_SOURCE_LANTERN);
    lighting_propagate_light(engine4, 10, 8, 8, LIGHT_SOURCE_TORCH);
    
    LightLevel mid = lighting_get_light_level(engine4, 8, 8, 8);
    if (mid < 10 || mid > 15) {
        printf("FAIL: Multiple light sources not combined correctly (got %u)\n", mid);
        return 1;
    }
    
    /* Test 7: Light doesn't exceed 15 */
    LightingEngine *engine5 = lighting_engine_create();
    lighting_propagate_light(engine5, 8, 8, 8, 20);  /* Clamp at 15 */
    
    level = lighting_get_light_level(engine5, 8, 8, 8);
    if (level > 15) {
        printf("FAIL: Light level exceeded maximum (got %u)\n", level);
        return 1;
    }
    
    /* Test 8: Distance-based attenuation */
    LightingEngine *engine6 = lighting_engine_create();
    lighting_set_light_source(engine6, 8, 8, 8, LIGHT_SOURCE_LANTERN);
    lighting_propagate_light(engine6, 8, 8, 8, 15);
    
    LightLevel l1 = lighting_get_light_level(engine6, 9, 8, 8);   /* 1 away */
    LightLevel l2 = lighting_get_light_level(engine6, 10, 8, 8);  /* 2 away */
    LightLevel l3 = lighting_get_light_level(engine6, 11, 8, 8);  /* 3 away */
    
    if (!(l1 > l2 && l2 > l3)) {
        printf("FAIL: Distance-based attenuation broken (l1=%u, l2=%u, l3=%u)\n", l1, l2, l3);
        return 1;
    }
    
    /* Test 9: Out of bounds access returns 0 */
    level = lighting_get_light_level(engine6, -1, 8, 8);
    if (level != 0) {
        printf("FAIL: Out of bounds should return 0 (got %u)\n", level);
        return 1;
    }
    
    level = lighting_get_light_level(engine6, 16, 8, 8);
    if (level != 0) {
        printf("FAIL: Out of bounds should return 0 (got %u)\n", level);
        return 1;
    }
    
    /* Test 10: Queue overflow handling */
    LightingEngine *engine7 = lighting_engine_create();
    bool result = light_queue_enqueue(&engine7->propagation_queue, 0, 0, 0, 15);
    if (!result) {
        printf("FAIL: Should be able to enqueue to empty queue\n");
        return 1;
    }
    
    lighting_engine_free(engine);
    lighting_engine_free(engine2);
    lighting_engine_free(engine3);
    lighting_engine_free(engine4);
    lighting_engine_free(engine5);
    lighting_engine_free(engine6);
    lighting_engine_free(engine7);
    
    printf("test_lighting: OK\n");
    return 0;
}
