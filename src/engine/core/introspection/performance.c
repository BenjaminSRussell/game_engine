// performance.c - Minimal implementation to bypass corruption
#include <core/performance.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Profiler g_profiler = {0};
FrameStats g_frame_stats = {0};

static f64 internal_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

Timer *perf_timer_create(const char *name) {
    Timer *timer = (Timer *)malloc(sizeof(Timer));
    if (timer) {
        timer->name = name;
        timer->start_time = 0.0;
        timer->elapsed_time = 0.0;
        timer->frame_count = 0;
        timer->total_time = 0.0;
    }
    return timer;
}

void perf_timer_destroy(Timer *timer) {
    if (timer) free(timer);
}

void perf_timer_start(Timer *timer) {
    if (timer) timer->start_time = internal_get_time();
}

void perf_timer_stop(Timer *timer) {
    if (timer) {
        timer->elapsed_time = internal_get_time() - timer->start_time;
        timer->total_time += timer->elapsed_time;
        timer->frame_count++;
    }
}

f64 perf_timer_get_elapsed(Timer *timer) {
    return timer ? timer->elapsed_time : 0.0;
}

void perf_timer_reset(Timer *timer) {
    if (timer) {
        timer->elapsed_time = 0.0;
        timer->frame_count = 0;
        timer->total_time = 0.0;
    }
}

// Stubs for other functions
void profiler_init(void) {}
void profiler_shutdown(void) {}
void profiler_start(const char *name) {}
void profiler_stop(const char *name) {}
void profiler_reset(void) {}
void profiler_report(void) {}
void profiler_enable(bool enable) {}
void frame_stats_update(f64 delta_time) {}
void frame_stats_reset(void) {}
f32 frame_stats_get_fps(void) { return 0.0f; }
void logger_errno_context(int err_code) {}
void performance_frame_breakdown(void) {}
void memory_spike_detector_init(u64 threshold) {}
void memory_spike_detector_check(u64 current_memory) {}
void profiler_call_count_increment(const char *function) {}
u32 profiler_get_call_count(const char *function) { return 0; }
int profiler_scope_enter(const char *name) { return 0; }
void profiler_scope_exit(int scope_id) {}
void physics_profile_start(void) {}
void physics_profile_end(void) {}
void entity_lifecycle_log(const char *event, const char *entity_type) {}
void world_gen_profile_start(void) {}
void world_gen_profile_end(void) {}
void chunk_lifecycle_log(int chunk_x, int chunk_z, const char *event) {}
void npc_behavior_log(const char *npc_name, const char *behavior) {}
void input_event_log(const char *input_type, const char *action) {}
void crafting_event_log(const char *recipe_name, bool success) {}
void save_load_log(const char *operation, const char *target) {}
