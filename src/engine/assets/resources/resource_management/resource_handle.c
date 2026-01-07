#include "assets/resources/resource_management/resource_handle.h"
#include <assert.h>

#define INDEX_BITS 20
#define GENERATION_BITS 12
#define INDEX_MASK ((1 << INDEX_BITS) - 1)
#define GENERATION_MASK ((1 << GENERATION_BITS) - 1)
#define MAX_INDICES (1 << INDEX_BITS)
#define MAX_GENERATIONS (1 << GENERATION_BITS)

void resource_handle_init_system(void) {
    // No global state needed for pure handle manipulation logic currently
}

void resource_handle_shutdown_system(void) {
    // No global state needed for pure handle manipulation logic currently
}

uint32_t resource_handle_index(resource_handle_t handle) {
    return handle.handle & INDEX_MASK;
}

uint32_t resource_handle_generation(resource_handle_t handle) {
    return (handle.handle >> INDEX_BITS) & GENERATION_MASK;
}

resource_handle_t resource_handle_create(uint32_t index, uint32_t generation) {
    assert(index < MAX_INDICES && "Resource index out of bounds");
    // Ensure generation wraps within 12 bits, but non-zero preferable if we want to treat 0 as potentially special
    // For now, simple masking
    uint32_t gen_masked = generation & GENERATION_MASK;
    resource_handle_t h;
    h.handle = (gen_masked << INDEX_BITS) | (index & INDEX_MASK);
    return h;
}

bool resource_handle_is_valid(resource_handle_t handle) {
    return handle.handle != INVALID_HANDLE_VALUE;
}

bool resource_handle_is_null(resource_handle_t handle) {
    return handle.handle == INVALID_HANDLE_VALUE;
}

bool buffer_handle_equals(buffer_handle_t a, buffer_handle_t b) {
    return a.id.handle == b.id.handle;
}

bool texture_handle_equals(texture_handle_t a, texture_handle_t b) {
    return a.id.handle == b.id.handle;
}
