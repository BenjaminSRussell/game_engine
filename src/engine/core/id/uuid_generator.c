#include "core/id/uuid_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * =================================================================================================
 *                          UUID GENERATOR IMPLEMENTATION
 * =================================================================================================
 */



void uuid_generate_v4(UUID* out_uuid) {
    if (!out_uuid) return;
    
    // Simple random generation (better random source should be used in prod)
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    
    for (int i = 0; i < 16; i++) {
        out_uuid->bytes[i] = rand() % 256;
    }
    
    // Set version (4)
    out_uuid->bytes[6] = (out_uuid->bytes[6] & 0x0F) | 0x40;
    // Set variant (10xxxxxx)
    out_uuid->bytes[8] = (out_uuid->bytes[8] & 0x3F) | 0x80;
}

void uuid_to_string(const UUID* uuid, char* out_str) {
    if (!uuid || !out_str) return;
    
    sprintf(out_str, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid->bytes[0], uuid->bytes[1], uuid->bytes[2], uuid->bytes[3],
        uuid->bytes[4], uuid->bytes[5],
        uuid->bytes[6], uuid->bytes[7],
        uuid->bytes[8], uuid->bytes[9],
        uuid->bytes[10], uuid->bytes[11], uuid->bytes[12], uuid->bytes[13], uuid->bytes[14], uuid->bytes[15]
    );
}

bool uuid_from_string(const char* str, UUID* out_uuid) {
    if (!str || !out_uuid) return false;
    
    if (strlen(str) != 36) return false;
    
    // Parse
    unsigned int bytes[16];
    int count = sscanf(str, 
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        &bytes[0], &bytes[1], &bytes[2], &bytes[3],
        &bytes[4], &bytes[5],
        &bytes[6], &bytes[7],
        &bytes[8], &bytes[9],
        &bytes[10], &bytes[11], &bytes[12], &bytes[13], &bytes[14], &bytes[15]
    );
    
    if (count != 16) return false;
    
    for(int i=0; i<16; i++) out_uuid->bytes[i] = (uint8_t)bytes[i];
    
    return true;
}

bool uuid_compare(const UUID* a, const UUID* b) {
    if (!a || !b) return false;
    return memcmp(a->bytes, b->bytes, 16) == 0;
}
