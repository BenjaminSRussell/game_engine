// include/core/time_system.h
//
// Purpose: Comprehensive time management system for the engine
//
#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#include <common.h>

// Time system state
typedef struct {
  f64 start_time;
  f64 current_time;
  f64 last_frame_time;
  f32 delta_time;
  f32 fixed_delta_time;
  f32 time_scale;
  bool paused;

  // Fixed timestep accumulator
  f32 accumulator;
  f32 fixed_step;

  // Frame statistics
  f32 fps;
  f32 frame_time_ms;
  f32 min_frame_time_ms;
  f32 max_frame_time_ms;
  u64 frame_count;

  // Time scaling
  f32 slow_motion_factor;
  bool slow_motion_active;

  // Interpolation factor for fixed timestep
  f32 interpolation_factor;
} TimeSystem;

// Initialize time system
void time_system_init(TimeSystem *time, f32 fixed_timestep);
void time_system_shutdown(TimeSystem *time);

// Update time (call once per frame)
void time_system_update(TimeSystem *time);

// Time control
void time_system_pause(TimeSystem *time);
void time_system_resume(TimeSystem *time);
void time_system_set_time_scale(TimeSystem *time, f32 scale);
void time_system_set_slow_motion(TimeSystem *time, f32 factor, bool active);

// Getters
f32 time_system_get_delta_time(const TimeSystem *time);
f32 time_system_get_fixed_delta_time(const TimeSystem *time);
f64 time_system_get_time(const TimeSystem *time);
f64 time_system_get_total_time(const TimeSystem *time);
f32 time_system_get_fps(const TimeSystem *time);
f32 time_system_get_interpolation_factor(const TimeSystem *time);
bool time_system_is_paused(const TimeSystem *time);

// High-resolution timer
f64 time_get_high_res_time(void);
f64 time_get_elapsed_seconds(f64 start_time);
u64 time_get_current_ms(void);
u64 get_time_nanos(void);
f32 nanos_to_ms(u64 nanos);

// Timestamp utilities
typedef struct {
  i32 year;
  i32 month;
  i32 day;
  i32 hour;
  i32 minute;
  i32 second;
  i32 millisecond;
} DateTime;

DateTime time_get_timestamp(void);
const char *time_format_timestamp(const DateTime *ts, char *buffer, u32 size);

#endif // TIME_SYSTEM_H
