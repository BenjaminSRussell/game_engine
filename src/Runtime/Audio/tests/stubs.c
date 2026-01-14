#include <core/services/weather_system.h>
#include <include/core/asset_importers.h>
#include <include/physics/physics.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "engine/include/core/logger.h"

// Weather System Stubs
WeatherType weather_sys_get_type(void) {
    return WEATHER_CLEAR;
}

float weather_sys_get_rain_intensity(void) {
    return 0.0f;
}

// Asset Importer Stubs
ImportedAudio* asset_importer_load_audio(const char* filepath) {
    fprintf(stderr, "[STUB] Loading audio: %s\n", filepath);
    return NULL;
}

// Physics Stubs
RaycastResult physics_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction, float max_distance) {
    RaycastResult result = {0};
    result.hit = false;
    return result;
}

// Logger Stubs
void logger_log(LogLevel level, const char *category, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}
