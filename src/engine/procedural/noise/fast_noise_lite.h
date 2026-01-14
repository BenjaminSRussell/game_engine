#ifndef FAST_NOISE_LITE_H
#define FAST_NOISE_LITE_H

// Stub implementation of FastNoiseLite for C
// Based on https://github.com/Auburn/FastNoiseLite

typedef struct {
  int noiseType;
  int rotationType3D;
  int fractalType;
  int cellularDistanceFunction;
  int cellularReturnType;
  float frequency;
  float lacunarity;
  float gain;
  float cellularJitter;
  int domainWarpType;
  float domainWarpAmp;
  int octaves;
  float weightedStrength;
  float pingPongStrength;
  float fractalBounding;
  long seed;
} fnl_state;

// Enums (Mapped to integers for C stub)
#define FNL_NOISE_OPENSIMPLEX2 0
#define FNL_NOISE_OPENSIMPLEX2S 1
#define FNL_NOISE_CELLULAR 2
#define FNL_NOISE_PERLIN 3
#define FNL_NOISE_VALUE_CUBIC 4
#define FNL_NOISE_VALUE 5

#define FNL_FRACTAL_NONE 0
#define FNL_FRACTAL_FBm 1
#define FNL_FRACTAL_RIDGED 2
#define FNL_FRACTAL_PINGPONG 3

static fnl_state fnlCreateState() {
  fnl_state state;
  state.noiseType = FNL_NOISE_OPENSIMPLEX2;
  state.rotationType3D = 1;
  state.fractalType = FNL_FRACTAL_NONE;
  state.frequency = 0.01f;
  state.octaves = 3;
  state.lacunarity = 2.0f;
  state.gain = 0.5f;
  state.weightedStrength = 0.0f;
  state.pingPongStrength = 2.0f;
  state.cellularDistanceFunction = 1;
  state.cellularReturnType = 1;
  state.cellularJitter = 1.0f;
  state.domainWarpType = 0;
  state.domainWarpAmp = 1.0f;
  return state;
}

static float fnlGetNoise2D(fnl_state *state, float x, float y) {
  return 0.0f; // TODO: Implement simple noise or link real library
}

static float fnlGetNoise3D(fnl_state *state, float x, float y, float z) {
  return 0.0f;
}

static void fnlDomainWarp2D(fnl_state *state, float *x, float *y) {
  // No-op stub
}

static void fnlDomainWarp3D(fnl_state *state, float *x, float *y, float *z) {
  // No-op stub
}

#endif // FAST_NOISE_LITE_H
