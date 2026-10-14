/* Issue #423: transition_update must call the real transition_complete.
 * weather_system.h's size_t include is issue #260; this TU includes stddef first.
 * Not linked against Engine.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "weather_system.h"

bool transition_start(WeatherTransition *transition, WeatherType from_weather,
                      WeatherType to_weather, float duration);
bool transition_update(WeatherTransition *transition, float delta_time,
                       const WeatherParameters *to_params);
bool transition_complete(WeatherTransition *transition);
WeatherParameters *weather_params_create(WeatherType type);

void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function) {
    (void)file;
    (void)line;
    (void)function;
    free(ptr);
}

static int fail(const char *msg) {
    fprintf(stderr, "weather transition: %s\n", msg);
    return 1;
}

int main(void) {
    WeatherTransition transition;
    WeatherParameters *to_params;
    bool ok;

    if (transition_complete(NULL)) {
        return fail("null transition_complete should be false");
    }

    memset(&transition, 0, sizeof(transition));
    if (!transition_start(&transition, WEATHER_CLEAR, WEATHER_RAIN, 1.0f)) {
        return fail("transition_start failed");
    }
    if (!transition.is_active || transition.progress != 0.0f) {
        return fail("transition did not start active");
    }

    to_params = weather_params_create(WEATHER_RAIN);
    if (!to_params) {
        return fail("weather_params_create failed");
    }

    ok = transition_update(&transition, 2.0f, to_params);
    free(to_params);
    if (!ok) {
        return fail("transition_update did not complete");
    }
    if (transition.is_active) {
        return fail("completed transition still active");
    }
    if (transition.progress != 1.0f) {
        return fail("completed progress is not 1");
    }
    if (transition.current_params.type != WEATHER_RAIN) {
        return fail("completed params are not the destination weather");
    }
    if (strcmp(transition.current_params.name, "Rain") != 0) {
        return fail("completed params did not copy the rain preset");
    }

    printf("weather transition_complete ok\n");
    return 0;
}
