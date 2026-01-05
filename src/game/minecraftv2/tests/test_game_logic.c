#include <assert.h>
#include <stdio.h>
#include "game/game.h>

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("\n[Test] %s\n", name)
#define ASSERT(condition, message) do { \
    if (!(condition)) { \
        printf("  FAIL: %s\n", message); \
        tests_failed++; \
    } else { \
        printf("  PASS: %s\n", message); \
        tests_passed++; \
    } \
} while(0)

void test_game_initialization(void) {
    TEST("Game Initialization");

    int rc = game_init();
    ASSERT(rc == 0, "game_init returns 0");

    struct GameState *s = game_state();
    ASSERT(s != NULL, "game_state() returns non-NULL");
    ASSERT(s->running == 1, "game running flag is 1");
    ASSERT(s->tick_count == 0, "initial tick_count is 0");

    game_shutdown();
}

void test_game_tick_counter(void) {
    TEST("Game Tick Counter");

    game_init();

    game_tick(1.0f);
    game_tick(1.0f);
    struct GameState *s = game_state();
    ASSERT(s->tick_count == 2, "tick_count incremented to 2");

    game_tick(0.5f);
    ASSERT(s->tick_count == 3, "tick_count incremented to 3");

    game_shutdown();
}

void test_game_shutdown(void) {
    TEST("Game Shutdown");

    game_init();
    struct GameState *s = game_state();
    ASSERT(s->running == 1, "game running before shutdown");

    game_shutdown();
    ASSERT(s->running == 0, "game running flag is 0 after shutdown");
}

void test_tick_when_not_running(void) {
    TEST("Tick When Not Running");

    game_init();
    struct GameState *s = game_state();

    game_shutdown();
    uint64_t ticks = s->tick_count;

    game_tick(1.0f);
    ASSERT(s->tick_count == ticks, "tick_count unchanged when not running");
}

void test_subsystem_access(void) {
    TEST("Subsystem Access");

    game_init();

    struct ECSWorld *ecs = game_get_ecs_world();
    ASSERT(ecs != NULL, "ECS world is accessible");

    struct GameModeState *mode = game_get_mode_state();
    ASSERT(mode != NULL, "Game mode state is accessible");

    game_shutdown();
}

void test_sequential_sessions(void) {
    TEST("Sequential Sessions");

    // Session 1
    game_init();
    game_tick(0.016f);
    game_tick(0.016f);
    struct GameState *s = game_state();
    ASSERT(s->tick_count == 2, "Session 1: tick_count is 2");
    game_shutdown();

    // Session 2
    game_init();
    s = game_state();
    ASSERT(s->running == 1, "Session 2: game running");
    ASSERT(s->tick_count == 0, "Session 2: tick_count reset");
    game_tick(0.016f);
    ASSERT(s->tick_count == 1, "Session 2: tick_count incremented");
    game_shutdown();
}

int main(void) {
    printf("\n================================\n");
    printf("Game Logic Tests\n");
    printf("================================\n");

    test_game_initialization();
    test_game_tick_counter();
    test_game_shutdown();
    test_tick_when_not_running();
    test_subsystem_access();
    test_sequential_sessions();

    printf("\n================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("================================\n\n");

    if (tests_failed > 0) {
        printf("FAIL: Some tests failed\n");
        return 1;
    }

    printf("OK: All tests passed\n");
    return 0;
}
