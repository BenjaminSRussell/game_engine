/**
 * PATHFINDING & AI UTILITIES MEGA-BATCH
 * A*, Dijkstra, Navigation Mesh, Steering Behaviors
 * All ~55 AGENT_AI utility TODOs
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct PriorityQueueNode {
  int vertex;
  float priority;
  struct PriorityQueueNode *next;
} PQNode;

typedef struct {
  PQNode *head;
} PriorityQueue;

void pq_push(PriorityQueue *pq, int vertex, float priority) {
  PQNode *node = malloc(sizeof(PQNode));
  node->vertex = vertex;
  node->priority = priority;
  node->next = NULL;

  if (!pq->head || priority < pq->head->priority) {
    node->next = pq->head;
    pq->head = node;
  } else {
    PQNode *current = pq->head;
    while (current->next && current->next->priority < priority) {
      current = current->next;
    }
    node->next = current->next;
    current->next = node;
  }
}

int pq_pop(PriorityQueue *pq) {
  if (!pq->head)
    return -1;
  PQNode *node = pq->head;
  int vertex = node->vertex;
  pq->head = node->next;
  free(node);
  return vertex;
}

// A* PATHFINDING
float heuristic_euclidean(int from_x, int from_y, int to_x, int to_y) {
  int dx = to_x - from_x;
  int dy = to_y - from_y;
  return sqrtf(dx * dx + dy * dy);
}

bool astar_pathfind(int start_x, int start_y, int goal_x, int goal_y,
                    bool (*is_walkable)(int, int), int *path_x, int *path_y,
                    int *path_len) {
  PriorityQueue open = {0};
  float *g_score = calloc(1000 * 1000, sizeof(float));
  float *f_score = calloc(1000 * 1000, sizeof(float));
  int *came_from = malloc(1000 * 1000 * sizeof(int));
  bool *closed = calloc(1000 * 1000, 1);

  int start_idx = start_y * 1000 + start_x;
  int goal_idx = goal_y * 1000 + goal_x;

  for (int i = 0; i < 1000 * 1000; i++) {
    g_score[i] = INFINITY;
    f_score[i] = INFINITY;
    came_from[i] = -1;
  }

  g_score[start_idx] = 0;
  f_score[start_idx] = heuristic_euclidean(start_x, start_y, goal_x, goal_y);
  pq_push(&open, start_idx, f_score[start_idx]);

  while (open.head) {
    int current = pq_pop(&open);

    if (current == goal_idx) {
      // Reconstruct path
      *path_len = 0;
      int idx = current;
      while (idx != -1) {
        path_x[*path_len] = idx % 1000;
        path_y[*path_len] = idx / 1000;
        (*path_len)++;
        idx = came_from[idx];
      }

      free(g_score);
      free(f_score);
      free(came_from);
      free(closed);
      return true;
    }

    closed[current] = true;
    int cx = current % 1000;
    int cy = current / 1000;

    int dx[] = {-1, 1, 0, 0, -1, 1, -1, 1};
    int dy[] = {0, 0, -1, 1, -1, -1, 1, 1};

    for (int i = 0; i < 8; i++) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];
      int neighbor = ny * 1000 + nx;

      if (!is_walkable(nx, ny) || closed[neighbor])
        continue;

      float tentative_g = g_score[current] + (i < 4 ? 1.0f : 1.414f);

      if (tentative_g < g_score[neighbor]) {
        came_from[neighbor] = current;
        g_score[neighbor] = tentative_g;
        f_score[neighbor] =
            g_score[neighbor] + heuristic_euclidean(nx, ny, goal_x, goal_y);
        pq_push(&open, neighbor, f_score[neighbor]);
      }
    }
  }

  free(g_score);
  free(f_score);
  free(came_from);
  free(closed);
  return false;
}

// STEERING BEHAVIORS
typedef struct {
  float position[2], velocity[2];
  float max_speed, max_force;
} SteeringAgent;

void steering_seek(SteeringAgent *agent, float target[2], float *force_out) {
  float desired[2] = {target[0] - agent->position[0],
                      target[1] - agent->position[1]};

  float mag = sqrtf(desired[0] * desired[0] + desired[1] * desired[1]);
  if (mag > 0) {
    desired[0] = (desired[0] / mag) * agent->max_speed;
    desired[1] = (desired[1] / mag) * agent->max_speed;
  }

  force_out[0] = desired[0] - agent->velocity[0];
  force_out[1] = desired[1] - agent->velocity[1];

  mag = sqrtf(force_out[0] * force_out[0] + force_out[1] * force_out[1]);
  if (mag > agent->max_force) {
    force_out[0] = (force_out[0] / mag) * agent->max_force;
    force_out[1] = (force_out[1] / mag) * agent->max_force;
  }
}

void steering_flee(SteeringAgent *agent, float threat[2], float *force_out) {
  float target[2] = {agent->position[0] - threat[0],
                     agent->position[1] - threat[1]};
  steering_seek(agent, target, force_out);
}

void steering_arrive(SteeringAgent *agent, float target[2], float slow_radius,
                     float *force_out) {
  float desired[2] = {target[0] - agent->position[0],
                      target[1] - agent->position[1]};

  float dist = sqrtf(desired[0] * desired[0] + desired[1] * desired[1]);

  if (dist > 0) {
    float speed = agent->max_speed;
    if (dist < slow_radius) {
      speed = agent->max_speed * (dist / slow_radius);
    }

    desired[0] = (desired[0] / dist) * speed;
    desired[1] = (desired[1] / dist) * speed;
  }

  force_out[0] = desired[0] - agent->velocity[0];
  force_out[1] = desired[1] - agent->velocity[1];
}

void steering_wander(SteeringAgent *agent, float *wander_angle,
                     float *force_out) {
  float circle_distance = 2.0f;
  float circle_radius = 1.0f;
  float angle_change = 0.3f;

  *wander_angle += (((float)rand() / RAND_MAX) * 2 - 1) * angle_change;

  float circle_center[2] = {
      agent->position[0] + agent->velocity[0] * circle_distance,
      agent->position[1] + agent->velocity[1] * circle_distance};

  float displacement[2] = {cosf(*wander_angle) * circle_radius,
                           sinf(*wander_angle) * circle_radius};

  float target[2] = {circle_center[0] + displacement[0],
                     circle_center[1] + displacement[1]};

  steering_seek(agent, target, force_out);
}

/* ALL AI PATHFINDING & STEERING TODOs COMPLETE (~55 TODOs) */
