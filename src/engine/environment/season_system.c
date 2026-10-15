#include "season_system.h"
#include <stdio.h>

static Season current_season = SEASON_SPRING;
static float time_in_season = 0.0f;
static const float season_duration = 60.0f * 5.0f; /* 5 minutes per season */

static void announce_season(Season season) {
    switch (season) {
        case SEASON_SPRING:
            printf("The season has changed to Spring.\n");
            break;
        case SEASON_SUMMER:
            printf("The season has changed to Summer.\n");
            break;
        case SEASON_AUTUMN:
            printf("The season has changed to Autumn.\n");
            break;
        case SEASON_WINTER:
            printf("The season has changed to Winter.\n");
            break;
        default:
            break;
    }
}

void season_system_init(void) {
    current_season = SEASON_SPRING;
    time_in_season = 0.0f;
    printf("Season system initialized.\n");
}

void season_system_update(float dt) {
    int steps;
    float whole;

    /* Ignore non-positive and non-finite dt so the clock cannot rewind or stall. */
    if (!(dt > 0.0f) || dt != dt) {
        return;
    }
    /* One tick may cross many seasons, but not enough to overflow the step count. */
    if (dt > season_duration * 64.0f) {
        dt = season_duration * 64.0f;
    }

    time_in_season += dt;
    if (time_in_season < season_duration) {
        return;
    }

    whole = time_in_season / season_duration;
    steps = (int)whole;
    if (steps < 1) {
        steps = 1;
    }
    time_in_season -= (float)steps * season_duration;
    if (time_in_season < 0.0f) {
        time_in_season = 0.0f;
    }
    current_season = (Season)(((int)current_season + steps) % 4);
    announce_season(current_season);
}

Season get_current_season(void) {
    return current_season;
}
