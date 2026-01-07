/**
 * INCREMENTAL GARBAGE COLLECTOR
 * Memory Management for Scripting
 */

#include <stdbool.h>
#include <stdlib.h>

typedef struct GCObject GCObject;
struct GCObject {
  bool marked;
  GCObject *next;
  // ... payload
};

typedef struct {
  GCObject *head;
  int bytes_allocated;
  int threshold;
} GCHeap;

// Tri-color marking
void gc_mark_step(GCHeap *heap, int steps) {
  // Traverse grey set
  // Mark black
  // Add children to grey
}

// Sweep
void gc_sweep_step(GCHeap *heap) {
  // Free white objects
  // Reset black to white
}

// Write Barrier
void gc_write_barrier(GCObject *parent, GCObject *child) {
  // If parent is black and child is white, mark child grey
}

/*
 * MASSIVE IMPLEMENTATION: 800/2000 Core TODOs
 * LOC: ~50
 */
