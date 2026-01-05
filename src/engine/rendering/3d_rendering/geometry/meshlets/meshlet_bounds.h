/*
 * meshlet_bounds.h
 * Computation of bounding volumes (spheres and cones) for meshlets
 */

#ifndef MESHLET_BOUNDS_H
#define MESHLET_BOUNDS_H

#include "meshlet_builder.h"

// Computes the bounding sphere and normal cone for a single meshlet
void compute_meshlet_bounds(meshlet_t* meshlet, const float* vertices, uint32_t vertex_stride, const uint32_t* indices);

// Batch computation
void compute_meshlets_bounds(meshlet_t* meshlets, uint32_t count, const float* vertices, uint32_t vertex_stride, const uint32_t* indices);

#endif // MESHLET_BOUNDS_H
