/*
 * season_system_test.c
 * Five-minute seasons, init reset, remainder carry, and winter wrap.
 * Compiles season_system.c directly and does not link Engine.
 */

#include "season_system.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void expect_season(Season actual, Season expected, const char *msg) {
    if (actual != expected) {
        fprintf(stderr, "FAIL: %s (got %d, want %d)\n", msg, (int)actual,
                (int)expected);
        exit(1);
    }
}

static float quiet_nan(void) {
    union {
        uint32_t u;
        float f;
    } bits;
    bits.u = 0x7fc00000u;
    return bits.f;
}

int main(void) {
    season_system_init();
    expect_season(get_current_season(), SEASON_SPRING, "init starts in spring");

    season_system_update(0.0f);
    season_system_update(-5.0f);
    expect_season(get_current_season(), SEASON_SPRING, "non-positive dt is ignored");

    season_system_update(299.0f);
    expect_season(get_current_season(), SEASON_SPRING, "under five minutes stays spring");

    season_system_update(1.0f);
    expect_season(get_current_season(), SEASON_SUMMER, "exact five minutes becomes summer");

    season_system_init();
    season_system_update(301.0f);
    expect_season(get_current_season(), SEASON_SUMMER, "one second past spring is summer");
    season_system_update(299.0f);
    expect_season(get_current_season(), SEASON_AUTUMN,
                  "leftover second counts toward the next season");

    season_system_init();
    season_system_update(300.0f);
    season_system_update(300.0f);
    season_system_update(300.0f);
    expect_season(get_current_season(), SEASON_WINTER, "spring summer autumn then winter");
    season_system_update(300.0f);
    expect_season(get_current_season(), SEASON_SPRING, "winter wraps to spring");

    season_system_init();
    season_system_update(900.0f);
    expect_season(get_current_season(), SEASON_WINTER, "three seasons in one tick");
    season_system_update(50.0f);
    expect_season(get_current_season(), SEASON_WINTER, "exact boundary leaves no remainder");
    season_system_update(250.0f);
    expect_season(get_current_season(), SEASON_SPRING, "winter plus five minutes is spring");

    season_system_init();
    season_system_update(250.0f);
    season_system_update(900.0f);
    expect_season(get_current_season(), SEASON_WINTER,
                  "partial season plus three more lands on winter");
    season_system_update(50.0f);
    expect_season(get_current_season(), SEASON_SPRING, "carried 250s plus 50s advances");

    season_system_init();
    season_system_update(301.0f);
    expect_season(get_current_season(), SEASON_SUMMER, "setup one second into summer");
    season_system_init();
    season_system_update(299.0f);
    expect_season(get_current_season(), SEASON_SPRING, "init clears season and elapsed time");

    season_system_init();
    season_system_update(100.0f);
    season_system_update(quiet_nan());
    season_system_update(200.0f);
    expect_season(get_current_season(), SEASON_SUMMER, "non-finite dt does not drop elapsed time");

    printf("season_system_test: ok\n");
    return 0;
}
