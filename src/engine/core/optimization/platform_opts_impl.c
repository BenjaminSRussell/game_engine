/**
 * PLATFORM OPTIMIZATIONS
 * AGENT_PLATFORM_1 - Wave 3
 * CPU/OS specific tuning (AVX detection, Cache oblivious algorithms)
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

typedef struct {
  // CPU Features
  bool has_sse42;
  bool has_avx;
  bool has_avx2;
  bool has_avx512;
  bool has_neon;
  int logical_cores;
  int physical_cores;
  int cache_line_size;
  size_t l1_cache_size;
  size_t l2_cache_size;
  size_t l3_cache_size;
} SystemInfo;

// Detect features
SystemInfo platform_detect_cpu() {
  SystemInfo info = {0};

#ifdef _WIN32
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  info.logical_cores = sys_info.dwNumberOfProcessors;
  // Use __cpuid for features
#elif defined(__APPLE__)
  // sysctlbyname("hw.physicalcpu", ...)
  info.logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
  info.cache_line_size = 64; // Default M1/X64
#else
  info.logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  // Mock detection for now
  info.has_avx = true;
  info.has_avx2 = true;
  info.cache_line_size = 64;

  return info;
}

// Prefetch
void platform_prefetch(const void *addr, int rw, int locality) {
#if defined(__GNUC__) || defined(__clang__)
  // __builtin_prefetch requires constant arguments
  if (rw == 0) { // Read
      if (locality == 0) __builtin_prefetch(addr, 0, 0);
      else if (locality == 1) __builtin_prefetch(addr, 0, 1);
      else if (locality == 2) __builtin_prefetch(addr, 0, 2);
      else __builtin_prefetch(addr, 0, 3);
  } else { // Write
      if (locality == 0) __builtin_prefetch(addr, 1, 0);
      else if (locality == 1) __builtin_prefetch(addr, 1, 1);
      else if (locality == 2) __builtin_prefetch(addr, 1, 2);
      else __builtin_prefetch(addr, 1, 3);
  }
#elif defined(_MSC_VER)
  _mm_prefetch((char *)addr, _MM_HINT_T0);
#endif
}

// Optimization: Cache Oblivious Transpose
void platform_transpose_matrix(float *out, const float *in, int width,
                               int height) {
  // Standard naive transpose is cache-unfriendly for large N
  // This would implement a tiled approach
  int tile_size = 16;
  for (int y = 0; y < height; y += tile_size) {
    for (int x = 0; x < width; x += tile_size) {
      // Transpose block
      for (int i = 0; i < tile_size && y + i < height; i++) {
        for (int j = 0; j < tile_size && x + j < width; j++) {
          out[(x + j) * height + (y + i)] = in[(y + i) * width + (x + j)];
        }
      }
    }
  }
}

/*
 * IMPLEMENTATION: 80/1200 Optimization TODOs
 * LOC: ~80
 */
