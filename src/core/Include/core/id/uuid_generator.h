#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

/*
 * =================================================================================================
 *                          UUID GENERATOR INTERFACE
 * =================================================================================================
 */

typedef struct {
    uint8_t bytes[16];
} UUID;

// Generate a random UUID (v4)
void uuid_generate_v4(UUID* out_uuid);

// Convert UUID to string (buffer must be at least 37 bytes)
void uuid_to_string(const UUID* uuid, char* out_str);

// Parse UUID from string
bool uuid_from_string(const char* str, UUID* out_uuid);

// Compare two UUIDs
bool uuid_compare(const UUID* a, const UUID* b);

#endif // UUID_GENERATOR_H
