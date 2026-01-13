#ifndef RESOURCE_HANDLE_H
#define RESOURCE_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Generic handle structure for GPU resources
 * 32 bits total:
 * - 20 bits: Index (max ~1 million resources)
 * - 12 bits: Generation (for stale handle detection)
 */
typedef struct {
  uint32_t handle;
} resource_handle_t;

typedef resource_handle_t ResourceHandle;

/**
 * @brief Typed handles for type safety
 */
typedef struct {
  resource_handle_t id;
} buffer_handle_t;
typedef struct {
  resource_handle_t id;
} texture_handle_t;
typedef struct {
  resource_handle_t id;
} sampler_handle_t;
typedef struct {
  resource_handle_t id;
} shader_handle_t;
typedef struct {
  resource_handle_t id;
} material_handle_t;
typedef struct {
  resource_handle_t id;
} accel_struct_handle_t;

#define INVALID_HANDLE_VALUE 0

// Helper to create an invalid handle
static inline resource_handle_t resource_handle_invalid(void) {
  resource_handle_t h = {INVALID_HANDLE_VALUE};
  return h;
}

static inline buffer_handle_t buffer_handle_invalid(void) {
  buffer_handle_t h = {{INVALID_HANDLE_VALUE}};
  return h;
}

static inline texture_handle_t texture_handle_invalid(void) {
  texture_handle_t h = {{INVALID_HANDLE_VALUE}};
  return h;
}

// Handle packing/unpacking
uint32_t resource_handle_index(resource_handle_t handle);
uint32_t resource_handle_generation(resource_handle_t handle);
resource_handle_t resource_handle_create(uint32_t index, uint32_t generation);

// Initializers
void resource_handle_init_system(void);
void resource_handle_shutdown_system(void);

// Validation
bool resource_handle_is_valid(resource_handle_t handle);
bool resource_handle_is_null(resource_handle_t handle);

// Type safe comparisons
bool buffer_handle_equals(buffer_handle_t a, buffer_handle_t b);
bool texture_handle_equals(texture_handle_t a, texture_handle_t b);

#ifdef __cplusplus
}
#endif

#endif // RESOURCE_HANDLE_H
