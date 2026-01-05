#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/* Minimal stubs for testing */
typedef unsigned int u32;
typedef int i32;
typedef float f32;
typedef unsigned char u8;
typedef unsigned long long u64;

#define CHUNK_SIZE 16

/* Test fixtures */
typedef struct {
    u32 block_id;
    i32 x, y, z;
    f32 velocity_y;
    bool has_gravity;
    bool falling;
} FallingBlockEntity;

typedef struct {
    FallingBlockEntity entities[64];
    u32 count;
} FallingBlockTest;

FallingBlockTest *test_create(void) {
    FallingBlockTest *test = malloc(sizeof(FallingBlockTest));
    if (test) {
        memset(test, 0, sizeof(FallingBlockTest));
    }
    return test;
}

void test_free(FallingBlockTest *test) {
    if (test) free(test);
}

/* Falling block simulation - basic gravity physics */
void falling_block_add(FallingBlockTest *test, u32 block_id, i32 x, i32 y, i32 z) {
    if (!test || test->count >= 64) return;
    FallingBlockEntity *entity = &test->entities[test->count++];
    entity->block_id = block_id;
    entity->x = x;
    entity->y = y;
    entity->z = z;
    entity->velocity_y = 0.0f;
    entity->has_gravity = true;
    entity->falling = true;
}

void falling_block_update(FallingBlockTest *test, f32 delta_time) {
    if (!test) return;
    
    const f32 GRAVITY = 32.0f; /* acceleration m/s^2 */
    
    for (u32 i = 0; i < test->count; i++) {
        FallingBlockEntity *entity = &test->entities[i];
        if (!entity->falling) continue;
        
        /* Apply gravity */
        entity->velocity_y -= GRAVITY * delta_time;
        
        /* Update position */
        f32 displacement = entity->velocity_y * delta_time;
        entity->y = (i32)(entity->y + displacement);
        
        /* Stop at y=0 (bedrock) */
        if (entity->y <= 0) {
            entity->y = 0;
            entity->velocity_y = 0;
            entity->falling = false;
        }
    }
}

int main(void) {
    FallingBlockTest *test = test_create();
    if (!test) {
        printf("FAIL: Could not create test\n");
        return 1;
    }
    
    /* Test 1: Single block falls */
    falling_block_add(test, 1, 0, 64, 0); /* Sand block at y=64 */
    if (test->count != 1 || test->entities[0].block_id != 1 || test->entities[0].y != 64) {
        printf("FAIL: Failed to add falling block\n");
        return 1;
    }
    
    /* Test 2: Gravity applied each frame */
    falling_block_update(test, 0.05f); /* 50ms frame */
    if (test->entities[0].velocity_y >= 0) {
        printf("FAIL: Gravity not applied (velocity_y=%.2f)\n", test->entities[0].velocity_y);
        return 1;
    }
    
    /* Test 3: Multiple blocks fall independently */
    FallingBlockTest *test2 = test_create();
    falling_block_add(test2, 1, 0, 64, 0); /* Sand */
    falling_block_add(test2, 2, 1, 32, 0); /* Gravel at lower height */
    
    falling_block_update(test2, 0.05f);
    
    /* Both blocks should have fallen (negative velocity) */
    if (test2->entities[0].velocity_y >= 0 || test2->entities[1].velocity_y >= 0) {
        printf("FAIL: Block physics not applied (v1=%.2f, v2=%.2f)\n", 
               test2->entities[0].velocity_y, test2->entities[1].velocity_y);
        return 1;
    }
    
    /* Test 4: Block stops at ground */
    FallingBlockTest *test3 = test_create();
    falling_block_add(test3, 1, 0, 1, 0); /* Sand block 1 unit above ground */
    
    /* Fall for 0.2 seconds (enough to hit ground) */
    for (int i = 0; i < 4; i++) {
        falling_block_update(test3, 0.05f);
    }
    
    if (test3->entities[0].y != 0 || test3->entities[0].falling) {
        printf("FAIL: Block not stopped at ground (y=%d, falling=%d)\n",
               test3->entities[0].y, test3->entities[0].falling);
        return 1;
    }
    
    /* Test 5: Multiple simultaneous falling blocks */
    FallingBlockTest *test4 = test_create();
    for (i32 i = 0; i < 10; i++) {
        falling_block_add(test4, 1, i % 4, 64 + i, 0);
    }
    
    if (test4->count != 10) {
        printf("FAIL: Failed to add 10 falling blocks (count=%u)\n", test4->count);
        return 1;
    }
    
    /* Update all */
    for (int frame = 0; frame < 5; frame++) {
        falling_block_update(test4, 0.05f);
    }
    
    /* All blocks should have fallen */
    for (u32 i = 0; i < test4->count; i++) {
        if (test4->entities[i].velocity_y >= 0) {
            printf("FAIL: Block %u not affected by gravity\n", i);
            return 1;
        }
    }
    
    test_free(test);
    test_free(test2);
    test_free(test3);
    test_free(test4);
    
    printf("test_falling_blocks: OK\n");
    return 0;
}
