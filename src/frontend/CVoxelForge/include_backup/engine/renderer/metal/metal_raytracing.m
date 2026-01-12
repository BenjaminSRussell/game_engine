/**
 * =================================================================================================
 *                          METAL RAY TRACING MANAGEMENT
 * =================================================================================================
 *
 * Handles creation of Acceleration Structures and Ray Tracing Pipelines.
 * Requires Metal 2.3+ and Apple Silicon (M1+) or AMD RDNA.
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>

// TODO(AGENT_MACOS_1): Implement primitive acceleration structure (BLAS)
// builder
//   - Create MTLAccelerationStructureTriangleGeometryDescriptor
//   - Batch geometry updates
//   - Build structures on GPU
//   - Difficulty: 8

// TODO(AGENT_MACOS_1): Implement instance acceleration structure (TLAS) builder
//   - Create MTLAccelerationStructureInstanceDescriptor buffer
//   - Update transforms per frame
//   - Rebuild/Refit TLAS
//   - Difficulty: 7

// TODO(AGENT_MACOS_1): Create acceleration structure scratch buffer manager
//   - Allocate scratch memory for build operations
//   - Reuse buffers to minimize allocations
//   - Difficulty: 5

// TODO(AGENT_MACOS_1): Implement ray tracing intersector
//   - Bind acceleration structure to shader (Argument Buffer)
//   - Configure intersection functions
//   - Difficulty: 6

// TODO(AGENT_MACOS_1): Create ray tracing pipeline state
//   - Link ray generation, intersection, and shading functions
//   - Difficulty: 6

// TODO(AGENT_MACOS_1): Implement motion blur support
//   - Use motion Keyframes in Acceleration Structures (Metal 3)
//   - Difficulty: 9
