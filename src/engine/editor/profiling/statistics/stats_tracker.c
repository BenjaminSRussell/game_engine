#include "editor/profiling/statistics/stats_tracker.h"
#include <string.h>

static stat_metric_t g_stats[MAX_STATS];
static u32 g_stat_count = 0;

void stats_init(void) {
    memset(g_stats, 0, sizeof(g_stats));
    g_stat_count = 0;
}

void stats_update_metric(const char* name, float value) {
    stat_metric_t* metric = NULL;
    for (u32 i = 0; i < g_stat_count; i++) {
        if (strcmp(g_stats[i].name, name) == 0) {
            metric = &g_stats[i];
            break;
        }
    }
    
    if (!metric && g_stat_count < MAX_STATS) {
        metric = &g_stats[g_stat_count++];
        strncpy(metric->name, name, 31);
        metric->min = value;
        metric->max = value;
    }
    
    if (metric) {
        metric->current = value;
        if (value < metric->min) metric->min = value;
        if (value > metric->max) metric->max = value;
        
        // Rolling average
        metric->sample_count++;
        metric->avg = metric->avg + (value - metric->avg) / (float)metric->sample_count;
        
        // History buffer
        metric->history[metric->history_index] = value;
        metric->history_index = (metric->history_index + 1) % STAT_HISTORY_SIZE;
    }
}

stat_metric_t* stats_get_metric(const char* name) {
    for (u32 i = 0; i < g_stat_count; i++) {
        if (strcmp(g_stats[i].name, name) == 0) {
            return &g_stats[i];
        }
    }
    return NULL;
}
