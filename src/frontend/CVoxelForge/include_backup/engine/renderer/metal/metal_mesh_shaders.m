/**
 * =================================================================================================
 *                          METAL MESH SHADER MANAGEMENT
 * =================================================================================================
 *
 * Handles Metal 3 Mesh Shaders (Object and Mesh stages).
 * Replaces traditional Vertex/Geometry-esque pipelines with compute-like
 * geometry processing.
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>

// TODO(AGENT_MACOS_1): Implement Object Shader stage setup
//   - Configure object function for cluster culling
//   - Output payload to mesh shader
//   - Difficulty: 8

// TODO(AGENT_MACOS_1): Implement Mesh Shader stage setup
//   - Configure mesh function for vertex/primitive generation
//   - Produce Metal Mesh primitives
//   - Difficulty: 8

// TODO(AGENT_MACOS_1): Create meshlet generation tool integration
//   - Import meshlets from asset compiler
//   - Layout data for efficient mesh shader access
//   - Difficulty: 7

// TODO(AGENT_MACOS_1): Implement payload structure definition
//   - Define shared struct between Object and Mesh shaders
//   - Difficulty: 4
