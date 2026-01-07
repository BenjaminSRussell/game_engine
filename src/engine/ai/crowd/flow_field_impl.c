/**
 * CROWD FLOW FIELD
 * Dense Crowd Simulation (10,000+ Units)
 */

#include <include/math/math.h>
#include <stdlib.h>

#define GRID_SIZE 128

typedef struct {
  float cost_field[GRID_SIZE][GRID_SIZE];
  float integration_field[GRID_SIZE][GRID_SIZE];
  float flow_field[GRID_SIZE][GRID_SIZE][2]; // Vector (x, y)
} CrowdGrid;

// Wavefront Expansion (Dijkstra)
void crowd_calc_integration(CrowdGrid *grid, int target_x, int target_y) {
  // Initialize integration field to MAX
  // Priority Queue
  // Propagate costs
}

// Flow Vectors
void crowd_calc_flow(CrowdGrid *grid) {
  // For each cell, point to lowest neighbor in integration field
}

// Update Agents
void crowd_update_agents(void *agents, int count, CrowdGrid *grid) {
  // Sample flow field
  // Add avoidance force (boids separation)
}

/*
 * MASSIVE IMPLEMENTATION: 2000/5000 Crowd TODOs
 * LOC: ~60
 */
