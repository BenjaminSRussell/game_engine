/* Issue #402: unit tests for the core logic in
 * src/engine/scene/world_building/weather/weather_impl.c.
 *
 * weather_system.h declares none of the functions tested here, so their
 * prototypes are repeated below. The unit is compiled directly and is not
 * linked against Engine; see tests/registry/test_weather_impl.cmake.
 *
 * Not asserted: names with spaces do not survive weather_params_save and
 * weather_params_load (load reads one word per value), and load ignores
 * ambient_sound. Those gaps belong to the weather_impl.c bug work.
 */
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "weather_system.h"

#define WEATHER_TYPE_COUNT 15
#define PARAMS_FILE "weather_impl_test_params.cfg"

_Static_assert(WEATHER_SLEET + 1 == WEATHER_TYPE_COUNT,
               "WEATHER_TYPE_COUNT must match the WeatherType enum");

WeatherParameters *weather_params_create(WeatherType type);
bool weather_params_lerp(const WeatherParameters *from, const WeatherParameters *to,
                         float t, WeatherParameters *result);
bool weather_params_load(WeatherParameters *params, const char *filename);
bool weather_params_save(const WeatherParameters *params, const char *filename);

WeatherZone *weather_zone_create(const char *name, const float bounds_min[3],
                                 const float bounds_max[3], WeatherType forced_weather);
bool weather_zone_check_position(const WeatherZone *zone, const float position[3]);
bool weather_zone_blend(const WeatherZone *zone, const float position[3],
                        float *blend_factor);

bool transition_start(WeatherTransition *transition, WeatherType from_weather,
                      WeatherType to_weather, float duration);
bool transition_update(WeatherTransition *transition, float delta_time,
                       const WeatherParameters *to_params);
bool transition_complete(WeatherTransition *transition);
bool transition_interrupt(WeatherTransition *transition, WeatherType new_weather,
                          float new_duration);

/* weather_impl.c releases its presets through UNIFIED_FREE. This unit does not
 * link the allocator, so forward to libc free. Presets come from malloc. */
void unified_memory_free(void *ptr, const char *file, int line, const char *function) {
    (void)file;
    (void)line;
    (void)function;
    free(ptr);
}

static int checks_run = 0;
static int checks_failed = 0;

static void check_true(const char *file, int line, const char *expr, bool ok) {
    checks_run++;
    if (!ok) {
        checks_failed++;
        fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expr);
    }
}

static void check_near(const char *file, int line, const char *expr,
                       float actual, float expected, float tol) {
    checks_run++;
    if (!(fabsf(actual - expected) <= tol)) {
        checks_failed++;
        fprintf(stderr, "%s:%d: %s is %g, expected %g (tolerance %g)\n", file, line,
                expr, (double)actual, (double)expected, (double)tol);
    }
}

#define CHECK(expr) check_true(__FILE__, __LINE__, #expr, (expr) ? true : false)
#define CHECK_NEAR(actual, expected, tol) \
    check_near(__FILE__, __LINE__, #actual, (float)(actual), (float)(expected), (float)(tol))

/* =================================================================================================
 *                                    WEATHER PARAMETERS
 * =================================================================================================
 */

static void test_params_create_every_type(void) {
    for (int t = 0; t < WEATHER_TYPE_COUNT; t++) {
        WeatherParameters *p = weather_params_create((WeatherType)t);
        CHECK(p != NULL);
        if (!p) {
            continue;
        }
        CHECK(p->type == (WeatherType)t);
        CHECK(p->name[0] != '\0');
        CHECK(p->has_precipitation == (p->precipitation_intensity > 0.0f));
        CHECK(p->sun_intensity_multiplier > 0.0f && p->sun_intensity_multiplier <= 1.0f);
        CHECK(p->visibility_range > 0.0f);
        CHECK(p->ambient_volume >= 0.0f && p->ambient_volume <= 1.0f);
        CHECK(p->sky_color[0] >= 0.0f && p->sky_color[0] <= 1.0f);
        CHECK(p->sky_color[1] >= 0.0f && p->sky_color[1] <= 1.0f);
        CHECK(p->sky_color[2] >= 0.0f && p->sky_color[2] <= 1.0f);

        /* Common defaults are applied after the per-type switch. */
        CHECK_NEAR(p->cloud_speed, 1.0f, 1e-6f);
        CHECK_NEAR(p->cloud_height, 1000.0f, 1e-3f);
        CHECK_NEAR(p->wind_direction[0], 1.0f, 1e-6f);
        CHECK_NEAR(p->wind_direction[1], 0.0f, 1e-6f);
        CHECK_NEAR(p->wind_direction[2], 0.0f, 1e-6f);
        free(p);
    }
}

static void test_params_names_are_distinct(void) {
    WeatherParameters *presets[WEATHER_TYPE_COUNT];

    for (int a = 0; a < WEATHER_TYPE_COUNT; a++) {
        presets[a] = weather_params_create((WeatherType)a);
    }
    for (int a = 0; a < WEATHER_TYPE_COUNT; a++) {
        for (int b = a + 1; b < WEATHER_TYPE_COUNT; b++) {
            CHECK(presets[a] != NULL && presets[b] != NULL &&
                  strcmp(presets[a]->name, presets[b]->name) != 0);
        }
    }
    for (int a = 0; a < WEATHER_TYPE_COUNT; a++) {
        free(presets[a]);
    }
}

static void test_params_spot_values(void) {
    WeatherParameters *clear = weather_params_create(WEATHER_CLEAR);
    WeatherParameters *thunder = weather_params_create(WEATHER_THUNDERSTORM);
    WeatherParameters *fog = weather_params_create(WEATHER_FOG);

    CHECK(clear != NULL && thunder != NULL && fog != NULL);
    if (clear && thunder && fog) {
        CHECK(strcmp(clear->name, "Clear") == 0);
        CHECK(!clear->has_precipitation);
        CHECK_NEAR(clear->visibility_range, 1000.0f, 1e-3f);

        CHECK(strcmp(thunder->name, "Thunderstorm") == 0);
        CHECK(thunder->has_precipitation);
        CHECK_NEAR(thunder->precipitation_intensity, 1.0f, 1e-6f);
        CHECK_NEAR(thunder->visibility_range, 80.0f, 1e-3f);
        CHECK(strcmp(thunder->ambient_sound, "thunderstorm") == 0);

        CHECK(strcmp(fog->name, "Fog") == 0);
        CHECK(!fog->has_precipitation);
        CHECK_NEAR(fog->fog_density, 0.8f, 1e-6f);
        CHECK_NEAR(fog->fog_start_distance, 5.0f, 1e-6f);
        CHECK_NEAR(fog->fog_end_distance, 50.0f, 1e-6f);
    }

    free(clear);
    free(thunder);
    free(fog);
}

static void test_params_lerp(void) {
    WeatherParameters *from = weather_params_create(WEATHER_CLEAR);
    WeatherParameters *to = weather_params_create(WEATHER_RAIN);
    WeatherParameters out;

    CHECK(from != NULL && to != NULL);
    if (!from || !to) {
        free(from);
        free(to);
        return;
    }
    memset(&out, 0, sizeof out);

    /* Endpoints reproduce their inputs. */
    CHECK(weather_params_lerp(from, to, 0.0f, &out));
    CHECK(out.type == WEATHER_CLEAR);
    CHECK_NEAR(out.visibility_range, 1000.0f, 1e-3f);
    CHECK_NEAR(out.cloud_coverage, 0.0f, 1e-6f);

    CHECK(weather_params_lerp(from, to, 1.0f, &out));
    CHECK(out.type == WEATHER_RAIN);
    CHECK_NEAR(out.visibility_range, 200.0f, 1e-3f);
    CHECK_NEAR(out.cloud_coverage, 1.0f, 1e-6f);

    /* Float fields blend linearly between the two inputs. */
    CHECK(weather_params_lerp(from, to, 0.5f, &out));
    CHECK_NEAR(out.cloud_coverage, 0.5f, 1e-6f);
    CHECK_NEAR(out.visibility_range, 600.0f, 1e-3f);

    CHECK(weather_params_lerp(from, to, 0.25f, &out));
    CHECK_NEAR(out.visibility_range, 800.0f, 1e-3f);
    CHECK_NEAR(out.sky_color[0], 0.485f, 1e-5f);
    CHECK_NEAR(out.precipitation_intensity, 0.15f, 1e-5f);

    /* Discrete fields (type, name, has_precipitation) follow one input, not a blend. */
    CHECK(out.type == WEATHER_CLEAR);
    CHECK(!out.has_precipitation);
    CHECK(strcmp(out.name, "Clear") == 0);

    CHECK(weather_params_lerp(from, to, 0.75f, &out));
    CHECK(out.type == WEATHER_RAIN);
    CHECK(out.has_precipitation);
    CHECK(strcmp(out.name, "Rain") == 0);
    CHECK_NEAR(out.visibility_range, 400.0f, 1e-3f);

    /* Missing pointers and t outside [0, 1] are rejected. */
    CHECK(!weather_params_lerp(NULL, to, 0.5f, &out));
    CHECK(!weather_params_lerp(from, NULL, 0.5f, &out));
    CHECK(!weather_params_lerp(from, to, 0.5f, NULL));
    CHECK(!weather_params_lerp(from, to, -0.01f, &out));
    CHECK(!weather_params_lerp(from, to, 1.01f, &out));

    free(from);
    free(to);
}

static void test_params_save_and_load(void) {
    WeatherParameters *rain = weather_params_create(WEATHER_RAIN);
    WeatherParameters loaded;
    FILE *f;

    CHECK(rain != NULL);
    if (!rain) {
        return;
    }

    /* Round trip of the keys that weather_params_load reads. */
    CHECK(weather_params_save(rain, PARAMS_FILE));
    memset(&loaded, 0, sizeof loaded);
    CHECK(weather_params_load(&loaded, PARAMS_FILE));
    CHECK(strcmp(loaded.name, "Rain") == 0);
    CHECK_NEAR(loaded.sky_color[0], rain->sky_color[0], 1e-3f);
    CHECK_NEAR(loaded.sky_color[1], rain->sky_color[1], 1e-3f);
    CHECK_NEAR(loaded.sky_color[2], rain->sky_color[2], 1e-3f);
    CHECK_NEAR(loaded.cloud_coverage, rain->cloud_coverage, 1e-3f);
    CHECK_NEAR(loaded.precipitation_intensity, rain->precipitation_intensity, 1e-3f);
    CHECK_NEAR(loaded.visibility_range, rain->visibility_range, 0.05f);

    /* Hand-written file: unknown keys are skipped and known keys are applied. */
    f = fopen(PARAMS_FILE, "w");
    CHECK(f != NULL);
    if (f) {
        fputs("cloud_coverage = 0.375\n", f);
        fputs("unknown_key = 9.0\n", f);
        fputs("visibility_range = 250.5\n", f);
        fclose(f);
    }
    memset(&loaded, 0, sizeof loaded);
    CHECK(weather_params_load(&loaded, PARAMS_FILE));
    CHECK_NEAR(loaded.cloud_coverage, 0.375f, 1e-6f);
    CHECK_NEAR(loaded.visibility_range, 250.5f, 1e-4f);

    CHECK(!weather_params_load(NULL, PARAMS_FILE));
    CHECK(!weather_params_load(&loaded, NULL));
    CHECK(!weather_params_load(&loaded, "weather_impl_test_missing.cfg"));
    CHECK(!weather_params_save(NULL, PARAMS_FILE));
    CHECK(!weather_params_save(rain, NULL));
    CHECK(!weather_params_save(rain, "weather_impl_test_no_dir/params.cfg"));

    (void)remove(PARAMS_FILE);
    free(rain);
}

/* =================================================================================================
 *                                    WEATHER ZONES
 * =================================================================================================
 */

static void test_zones(void) {
    const float bounds_min[3] = {0.0f, 0.0f, 0.0f};
    const float bounds_max[3] = {100.0f, 50.0f, 100.0f};
    const float inside[3] = {50.0f, 25.0f, 50.0f};
    const float left_of[3] = {-0.1f, 25.0f, 50.0f};
    const float above[3] = {50.0f, 50.1f, 50.0f};
    const float near_min_x[3] = {3.0f, 25.0f, 50.0f};
    const float near_max_z[3] = {50.0f, 25.0f, 98.0f};
    const char *long_name = "ZoneNameLongerThanThirtyOneCharacters";
    WeatherZone *desert;
    WeatherZone *named;
    uint32_t first_id;
    uint32_t last_id;
    float blend = -1.0f;

    CHECK(weather_zone_create(NULL, bounds_min, bounds_max, WEATHER_CLEAR) == NULL);
    CHECK(weather_zone_create("Null", NULL, bounds_max, WEATHER_CLEAR) == NULL);
    CHECK(weather_zone_create("Null", bounds_min, NULL, WEATHER_CLEAR) == NULL);

    desert = weather_zone_create("Desert", bounds_min, bounds_max, WEATHER_SANDSTORM);
    CHECK(desert != NULL);
    if (!desert) {
        return;
    }
    first_id = desert->id;
    CHECK(first_id > 0);
    CHECK(strcmp(desert->name, "Desert") == 0);
    CHECK(desert->forced_weather == WEATHER_SANDSTORM);
    CHECK(desert->override_global);
    CHECK_NEAR(desert->blend_distance, 10.0f, 1e-6f);
    for (int i = 0; i < WEATHER_TYPE_COUNT; i++) {
        CHECK_NEAR(desert->probability_modifiers[i], 1.0f, 1e-6f);
    }

    /* Bounds are inclusive on every axis. */
    CHECK(weather_zone_check_position(desert, inside));
    CHECK(weather_zone_check_position(desert, bounds_min));
    CHECK(weather_zone_check_position(desert, bounds_max));
    CHECK(!weather_zone_check_position(desert, left_of));
    CHECK(!weather_zone_check_position(desert, above));
    CHECK(!weather_zone_check_position(NULL, inside));
    CHECK(!weather_zone_check_position(desert, NULL));

    /* Blend is 1 once a point is at least blend_distance from every edge, linear
     * within blend_distance of an edge, and 0 on the minimum corner. */
    CHECK(weather_zone_blend(desert, inside, &blend));
    CHECK_NEAR(blend, 1.0f, 1e-6f);
    CHECK(weather_zone_blend(desert, near_min_x, &blend));
    CHECK_NEAR(blend, 0.3f, 1e-5f);
    CHECK(weather_zone_blend(desert, near_max_z, &blend));
    CHECK_NEAR(blend, 0.2f, 1e-5f);
    CHECK(weather_zone_blend(desert, bounds_min, &blend));
    CHECK_NEAR(blend, 0.0f, 1e-6f);
    blend = -1.0f;
    CHECK(!weather_zone_blend(desert, left_of, &blend));
    CHECK_NEAR(blend, 0.0f, 1e-6f);
    CHECK(!weather_zone_blend(NULL, inside, &blend));
    CHECK(!weather_zone_blend(desert, inside, NULL));

    /* Names are stored in at most 31 characters plus the terminator. Ids count up. */
    named = weather_zone_create(long_name, bounds_min, bounds_max, WEATHER_CLEAR);
    CHECK(named != NULL);
    if (!named) {
        return;
    }
    CHECK(strlen(named->name) == 31);
    CHECK(strncmp(named->name, long_name, 31) == 0);
    CHECK(named->id == first_id + 1);

    /* Creating past the initial capacity reallocates the zone array. Each pointer
     * is used before the next create, since a reallocation invalidates it. */
    last_id = named->id;
    for (int i = 0; i < 20; i++) {
        WeatherZone *grown = weather_zone_create("Grow", bounds_min, bounds_max, WEATHER_CLEAR);
        CHECK(grown != NULL);
        if (!grown) {
            break;
        }
        CHECK(grown->id == last_id + 1);
        CHECK(strcmp(grown->name, "Grow") == 0);
        last_id = grown->id;
    }
}

/* =================================================================================================
 *                                    WEATHER TRANSITION
 * =================================================================================================
 */

static void test_transition_start_rejects_bad_input(void) {
    WeatherTransition t;
    memset(&t, 0, sizeof t);

    CHECK(!transition_start(NULL, WEATHER_CLEAR, WEATHER_RAIN, 1.0f));
    CHECK(!transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, 0.0f));
    CHECK(!transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, -1.0f));
    CHECK(!t.is_active);

    CHECK(transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, 4.0f));
    CHECK(t.is_active);
    CHECK(t.from_weather == WEATHER_CLEAR);
    CHECK(t.to_weather == WEATHER_RAIN);
    CHECK_NEAR(t.duration, 4.0f, 1e-6f);
    CHECK_NEAR(t.progress, 0.0f, 1e-6f);
    CHECK(t.current_params.type == WEATHER_CLEAR);
    CHECK(strcmp(t.current_params.name, "Clear") == 0);
}

static void test_transition_update_and_completion(void) {
    WeatherTransition t;
    WeatherParameters *to = weather_params_create(WEATHER_RAIN);

    CHECK(to != NULL);
    if (!to) {
        return;
    }
    memset(&t, 0, sizeof t);
    CHECK(!transition_update(&t, 1.0f, to));

    CHECK(transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, 4.0f));
    CHECK(!transition_update(NULL, 1.0f, to));
    CHECK(!transition_update(&t, 1.0f, NULL));

    /* 1 of 4 seconds: progress 0.25, so parameters are 75% from and 25% to. */
    CHECK(transition_update(&t, 1.0f, to));
    CHECK(t.is_active);
    CHECK_NEAR(t.progress, 0.25f, 1e-6f);
    CHECK_NEAR(t.current_params.cloud_coverage, 0.25f, 1e-5f);
    CHECK_NEAR(t.current_params.visibility_range, 800.0f, 1e-3f);
    CHECK_NEAR(t.current_params.sky_color[0], 0.485f, 1e-5f);
    CHECK(t.current_params.type == WEATHER_CLEAR);

    /* Reaching the duration completes the transition on the destination preset. */
    CHECK(transition_update(&t, 3.0f, to));
    CHECK(!t.is_active);
    CHECK_NEAR(t.progress, 1.0f, 1e-6f);
    CHECK(t.current_params.type == WEATHER_RAIN);
    CHECK(strcmp(t.current_params.name, "Rain") == 0);
    CHECK_NEAR(t.current_params.visibility_range, 200.0f, 1e-3f);

    CHECK(!transition_update(&t, 1.0f, to));
    free(to);
}

static void test_transition_complete_direct(void) {
    WeatherTransition t;
    memset(&t, 0, sizeof t);

    CHECK(!transition_complete(NULL));
    CHECK(transition_start(&t, WEATHER_FOG, WEATHER_SANDSTORM, 2.0f));
    CHECK(transition_complete(&t));
    CHECK(!t.is_active);
    CHECK_NEAR(t.progress, 1.0f, 1e-6f);
    CHECK(t.current_params.type == WEATHER_SANDSTORM);
    CHECK(strcmp(t.current_params.name, "Sandstorm") == 0);
    CHECK_NEAR(t.current_params.visibility_range, 40.0f, 1e-3f);
}

static void test_transition_interrupt(void) {
    WeatherTransition t;
    WeatherParameters *to = weather_params_create(WEATHER_RAIN);

    CHECK(to != NULL);
    if (!to) {
        return;
    }
    memset(&t, 0, sizeof t);
    CHECK(!transition_interrupt(NULL, WEATHER_SNOW, 2.0f));

    /* Before the midpoint, the new transition starts from the original source. */
    CHECK(transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, 4.0f));
    CHECK(transition_update(&t, 1.0f, to));
    CHECK(transition_interrupt(&t, WEATHER_SNOW, 2.0f));
    CHECK(t.is_active);
    CHECK(t.from_weather == WEATHER_CLEAR);
    CHECK(t.to_weather == WEATHER_SNOW);
    CHECK_NEAR(t.duration, 2.0f, 1e-6f);
    CHECK_NEAR(t.progress, 0.0f, 1e-6f);

    /* Past the midpoint, the new transition starts from the destination. */
    memset(&t, 0, sizeof t);
    CHECK(transition_start(&t, WEATHER_CLEAR, WEATHER_RAIN, 4.0f));
    CHECK(transition_update(&t, 3.0f, to));
    CHECK(transition_interrupt(&t, WEATHER_SNOW, 2.0f));
    CHECK(t.is_active);
    CHECK(t.from_weather == WEATHER_RAIN);
    CHECK(t.to_weather == WEATHER_SNOW);
    CHECK_NEAR(t.progress, 0.0f, 1e-6f);

    free(to);
}

int main(void) {
    test_params_create_every_type();
    test_params_names_are_distinct();
    test_params_spot_values();
    test_params_lerp();
    test_params_save_and_load();
    test_zones();
    test_transition_start_rejects_bad_input();
    test_transition_update_and_completion();
    test_transition_complete_direct();
    test_transition_interrupt();

    if (checks_failed != 0) {
        fprintf(stderr, "weather_impl: %d of %d checks failed\n", checks_failed, checks_run);
        return 1;
    }
    printf("weather_impl: %d checks passed\n", checks_run);
    return 0;
}
