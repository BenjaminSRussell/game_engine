#ifndef RENDERING_STATS_TRACKER_H
#define RENDERING_STATS_TRACKER_H

#include "include/common.h"

#define MAX_STATS 64
#define STAT_HISTORY_SIZE 128

typedef struct stat_metric {
    char name[32];
    float current;
    float min;
    float max;
    float avg;
    float history[STAT_HISTORY_SIZE];
    u32 history_index;
    u32 sample_count;
} stat_metric_t;

void stats_init(void);
void stats_update_metric(const char* name, float value);
stat_metric_t* stats_get_metric(const char* name);

#endif // RENDERING_STATS_TRACKER_H
