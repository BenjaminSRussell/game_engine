// src/engine/core/memory_legacy_compat.c
//
// Purpose: Legacy compatibility for unified memory system
//

#include "core/memory.h"

// Global allocator instances for compatibility
void *g_persistent_allocator = NULL;
void *g_temp_allocator = NULL;
