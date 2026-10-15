/* Issue #424: weather_serialization.c must compile and round-trip its JSON and binary formats.
 * Engine does not build on main, so tests/registry/test_weather_serialization.cmake compiles
 * weather_serialization.c and weather_system.c (for weather_manager_init) directly.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "weather_system.h"

bool weather_manager_serialize(const WeatherManager *manager, char *buffer,
                               size_t buffer_size);
bool weather_manager_deserialize(WeatherManager *manager, const char *json_data);
bool weather_manager_serialize_binary(const WeatherManager *manager,
                                      uint8_t *buffer, size_t *size);
bool weather_manager_deserialize_binary(WeatherManager *manager,
                                        const uint8_t *buffer, size_t size);

/* weather_system.c releases memory through the unified allocator, which this target does not link. */
void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function) {
    (void)file;
    (void)line;
    (void)function;
    free(ptr);
}

#define JSON_CAPACITY (64u * 1024u)

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
            failures++;                                                      \
        }                                                                    \
    } while (0)

/* Zeroed storage. The deserializers call weather_manager_init themselves. */
static WeatherManager *zeroed_manager(void) {
    return (WeatherManager *)calloc(1, sizeof(WeatherManager));
}

static void free_manager(WeatherManager *manager) {
    if (manager) {
        weather_manager_shutdown(manager);
        free(manager);
    }
}

static WeatherManager *build_sample(void) {
    WeatherManager *m = zeroed_manager();
    if (!m) {
        return NULL;
    }
    weather_manager_init(m);

    m->current_weather = WEATHER_RAIN;
    m->transition.from_weather = WEATHER_CLEAR;
    m->transition.to_weather = WEATHER_THUNDERSTORM;
    m->transition.duration = 2.5f;
    m->transition.progress = 0.25f;
    m->transition.is_active = true;

    m->time_of_day.current_time = 13.5f;
    m->time_of_day.time_scale = 60.0f;
    m->time_of_day.is_paused = true;
    m->time_of_day.day = 3u;
    m->time_of_day.month = 7u;
    m->time_of_day.year = 2026u;
    m->time_of_day.day_length_hours = 24.0f;
    m->time_of_day.current_settings.sun_elevation = 0.5f;
    m->time_of_day.current_settings.sun_color[1] = 0.75f;
    m->time_of_day.current_settings.fog_color[2] = 0.125f;

    m->clouds.enabled = false;
    m->clouds.coverage = 0.75f;
    m->clouds.wind_direction[1] = 0.5f;
    m->clouds.shape_noise = 42u;
    m->clouds.ray_march_steps = 96;

    m->precipitation.active = true;
    m->precipitation.type = WEATHER_RAIN;
    m->precipitation.intensity = 0.5f;
    m->precipitation.max_particles = 4096u;
    m->precipitation.splash_texture = 11u;

    m->lightning.strike_frequency = 0.125f;
    m->lightning.trigger_thunder = false;
    m->lightning.flash_duration = 0.25f;

    m->auto_weather_enabled = false;
    m->weather_change_interval = 300.0f;
    m->weather_probabilities[2] = 0.5f;
    m->seasonal_weather = true;
    m->current_season = 2u;

    snprintf(m->weather_presets[3].name, sizeof(m->weather_presets[3].name),
             "%s", "Custom");
    m->weather_presets[3].cloud_coverage = 0.875f;
    m->weather_presets[3].sky_color[0] = 0.5f;

    m->zones = (WeatherZone *)calloc(2, sizeof(WeatherZone));
    if (!m->zones) {
        free_manager(m);
        return NULL;
    }
    m->zone_count = 2u;
    m->zones[0].id = 7u;
    /* Commas and braces inside a string must not confuse the scanner. */
    snprintf(m->zones[0].name, sizeof(m->zones[0].name), "%s",
             "Storm, north}");
    m->zones[0].bounds_min[0] = 1.0f;
    m->zones[0].bounds_min[1] = 2.0f;
    m->zones[0].bounds_min[2] = 3.0f;
    m->zones[0].bounds_max[0] = 4.0f;
    m->zones[0].bounds_max[1] = 5.0f;
    m->zones[0].bounds_max[2] = 6.0f;
    m->zones[0].blend_distance = 8.0f;
    m->zones[0].forced_weather = WEATHER_FOG;
    m->zones[0].override_global = true;
    m->zones[1].id = 9u;
    snprintf(m->zones[1].name, sizeof(m->zones[1].name), "%s", "Plains");
    return m;
}

/* Everything except zones. The binary format carries this part. */
static void check_scalars(const WeatherManager *m) {
    CHECK(m->current_weather == WEATHER_RAIN);
    CHECK(m->transition.from_weather == WEATHER_CLEAR);
    CHECK(m->transition.to_weather == WEATHER_THUNDERSTORM);
    CHECK(m->transition.duration == 2.5f);
    CHECK(m->transition.progress == 0.25f);
    CHECK(m->transition.is_active);

    CHECK(m->time_of_day.current_time == 13.5f);
    CHECK(m->time_of_day.time_scale == 60.0f);
    CHECK(m->time_of_day.is_paused);
    CHECK(m->time_of_day.day == 3u);
    CHECK(m->time_of_day.month == 7u);
    CHECK(m->time_of_day.year == 2026u);
    CHECK(m->time_of_day.day_length_hours == 24.0f);
    CHECK(m->time_of_day.current_settings.sun_elevation == 0.5f);
    CHECK(m->time_of_day.current_settings.sun_color[1] == 0.75f);
    CHECK(m->time_of_day.current_settings.fog_color[2] == 0.125f);

    CHECK(!m->clouds.enabled);
    CHECK(m->clouds.coverage == 0.75f);
    CHECK(m->clouds.wind_direction[1] == 0.5f);
    CHECK(m->clouds.shape_noise == 42u);
    CHECK(m->clouds.ray_march_steps == 96);

    CHECK(m->precipitation.active);
    CHECK(m->precipitation.type == WEATHER_RAIN);
    CHECK(m->precipitation.intensity == 0.5f);
    CHECK(m->precipitation.max_particles == 4096u);
    CHECK(m->precipitation.splash_texture == 11u);

    CHECK(m->lightning.strike_frequency == 0.125f);
    CHECK(!m->lightning.trigger_thunder);
    CHECK(m->lightning.flash_duration == 0.25f);

    CHECK(!m->auto_weather_enabled);
    CHECK(m->weather_change_interval == 300.0f);
    CHECK(m->weather_probabilities[2] == 0.5f);
    CHECK(m->seasonal_weather);
    CHECK(m->current_season == 2u);

    CHECK(strcmp(m->weather_presets[3].name, "Custom") == 0);
    CHECK(m->weather_presets[3].cloud_coverage == 0.875f);
    CHECK(m->weather_presets[3].sky_color[0] == 0.5f);
}

static void check_zones(const WeatherManager *m) {
    CHECK(m->zone_count == 2u);
    if (m->zone_count != 2u || !m->zones) {
        return;
    }
    CHECK(m->zones[0].id == 7u);
    CHECK(strcmp(m->zones[0].name, "Storm, north}") == 0);
    CHECK(m->zones[0].bounds_min[0] == 1.0f);
    CHECK(m->zones[0].bounds_min[1] == 2.0f);
    CHECK(m->zones[0].bounds_min[2] == 3.0f);
    CHECK(m->zones[0].bounds_max[2] == 6.0f);
    CHECK(m->zones[0].blend_distance == 8.0f);
    CHECK(m->zones[0].forced_weather == WEATHER_FOG);
    CHECK(m->zones[0].override_global);
    CHECK(m->zones[1].id == 9u);
    CHECK(strcmp(m->zones[1].name, "Plains") == 0);
    CHECK(!m->zones[1].override_global);
}

/* True if a ',' is followed (ignoring whitespace) by '}' or ']'. The writer must not emit those. */
static bool has_trailing_comma(const char *text) {
    for (const char *p = text; *p != '\0'; p++) {
        if (*p != ',') {
            continue;
        }
        const char *q = p + 1;
        while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
            q++;
        }
        if (*q == '}' || *q == ']') {
            return true;
        }
    }
    return false;
}

static void test_json_round_trip(void) {
    WeatherManager *src = build_sample();
    WeatherManager *dst = zeroed_manager();
    char *json = (char *)malloc(JSON_CAPACITY);
    CHECK(src != NULL);
    CHECK(dst != NULL);
    CHECK(json != NULL);
    if (!src || !dst || !json) {
        free(json);
        free_manager(dst);
        free_manager(src);
        return;
    }

    CHECK(weather_manager_serialize(src, json, JSON_CAPACITY));
    CHECK(!has_trailing_comma(json));
    CHECK(weather_manager_deserialize(dst, json));
    check_scalars(dst);
    check_zones(dst);

    free(json);
    free_manager(dst);
    free_manager(src);
}

/* Hand-written JSON: nested objects, zone array followed by other objects, and a probability array. */
static const char kHandWritten[] =
    "{ \"version\": 1, \"current_weather\": 4, "
    "\"transition\": { \"from_weather\": 1, \"to_weather\": 3, \"duration\": 2.0, "
    "\"progress\": 0.5, \"is_active\": true }, "
    "\"zones\": [ { \"id\": 1, \"name\": \"A\", \"bounds_min\": [0, 0, 0], "
    "\"bounds_max\": [1, 1, 1], \"blend_distance\": 2.0, \"forced_weather\": 5, "
    "\"override_global\": false }, { \"id\": 2, \"name\": \"B\", "
    "\"bounds_min\": [1, 1, 1], \"bounds_max\": [2, 2, 2], \"blend_distance\": 3.0, "
    "\"forced_weather\": 0, \"override_global\": true } ], "
    "\"auto_weather\": { \"enabled\": false, \"change_interval\": 90.0, "
    "\"next_change\": 45.0, \"probabilities\": [0.0, 0.5, 1.0, 0.25, 0.0, 0.0, 0.0, "
    "0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0] }, "
    "\"seasonal\": { \"enabled\": true, \"current_season\": 3 } }";

static void test_json_hand_written(void) {
    WeatherManager *m = zeroed_manager();
    CHECK(m != NULL);
    if (!m) {
        return;
    }
    CHECK(weather_manager_deserialize(m, kHandWritten));

    CHECK(m->current_weather == WEATHER_LIGHT_RAIN);
    CHECK(m->transition.from_weather == WEATHER_PARTLY_CLOUDY);
    CHECK(m->transition.to_weather == WEATHER_OVERCAST);
    CHECK(m->transition.duration == 2.0f);
    CHECK(m->transition.progress == 0.5f);
    CHECK(m->transition.is_active);

    /* The probability array is followed by other objects; zone_count must stop at the zone array. */
    CHECK(m->zone_count == 2u);
    if (m->zone_count == 2u && m->zones) {
        CHECK(m->zones[0].id == 1u);
        CHECK(strcmp(m->zones[0].name, "A") == 0);
        CHECK(m->zones[0].bounds_max[1] == 1.0f);
        CHECK(m->zones[0].forced_weather == WEATHER_RAIN);
        CHECK(!m->zones[0].override_global);
        CHECK(m->zones[1].id == 2u);
        CHECK(strcmp(m->zones[1].name, "B") == 0);
        CHECK(m->zones[1].bounds_min[0] == 1.0f);
        CHECK(m->zones[1].blend_distance == 3.0f);
        CHECK(m->zones[1].override_global);
    }

    CHECK(!m->auto_weather_enabled);
    CHECK(m->weather_change_interval == 90.0f);
    CHECK(m->next_weather_change == 45.0f);
    CHECK(m->weather_probabilities[1] == 0.5f);
    CHECK(m->weather_probabilities[14] == 1.0f);

    CHECK(m->seasonal_weather);
    CHECK(m->current_season == 3u);

    free_manager(m);
}

static void test_json_rejects_bad_values(void) {
    WeatherManager *m = NULL;

    /* A number followed by junk is not an int. The field keeps its init default. */
    m = zeroed_manager();
    CHECK(m != NULL);
    if (m) {
        CHECK(weather_manager_deserialize(
            m, "{ \"version\": 1, \"current_weather\": 4x }"));
        CHECK(m->current_weather == WEATHER_CLEAR);
        free_manager(m);
    }

    /* Out-of-range weather index: it would index weather_presets, so it is ignored. */
    m = zeroed_manager();
    CHECK(m != NULL);
    if (m) {
        CHECK(weather_manager_deserialize(
            m, "{ \"version\": 1, \"current_weather\": 99 }"));
        CHECK(m->current_weather == WEATHER_CLEAR);
        free_manager(m);
    }

    /* Wrong format version is rejected outright. */
    m = zeroed_manager();
    CHECK(m != NULL);
    if (m) {
        CHECK(!weather_manager_deserialize(m, "{ \"version\": 2 }"));
        free_manager(m);
    }

    /* A string longer than the 32-byte name field is rejected, not truncated silently. */
    m = zeroed_manager();
    CHECK(m != NULL);
    if (m) {
        CHECK(weather_manager_deserialize(
            m, "{ \"version\": 1, \"zones\": [ { \"id\": 5, "
               "\"name\": \"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\" } ] }"));
        CHECK(m->zone_count == 1u);
        if (m->zone_count == 1u && m->zones) {
            CHECK(m->zones[0].id == 5u);
            CHECK(m->zones[0].name[0] == '\0');
        }
        free_manager(m);
    }
}

static void test_binary_round_trip(void) {
    const size_t required = 3u * sizeof(uint32_t) + sizeof(WeatherManager);
    size_t size = required + 256u;
    uint8_t *bin = NULL;
    WeatherManager *src = build_sample();
    WeatherManager *dst = zeroed_manager();
    size_t small = required - 1u;

    CHECK(src != NULL);
    CHECK(dst != NULL);
    bin = (uint8_t *)calloc(1, size);
    CHECK(bin != NULL);
    if (!src || !dst || !bin) {
        free(bin);
        free_manager(dst);
        free_manager(src);
        return;
    }

    /* Caller buffer is larger than needed; the call reports the exact size used. */
    CHECK(weather_manager_serialize_binary(src, bin, &size));
    CHECK(size == required);

    CHECK(weather_manager_deserialize_binary(dst, bin, size));
    check_scalars(dst);
    /* Pointers are process-local and are not restored from binary data. */
    CHECK(dst->zones == NULL);
    CHECK(dst->zone_count == 0u);

    /* A flipped payload byte fails the CRC. */
    bin[size - 1u] ^= 0x5Au;
    CHECK(!weather_manager_deserialize_binary(dst, bin, size));
    bin[size - 1u] ^= 0x5Au;

    /* A wrong magic number is rejected. */
    bin[0] ^= 0xFFu;
    CHECK(!weather_manager_deserialize_binary(dst, bin, size));
    bin[0] ^= 0xFFu;

    /* A buffer that is too small fails and reports the required size. */
    CHECK(!weather_manager_serialize_binary(src, bin, &small));
    CHECK(small == required);

    free(bin);
    free_manager(dst);
    free_manager(src);
}

int main(void) {
    test_json_round_trip();
    test_json_hand_written();
    test_json_rejects_bad_values();
    test_binary_round_trip();

    if (failures != 0) {
        fprintf(stderr, "test_weather_serialization: %d check(s) failed\n",
                failures);
        return 1;
    }
    printf("test_weather_serialization: all checks passed\n");
    return 0;
}
