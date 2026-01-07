// Tonemapping API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/tonemapping_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock implementation state
static TonemapOperator g_operator = TONEMAP_ACES;
static float g_exposure = 1.0f;
static float g_white_point = 11.2f;

void tonemap_set_operator(TonemapOperator op) {
  g_operator = op;
  LOG_INFO("Tonemap operator set to: %d", op);
}

TonemapOperator tonemap_get_operator(void) { return g_operator; }

void tonemap_set_exposure(float exposure) { g_exposure = exposure; }

float tonemap_get_exposure(void) { return g_exposure; }

void tonemap_set_white_point(float white_point) { g_white_point = white_point; }

float tonemap_get_white_point(void) { return g_white_point; }
