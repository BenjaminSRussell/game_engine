#ifndef SEASON_SYSTEM_H
#define SEASON_SYSTEM_H

typedef enum {
    SEASON_SPRING,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER
} Season;

void season_system_init(void);
void season_system_update(float dt);
Season get_current_season(void);

#endif // SEASON_SYSTEM_H
