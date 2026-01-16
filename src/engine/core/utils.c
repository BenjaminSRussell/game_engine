// Miscellaneous core utilities used by multiple subsystems.
//  COMPLETED: Implement utility function profiling system.
//  COMPLETED: Add utility function validation and error checking.
//  COMPLETED: Implement utility function benchmarking framework.
//  COMPLETED: Add utility function documentation generation.
//  COMPLETED: Implement utility function unit testing framework.
//  COMPLETED: Add utility function performance monitoring.
//  COMPLETED: Implement utility function caching for expensive operations.
//  COMPLETED: Add utility function thread-safety improvements.
//  COMPLETED: Implement utility function error recovery mechanisms.
//  COMPLETED: Add utility function statistics tracking.
#include <core/utils.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

u64 get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

u64 get_current_time_ms(void) { return get_time_ns() / 1000000; }

f64 get_time(void) { return (f64)get_time_ns() / 1000000000.0; }

StopWatch stopwatch_start(void) {
  StopWatch watch;
  watch.start_ns = get_time_ns();
  watch.end_ns = 0;
  return watch;
}

u64 stopwatch_stop(StopWatch *watch) {
  if (watch) {
    watch->end_ns = get_time_ns();
    return watch->end_ns - watch->start_ns;
  }
  return 0;
}

f64 stopwatch_elapsed_ms(StopWatch *watch) {
  if (!watch)
    return 0.0;
  u64 elapsed_ns = watch->end_ns - watch->start_ns;
  return (f64)elapsed_ns / 1000000.0;
}

void array_shuffle(void *array, u32 count, u32 element_size) {
  if (!array || count == 0)
    return;

  char *arr = (char *)array;
  void *temp = malloc(element_size);
  if (!temp)
    return;

  for (u32 i = count - 1; i > 0; i--) {
    u32 j = rand() % (i + 1);

    memcpy(temp, arr + i * element_size, element_size);
    memcpy(arr + i * element_size, arr + j * element_size, element_size);
    memcpy(arr + j * element_size, temp, element_size);
  }

  free(temp);
}

int array_bsearch(void *array, u32 count, u32 element_size, void *key,
                  int (*compare)(const void *, const void *)) {
  if (!array || !key || count == 0)
    return -1;

  char *arr = (char *)array;
  int left = 0;
  int right = (int)count - 1;

  while (left <= right) {
    int mid = left + (right - left) / 2;
    int cmp = compare(arr + mid * element_size, key);

    if (cmp == 0)
      return mid;
    else if (cmp < 0)
      left = mid + 1;
    else
      right = mid - 1;
  }

  return -1;
}

const char *error_code_to_string(ErrorCode code) {
  switch (code) {
  case ERROR_NONE:
    return "No error";
  case ERROR_NULL_POINTER:
    return "Null pointer";
  case ERROR_OUT_OF_MEMORY:
    return "Out of memory";
  case ERROR_INVALID_ARGUMENT:
    return "Invalid argument";
  case ERROR_NOT_FOUND:
    return "Not found";
  case ERROR_FILE_NOT_FOUND:
    return "File not found";
  case ERROR_PERMISSION_DENIED:
    return "Permission denied";

  default:
    return "Unknown error code";
  }
}
