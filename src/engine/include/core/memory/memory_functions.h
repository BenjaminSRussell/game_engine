#pragma once

// Basic memory function declarations for compatibility
void *memory_alloc(u32 size, const char *file, u32 line);
void *memory_calloc(u32 count, u32 size, const char *file, u32 line);
void *memory_realloc(void *ptr, u32 new_size, const char *file, u32 line);
void memory_free(void *ptr);
