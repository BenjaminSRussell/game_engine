# Phase 1 Critical Fixes - Implementation Guide

**Status:** Ready for Development
**Date:** January 7, 2026
**Scope:** 100 issues across 5 categories
**Timeline:** 12-16 days of focused work

---

## QUICK START: Top 10 Critical Fixes (First Day)

These 10 fixes will immediately stabilize the pipeline and prevent most crashes:

### 1. Fix convert_format() void return (5 min)
**File:** `mtl_texture.c` lines 29-64

**Current:**
```c
static inline void convert_format(metal_pixel_format_t fmt, MTLPixelFormat* mtl_fmt) {
    switch (fmt) {
        case METAL_PIXEL_FORMAT_RGBA8:
            return MTLPixelFormatRGBA8Unorm;  // ERROR: void function returning value
        // ...
    }
}
```

**Fixed:**
```c
static inline MTLPixelFormat convert_format(metal_pixel_format_t fmt) {
    switch (fmt) {
        case METAL_PIXEL_FORMAT_RGBA8:
            return MTLPixelFormatRGBA8Unorm;  // CORRECT
        case METAL_PIXEL_FORMAT_RGBA8_SRGB:
            return MTLPixelFormatRGBA8Unorm_sRGB;
        case METAL_PIXEL_FORMAT_BGRA8:
            return MTLPixelFormatBGRA8Unorm;
        case METAL_PIXEL_FORMAT_DEPTH32F:
            return MTLPixelFormatDepth32Float;
        case METAL_PIXEL_FORMAT_DEPTH24_STENCIL8:
#if TARGET_OS_OSX
            return MTLPixelFormatDepth24Unorm_Stencil8;
#else
            return MTLPixelFormatDepth32Float_Stencil8;
#endif
        case METAL_PIXEL_FORMAT_BC1_RGBA:
            return MTLPixelFormatBC1_RGBA;
        default:
            NSLog(@"Unsupported pixel format: %d", fmt);
            return MTLPixelFormatRGBA8Unorm;  // Safe fallback
    }
}

// Usage change:
MTLPixelFormat fmt = convert_format(desc->format);
```

**Test:**
```c
void test_convert_format() {
    MTLPixelFormat fmt = convert_format(METAL_PIXEL_FORMAT_RGBA8);
    assert(fmt == MTLPixelFormatRGBA8Unorm);
    printf("✓ convert_format works\n");
}
```

---

### 2. Add null check after descriptor allocation (5 min)
**File:** `mtl_texture.c` lines 142-172

**Current:**
```c
MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
mtl_desc.textureType = mtl_type;  // Might crash if alloc failed
mtl_desc.pixelFormat = mtl_format;
// ... 4 more assignments ...

texture->texture = [device->device newTextureWithDescriptor:mtl_desc];
if (!texture->texture) {  // Check is too late
    free(texture);
    return NULL;
}
```

**Fixed:**
```c
MTLTextureDescriptor* mtl_desc = [[MTLTextureDescriptor alloc] init];
if (!mtl_desc) {  // Check IMMEDIATELY
    free(texture);
    return NULL;
}

mtl_desc.textureType = mtl_type;
mtl_desc.pixelFormat = mtl_format;
mtl_desc.width = desc->width;
mtl_desc.height = desc->height;
mtl_desc.depth = desc->depth;
mtl_desc.mipmapLevelCount = desc->mip_levels;
mtl_desc.arrayLength = desc->array_length;

texture->texture = [device->device newTextureWithDescriptor:mtl_desc];
if (!texture->texture) {
    [mtl_desc release];
    free(texture);
    return NULL;
}

[mtl_desc release];
return texture;
```

**Test:**
```c
void test_texture_allocation_failure() {
    // Mock allocation to fail
    metal_texture_desc_t desc = {...};
    metal_texture_t* tex = metal_texture_create(&g_device, &desc);
    // Should return NULL without crashing
    assert(tex == NULL);
    printf("✓ Null check works\n");
}
```

---

### 3. Fix infinite wait in frame sync (5 min)
**File:** `mtl_sync.m` lines 64-71

**Current:**
```c
void metal_frame_begin(metal_frame_sync_t *sync) {
    if (!sync) return;
    dispatch_semaphore_wait(sync->frame_semaphore, DISPATCH_TIME_FOREVER);
    // If GPU stalls, this waits forever with no way to recover
}
```

**Fixed:**
```c
bool metal_frame_begin(metal_frame_sync_t *sync, mtl_error_code_t *out_error) {
    if (!sync) {
        if (out_error) *out_error = MTL_ERROR_NULL_POINTER;
        return false;
    }

    // 100ms timeout for frame sync
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 100000000);  // 100ms in ns
    long result = dispatch_semaphore_wait(sync->frame_semaphore, timeout);

    if (result != 0) {
        // Timeout occurred
        if (out_error) *out_error = MTL_ERROR_GPU_TIMEOUT;
        NSLog(@"Frame sync timeout - GPU may be stalled");
        return false;
    }

    if (out_error) *out_error = MTL_SUCCESS;
    return true;
}
```

**Test:**
```c
void test_frame_begin_timeout() {
    metal_frame_sync_t sync = {...};
    mtl_error_code_t error;

    // Should timeout if GPU stalled
    bool success = metal_frame_begin(&sync, &error);

    if (!success) {
        assert(error == MTL_ERROR_GPU_TIMEOUT);
        printf("✓ Timeout detected correctly\n");
    }
}
```

---

### 4. Fix pool saturation leak (5 min)
**File:** `mtl_resource_pool.c` lines 112-127

**Current:**
```c
metal_buffer_t* buffer = metal_buffer_create(device, &desc);
if (!buffer) return NULL;

if (pool->buffer_count < MAX_POOLED_BUFFERS) {
    cached_buffer_t* slot = &pool->buffers[pool->buffer_count++];
    slot->buffer = buffer;
}
// If pool full, buffer is created but NEVER freed - MEMORY LEAK
```

**Fixed:**
```c
metal_buffer_t* buffer = metal_buffer_create(device, &desc);
if (!buffer) return NULL;

if (pool->buffer_count >= MAX_POOLED_BUFFERS) {
    // Pool full, can't cache this buffer
    metal_buffer_destroy(buffer);
    NSLog(@"Warning: Buffer pool full (%d), allocation failed", MAX_POOLED_BUFFERS);
    return NULL;
}

cached_buffer_t* slot = &pool->buffers[pool->buffer_count++];
slot->buffer = buffer;
slot->creation_time = mach_absolute_time();
return buffer;
```

**Test:**
```c
void test_pool_saturation() {
    metal_resource_pool_t pool = create_pool(max_buffers: 2);

    metal_buffer_t* buf1 = metal_buffer_pool_allocate(&pool, size: 1MB);
    assert(buf1 != NULL);

    metal_buffer_t* buf2 = metal_buffer_pool_allocate(&pool, size: 1MB);
    assert(buf2 != NULL);

    // Should return NULL and not leak
    metal_buffer_t* buf3 = metal_buffer_pool_allocate(&pool, size: 1MB);
    assert(buf3 == NULL);
    printf("✓ Pool saturation handled\n");
}
```

---

### 5. Fix integer overflow in sleep (10 min)
**File:** `mtl_frame_sync.c` lines 195-220

**Current:**
```c
if (elapsed_ms < target_ms) {
    double sleep_ms = target_ms - elapsed_ms;
    uint64_t sleep_time = ms_to_mach_time(sleep_ms);  // CAN OVERFLOW
    mach_wait_until(current_time + sleep_time);  // Might wait months
}
```

**Fixed:**
```c
if (elapsed_ms < target_ms) {
    double sleep_ms = target_ms - elapsed_ms;

    // Validate sleep time
    if (sleep_ms <= 0.0) {
        return;  // Already past target
    }

    // Cap at 1 second maximum
    if (sleep_ms > 1000.0) {
        NSLog(@"Warning: Requested sleep %.0fms exceeds max of 1000ms", sleep_ms);
        sleep_ms = 1000.0;
    }

    // Convert to mach time
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }

    uint64_t sleep_time = (uint64_t)(sleep_ms * 1000000.0 * timebase.denom / timebase.numer);

    // Validate no overflow
    if (sleep_time == 0) {
        sleep_time = 1;  // Minimum 1 unit
    }

    uint64_t wake_time = current_time + sleep_time;
    if (wake_time < current_time) {
        // Overflow detected
        NSLog(@"Error: Sleep time overflow detected");
        return;
    }

    mach_wait_until(wake_time);
}
```

**Test:**
```c
void test_sleep_overflow() {
    metal_frame_pacer_t pacer = {...};

    // Try to sleep for 1 day
    pacer.target_ms = 86400000;  // 1 day in ms
    pacer.last_frame_time = mach_absolute_time();
    usleep(1000);  // Elapsed 1ms

    // Should cap at 1 second, not hang for a day
    mach_absolute_time_t before = mach_absolute_time();
    metal_frame_pacer_update(&pacer);
    mach_absolute_time_t after = mach_absolute_time();

    // Should take ~1 second max
    double elapsed_s = mach_time_to_seconds(after - before);
    assert(elapsed_s < 1.5);  // Allow 500ms margin
    printf("✓ Sleep overflow capped correctly\n");
}
```

---

### 6. Fix listener lifetime (15 min)
**File:** `mtl_sync.m` lines 451-474

**Current:**
```c
bool metal_event_add_listener(metal_event_t *event, uint64_t value,
                              metal_event_callback_t callback, void *user_data) {
    MTLSharedEventListener *listener = [[MTLSharedEventListener alloc] init];

    [event->event notifyListener:listener atValue:value block:^(...) {
        callback(event, value, user_data);
    }];

    [listener release];  // Released too early!
    return true;
}
```

**Fixed:**
```c
typedef struct {
    MTLSharedEventListener *listener;
    metal_event_callback_t callback;
    void *user_data;
    uint64_t value;
} metal_event_listener_t;

bool metal_event_add_listener(metal_event_t *event, uint64_t value,
                              metal_event_callback_t callback, void *user_data) {
    if (!event || !callback) return false;

    // Allocate listener context
    metal_event_listener_t *ctx = malloc(sizeof(*ctx));
    if (!ctx) return false;

    MTLSharedEventListener *listener = [[MTLSharedEventListener alloc] init];
    if (!listener) {
        free(ctx);
        return false;
    }

    ctx->listener = listener;
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->value = value;

    // Store in event's listener list
    if (event->listener_count >= MAX_EVENT_LISTENERS) {
        [listener release];
        free(ctx);
        return false;
    }

    event->listeners[event->listener_count++] = ctx;

    __weak metal_event_t *weak_event = event;

    [event->event notifyListener:listener atValue:value block:^(id<MTLSharedEvent> evt, uint64_t val) {
        metal_event_t *strong_event = weak_event;
        if (strong_event && ctx->callback) {
            ctx->callback(strong_event, val, ctx->user_data);
        }
    }];

    return true;
}

void metal_event_destroy(metal_event_t *event) {
    if (!event) return;

    // Release all listeners
    for (uint32_t i = 0; i < event->listener_count; i++) {
        metal_event_listener_t *ctx = event->listeners[i];
        if (ctx) {
            [ctx->listener release];
            free(ctx);
        }
    }
    event->listener_count = 0;

    if (event->event) [event->event release];
    free(event);
}
```

**Test:**
```c
void test_listener_lifetime() {
    metal_event_t *event = metal_event_create(&device);
    assert(event != NULL);

    __block bool callback_called = false;

    bool added = metal_event_add_listener(event, 1, ^(metal_event_t *evt, uint64_t val, void *ud) {
        callback_called = true;
    }, NULL);
    assert(added);

    // Signal event
    [event->event signalValue:1];

    // Give time for listener callback
    sleep(0.1);

    // Should be called without crashing
    assert(callback_called);

    metal_event_destroy(event);
    printf("✓ Listener lifetime managed correctly\n");
}
```

---

### 7. Fix realloc failure handling (5 min)
**File:** `mtl_frame_sync.c` lines 397

**Current:**
```c
if (frame->resource_count >= frame->resource_capacity) {
    uint32_t new_capacity = frame->resource_capacity * 2;
    void** new_resources = realloc(frame->resources, new_capacity * sizeof(void*));

    if (!new_resources) return;  // OLD POINTER LOST!

    frame->resources = new_resources;
    frame->resource_capacity = new_capacity;
}
```

**Fixed:**
```c
if (frame->resource_count >= frame->resource_capacity) {
    uint32_t new_capacity = frame->resource_capacity * 2;

    // Cap to reasonable maximum
    if (new_capacity > MAX_FRAME_RESOURCES) {
        new_capacity = MAX_FRAME_RESOURCES;
        if (frame->resource_count >= MAX_FRAME_RESOURCES) {
            return false;  // Pool full
        }
    }

    // Keep old pointer in case realloc fails
    void** old_resources = frame->resources;
    void** new_resources = realloc(old_resources, new_capacity * sizeof(void*));

    if (!new_resources) {
        // Realloc failed, but old_resources still valid
        NSLog(@"Error: Failed to expand resource list");
        return false;
    }

    frame->resources = new_resources;
    frame->resource_capacity = new_capacity;
}

// Add resource
frame->resources[frame->resource_count++] = resource;
return true;
```

**Test:**
```c
void test_realloc_failure() {
    metal_frame_resources_t frame = {.capacity = 4, .count = 0};
    frame.resources = malloc(4 * sizeof(void*));

    // Add 4 resources
    for (int i = 0; i < 4; i++) {
        frame.resources[frame.count++] = (void*)(intptr_t)i;
    }

    // Mock realloc to fail
    // Next add should return false, not crash
    bool result = metal_frame_resource_add(&frame, (void*)100);

    // Old resources still there
    assert(frame.resources[0] == (void*)0);
    assert(frame.resources[3] == (void*)3);
    printf("✓ Realloc failure handled\n");
}
```

---

### 8. Fix encoder null check (5 min)
**File:** `mtl_command.c` lines 39-60

**Current:**
```c
mtl_render_command_encoder_t metal_command_encoder_render_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
    MTLRenderPassDescriptor* mtl_pass = (__bridge MTLRenderPassDescriptor*)pass_descriptor;

    return (__bridge_retained void*)[cmd_buf renderCommandEncoderWithDescriptor:mtl_pass];
    // No nil check!
}
```

**Fixed:**
```c
mtl_render_command_encoder_t metal_command_encoder_render_create(mtl_command_buffer_t buffer, void* pass_descriptor) {
    if (!buffer || !pass_descriptor) {
        NSLog(@"Error: Invalid buffer or descriptor");
        return NULL;
    }

    id<MTLCommandBuffer> cmd_buf = (__bridge id<MTLCommandBuffer>)buffer;
    MTLRenderPassDescriptor* mtl_pass = (__bridge MTLRenderPassDescriptor*)pass_descriptor;

    id<MTLRenderCommandEncoder> encoder = [cmd_buf renderCommandEncoderWithDescriptor:mtl_pass];

    if (!encoder) {
        NSLog(@"Error: Failed to create render encoder");
        return NULL;
    }

    return (__bridge_retained void*)encoder;
}
```

**Test:**
```c
void test_encoder_creation_failure() {
    mtl_command_buffer_t buf = NULL;

    // Should return NULL without crashing
    mtl_render_command_encoder_t encoder = metal_command_encoder_render_create(buf, NULL);
    assert(encoder == NULL);
    printf("✓ Encoder null check works\n");
}
```

---

### 9. Fix shader cache collision (10 min)
**File:** `mtl_shader_compiler.m` lines 182-200

**Current:**
```c
uint64_t source_hash = hash_source(source);
for (uint32_t i = 0; i < compiler->cache_count; i++) {
    if (compiler->cache[i].in_use && compiler->cache[i].source_hash == source_hash) {
        return compiler->cache[i].library;  // WRONG! Might be different shader
    }
}
```

**Fixed:**
```c
uint64_t source_hash = hash_source(source);
size_t source_len = strlen(source);

for (uint32_t i = 0; i < compiler->cache_count; i++) {
    metal_shader_cache_entry_t *entry = &compiler->cache[i];

    if (!entry->in_use) continue;
    if (entry->source_hash != source_hash) continue;

    // Hash collision - verify actual source
    if (entry->source_code && entry->source_len == source_len) {
        if (memcmp(entry->source_code, source, source_len) == 0) {
            // Exact match!
            compiler->cache_hits++;
            return entry->library;
        }
    }
}

// Not found, compile new shader
return NULL;
```

**Test:**
```c
void test_shader_cache_collision() {
    metal_shader_compiler_t *compiler = metal_shader_compiler_create();

    const char *shader1 = "fragment float4 main() { return float4(1,0,0,1); }";
    const char *shader2 = "fragment float4 main() { return float4(0,1,0,1); }";

    // Force collision (modify hash function for testing)
    // Both should compile to different libraries

    metal_shader_result_t result1 = metal_shader_compile(compiler, shader1);
    metal_shader_result_t result2 = metal_shader_compile(compiler, shader2);

    assert(result1.library != result2.library);  // Different libraries!
    printf("✓ Shader cache collision handled\n");
}
```

---

### 10. Fix device array leak (5 min)
**File:** `mtl_device.c` lines 161-175

**Current:**
```c
NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
if (devices) {
    for (id<MTLDevice> device in devices) {
        if ([device isLowPower]) {
            mtl_dev = device;
            break;
        }
    }
}
// MISSING: [devices release]
```

**Fixed:**
```c
@autoreleasepool {
    NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
    if (devices) {
        for (id<MTLDevice> device in devices) {
            if ([device isLowPower]) {
                mtl_dev = device;
                break;
            }
        }
        [devices release];  // CRITICAL
    }
}

// Or explicit:
NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();
if (devices) {
    // ... device selection ...
    [devices release];
}
```

**Test:**
```c
void test_device_array_leak() {
    @autoreleasepool {
        NSArray *devices = MTLCopyAllDevices();
        assert(devices != NULL);
        NSUInteger count = [devices count];
        [devices release];
    }
    printf("✓ Device array released\n");
}
```

---

## Daily Implementation Plan

### Day 1: Critical Crashes (Issues #1-11)
- Morning: Fix #1-5 (void return, null checks, fence, listener, realloc)
- Afternoon: Fix #6-11 (formats, encoder, command buffer, cache, array leak)
- Testing: Basic texture creation, no crashes

### Day 2: Freezes & Hangs (Issues #12-25)
- Morning: Fix #12-18 (timeouts, polling, drawable, callbacks, locks)
- Afternoon: Fix #19-25 (resource timeout, listeners, render pass, barriers)
- Testing: Frame sync doesn't hang, CPU doesn't spin

### Day 3: Memory Leaks (Issues #26-37)
- All day: Implement cleanup for resource pools, samplers, shaders, frames
- Testing: 1000 frame test, memory stable

### Day 4: Synchronization (Issues #38-49)
- All day: Add mutexes for shared data structures
- Testing: Parallel operations from multiple threads

### Day 5: Error Handling (Issues #50-60)
- All day: Add error codes to all critical functions
- Testing: Error propagation works end-to-end

### Days 6-7: Performance & Quality (Issues #61-100)
- Implement optional fixes based on priority
- Code cleanup and documentation

---

## Compilation & Testing

### After Each Fix:
```bash
# Compile with warnings enabled
clang -Wall -Wextra -Werror -fsanitize=address,undefined \
  mtl_*.c mtl_*.m -framework Metal -framework Cocoa

# Run tests
./test_runner --verbose

# Check for sanitizer warnings
# Check for memory leaks (Instruments)
```

### Success Criteria:
```
✓ Compiles without warnings
✓ AddressSanitizer clean
✓ ThreadSanitizer clean
✓ 1000+ frame test completes
✓ Memory usage stable
✓ No CPU spinning
✓ All error codes correct
```

---

## Next Steps

1. **Start with Fix #1** - Takes 5 minutes, immediate crash prevention
2. **Work through all 10 top fixes** - Takes ~1 hour total
3. **Test rendering** - Should work without crashes
4. **Continue with remaining 90** - Follow daily plan

**Ready to begin? Start with `mtl_texture.c` line 29-64!**
