// src/core/time_system.c
//
// Purpose: Time management system implementation
//
#include "../include/core/time_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// High-resolution timer
f64 time_get_high_res_time(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (f64)counter.QuadPart / (f64)frequency.QuadPart;
#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    return (f64)mach_absolute_time() * (f64)timebase.numer / (f64)timebase.denom / 1e9;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec + (f64)ts.tv_nsec / 1e9;
#endif
}

f64 time_get_elapsed_seconds(f64 start_time) {
    return time_get_high_res_time() - start_time;
}

u64 time_get_current_ms(void) {
    return (u64)(time_get_high_res_time() * 1000.0);
}

void time_system_init(TimeSystem* time, f32 fixed_timestep) {
    if (!time) return;
    
    memset(time, 0, sizeof(TimeSystem));
    time->start_time = time_get_high_res_time();
    time->current_time = time->start_time;
    time->last_frame_time = time->start_time;
    time->fixed_delta_time = fixed_timestep;
    time->fixed_step = fixed_timestep;
    time->time_scale = 1.0f;
    time->paused = false;
    time->accumulator = 0.0f;
    time->fps = 60.0f;
    time->min_frame_time_ms = 1000.0f;
    time->max_frame_time_ms = 0.0f;
}

void time_system_shutdown(TimeSystem* time) {
    (void)time;
}

void time_system_update(TimeSystem* time) {
    if (!time || time->paused) return;
    
    // Get current time
    time->current_time = time_get_high_res_time();
    
    // Calculate delta time
    f64 frame_time = time->current_time - time->last_frame_time;
    time->delta_time = (f32)frame_time * time->time_scale;
    time->last_frame_time = time->current_time;
    
    // Clamp delta time to prevent spiral of death
    const f32 MAX_DELTA_TIME = 0.25f; // 250ms max
    if (time->delta_time > MAX_DELTA_TIME) {
        time->delta_time = MAX_DELTA_TIME;
    }
    
    // Update accumulator for fixed timestep
    time->accumulator += time->delta_time;
    
    // Calculate interpolation factor
    if (time->accumulator >= time->fixed_step) {
        time->interpolation_factor = 1.0f;
    } else {
        time->interpolation_factor = time->accumulator / time->fixed_step;
    }
    
    // Update frame statistics
    time->frame_count++;
    time->frame_time_ms = time->delta_time * 1000.0f;
    
    // Calculate FPS (smoothed)
    const f32 ALPHA = 0.1f;
    f32 instant_fps = 1.0f / time->delta_time;
    time->fps = time->fps * (1.0f - ALPHA) + instant_fps * ALPHA;
    
    // Track min/max frame times
    if (time->frame_time_ms < time->min_frame_time_ms) {
        time->min_frame_time_ms = time->frame_time_ms;
    }
    if (time->frame_time_ms > time->max_frame_time_ms) {
        time->max_frame_time_ms = time->frame_time_ms;
    }
    
    // Apply slow motion if active
    if (time->slow_motion_active) {
        time->delta_time *= time->slow_motion_factor;
    }
}

void time_system_pause(TimeSystem* time) {
    if (time) {
        time->paused = true;
    }
}

void time_system_resume(TimeSystem* time) {
    if (time) {
        time->paused = false;
    }
}

void time_system_set_time_scale(TimeSystem* time, f32 scale) {
    if (time) {
        time->time_scale = scale;
        if (time->time_scale < 0.0f) time->time_scale = 0.0f;
        if (time->time_scale > 10.0f) time->time_scale = 10.0f;
    }
}

void time_system_set_slow_motion(TimeSystem* time, f32 factor, bool active) {
    if (time) {
        time->slow_motion_factor = factor;
        time->slow_motion_active = active;
    }
}

f32 time_system_get_delta_time(const TimeSystem* time) {
    return time ? time->delta_time : 0.0f;
}

f32 time_system_get_fixed_delta_time(const TimeSystem* time) {
    return time ? time->fixed_delta_time : 0.0f;
}

f64 time_system_get_time(const TimeSystem* time) {
    return time ? time->current_time : 0.0;
}

f64 time_system_get_total_time(const TimeSystem* time) {
    return time ? (time->current_time - time->start_time) : 0.0;
}

f32 time_system_get_fps(const TimeSystem* time) {
    return time ? time->fps : 0.0f;
}

f32 time_system_get_interpolation_factor(const TimeSystem* time) {
    return time ? time->interpolation_factor : 0.0f;
}

bool time_system_is_paused(const TimeSystem* time) {
    return time ? time->paused : false;
}

Timestamp time_get_timestamp(void) {
    Timestamp ts = {0};
    
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    ts.year = st.wYear;
    ts.month = st.wMonth;
    ts.day = st.wDay;
    ts.hour = st.wHour;
    ts.minute = st.wMinute;
    ts.second = st.wSecond;
    ts.millisecond = st.wMilliseconds;
#else
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    struct tm* tm_info = localtime(&spec.tv_sec);
    ts.year = tm_info->tm_year + 1900;
    ts.month = tm_info->tm_mon + 1;
    ts.day = tm_info->tm_mday;
    ts.hour = tm_info->tm_hour;
    ts.minute = tm_info->tm_min;
    ts.second = tm_info->tm_sec;
    ts.millisecond = (i32)(spec.tv_nsec / 1000000);
#endif
    
    return ts;
}

const char* time_format_timestamp(const Timestamp* ts, char* buffer, u32 size) {
    if (!ts || !buffer || size == 0) return NULL;
    
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             ts->year, ts->month, ts->day,
             ts->hour, ts->minute, ts->second, ts->millisecond);
    
    return buffer;
}


