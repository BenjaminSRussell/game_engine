/**
 * =================================================================================================
 *                          METAL ARGUMENT BUFFER MANAGEMENT
 * =================================================================================================
 *
 * Handles creation and encoding of Argument Buffers (Bindless Resources).
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>

// TODO(AGENT_MACOS_1): Implement Argument Buffer Encoder cache
//   - Create properties for argument buffer layout
//   - Cache MTLArgumentEncoder for reuse
//   - Difficulty: 5

// TODO(AGENT_MACOS_1): Create tier 2 argument buffer manager
//   - Support pointers to resources in buffers
//   - Allow massive numbers of textures (bindless)
//   - Difficulty: 6

// TODO(AGENT_MACOS_1): Implement resource residency tracking for argument
// buffers
//   - "useResource" calls for all resources referenced in buffer
//   - Use MTLRenderCommandEncoder useHeap for heap-based resources
//   - Difficulty: 7

// TODO(AGENT_MACOS_1): Create nested argument buffer support
//   - Argument buffers pointing to other argument buffers
//   - Difficulty: 6
