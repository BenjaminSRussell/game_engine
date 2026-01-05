#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int u32;
typedef int i32;
typedef float f32;

/* Mock game state for testing */
typedef struct {
    u32 tick_count;
    f32 game_time;
    bool is_paused;
    i32 player_x, player_y, player_z;
    f32 player_health;
    u32 active_subsystems;
} GameState;

/* Subsystem flags */
#define SUBSYSTEM_PLAYER       (1 << 0)
#define SUBSYSTEM_PHYSICS      (1 << 1)
#define SUBSYSTEM_RENDER       (1 << 2)
#define SUBSYSTEM_AUDIO        (1 << 3)
#define SUBSYSTEM_CRAFTING     (1 << 4)
#define SUBSYSTEM_AI           (1 << 5)
#define SUBSYSTEM_WORLD        (1 << 6)
#define SUBSYSTEM_EFFECTS      (1 << 7)

typedef struct {
    GameState *state;
    u32 tick_delta;
    f32 time_scale;
} GameEngine;

GameEngine *game_create(void) {
    GameEngine *engine = malloc(sizeof(GameEngine));
    if (!engine) return NULL;
    
    engine->state = malloc(sizeof(GameState));
    if (!engine->state) {
        free(engine);
        return NULL;
    }
    
    memset(engine->state, 0, sizeof(GameState));
    engine->state->player_health = 20.0f;
    engine->tick_delta = 0;
    engine->time_scale = 1.0f;
    
    return engine;
}

void game_destroy(GameEngine *engine) {
    if (engine) {
        free(engine->state);
        free(engine);
    }
}

void game_enable_subsystem(GameEngine *engine, u32 subsystem) {
    if (engine) {
        engine->state->active_subsystems |= subsystem;
    }
}

void game_disable_subsystem(GameEngine *engine, u32 subsystem) {
    if (engine) {
        engine->state->active_subsystems &= ~subsystem;
    }
}

bool game_subsystem_active(GameEngine *engine, u32 subsystem) {
    if (engine) {
        return (engine->state->active_subsystems & subsystem) != 0;
    }
    return false;
}

void game_tick(GameEngine *engine, f32 delta_time) {
    if (!engine || engine->state->is_paused) return;
    
    GameState *state = engine->state;
    
    /* Advance game time */
    state->game_time += delta_time * engine->time_scale;
    state->tick_count++;
    engine->tick_delta++;
    
    /* Physics simulation */
    if (game_subsystem_active(engine, SUBSYSTEM_PHYSICS)) {
        /* Simulate gravity - player falls if not on ground */
        if (state->player_y > 0) {
            state->player_y--;
        }
    }
    
    /* World/AI update */
    if (game_subsystem_active(engine, SUBSYSTEM_WORLD)) {
        /* Update chunks, etc */
    }
    
    /* Crafting/inventory updates */
    if (game_subsystem_active(engine, SUBSYSTEM_CRAFTING)) {
        /* Process queued recipes */
    }
    
    /* Audio/particles */
    if (game_subsystem_active(engine, SUBSYSTEM_EFFECTS)) {
        /* Update active effects */
    }
}

int main(void) {
    /* Test 1: Create and destroy engine */
    GameEngine *engine = game_create();
    if (!engine || !engine->state) {
        printf("FAIL: Could not create game engine\n");
        return 1;
    }
    
    if (engine->state->tick_count != 0 || engine->state->game_time != 0.0f) {
        printf("FAIL: Initial state not zero\n");
        return 1;
    }
    
    /* Test 2: Enable subsystems */
    game_enable_subsystem(engine, SUBSYSTEM_PLAYER);
    game_enable_subsystem(engine, SUBSYSTEM_PHYSICS);
    
    if (!game_subsystem_active(engine, SUBSYSTEM_PLAYER) ||
        !game_subsystem_active(engine, SUBSYSTEM_PHYSICS)) {
        printf("FAIL: Subsystems not enabled\n");
        return 1;
    }
    
    if (game_subsystem_active(engine, SUBSYSTEM_AUDIO)) {
        printf("FAIL: Audio subsystem should not be active\n");
        return 1;
    }
    
    /* Test 3: Game tick increments time */
    game_tick(engine, 0.016f); /* 16ms frame */
    
    if (engine->state->tick_count != 1 || engine->state->game_time < 0.016f) {
        printf("FAIL: Tick not incremented (ticks=%u, time=%.4f)\n",
               engine->state->tick_count, engine->state->game_time);
        return 1;
    }
    
    /* Test 4: Physics simulation updates */
    engine->state->player_y = 100;
    game_tick(engine, 0.016f);
    
    if (engine->state->player_y != 99) {
        printf("FAIL: Player did not fall (y=%d)\n", engine->state->player_y);
        return 1;
    }
    
    /* Test 5: Time scale multiplier */
    f32 time_before = engine->state->game_time;
    engine->time_scale = 2.0f;
    game_tick(engine, 0.016f);
    f32 time_after = engine->state->game_time;
    
    if ((time_after - time_before) < 0.032f) {
        printf("FAIL: Time scale not applied (delta=%.4f)\n", time_after - time_before);
        return 1;
    }
    
    /* Test 6: Pause system */
    f32 paused_time = engine->state->game_time;
    engine->state->is_paused = true;
    game_tick(engine, 0.016f);
    
    if (engine->state->game_time != paused_time) {
        printf("FAIL: Time advanced while paused\n");
        return 1;
    }
    
    /* Test 7: Disable physics stops player falling */
    engine->state->is_paused = false;
    game_disable_subsystem(engine, SUBSYSTEM_PHYSICS);
    engine->state->player_y = 100;
    
    game_tick(engine, 0.016f);
    
    if (engine->state->player_y != 100) {
        printf("FAIL: Player fell with physics disabled\n");
        return 1;
    }
    
    /* Test 8: Multiple simultaneous subsystems */
    game_enable_subsystem(engine, SUBSYSTEM_PHYSICS);
    game_enable_subsystem(engine, SUBSYSTEM_CRAFTING);
    game_enable_subsystem(engine, SUBSYSTEM_AUDIO);
    game_enable_subsystem(engine, SUBSYSTEM_AI);
    
    u32 mask = SUBSYSTEM_PHYSICS | SUBSYSTEM_CRAFTING | SUBSYSTEM_AUDIO | SUBSYSTEM_AI;
    if ((engine->state->active_subsystems & mask) != mask) {
        printf("FAIL: Multiple subsystems not enabled properly\n");
        return 1;
    }
    
    /* Test 9: Tick count increases monotonically */
    u32 prev_ticks = engine->state->tick_count;
    for (int i = 0; i < 10; i++) {
        game_tick(engine, 0.016f);
    }
    
    if (engine->state->tick_count != prev_ticks + 10) {
        printf("FAIL: Tick count not monotonic (prev=%u, current=%u)\n",
               prev_ticks, engine->state->tick_count);
        return 1;
    }
    
    game_destroy(engine);
    
    printf("test_game_state: OK\n");
    return 0;
}
