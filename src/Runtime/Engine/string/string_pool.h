#ifndef STRING_POOL_H
#define STRING_POOL_H

#include <stddef.h>

typedef struct StringPool StringPool;

StringPool* string_pool_create(size_t capacity);
void string_pool_destroy(StringPool* pool);
const char* string_intern(StringPool* pool, const char* str);
void string_release(StringPool* pool, const char* str);

#endif // STRING_POOL_H
