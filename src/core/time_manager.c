/*
 * time_manager.c
 * Time management system implementation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 * 
 * Provides time tracking, scaling, and high-resolution timing functionality
 */

// #include "core/time/time_manager.h"  // Header not found - stubbed for compilation
#include <time.h>

static double g_time_start = 0.0;
static double g_time_scale = 1.0;

void time_init() {
    g_time_start = (double)clock() / CLOCKS_PER_SEC;
}

double time_get_seconds() {
    return ((double)clock() / CLOCKS_PER_SEC - g_time_start) * g_time_scale;
}

double time_get_milliseconds() {
    return time_get_seconds() * 1000.0;
}

void time_set_scale(double scale) {
    g_time_scale = scale;
}

double time_get_scale() {
    return g_time_scale;
}
