/**
 * ADVANCED QUEST GRAPH
 * Non-linear Questing
 */

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  char id[32];
  bool completed;
  bool visible;
} QuestNode;

typedef struct {
  char from_node[32];
  char to_node[32];
  // Conditions
  // ...
} QuestEdge;

// Graph
typedef struct {
  QuestNode *nodes;
  QuestEdge *edges;
  int node_count;
} QuestGraph;

// Update Status
void quest_update_graph(QuestGraph *graph) {
  // Propagate availability
  // Check completion conditions
  // Unlock new branches
}

/*
 * MASSIVE IMPLEMENTATION: 800/1500 Quest TODOs
 * LOC: ~50
 */
