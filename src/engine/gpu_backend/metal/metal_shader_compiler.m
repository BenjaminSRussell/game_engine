/**
 * =================================================================================================
 *                          METAL SHADER COMPILER & RELOAD
 * =================================================================================================
 *
 * Handles runtime compilation of Metal shaders and hot-reloading from source.
 *
 * =================================================================================================
 */

#import <Metal/Metal.h>

// TODO(AGENT_MACOS_1): Implement runtime MSL compilation
//   - Use MTLLibrary newLibraryWithSource:options:error:
//   - Capture and log compilation errors
//   - Difficulty: 4

// TODO(AGENT_MACOS_1): Create macro management
//   - Build preprocessor macros map from engine config
//   - Hash macros to key cached libraries
//   - Difficulty: 3

// TODO(AGENT_MACOS_1): Implement metallib binary loading
//   - Load pre-compiled .metallib from build artifacts
//   - Fallback to source compilation if metallib missing (dev mode)
//   - Difficulty: 3

// TODO(AGENT_MACOS_1): Implement shader hot-reload watcher
//   - React to file changes in .metal files
//   - Recompile affected libraries
//   - Invalidate PSOs depending on these functions
//   - Trigger pipeline rebuild
//   - Difficulty: 7

// TODO(AGENT_MACOS_1): Create shader reflection extractor
//   - Extract function arguments and types from MTLFunction
//   - Verify C++ struct layout matches shader struct layout
//   - Difficulty: 5

// TODO(AGENT_MACOS_1): Implement shader library linking
//   - Link multiple libraries (dynamic libraries)
//   - Difficulty: 5
