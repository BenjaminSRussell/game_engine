/**
 * @file core.h
 * @brief Main Core subsystem header - includes all core functionality
 */

#ifndef VOXELFORGE_CORE_H
#define VOXELFORGE_CORE_H

// Core Types
#include "Core/Public/core_types.h"

// Memory
#include "Core/Public/Memory/memory_api.h"
#include "Core/Public/Memory/memory_types.h"

// Math - Vectors
#include "Core/Public/Math/Vector/vec2.h"
#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/Math/Vector/vec4.h"

// Math - Matrix & Quaternion
#include "Core/Public/Math/Matrix/mat4.h"
#include "Core/Public/Math/Quaternion/quat.h"

// Math - Geometry
#include "Core/Public/Math/Geometry/aabb.h"
#include "Core/Public/Math/Geometry/frustum.h"

// Threading
#include "Core/Public/Threading/thread_api.h"
#include "Core/Public/Threading/thread_types.h"

// Logging
#include "Core/Public/Logging/logger.h"

// Containers
#include "Core/Public/Containers/Array/dynamic_array.h"
#include "Core/Public/Containers/HashMap/hashmap.h"

// ============================================================================
// Core Initialization
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize all core subsystems.
 * Must be called before using any core functionality.
 */
VF_API VF_Result vf_core_init(void);

/**
 * Shutdown all core subsystems.
 * Should be called before application exit.
 */
VF_API void vf_core_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_CORE_H
