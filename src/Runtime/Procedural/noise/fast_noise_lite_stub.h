#ifndef FAST_NOISE_LITE_H
#define FAST_NOISE_LITE_H
// Stub for FastNoiseLite to allow compilation
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
} fnl_state;

static fnl_state fnlCreateState() {
  fnl_state state = {0};
  return state;
}

static float fnlGetNoise2D(fnl_state *state, float x, float y) { return 0.0f; }
static float fnlGetNoise3D(fnl_state *state, float x, float y, float z) {
  return 0.0f;
}
#endif
