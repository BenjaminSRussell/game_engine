#include "ai/ai_types.h"
#include "core/core.h"
#include <string.h>
#include <stdlib.h>

#define MAX_EPISODES 1000
#define MAX_STATE_SIZE 64

typedef struct {
    float state[MAX_STATE_SIZE];
    int action;
    float reward;
} Experience;

typedef struct {
    Experience *buffer;
    int capacity;
    int write_index;
    int count;
    char mode[32]; // "imitation" or "reinforcement"
} DataPipeline;

static DataPipeline g_pipeline = {0};

void data_pipeline_init(int buffer_size, const char *mode) {
    g_pipeline.buffer = calloc(buffer_size, sizeof(Experience));
    g_pipeline.capacity = buffer_size;
    g_pipeline.write_index = 0;
    g_pipeline.count = 0;
    strncpy(g_pipeline.mode, mode, sizeof(g_pipeline.mode) - 1);
}

void data_pipeline_record(float *state, int state_size, int action, float reward) {
    if (!g_pipeline.buffer) return;
    
    Experience *exp = &g_pipeline.buffer[g_pipeline.write_index];
    memcpy(exp->state, state, sizeof(float) * state_size);
    exp->action = action;
    exp->reward = reward;
    
    g_pipeline.write_index = (g_pipeline.write_index + 1) % g_pipeline.capacity;
    if (g_pipeline.count < g_pipeline.capacity) g_pipeline.count++;
}

void data_pipeline_export_csv(const char *filename) {
    // Export to CSV for Python training
    FILE *f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "state,action,reward\n");
    for (int i = 0; i < g_pipeline.count; i++) {
        Experience *exp = &g_pipeline.buffer[i];
        fprintf(f, "\"");
        for (int j = 0; j < MAX_STATE_SIZE; j++) {
            fprintf(f, "%f", exp->state[j]);
            if (j < MAX_STATE_SIZE - 1) fprintf(f, ",");
        }
        fprintf(f, "\",%d,%f\n", exp->action, exp->reward);
    }
    
    fclose(f);
}

void data_pipeline_sample_balanced(Experience *output, int count) {
    // Balanced sampling (simplified - random sampling for now)
    for (int i = 0; i < count; i++) {
        int idx = rand() % g_pipeline.count;
        output[i] = g_pipeline.buffer[idx];
    }
}

void data_pipeline_shutdown() {
    if (g_pipeline.buffer) free(g_pipeline.buffer);
    g_pipeline.buffer = NULL;
}
