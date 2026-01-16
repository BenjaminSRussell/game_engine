# Metal Resource Management - Testing Guide

## Overview

This directory contains comprehensive tests and benchmarks for the Metal GPU resource management system.

## Test Suite

### Unit Tests

**File:** `mtl_resource_tests.c`

**Coverage:**
-  Buffer creation and lifecycle
-  Ring buffer allocation and statistics
-  Buffer sub-allocator (pooling)
-  Texture creation (2D, MSAA)
-  Sampler creation and caching
-  Resource pool acquire/return/cleanup
-  Transfer manager initialization

### Performance Benchmarks

**Included:**
1. **Buffer Allocation** - Direct vs Sub-allocator speedup
2. **Sampler Caching** - Cache hit rate measurement

## Building Tests

### macOS

```bash
cd src/engine/rendering/3d_rendering/backend/metal/tests

# Compile test suite
clang -framework Metal -framework Foundation \
      -I.. \
      -o mtl_tests \
      mtl_resource_tests.c \
      ../mtl_device.c \
      ../mtl_buffer.c \
      ../mtl_texture.c \
      ../mtl_transfer.c \
      ../mtl_resource_pool.c \
      ../mtl_sampler.c

# Run tests
./mtl_tests
```

## Expected Output

```

      Metal Resource Management - Test Suite             


Running Unit Tests...

Running: buffer_creation
   PASSED

Running: ring_buffer_allocation
   PASSED

...

Running Performance Benchmarks...

=== Buffer Allocation Benchmark ===
Direct allocation: 45.23 ms (1000 iterations)
Sub-allocator:     8.12 ms (1000 iterations)
Speedup: 5.57x

=== Sampler Caching Benchmark ===
Created 1000 samplers in 12.34 ms
Cache hits: 999 (99.9%)
Cache misses: 1


                    Test Summary                          

Total:  10 tests
Passed: 10 tests 
Failed: 0 tests 

 ALL TESTS PASSED!
```

## Profiling with Instruments

### Memory Leak Detection

```bash
# Build with debug symbols
clang -g -framework Metal -framework Foundation \
      -I.. -o mtl_tests mtl_resource_tests.c ...

# Run with leaks
leaks --atExit -- ./mtl_tests

# Or use Instruments GUI
instruments -t Leaks ./mtl_tests
```

**Expected:** Zero leaks (ARC manages Metal objects)

### Performance Analysis

```bash
# Allocations instrument
instruments -t Allocations ./mtl_tests

# Time Profiler
instruments -t "Time Profiler" ./mtl_tests
```

## Multi-threaded Stress Test

**Create:** `mtl_thread_stress_test.c`

```c
#include <pthread.h>

#define THREAD_COUNT 8
#define ITERATIONS 1000

void* worker_thread(void* arg) {
    metal_device_t* device = (metal_device_t*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Test concurrent allocation
        metal_sampler_t* s = metal_sampler_create_linear_clamp(device);
        metal_sampler_destroy(s);
        
        // Test concurrent sub-allocation
        // ... allocator operations ...
    }
    
    return NULL;
}

int main() {
    metal_device_t* device = metal_device_create(0);
    pthread_t threads[THREAD_COUNT];
    
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, worker_thread, device);
    }
    
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf(" Thread stress test passed\n");
    return 0;
}
```

## Integration Testing

### With Actual Rendering

```c
// In your renderer setup:
metal_device_t* device = metal_device_create(0);
metal_transfer_manager_t* transfer = metal_transfer_manager_create(device);
metal_resource_pool_t* pool = metal_resource_pool_create(device);

// Create samplers
metal_sampler_t* linear_sampler = metal_sampler_create_linear_repeat(device);

// Per-frame rendering:
void render_frame() {
    // Acquire resources
    metal_buffer_t* vertex_buf = metal_pool_acquire_buffer(pool, &vertex_desc);
    
    // Upload data
    metal_transfer_upload_buffer_staged(transfer, vertex_buf, data, size, 0, NULL, NULL);
    metal_transfer_flush(transfer);
    
    // Render...
    
    // Return to pool
    metal_pool_return_buffer(pool, vertex_buf);
    
    // Periodic cleanup (every 2 seconds @ 60fps)
    if (frame % 120 == 0) {
        metal_pool_cleanup(pool, 120);
    }
}
```

## Validation

### Metal Validation Layer

Enable in Xcode:
1. Product  Scheme  Edit Scheme
2. Run  Diagnostics  Metal
3. Check "API Validation"
4. Check "Shader Validation"

OR via environment:

```bash
export METAL_DEVICE_WRAPPER_TYPE=1
export METAL_DEBUG_ERROR_MODE=0
./mtl_tests
```

## Test Checklist

- [x] Unit tests compile
- [ ] All unit tests pass
- [ ] No memory leaks (Instruments)
- [ ] Performance benchmarks run
- [ ] Sampler cache hit rate > 95%
- [ ] Sub-allocator shows speedup
- [ ] Thread stress test passes
- [ ] Integration test renders correctly
- [ ] Metal validation passes
- [ ] No crashes under load

## Continuous Integration

### GitHub Actions Example

```yaml
name: Metal Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Tests
        run: |
          cd src/engine/rendering/3d_rendering/backend/metal/tests
          make
      - name: Run Tests
        run: ./mtl_tests
      - name: Check Leaks
        run: leaks --atExit -- ./mtl_tests
```

## Troubleshooting

### Common Issues

**"Device creation failed"**
- Ensure Metal is supported (macOS 10.11+, iOS 8+)
- Check GPU availability

**Link errors**
- Make sure to link `-framework Metal -framework Foundation`
- Include all .c files in compilation

**Thread safety failures**
- Verify pthread mutexes are initialized
- Check for data races with Thread Sanitizer: `-fsanitize=thread`

## Next Steps

1. Run test suite: `make && ./mtl_tests`
2. Profile: `instruments -t Leaks ./mtl_tests`
3. Stress test: Create multi-threaded test
4. Integrate: Use in actual renderer
5. Validate: Enable Metal validation layer

## Contact

For issues or improvements, see main documentation.
