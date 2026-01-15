#ifndef INIT_PIPELINE_H
#define INIT_PIPELINE_H

#include <stdbool.h>

// Error codes for initialization
typedef enum {
  INIT_SUCCESS = 0,
  INIT_ERROR_CONFIG,
  INIT_ERROR_WINDOW,
  INIT_ERROR_RENDERER,
  INIT_ERROR_AUDIO,
  INIT_ERROR_WEATHER,
  INIT_ERROR_PHYSICS,
  INIT_ERROR_ECS,
  INIT_ERROR_RESOURCES,
  INIT_ERROR_THREADING,
  INIT_ERROR_VFS,
  INIT_ERROR_SAVE_SYSTEM
} InitError;

typedef struct {
  bool success;
  InitError error;
  const char *message;
} InitResult;

// Initialize the entire game pipeline
// Returns result struct indicating success or specific failure
InitResult game_init(void);

#endif
