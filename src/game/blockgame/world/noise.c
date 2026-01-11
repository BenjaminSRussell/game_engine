// Noise functions for terrain generation.
// Roadmap: docs/WORLD_ROADMAP.md.
#include <math.h>
#include <stdlib.h>
#include <world/generator.h>

// Simple Perlin-like noise (simplified implementation)
static f32 fade(f32 t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

static f32 lerp(f32 a, f32 b, f32 t) { return a + t * (b - a); }

static f32 grad(i32 hash, f32 x, f32 y, f32 z) {
  i32 h = hash & 15;
  f32 u = h < 8 ? x : y;
  f32 v = h < 4 ? y : h == 12 || h == 14 ? x : z;
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Simple hash function
static i32 hash(i32 x, i32 y, i32 z, u32 seed) {
  i32 h = (x * 73856093) ^ (y * 19349663) ^ (z * 83492791) ^ (i32)seed;
  return h & 0x7FFFFFFF;
}

// Perlin noise
f32 noise_perlin(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  // Scale by frequency
  x *= gen->frequency;
  y *= gen->frequency;
  z *= gen->frequency;

  // Grid coordinates
  i32 X = (i32)floorf(x) & 255;
  i32 Y = (i32)floorf(y) & 255;
  i32 Z = (i32)floorf(z) & 255;

  // Fractional parts
  x -= floorf(x);
  y -= floorf(y);
  z -= floorf(z);

  // Fade curves
  f32 u = fade(x);
  f32 v = fade(y);
  f32 w = fade(z);

  // Hash coordinates
  i32 A = hash(X, Y, Z, gen->seed) & 255;
  i32 AA = hash(A, A, A, gen->seed) & 255;
  i32 AB = hash(A, A + 1, A, gen->seed) & 255;
  i32 B = hash(X + 1, Y, Z, gen->seed) & 255;
  i32 BA = hash(B, B, B, gen->seed) & 255;
  i32 BB = hash(B, B + 1, B, gen->seed) & 255;

  // Interpolate
  f32 result = lerp(
      lerp(lerp(grad(AA, x, y, z), grad(BA, x - 1, y, z), u),
           lerp(grad(AB, x, y - 1, z), grad(BB, x - 1, y - 1, z), u), v),
      lerp(lerp(grad(AA + 1, x, y, z - 1), grad(BA + 1, x - 1, y, z - 1), u),
           lerp(grad(AB + 1, x, y - 1, z - 1),
                grad(BB + 1, x - 1, y - 1, z - 1), u),
           v),
      w);

  return result * gen->amplitude;
}

// Simplex noise implementation for improved performance
static const i32 SIMPLEX_GRADIENTS[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

static const f32 SIMPLEX_F3 = 1.0f / 3.0f;
static const f32 SIMPLEX_G3 = 1.0f / 6.0f;

static f32 simplex_dot(const i32 *grad, f32 x, f32 y, f32 z) {
  return grad[0] * x + grad[1] * y + grad[2] * z;
}

f32 noise_simplex(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  // Scale by frequency
  x *= gen->frequency;
  y *= gen->frequency;
  z *= gen->frequency;

  // Skew the input space to determine which simplex cell we're in
  f32 s = (x + y + z) * SIMPLEX_F3;
  i32 i = (i32)floorf(x + s);
  i32 j = (i32)floorf(y + s);
  i32 k = (i32)floorf(z + s);

  // Unskew to get the actual cell coordinates
  f32 t = (i + j + k) * SIMPLEX_G3;
  f32 X0 = i - t;
  f32 Y0 = j - t;
  f32 Z0 = k - t;

  // The x,y,z distances from the cell origin
  f32 x0 = x - X0;
  f32 y0 = y - Y0;
  f32 z0 = z - Z0;

  // Determine which simplex we are in
  i32 i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
  i32 i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

  if (x0 >= y0) {
    if (y0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    } else if (x0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    }
  } else {
    if (y0 < z0) {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else if (x0 < z0) {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    }
  }

  // Offsets for second corner in (x,y,z) coords
  f32 x1 = x0 - i1 + SIMPLEX_G3;
  f32 y1 = y0 - j1 + SIMPLEX_G3;
  f32 z1 = z0 - k1 + SIMPLEX_G3;

  // Offsets for third corner in (x,y,z) coords
  f32 x2 = x0 - i2 + 2.0f * SIMPLEX_G3;
  f32 y2 = y0 - j2 + 2.0f * SIMPLEX_G3;
  f32 z2 = z0 - k2 + 2.0f * SIMPLEX_G3;

  // Offsets for last corner in (x,y,z) coords
  f32 x3 = x0 - 1.0f + 3.0f * SIMPLEX_G3;
  f32 y3 = y0 - 1.0f + 3.0f * SIMPLEX_G3;
  f32 z3 = z0 - 1.0f + 3.0f * SIMPLEX_G3;

  // Work out the hashed gradient indices of the four simplex corners
  i32 ii = i & 255;
  i32 jj = j & 255;
  i32 kk = k & 255;

  // Calculate the contribution from the four corners
  f32 n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;

  // Compute the kernel for each corner
  f32 t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
  if (t0 > 0.0f) {
    t0 *= t0;
    i32 grad_index = hash(ii, jj, kk, gen->seed) % 12;
    n0 = t0 * t0 * simplex_dot(SIMPLEX_GRADIENTS[grad_index], x0, y0, z0);
  }

  f32 t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
  if (t1 > 0.0f) {
    t1 *= t1;
    i32 grad_index = hash(ii + i1, jj + j1, kk + k1, gen->seed) % 12;
    n1 = t1 * t1 * simplex_dot(SIMPLEX_GRADIENTS[grad_index], x1, y1, z1);
  }

  f32 t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
  if (t2 > 0.0f) {
    t2 *= t2;
    i32 grad_index = hash(ii + i2, jj + j2, kk + k2, gen->seed) % 12;
    n2 = t2 * t2 * simplex_dot(SIMPLEX_GRADIENTS[grad_index], x2, y2, z2);
  }

  f32 t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
  if (t3 > 0.0f) {
    t3 *= t3;
    i32 grad_index = hash(ii + 1, jj + 1, kk + 1, gen->seed) % 12;
    n3 = t3 * t3 * simplex_dot(SIMPLEX_GRADIENTS[grad_index], x3, y3, z3);
  }

  // Add contributions from each corner to get the final noise value
  f32 result = 32.0f * (n0 + n1 + n2 + n3);
  return result * gen->amplitude;
}

// Fractional Brownian Motion (FBM)
f32 noise_fbm(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  f32 value = 0.0f;
  f32 amplitude = gen->amplitude;
  f32 frequency = gen->frequency;

  for (u32 i = 0; i < gen->octaves; i++) {
    value += noise_perlin(gen, x * frequency, y * frequency, z * frequency) *
             amplitude;
    frequency *= gen->lacunarity;
    amplitude *= gen->persistence;
  }

  return value;
}

// Ridged multi-fractal noise (for mountainous terrain)
// Based on Musgrave's ridged multifractal algorithm
f32 noise_ridged(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  f32 result = 0.0f;
  f32 frequency = gen->frequency;
  f32 weight = 1.0f;
  f32 signal;

  // Get first octave
  signal = noise_perlin(gen, x * frequency, y * frequency, z * frequency);
  // Make ridges by taking absolute value and inverting
  signal = fabsf(signal);
  signal = 1.0f - signal;
  // Square for sharper ridges
  signal *= signal;
  result = signal;

  // Subsequent octaves with weight modulation
  for (u32 i = 1; i < gen->octaves; i++) {
    frequency *= gen->lacunarity;

    // Weight successive contributions by previous signal
    weight = signal * 2.0f;
    if (weight > 1.0f)
      weight = 1.0f;
    if (weight < 0.0f)
      weight = 0.0f;

    signal = noise_perlin(gen, x * frequency, y * frequency, z * frequency);
    signal = fabsf(signal);
    signal = 1.0f - signal;
    signal *= signal;
    signal *= weight;

    f32 amplitude = powf(gen->persistence, (f32)i);
    result += signal * amplitude;
  }

  return result * gen->amplitude;
}

// Worley noise (cellular noise) for cave systems and cellular textures
f32 noise_worley(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  // Scale by frequency
  x *= gen->frequency;
  y *= gen->frequency;
  z *= gen->frequency;

  // Get cell coordinates
  i32 cell_x = (i32)floorf(x);
  i32 cell_y = (i32)floorf(y);
  i32 cell_z = (i32)floorf(z);

  // Get fractional part within cell
  f32 fx = x - (f32)cell_x;
  f32 fy = y - (f32)cell_y;
  f32 fz = z - (f32)cell_z;

  f32 min_distance = 1.0f;

  // Check neighboring cells (3x3x3 neighborhood)
  for (i32 i = -1; i <= 1; i++) {
    for (i32 j = -1; j <= 1; j++) {
      for (i32 k = -1; k <= 1; k++) {
        i32 neighbor_x = cell_x + i;
        i32 neighbor_y = cell_y + j;
        i32 neighbor_z = cell_z + k;

        // Generate pseudo-random point within this cell
        u32 hash_val = hash(neighbor_x, neighbor_y, neighbor_z, gen->seed);

        // Use hash to generate point coordinates within cell
        f32 point_x = (f32)(hash_val & 0xFF) / 255.0f + (f32)i;
        f32 point_y = (f32)((hash_val >> 8) & 0xFF) / 255.0f + (f32)j;
        f32 point_z = (f32)((hash_val >> 16) & 0xFF) / 255.0f + (f32)k;

        // Calculate distance to this point
        f32 dx = fx - point_x;
        f32 dy = fy - point_y;
        f32 dz = fz - point_z;
        f32 distance = sqrtf(dx * dx + dy * dy + dz * dz);

        if (distance < min_distance) {
          min_distance = distance;
        }
      }
    }
  }

  return min_distance * gen->amplitude;
}

// Billow noise (cloud-like formations)
f32 noise_billow(NoiseGenerator *gen, f32 x, f32 y, f32 z) {
  if (!gen)
    return 0.0f;

  f32 result = 0.0f;
  f32 amplitude = gen->amplitude;
  f32 frequency = gen->frequency;

  for (u32 i = 0; i < gen->octaves; i++) {
    f32 signal = noise_perlin(gen, x * frequency, y * frequency, z * frequency);
    // Take absolute value to create billowing effect
    signal = fabsf(signal);
    result += signal * amplitude;

    frequency *= gen->lacunarity;
    amplitude *= gen->persistence;
  }

  return result;
}
