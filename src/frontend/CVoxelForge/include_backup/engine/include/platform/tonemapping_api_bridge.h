// Tonemapping API Bridge
// Exposes HDR tonemapping controls to VoxelForgeStudio

#ifndef TONEMAPPING_API_BRIDGE_H
#define TONEMAPPING_API_BRIDGE_H

#include "../common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Tonemapping API
// ============================================================================

typedef enum {
  TONEMAP_NONE = 0,
  TONEMAP_REINHARD = 1,
  TONEMAP_FILMIC = 2,
  TONEMAP_ACES = 3,
  TONEMAP_UNCHARTED2 = 4
} TonemapOperator;

/// Set tonemapping operator
void tonemap_set_operator(TonemapOperator op);

/// Get tonemapping operator
TonemapOperator tonemap_get_operator(void);

/// Set exposure
void tonemap_set_exposure(float exposure);

/// Get exposure
float tonemap_get_exposure(void);

/// Set white point (for Reinhard)
void tonemap_set_white_point(float white_point);

/// Get white point
float tonemap_get_white_point(void);

#ifdef __cplusplus
}
#endif

#endif // TONEMAPPING_API_BRIDGE_H
