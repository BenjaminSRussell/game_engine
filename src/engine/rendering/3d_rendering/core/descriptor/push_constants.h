/*
 * push_constants.h
 * Push constant management
 */

#ifndef CORE_PUSH_CONSTANTS_H
#define CORE_PUSH_CONSTANTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PUSH_CONSTANT_SIZE 128

typedef struct {
    uint32_t stage_flags; // Shader stages that can access this range
    uint32_t offset;
    uint32_t size;
} push_constant_range_t;

typedef struct {
    push_constant_range_t ranges[4]; // Usually only need a few ranges
    uint32_t range_count;
} push_constant_layout_t;

// Validate a push constant update against a layout
bool push_constants_validate(const push_constant_layout_t* layout, uint32_t offset, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PUSH_CONSTANTS_H */
