#include "season_system.h"
#include <stdio.h>

static Season current_season = SEASON_SPRING;
static float time_in_season = 0.0f;
static const float season_duration = 60.0f * 5; // 5 minutes per season

void season_system_init() {
    printf("Season system initialized.\n");
}

void season_system_update(float dt) {
    time_in_season += dt;
    if (time_in_season > season_duration) {
        time_in_season = 0;
        current_season = (current_season + 1) % 4;
        
        switch(current_season) {
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
        }
    }
}

Season get_current_season() {
    return current_season;
}

