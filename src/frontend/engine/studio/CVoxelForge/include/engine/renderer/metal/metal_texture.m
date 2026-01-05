/**
 * =================================================================================================
 *                              METAL TEXTURE MANAGEMENT
 * =================================================================================================
 *
 * Handles Metal texture creation, upload, compression, and binding.
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>

// TODO(AGENT_MACOS_1): Implement texture upload system
//   - Use shared buffer for staging
//   - Blit from staging buffer to private texture
//   - Handle row alignment and slice padding
//   - Difficulty: 5

// TODO(AGENT_MACOS_1): Create texture compression helpers
//   - Support BC1-BC7 decoding (via software if hardware unsupported on older
//   Macs)
//   - Support ASTC (native on Apple Silicon)
//   - Difficulty: 4

// TODO(AGENT_MACOS_1): Implement bindless texture heap
//   - Allocate massive sparse heap
//   - Manage texture indices for shader access (argument buffers)
//   - Handle fragmentation in heap
//   - Difficulty: 7

// TODO(AGENT_MACOS_1): Create render target pool
//   - Reuse transient render targets (GBuffers)
//   - Memoryless render targets for tile-based GPUs (Apple Silicon
//   optimization)
//   - Difficulty: 5

// TODO(AGENT_MACOS_1): Implement texture view creation
//   - Create views with different formats (sRGB vs Linear)
//   - Create views for texture arrays/cubemaps slices
//   - Difficulty: 3

// TODO(AGENT_MACOS_1): Implement texture streaming logic
//   - Support partial mipmap residency (sparse textures)
//   - Stream in higher mips based on visibility
//   - Difficulty: 8

// TODO(AGENT_MACOS_1): Create texture swizzle helpers
//   - Handle channel remapping (R <-> B)
//   - Difficulty: 2
