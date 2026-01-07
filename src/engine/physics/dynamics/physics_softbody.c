#include "physics/core/physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>
#include <stdbool.h>

// ROPE SIMULATION (Verlet integration)
typedef struct {
  float position[3], old_position[3];
  float mass;
  bool pinned;
} RopePoint;

typedef struct RopeSimulation {
  RopePoint *points;
  int point_count;
  float segment_length;
  float stiffness;
} RopeSimulation;

RopeSimulation *rope_create(float start[3], float end[3], int segments) {
  RopeSimulation *rope = (RopeSimulation*)calloc(1, sizeof(RopeSimulation));
  rope->point_count = segments + 1;
  rope->points = (RopePoint*)calloc(rope->point_count, sizeof(RopePoint));

  for (int i = 0; i <= segments; i++) {
    float t = (float)i / segments;
    rope->points[i].position[0] = start[0] + (end[0] - start[0]) * t;
    rope->points[i].position[1] = start[1] + (end[1] - start[1]) * t;
    rope->points[i].position[2] = start[2] + (end[2] - start[2]) * t;
    memcpy(rope->points[i].old_position, rope->points[i].position,
           sizeof(float) * 3);
    rope->points[i].mass = 1.0f;
  }

  rope->points[0].pinned = true;
  rope->segment_length = sqrtf((end[0] - start[0]) * (end[0] - start[0]) +
                               (end[1] - start[1]) * (end[1] - start[1]) +
                               (end[2] - start[2]) * (end[2] - start[2])) /
                         segments;
  rope->stiffness = 0.8f;

  return rope;
}

void rope_update(RopeSimulation *rope, float dt) {
  if (!rope) return;

  // Verlet integration
  for (int i = 0; i < rope->point_count; i++) {
    if (rope->points[i].pinned)
      continue;

    float temp[3];
    memcpy(temp, rope->points[i].position, sizeof(float) * 3);

    for (int j = 0; j < 3; j++) {
      rope->points[i].position[j] =
          2 * rope->points[i].position[j] - rope->points[i].old_position[j];
    }

    rope->points[i].position[1] -= 9.8f * dt * dt; // Gravity

    memcpy(rope->points[i].old_position, temp, sizeof(float) * 3);
  }

  // Constraint iteration
  for (int iter = 0; iter < 5; iter++) {
    for (int i = 0; i < rope->point_count - 1; i++) {
      float diff[3];
      for (int j = 0; j < 3; j++) {
        diff[j] = rope->points[i + 1].position[j] - rope->points[i].position[j];
      }

      float dist =
          sqrtf(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
      if (dist < 0.0001f) continue;
      
      float error = (dist - rope->segment_length) / dist;

      for (int j = 0; j < 3; j++) {
        float correction = diff[j] * error * rope->stiffness;
        if (!rope->points[i].pinned) {
          rope->points[i].position[j] += correction * 0.5f;
        }
        if (!rope->points[i + 1].pinned) {
          rope->points[i + 1].position[j] -= correction * 0.5f;
        }
      }
    }
  }
}
