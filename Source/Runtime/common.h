/**
 * Source/Runtime/common.h
 *
 * Compatibility header for new modular runtime architecture.
 * This header bridges the new Source/Runtime modules to the existing
 * engine codebase, ensuring clean separation while maximizing code reuse.
 */

#ifndef SOURCE_RUNTIME_COMMON_H
#define SOURCE_RUNTIME_COMMON_H

// Include the existing engine's common header
// This provides all type definitions (f32, i32, u32, Vec3, etc.)
// and common utilities used throughout the engine
#include "../../src/engine/include/common.h"

// Common.h provides:
// - Type definitions (f32, i32, u32, u64, etc.)
// - Math types (Vec3, Vec4, Mat4, Quat, etc.)
// - Logger macros (LOG_INFO, LOG_ERROR, LOG_WARNING, etc.)
// - Standard library includes
// - Platform detection macros
// - Memory and utility functions

#endif // SOURCE_RUNTIME_COMMON_H
