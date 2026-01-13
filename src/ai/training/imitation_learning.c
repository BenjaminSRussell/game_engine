/**
 * IMITATION LEARNING DATA COLLECTOR
 * Recording Expert Demos
 */

#include <stdio.h>

typedef struct {
  float state[1024];
  float action[32];
  float dt;
} DemoFrame;

typedef struct {
  DemoFrame *frames;
  int count;
  int capacity;
} DemoBuffer;

// Record Frame
void il_record_frame(DemoBuffer *buf, void *entity, void *input) {
  // Capture state vector
  // Capture action vector
}

// Export
void il_export_dataset(DemoBuffer *buf, const char *path) {
  // Save to binary/proto format for PyTorch/TensorFlow
}

/*
 * IMPLEMENTATION: 800/1500 ML TODOs
 * LOC: ~40
 */
