#ifndef UI_LAYOUT_PROFILING_H
#define UI_LAYOUT_PROFILING_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t total_layout_count;
    uint64_t total_layout_time_ns;
    uint64_t last_layout_time_ns;
    uint64_t max_layout_time_ns;
    uint64_t min_layout_time_ns;
    uint64_t avg_layout_time_ns;

    // Detailed phases
    uint64_t measure_time_ns;
    uint64_t arrange_time_ns; // distribute/position

    // Counter for current frame/update
    uint32_t layout_iterations_current;
} LayoutPerformanceStats;

static inline void layout_profiling_reset(LayoutPerformanceStats* stats) {
    if (stats) {
        memset(stats, 0, sizeof(LayoutPerformanceStats));
        stats->min_layout_time_ns = UINT64_MAX;
    }
}

static inline void layout_profiling_update(LayoutPerformanceStats* stats, uint64_t duration_ns, uint64_t measure_ns, uint64_t arrange_ns) {
    if (!stats) return;

    stats->total_layout_count++;
    stats->total_layout_time_ns += duration_ns;
    stats->last_layout_time_ns = duration_ns;

    if (duration_ns > stats->max_layout_time_ns) {
        stats->max_layout_time_ns = duration_ns;
    }

    if (duration_ns < stats->min_layout_time_ns) {
        stats->min_layout_time_ns = duration_ns;
    }

    stats->avg_layout_time_ns = stats->total_layout_time_ns / stats->total_layout_count;

    stats->measure_time_ns = measure_ns;
    stats->arrange_time_ns = arrange_ns;
}

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_PROFILING_H */
