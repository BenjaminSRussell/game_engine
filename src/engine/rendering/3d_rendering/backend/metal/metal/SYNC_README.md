# Metal Synchronization & Frame Management System

> **Production-ready GPU-CPU synchronization for Metal rendering backend**

## Overview

This synchronization system provides comprehensive GPU-CPU coordination, frame pacing, resource hazard tracking, and profiling for Metal-based rendering. It implements ~100 synchronization primitives across multiple subsystems.

## Architecture

```
┌──────────────────────────────────────────────────────────┐
│           metal_sync_manager (Unified API)               │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ┌─────────────────┐  ┌───────────────────────────┐    │
│  │  Frame Sync     │  │   Frame Pacer             │    │
│  │  (mtl_sync)     │  │   (mtl_frame_sync)        │    │
│  ├─────────────────┤  ├───────────────────────────┤    │
│  │• Semaphores     │  │• Fixed/Variable/Adaptive  │    │
│  │• Shared Events  │  │• Budget Enforcement       │    │
│  │• Fences         │  │• Drop Detection           │    │
│  │• Triple Buffer  │  │• Percentile Stats         │    │
│  └─────────────────┘  └───────────────────────────┘    │
│                                                          │
│  ┌─────────────────┐  ┌───────────────────────────┐    │
│  │  Hazard Track   │  │   GPU Timestamps          │    │
│  │  (mtl_hazard)   │  │   (mtl_timestamp)         │    │
│  ├─────────────────┤  ├───────────────────────────┤    │
│  │• RAW/WAR/WAW    │  │• Counter Buffers          │    │
│  │• Auto Barriers  │  │• Nanosecond Precision     │    │
│  │• Dependency     │  │• Profiling Regions        │    │
│  │  Graph          │  │• CPU Correlation          │    │
│  └─────────────────┘  └───────────────────────────┘    │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │  VSync Integration (mtl_vsync)                  │   │
│  ├─────────────────────────────────────────────────┤   │
│  │• CVDisplayLink                                 │   │
│  │• Frame Jitter Measurement                       │   │
│  │• Refresh Rate Detection                         │   │
│  └─────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────┘
```

## Features

### ✅ Frame Synchronization
- **Triple buffering** with dispatch semaphores
- **MTLSharedEvent** for GPU completion tracking
- **Timeout support** with deadlock prevention
- **Frame index tracking** across submissions

### ✅ Advanced Frame Pacing
- **Fixed, variable, and adaptive** pacing modes
- **Frame budget enforcement** with drop detection
- **Percentile statistics** (95th, 99th)
- **Rolling window** analysis (configurable size)

### ✅ Resource Hazard Tracking
- **Automatic detection** of RAW/WAR/WAW hazards
- **Conservative/Opportunistic** barrier insertion policies
- **Dependency graph** for resource access tracking
- **Cache coherence** state machine
- **Comprehensive analytics** and reporting

### ✅ GPU Timestamps & Profiling
- **MTLCounterSampleBuffer** for precise timing
- **Nanosecond-precision** GPU timing
- **Per-label profiling** regions
- **CPU-GPU timestamp** correlation
- **Automatic statistics** aggregation

### ✅ VSync Integration
- **CVDisplayLink** for display synchronization
- **Jitter measurement** (frame time variance)
- **Refresh rate detection**
- **VBlank callbacks**

## Quick Start

###Simple Usage (Recommended)

```c
#import "mtl_sync_manager.h"

// Configure synchronization
metal_sync_manager_config_t config = {
    .max_frames_in_flight = 3,
    .target_fps = 60,
    .enable_vsync = true,
    .enable_hazard_tracking = true,
    .enable_gpu_timestamps = true,
    .barrier_policy = METAL_BARRIER_POLICY_CONSERVATIVE
};

// Create manager
metal_sync_manager_t* sync = metal_sync_manager_create(device, &config);

// Render loop
while (running) {
    if (metal_sync_manager_begin_frame(sync)) {
        // Encode rendering...
        metal_sync_manager_end_frame(sync, cmd_buffer);
    }
}

// Print comprehensive statistics
metal_sync_manager_print_report(sync);

// Cleanup
metal_sync_manager_destroy(sync);
```

### Advanced Usage (Manual Control)

```c
// Individual component creation
metal_frame_sync_t* frame_sync = metal_frame_sync_create(device, 3);
metal_frame_pacer_t* pacer = metal_frame_pacer_create(&pacer_config);
metal_barrier_manager_t* barriers = metal_barrier_manager_create(device, policy);
metal_timestamp_pool_t* timestamps = metal_timestamp_pool_create(device, &ts_config);

// Manual frame management
metal_frame_begin(frame_sync);
metal_frame_pacer_begin_frame(pacer);

// GPU profiling
uint32_t query = metal_timestamp_begin(timestamps, cmd_buffer, "Render Pass");
// ... encoding ...
metal_timestamp_end(timestamps, cmd_buffer, query);

// Hazard tracking
metal_barrier_manager_access_resource(barriers, &access_desc, cmd_buffer);

// Frame end
metal_frame_end(frame_sync, cmd_buffer);
metal_frame_pacer_end_frame(pacer, gpu_time_ms);
```

## API Reference

### Core Modules

#### `mtl_sync_manager.h` - Unified API
- `metal_sync_manager_create()` - Create unified manager
- `metal_sync_manager_begin_frame()` - Start frame with full sync
- `metal_sync_manager_end_frame()` - End and submit frame
- `metal_sync_manager_get_stats()` - Get comprehensive statistics
- `metal_sync_manager_print_report()` - Print detailed report

#### `mtl_sync.h` - Frame & Event Synchronization
- `metal_frame_sync_create()` - Create frame sync object
- `metal_frame_begin()` - Wait for frame slot
- `metal_frame_end()` - Release frame and track completion
- `metal_event_create()` - Create MTLSharedEvent wrapper
- `metal_event_wait()` - CPU wait for GPU event
- `metal_fence_create()` - Create GPU-GPU fence

#### `mtl_frame_sync.h` - Advanced Pacing
- `metal_frame_pacer_create()` - Create frame pacer
- `metal_frame_pacer_begin_frame()` - Apply pacing logic
- `metal_frame_pacer_end_frame()` - Record frame timing
- `metal_frame_pacer_get_stats()` - Get timing statistics
- `metal_frame_pacer_is_within_budget()` - Check budget status

#### `mtl_hazard_tracking.h` - Resource Dependencies
- `metal_dependency_graph_create()` - Create dependency graph
- `metal_dependency_graph_check_hazards()` - Detect hazards
- `metal_barrier_manager_create()` - Create barrier manager
- `metal_barrier_manager_access_resource()` - Track access + insert barriers
- `metal_generate_hazard_report()` - Generate analytics report

#### `mtl_timestamp.h` - GPU Profiling
- `metal_timestamp_pool_create()` - Create timestamp pool
- `metal_timestamp_begin()` - Start GPU timer
- `metal_timestamp_end()` - End GPU timer
- `metal_timestamp_get_result()` - Retrieve timing result
- `metal_timestamp_print_stats()` - Print profiling statistics

#### `mtl_vsync.h` - Display Synchronization
- `metal_vsync_manager_create()` - Create VSync manager
- `metal_vsync_manager_start()` - Start VSync callbacks
- `metal_vsync_manager_wait_for_vblank()` - Blocking VBlank wait
- `metal_vsync_manager_get_stats()` - Get jitter statistics

## Performance Characteristics

| Component | Overhead | Resolution | Notes |
|-----------|----------|------------|-------|
| Frame Sync | ~1-5µs | Semaphore dispatch | Per-frame wait |
| Timestamps | ~10-50ns | ~1ns | Per query pair |
| VSync Callback | <100µs | Display refresh | macOS CVDisplayLink |
| Hazard Check | O(1) avg | Per access | Hash-based lookup |
| Barrier Insert | ~2-10µs | Per barrier | Fence creation |

## Configuration Guidelines

### Target 60 FPS (16.67ms budget)
```c
.max_frames_in_flight = 3,
.target_fps = 60,
.enable_vsync = true,
.enable_adaptive_pacing = true,
.adaptive_threshold_ms = 20.0
```

### Target 144 FPS (6.94ms budget)
```c
.max_frames_in_flight = 2,                    // Lower latency
.target_fps = 144,
.enable_vsync = false,                   // Often unsupported
.enable_adaptive_pacing = true,
.adaptive_threshold_ms = 10.0
```

### Ultra-Low Latency
```c
.max_frames_in_flight = 1,                    // Single frame
.target_fps = 60,
.enable_vsync = false,
.enable_adaptive_pacing = false
```

## Best Practices

### 1. Always Use Unified Manager
```c
// ✅ Recommended
metal_sync_manager_t* sync = metal_sync_manager_create(device, &config);

// ❌ Only use manual components for special cases
metal_frame_sync_t* frame_sync = metal_frame_sync_create(device, 3);
```

### 2. Profile GPU Work
```c
uint32_t query = metal_timestamp_begin(pool, cmd, "Critical Section");
// ... critical GPU work ...
metal_timestamp_end(pool, cmd, query);

// Check results later
metal_timestamp_result_t result;
if (metal_timestamp_get_result(pool, query, &result)) {
    if (result.gpu_timestamp_ns > 5000000) { // > 5ms
        printf("WARNING: Critical section taking %.2f ms\n",
               result.gpu_timestamp_ns / 1000000.0);
    }
}
```

### 3. Monitor Frame Drops
```c
void on_frame_drop(uint64_t frame_idx, double exceeded_ms, void* user_data) {
    printf("Frame %llu dropped, exceeded budget by %.2f ms\n", 
           frame_idx, exceeded_ms);
    // Consider reducing quality settings...
}

metal_frame_pacer_set_drop_callback(pacer, on_frame_drop, NULL);
```

### 4. Use Conservative Barriers During Development
```c
// Development: Catch all hazards
.barrier_policy = METAL_BARRIER_POLICY_CONSERVATIVE

// Production: Optimize after validation
.barrier_policy = METAL_BARRIER_POLICY_OPPORTUNISTIC
```

## Troubleshooting

### High Frame Drop Rate
- **Reduce target FPS** or **enable adaptive pacing**
- **Profile GPU work** with timestamps
- **Check hazard report** for excessive barriers

### GPU Stalls
- **Enable hazard tracking** to detect RAW/WAR/WAW issues
- **Review barrier insertion** statistics
- **Verify resource lifetimes** across frames

### High Synchronization Overhead
- **Reduce max_frames_in_flight** (less semaphore contention)
- **Disable GPU timestamps** in production builds
- **Use opportunistic barrier** policy

### VSync Jitter
- **Check system load** (background processes)
- **Verify refresh rate** detection
- **Monitor dropped VBlanks**

## Platform Notes

### macOS
- Full support for all features
- CVDisplayLink provides hardware VSync
- Metal counters available on all GPUs

### iOS/iPadOS
- VSync uses CADisplayLink (not CVDisplayLink)
- Some GPU counters may be unavailable
- ProMotion displays (120Hz) fully supported

### tvOS
- Similar to iOS
- Fixed 60Hz displays

## Examples

See `mtl_sync_example.c` for comprehensive usage examples including:
- Complete frame rendering with all features
- Manual synchronization with individual components
- Hazard tracking and automatic barriers
- GPU profiling and statistics

## License

Part of the Advanced 3D Rendering Engine  
Platform subsystem - Metal backend

## See Also

- [mtl_device.h](mtl_device.h) - Metal device management
- [mtl_command_buffer.h](mtl_command_buffer.h) - Command buffer lifecycle
- [mtl_encoder.h](mtl_encoder.h) - Render/compute encoding
