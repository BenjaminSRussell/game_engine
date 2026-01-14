
// Core Memory System Facade
// This file acts as the translation unit for the memory module,
// including the private implementation files to allow them to share internal
// state/definitions declared in memory_private.h

// Ensure Includes
#include "Public/unified_memory.h"
#include "Private/memory_private.h"

// Include Private Implementation Files
// (We are doing a unity build style here for the memory module for simplicity
// of internal state sharing without exposing g_unified_memory globally beyond
// this translation unit if we chose to make it static, but we made it extern in
// memory_private.h so we can compile them separately if we add them to CMake.
// For now, let's keep them separate as 'Source/Runtime/Core/Memory/Private/*.c'
// files to be compiled individually.)

// Wait, if I compile them individually, they need to include memory_private.h
// and link against g_unified_memory. memory_tracking.c already defines
// g_unified_memory? No, let's check. memory_tracking.c has
// `unified_memory_state_t g_unified_memory = {0};` memory_core.c creates it?
// No. I should define it in ONE place. memory_tracking.c defined it.
// memory_core.c needs it.
// memory_profiler.c needs it.
// So memory_tracking.c is the "primary" state holder? Or create a
// memory_state.c? Or just put it in memory_core.c.

// Let's Correct: memory_tracking.c had `unified_memory_state_t g_unified_memory
// = {0};` in my previous step. memory_core.c uses it. I should allow that.

// This file `Source/Runtime/Core/Memory/unified_memory.c` will just be a thin
// wrapper or empty if we are adding the Private files to the build system. But
// legacy build might expect `unified_memory.c` to contain EVERYTHING. To
// support legacy / simplicty, I will make this file INCLUDE the private .c
// files. This preserves the "single object file" behavior if the build system
// just looks for unified_memory.c.

#include "Private/memory_core.c"
#include "Private/memory_profiler.c"
#include "Private/memory_tracking.c"

// Stubs for missing headers if any
// unified_logging.h was used in original. I replaced macros with printf in my
// extraction. So no extra includes needed besides standard ones.
