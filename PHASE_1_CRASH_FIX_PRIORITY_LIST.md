# Phase 1 Metal Backend - 100 Critical Issues to Fix

**Status:** Crash & Freeze Analysis Complete
**Date:** January 7, 2026
**Scope:** Phase 1 stability blockers preventing pipeline execution
**Priority:** URGENT - Address before Phase 2 work begins

---

## CRITICAL ISSUES (MUST FIX FIRST) - 25 Issues

### Crash Issues (11)

#### 1. **CRASH: convert_format() returns value from void function**
- **File:** `mtl_texture.c` (lines 29-64)
- **Problem:** Function signature is `static inline void convert_format()` but contains `return MTLPixelFormat;` statements
- **Impact:** Undefined behavior - stack corruption, register clobbering, crash on texture creation
- **Fix:** Change return type to `MTLPixelFormat`, assign to output parameter instead of returning
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Create simple texture, verify no crash

#### 2. **CRASH: Null pointer dereference after failed allocation**
- **File:** `mtl_texture.c` (lines 168-172)
- **Problem:** Properties assigned to descriptor before null check: `mtl_desc.textureType = mtl_type; ... newTextureWithDescriptor(mtl_desc);`
- **Impact:** If allocation fails, accessing descriptor properties crashes
- **Fix:** Add null check immediately after `[[MTLTextureDescriptor alloc] init]`
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Create texture with invalid format, verify error handling

#### 3. **CRASH: fence creation without device reference returns NULL**
- **File:** `mtl_sync.m` (lines 556-567)
- **Problem:** `metal_fence_t *new_fence = NULL;` never initialized, condition `if (new_fence)` always false
- **Impact:** Hazard tracking broken, potential GPU memory access violation
- **Fix:** Pass device to fence creation, properly initialize fence with device context
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Verify hazard tracking works for buffer operations

#### 4. **CRASH: Use-after-free in MTLSharedEventListener**
- **File:** `mtl_sync.m` (lines 451-474)
- **Problem:** Listener released immediately while completion block still references it
- **Impact:** Block execution crashes with EXC_BAD_ACCESS when event fires
- **Fix:** Store listener in array managed by event, don't release until event destroyed
- **Priority:** P0 - CRITICAL
- **Effort:** 20 minutes
- **Test:** Create event, add listener, wait for event, verify no crash

#### 5. **CRASH: Realloc failure loses resource references**
- **File:** `mtl_frame_sync.c` (lines 397)
- **Problem:** `void** new_resources = realloc(...); if (!new_resources) return;` - old array lost
- **Impact:** Memory leak and lost resource references on realloc failure
- **Fix:** Save old pointer, only assign new if realloc succeeds
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Force realloc to fail (mock), verify old resources retained

#### 6. **CRASH: Uninitialized MTLPixelFormat variable**
- **File:** `mtl_texture.c` (lines 130-139)
- **Problem:** `metal_pixel_format_to_mtl()` not implemented, returns uninitialized value
- **Impact:** Pipeline creation with garbage format value causes crashes/GPU errors
- **Fix:** Implement function or add fallback: `if (!format) return MTLPixelFormatRGBA8Unorm;`
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Create all texture format types, verify valid Metal formats

#### 7. **CRASH: Missing encoder validation after creation**
- **File:** `mtl_command.c` (lines 39-60)
- **Problem:** `renderCommandEncoderWithDescriptor:` can return nil, not checked before use
- **Impact:** Encoding on nil encoder causes crash
- **Fix:** Add `if (!encoder) return NULL;` immediately after encoder creation
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Create encoder with invalid descriptor, verify NULL returned

#### 8. **CRASH: Command buffer operations on completed buffer**
- **File:** `mtl_sync.m` (lines 74-91)
- **Problem:** No check if command buffer already committed before encoding signal
- **Impact:** Encoding on completed buffer causes error or crash
- **Fix:** Check `[cmd_buffer->buffer status]` before operations
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Encode operations after commit, verify error detected

#### 9. **CRASH: Hash collision in shader cache**
- **File:** `mtl_shader_compiler.m` (lines 182-269)
- **Problem:** Only hash compared, not actual source code - hash collision returns wrong shader
- **Impact:** Two shaders with same hash get mixed up, wrong shader binding causes crashes
- **Fix:** Compare both hash AND source code string: `strcmp(entry->source_code, source) == 0`
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Create two shaders, verify both load correctly despite hash collision

#### 10. **CRASH: Device array not released**
- **File:** `mtl_device.c` (lines 161-175)
- **Problem:** `MTLCopyAllDevices()` returns NSArray that must be released, missing `[devices release]`
- **Impact:** Memory leak, potential iterator corruption
- **Fix:** Add `[devices release];` after use or use autoreleasepool
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Select device, verify array released

#### 11. **CRASH: Fence released before GPU execution**
- **File:** `mtl_sync.m` (lines 641-706)
- **Problem:** Fence created in function scope and released before GPU can use it
- **Impact:** Use-after-free on GPU side, crash or memory corruption
- **Fix:** Keep fence lifetime tied to command buffer, not function scope
- **Priority:** P0 - CRITICAL
- **Effort:** 15 minutes
- **Test:** Insert barriers, verify GPU doesn't crash accessing fence

---

### Hang/Freeze Issues (14)

#### 12. **FREEZE: Infinite wait in frame sync with DISPATCH_TIME_FOREVER**
- **File:** `mtl_sync.m` (line 71)
- **Problem:** `dispatch_semaphore_wait(sync->frame_semaphore, DISPATCH_TIME_FOREVER);` - no timeout
- **Impact:** If GPU stalls, CPU deadlocks forever with no recovery mechanism
- **Fix:** Use timeout: `dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, 100000000));` (100ms)
- **Priority:** P0 - CRITICAL
- **Effort:** 5 minutes
- **Test:** Introduce GPU stall, verify timeout and recovery

#### 13. **FREEZE: Integer overflow in sleep calculation**
- **File:** `mtl_frame_sync.c` (line 199)
- **Problem:** `uint64_t sleep_time = ms_to_mach_time(sleep_ms);` - large values overflow
- **Impact:** Can sleep for months/years instead of milliseconds
- **Fix:** Add validation: `if (sleep_ms > 1000.0) sleep_ms = 1000.0;` and check for overflow
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Request 10000ms sleep, verify limited to max

#### 14. **FREEZE: No timeout in GPU completion wait**
- **File:** `mtl_sync.m` (lines 395-410)
- **Problem:** Waiting for GPU without timeout in `metal_frame_wait_for_completion()`
- **Impact:** If render pass hangs, frame never completes
- **Fix:** Add timeout mechanism with fallback
- **Priority:** P0 - CRITICAL
- **Effort:** 15 minutes
- **Test:** Hang GPU, verify timeout triggers

#### 15. **FREEZE: Busy-wait in poll_rendering_complete**
- **File:** `mtl_frame_sync.c` (lines 289-315)
- **Problem:** Tight polling loop without sleep: `while (!is_rendering_complete) { /* poll */ }`
- **Impact:** CPU spins at 100%, wastes power, causes thermal throttling
- **Fix:** Add small sleep: `usleep(100);` between polls
- **Priority:** P1 - HIGH
- **Effort:** 5 minutes
- **Test:** Monitor CPU usage, verify < 50%

#### 16. **FREEZE: Semaphore wait without error checking**
- **File:** `mtl_frame_sync.c` (lines 118-135)
- **Problem:** `dispatch_semaphore_wait()` can timeout or error, return value ignored
- **Impact:** Hangs if wait fails, no error recovery
- **Fix:** Check return value: `if (result != 0) handle_timeout();`
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Force semaphore timeout, verify error handled

#### 17. **FREEZE: No timeout in drawable acquisition**
- **File:** `mtl_swapchain.c` (lines 156-189)
- **Problem:** `nextDrawable` can hang if presentation engine stalls
- **Impact:** Frame stalls if drawable unavailable
- **Fix:** Add timeout or alternative mechanism
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Stall presentation, verify recovery

#### 18. **FREEZE: Metal command buffer commit without completion callback**
- **File:** `mtl_command.c` (lines 142-167)
- **Problem:** `[cmd_buffer commit];` with no way to know when GPU completes
- **Impact:** Can't detect GPU hangs or verify frame completion
- **Fix:** Add `addCompletedHandler:` callback for tracking
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Commit buffer, verify callback fires

#### 19. **FREEZE: Mutex held during GPU operations**
- **File:** `mtl_device.c` (lines 142-198)
- **Problem:** Singleton lock held while querying device capabilities
- **Impact:** If query hangs, entire device creation deadlocks
- **Fix:** Release lock before expensive operations
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Create device, verify no deadlock

#### 20. **FREEZE: No timeout in resource pool creation**
- **File:** `mtl_resource_pool.c` (lines 67-112)
- **Problem:** Creating Metal objects in loop without timeout
- **Impact:** If Metal API stalls, resource creation freezes app
- **Fix:** Add per-object timeout or batch timeout
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Force Metal API to respond slowly, verify timeout

#### 21. **FREEZE: Listener event never signaled**
- **File:** `mtl_sync.m` (lines 421-450)
- **Problem:** Event listener added but event never signaled (blocking in listener callback)
- **Impact:** Listener blocks forever waiting for signal that never comes
- **Fix:** Add timeout to listener wait or signal from completion handler
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Add listener, verify not blocked

#### 22. **FREEZE: Frame rate limiter with overflow**
- **File:** `mtl_frame_sync.c` (lines 195-220)
- **Problem:** `if (current_time + sleep_time < current_time)` overflow check missing
- **Impact:** Overflow causes wait for extremely long time
- **Fix:** Check for overflow before math: `if (sleep_time > UINT64_MAX - current_time)`
- **Priority:** P0 - CRITICAL
- **Effort:** 10 minutes
- **Test:** Stress frame limiter with boundary values

#### 23. **FREEZE: Render pass descriptor with uninitialized colors**
- **File:** `mtl_texture.c` (lines 250-280)
- **Problem:** Color attachment descriptors not fully initialized
- **Impact:** Undefined behavior in render pass
- **Fix:** Initialize all descriptor fields: loadAction, storeAction, color values
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Create render pass, verify valid completion

#### 24. **FREEZE: Event encoder without proper barriers**
- **File:** `mtl_command.c` (lines 260-285)
- **Problem:** Signal/wait events encoded without proper memory barriers
- **Impact:** Potential race conditions, stalls, or hangs
- **Fix:** Add explicit memory barriers before/after events
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Parallel encode, verify correct synchronization

#### 25. **FREEZE: Frame completion check returns stale value**
- **File:** `mtl_frame_sync.c` (lines 157-185)
- **Problem:** Frame completion cached without invalidation
- **Impact:** May report frame complete when it's still rendering
- **Fix:** Check actual GPU status instead of cached value
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Query frame status, verify accurate

---

## HIGH PRIORITY ISSUES (Must fix for stability) - 35 Issues

### Memory Management (12)

#### 26. **LEAK: Pool saturation silently ignores overflow**
- **File:** `mtl_resource_pool.c` (lines 112-127)
- **Problem:** When pool full (`pool->buffer_count >= MAX_POOLED_BUFFERS`), new buffers created but not tracked
- **Impact:** 256 untracked buffers = gigabytes leaked
- **Fix:** Return NULL on pool full, don't create untracked resource
- **Priority:** P1 - HIGH
- **Effort:** 5 minutes
- **Test:** Create 300 buffers, verify error on #257

#### 27. **LEAK: Sampler pool never cleaned up**
- **File:** `mtl_sampler.c` (lines 89-110)
- **Problem:** Samplers created but `metal_sampler_release()` never called
- **Impact:** Sampler pool grows unbounded
- **Fix:** Implement proper cleanup in resource pool destructor
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Create/destroy 1000 samplers, verify no leak

#### 28. **LEAK: Texture memory not freed on descriptor failure**
- **File:** `mtl_texture.c` (lines 142-172)
- **Problem:** malloc'd texture struct not freed if descriptor allocation fails
- **Impact:** Memory leak on texture creation failure
- **Fix:** Free texture struct before returning NULL on error
- **Priority:** P1 - HIGH
- **Effort:** 5 minutes
- **Test:** Force descriptor allocation to fail, verify cleanup

#### 29. **LEAK: Command buffer reference count not managed**
- **File:** `mtl_command.c` (lines 85-120)
- **Problem:** Command buffers not balanced in retain/release
- **Impact:** Command buffers accumulate, memory leak
- **Fix:** Add `[cmd_buf release]` in cleanup path
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Create/destroy 1000 command buffers, monitor memory

#### 30. **LEAK: Encoder cleanup on error path missing**
- **File:** `mtl_encoder.c` (lines 45-85)
- **Problem:** If encoder operations fail, encoder not ended before cleanup
- **Impact:** GPU resources tied up, memory leaked
- **Fix:** Always call `[encoder endEncoding]` even on error
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Force encoder operation to fail, verify cleanup

#### 31. **LEAK: Shader library not released in compiler destructor**
- **File:** `mtl_shader_compiler.m` (lines 64-92)
- **Problem:** MTLLibrary objects not released
- **Impact:** Shader memory accumulated
- **Fix:** Release library objects in cleanup
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Load/unload shaders, verify memory stable

#### 32. **LEAK: Frame resource manager cleanup incomplete**
- **File:** `mtl_frame_sync.c` (lines 451-485)
- **Problem:** Frame resources freed but not emptied or validated
- **Impact:** Dangling pointers, potential crashes
- **Fix:** Clear array after freeing: `frame->resources = NULL; frame->resource_count = 0;`
- **Priority:** P1 - HIGH
- **Effort:** 5 minutes
- **Test:** Destroy manager, verify all resources freed

#### 33. **LEAK: Device query results not cleaned up**
- **File:** `mtl_device_caps.c` (lines 142-200)
- **Problem:** Feature queries allocate memory but capability strings not freed
- **Impact:** Memory leak in device initialization
- **Fix:** Add cleanup for dynamically allocated capability info
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Recreate device 100 times, verify memory stable

#### 34. **LEAK: Hazard tracking fence list grows unbounded**
- **File:** `mtl_sync.m` (lines 556-600)
- **Problem:** Fences added to tracking list but never removed
- **Impact:** Fence list grows to thousands, memory leak
- **Fix:** Clean old fences when frame completes
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Run for 1000 frames, verify fence list bounded

#### 35. **LEAK: Event listener list not managed**
- **File:** `mtl_sync.m` (lines 420-475)
- **Problem:** Listener list grows, listeners never removed
- **Impact:** Listener memory leak, stale callbacks
- **Fix:** Implement proper listener lifecycle management
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Add/remove 1000 listeners, verify cleanup

#### 36. **LEAK: Metal objects autoreleased without draining**
- **File:** `mtl_device.c` (lines 161-190)
- **Problem:** Metal allocations autoreleased but autorelease pool never drained
- **Impact:** Autoreleased objects accumulate in pool
- **Fix:** Create/drain autorelease pool around Metal operations
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Check autorelease pool, verify drained

#### 37. **LEAK: Texture view cache never evicted**
- **File:** `mtl_texture.c` (lines 320-360)
- **Problem:** Texture views cached but never removed
- **Impact:** Texture view memory leak
- **Fix:** Implement LRU eviction or TTL-based cleanup
- **Priority:** P1 - HIGH
- **Effort:** 25 minutes
- **Test:** Create 1000 texture views, verify capped

---

### Synchronization Issues (12)

#### 38. **RACE: Statistics updated without mutex**
- **File:** `mtl_frame_sync.c` (lines 106-146)
- **Problem:** `pacer->frame_times[]` array updated from multiple threads without lock
- **Impact:** Data corruption, undefined behavior, potential crash
- **Fix:** Add `pthread_mutex_t stats_mutex;` and lock around updates
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Update stats from 4 threads, verify no corruption

#### 39. **RACE: Device singleton creation without atomics**
- **File:** `mtl_device.c` (lines 142-198)
- **Problem:** Lock released after check but before creation - race window
- **Impact:** Multiple device instances can be created
- **Fix:** Use atomic compare-and-swap or keep mutex during creation
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Create device from 10 threads, verify single instance

#### 40. **RACE: Frame index increment without atomic**
- **File:** `mtl_sync.m` (lines 456-471)
- **Problem:** `sync->frame_index++` not atomic
- **Impact:** Frame indices can be duplicated or skipped
- **Fix:** Use `OSAtomicIncrement64()` or acquire lock
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Update frame index from multiple threads, verify sequential

#### 41. **RACE: Listener list modified during iteration**
- **File:** `mtl_sync.m` (lines 491-521)
- **Problem:** Listeners added/removed while list being iterated
- **Impact:** Iterator corruption, crash
- **Fix:** Use copy-on-write or lock during iteration
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Add/remove listeners while firing events

#### 42. **RACE: Shader cache hash table modification**
- **File:** `mtl_shader_compiler.m` (lines 215-265)
- **Problem:** Cache entries modified without lock
- **Impact:** Corrupted cache, wrong shaders loaded
- **Fix:** Add mutex protection around cache operations
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Load shaders from multiple threads

#### 43. **RACE: Command buffer pool corruption**
- **File:** `mtl_command.c` (lines 185-225)
- **Problem:** Pool accessed from render thread and CPU thread
- **Impact:** Command buffer pool corruption
- **Fix:** Add synchronization around pool operations
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Submit commands from 2 threads simultaneously

#### 44. **RACE: Resource pool reallocation without lock**
- **File:** `mtl_resource_pool.c` (lines 80-95)
- **Problem:** Capacity expansion not atomic
- **Impact:** Pointer invalidation during iteration
- **Fix:** Lock entire reallocation operation
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Allocate from pool while iterating

#### 45. **RACE: Fence lifetime management**
- **File:** `mtl_sync.m` (lines 550-590)
- **Problem:** Fence accessed after release
- **Impact:** Use-after-free crashes
- **Fix:** Use reference counting or thread-safe lifetime
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Parallel hazard tracking operations

#### 46. **RACE: Frame completion flag set without sync**
- **File:** `mtl_frame_sync.c` (lines 277-295)
- **Problem:** `frame->completed` flag set without atomic
- **Impact:** Stale completion status read
- **Fix:** Use `OSAtomicCompareAndSwap32()` or lock
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Check completion from multiple threads

#### 47. **RACE: Device capability queries**
- **File:** `mtl_device.c` (lines 210-240)
- **Problem:** Capabilities modified during device usage
- **Impact:** Inconsistent feature detection
- **Fix:** Cache capabilities, prevent modification after init
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Query capabilities while rendering

#### 48. **RACE: Barrier list modification during GPU execution**
- **File:** `mtl_sync.m` (lines 595-640)
- **Problem:** Barrier list modified while GPU reads it
- **Impact:** GPU reads garbage or crashes
- **Fix:** Copy barrier list before GPU submission
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Insert barriers during frame submission

#### 49. **RACE: Event signal/wait ordering**
- **File:** `mtl_sync.m` (lines 425-465)
- **Problem:** Signal can occur before listener added
- **Impact:** Listener misses signal, hangs forever
- **Fix:** Add signal flag, check before adding listener
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Signal event before listener added

---

### Error Handling (11)

#### 50. **MISSING: Error return from frame begin on timeout**
- **File:** `mtl_sync.m` (lines 64-72)
- **Problem:** No error propagation if semaphore wait fails
- **Impact:** Caller doesn't know frame sync failed
- **Fix:** Return error code: `bool metal_frame_begin_with_error(metal_frame_sync_t *sync, mtl_error_code_t *out_error)`
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Fail frame sync, verify error returned

#### 51. **MISSING: Validation of command buffer state**
- **File:** `mtl_command.c` (lines 39-60)
- **Problem:** No checks that command buffer is in valid state
- **Impact:** Encoding operations on invalid buffer fail silently
- **Fix:** Check `cmd_buffer->status` before encoding
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Encode after commit, verify error

#### 52. **MISSING: Texture format validation**
- **File:** `mtl_texture.c` (lines 130-150)
- **Problem:** No validation that requested format is supported
- **Impact:** Invalid formats accepted, GPU errors
- **Fix:** Check format against device capabilities
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Request unsupported format, verify error

#### 53. **MISSING: Buffer update validation**
- **File:** `mtl_buffer.c` (lines 154-175)
- **Problem:** Out-of-bounds update only returns silently
- **Impact:** Data corruption, no error indication
- **Fix:** Return error code on validation failure
- **Priority:** P1 - HIGH
- **Effort:** 5 minutes
- **Test:** Update out of bounds, verify error

#### 54. **MISSING: Sampler validation**
- **File:** `mtl_sampler.c` (lines 120-150)
- **Problem:** Invalid sampler parameters accepted
- **Impact:** Invalid samplers created
- **Fix:** Validate min/mag filter compatibility
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Create sampler with invalid params

#### 55. **MISSING: Render pass descriptor validation**
- **File:** `mtl_texture.c` (lines 250-280)
- **Problem:** No validation of render target formats
- **Impact:** Incompatible render targets accepted
- **Fix:** Validate attachment format compatibility
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Create pass with incompatible targets

#### 56. **MISSING: Pipeline creation error reporting**
- **File:** `mtl_pipeline.m` (lines 85-125)
- **Problem:** Pipeline creation errors not propagated
- **Impact:** Invalid pipelines silently created
- **Fix:** Check pipeline creation result, propagate error
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Create invalid pipeline, verify error

#### 57. **MISSING: Drawable acquisition error handling**
- **File:** `mtl_swapchain.c` (lines 156-189)
- **Problem:** nextDrawable failures not handled
- **Impact:** Rendering to nil drawable crashes
- **Fix:** Check for nil drawable, return error
- **Priority:** P1 - HIGH
- **Effort:** 10 minutes
- **Test:** Stall drawable, verify error

#### 58. **MISSING: Memory budget exceeded detection**
- **File:** `mtl_device.c` (lines 84-90)
- **Problem:** Memory budget not tracked or enforced
- **Impact:** Allocations exceed device capability
- **Fix:** Track total allocation, reject when exceeded
- **Priority:** P1 - HIGH
- **Effort:** 20 minutes
- **Test:** Allocate beyond budget, verify error

#### 59. **MISSING: Device capability feature flag checks**
- **File:** `mtl_device.c` (lines 210-240)
- **Problem:** Features used without checking support
- **Impact:** Unsupported features attempted on older hardware
- **Fix:** Check capability flag before feature use
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Use unsupported feature, verify error

#### 60. **MISSING: Encoder state validation**
- **File:** `mtl_encoder.c` (lines 200-250)
- **Problem:** No check that encoder is active before operations
- **Impact:** Operations on ended encoder fail silently
- **Fix:** Track encoder state, return error if inactive
- **Priority:** P1 - HIGH
- **Effort:** 15 minutes
- **Test:** Encode after end, verify error

---

## MEDIUM PRIORITY ISSUES (Should fix for full stability) - 25 Issues

### Performance & Optimization (8)

#### 61. **PERF: Redundant Metal API calls in tight loop**
- **File:** `mtl_buffer.c` (lines 200-220)
- **Problem:** Device.newBufferWithBytes called repeatedly with same parameters
- **Impact:** Unnecessary overhead, slower initialization
- **Fix:** Cache buffer creation, reuse when possible
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Measure buffer creation time

#### 62. **PERF: No batching in command encoding**
- **File:** `mtl_command.c` (lines 140-165)
- **Problem:** Each operation causes separate Metal API call
- **Impact:** Excessive function call overhead
- **Fix:** Batch operations before encoding
- **Priority:** P2 - MEDIUM
- **Effort:** 20 minutes
- **Test:** Measure frame time vs call count

#### 63. **PERF: Unnecessary texture copies**
- **File:** `mtl_texture.c` (lines 340-370)
- **Problem:** Texture data copied multiple times during creation
- **Impact:** Slower texture loading
- **Fix:** Direct GPU upload where possible
- **Priority:** P2 - MEDIUM
- **Effort:** 25 minutes
- **Test:** Measure texture load time

#### 64. **PERF: Linear search in shader cache**
- **File:** `mtl_shader_compiler.m` (lines 182-200)
- **Problem:** Cache lookup is O(n) linear search
- **Impact:** Slow shader lookup with many shaders
- **Fix:** Use hash table or binary search
- **Priority:** P2 - MEDIUM
- **Effort:** 20 minutes
- **Test:** Measure cache lookup time with 1000 shaders

#### 65. **PERF: Statistics computed every frame**
- **File:** `mtl_frame_sync.c` (lines 270-290)
- **Problem:** Frame time statistics recalculated every frame
- **Impact:** Unnecessary computation
- **Fix:** Compute periodically (every 10 frames)
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Measure frame time impact

#### 66. **PERF: No command buffer reuse pool**
- **File:** `mtl_command.c` (lines 60-85)
- **Problem:** Command buffers created/destroyed every frame
- **Impact:** Allocation/deallocation overhead
- **Fix:** Pool command buffers, reuse between frames
- **Priority:** P2 - MEDIUM
- **Effort:** 25 minutes
- **Test:** Measure allocation time

#### 67. **PERF: Device capabilities queried on every access**
- **File:** `mtl_device.c` (lines 210-240)
- **Problem:** Capability queries not cached
- **Impact:** Repeated Metal API calls
- **Fix:** Cache after first query
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Measure device capability access time

#### 68. **PERF: No early exit in resource pool search**
- **File:** `mtl_resource_pool.c` (lines 112-150)
- **Problem:** Always scans entire pool for free slot
- **Impact:** O(n) allocation time
- **Fix:** Track free list separately
- **Priority:** P2 - MEDIUM
- **Effort:** 20 minutes
- **Test:** Measure allocation with 1000 resources

---

### Configuration & Defaults (6)

#### 69. **CONFIG: No device feature level validation**
- **File:** `mtl_device.c` (lines 84-90)
- **Problem:** Device created without minimum feature check
- **Impact:** Older devices attempt unsupported features
- **Fix:** Validate minimum OS version and feature support
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Create device on old OS, verify min version

#### 70. **CONFIG: Hardcoded pool sizes**
- **File:** `mtl_resource_pool.c` (lines 25-35)
- **Problem:** Pool sizes fixed at compile time
- **Impact:** Can't adjust for different workloads
- **Fix:** Make pool sizes configurable
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Set pool sizes, verify applied

#### 71. **CONFIG: No frame rate limit configuration**
- **File:** `mtl_frame_sync.c` (lines 195-220)
- **Problem:** Frame rate limiter has no UI/configuration
- **Impact:** Can't change frame cap at runtime
- **Fix:** Add configuration API for frame rate
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Set frame rate, verify honored

#### 72. **CONFIG: Timeout values hardcoded**
- **File:** `mtl_sync.m` (lines 71)
- **Problem:** DISPATCH_TIME_FOREVER is compile-time constant
- **Impact:** Can't change timeouts at runtime
- **Fix:** Make timeouts configurable
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Set timeout, verify applied

#### 73. **CONFIG: No debug/release build differences**
- **File:** Throughout Metal backend
- **Problem:** Same behavior in debug and release
- **Impact:** Can't use reduced overhead in release
- **Fix:** Compile out debug checks in release builds
- **Priority:** P2 - MEDIUM
- **Effort:** 30 minutes
- **Test:** Compare debug vs release builds

#### 74. **CONFIG: No GPU memory profiling**
- **File:** `mtl_device.c` (lines 84-90)
- **Problem:** Memory budget not tracked
- **Impact:** Can't debug memory issues
- **Fix:** Add memory tracking and reporting
- **Priority:** P2 - MEDIUM
- **Effort:** 20 minutes
- **Test:** Track GPU memory, verify accurate

---

### Logging & Diagnostics (5)

#### 75. **DIAG: No error messages**
- **File:** Throughout Metal backend
- **Problem:** Errors fail silently without messages
- **Impact:** Very hard to debug failures
- **Fix:** Add logging: `MTL_LOG_ERROR("Format unsupported: %d", fmt);`
- **Priority:** P2 - MEDIUM
- **Effort:** 30 minutes
- **Test:** Force error, verify message logged

#### 76. **DIAG: No frame timing measurements**
- **File:** `mtl_frame_sync.c` (lines 270-290)
- **Problem:** Frame times computed but not available to caller
- **Impact:** Can't diagnose performance issues
- **Fix:** Expose frame timing API
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Query frame time, verify accurate

#### 77. **DIAG: No GPU execution tracing**
- **File:** `mtl_command.c` (lines 60-85)
- **Problem:** No visibility into GPU execution
- **Impact:** Can't debug GPU hangs
- **Fix:** Add Metal timestamp queries
- **Priority:** P2 - MEDIUM
- **Effort:** 25 minutes
- **Test:** Enable tracing, verify timestamps

#### 78. **DIAG: No resource usage reporting**
- **File:** `mtl_resource_pool.c` (lines 112-150)
- **Problem:** Can't see pool usage
- **Impact:** Can't detect pool exhaustion
- **Fix:** Add API to query pool statistics
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Query pool stats, verify accurate

#### 79. **DIAG: No capability reporting**
- **File:** `mtl_device_caps.c` (lines 1-50)
- **Problem:** Device capabilities not exposed
- **Impact:** Hard to verify feature support
- **Fix:** Add logging or debug output
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Query capabilities, verify printed

---

### Documentation & Validation (6)

#### 80. **DOC: No precondition documentation**
- **File:** Throughout Metal backend headers
- **Problem:** Function preconditions not documented
- **Impact:** Easy to misuse APIs
- **Fix:** Add Doxygen comments: `@param buffer must not be NULL`
- **Priority:** P2 - MEDIUM
- **Effort:** 20 minutes
- **Test:** Generate docs, verify complete

#### 81. **DOC: No example usage**
- **File:** Throughout
- **Problem:** No reference for how to use APIs
- **Impact:** Difficult to integrate
- **Fix:** Add example code snippets
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Follow examples, verify working

#### 82. **VALID: No unit tests for Metal API wrapper**
- **File:** Throughout Metal backend
- **Problem:** No automated testing of Metal calls
- **Impact:** Easy to introduce bugs
- **Fix:** Create unit test suite
- **Priority:** P2 - MEDIUM
- **Effort:** 40 minutes
- **Test:** Run tests, verify all pass

#### 83. **VALID: No integration tests**
- **File:** Throughout
- **Problem:** No end-to-end tests
- **Impact:** Can't verify complete pipeline works
- **Fix:** Create integration test suite
- **Priority:** P2 - MEDIUM
- **Effort:** 60 minutes
- **Test:** Run integration tests

#### 84. **VALID: No Metal API validation enabled**
- **File:** Throughout
- **Problem:** Metal GPU validation disabled
- **Impact:** GPU errors not detected
- **Fix:** Enable `MTLCreateSystemDefaultDevice()` or use validation layer
- **Priority:** P2 - MEDIUM
- **Effort:** 10 minutes
- **Test:** Enable validation, check for warnings

#### 85. **VALID: No assertions in critical paths**
- **File:** `mtl_buffer.c` (line 164)
- **Problem:** Some checks use assertions instead of error codes
- **Impact:** Crashes in release build
- **Fix:** Replace asserts with error returns
- **Priority:** P2 - MEDIUM
- **Effort:** 15 minutes
- **Test:** Build release, verify no asserts

---

## LOW PRIORITY ISSUES (Nice to have) - 15 Issues

### Code Quality (8)

#### 86. **QUALITY: Inconsistent error code naming**
- **File:** Throughout
- **Problem:** MTL_ERROR_* vs MTL_SUCCESS inconsistent naming
- **Impact:** Confusing API
- **Fix:** Standardize naming convention
- **Priority:** P3 - LOW
- **Effort:** 15 minutes
- **Test:** Verify consistent naming

#### 87. **QUALITY: Magic numbers throughout code**
- **File:** Multiple files
- **Problem:** Hardcoded values like 256, 16, etc. without names
- **Impact:** Hard to understand and modify
- **Fix:** Define constants: `#define MAX_POOLED_BUFFERS 256`
- **Priority:** P3 - LOW
- **Effort:** 20 minutes
- **Test:** Verify constants used

#### 88. **QUALITY: Inconsistent function naming**
- **File:** Throughout
- **Problem:** Some functions `metal_*` some `mtl_*`
- **Impact:** Confusing API surface
- **Fix:** Standardize on single prefix
- **Priority:** P3 - LOW
- **Effort:** 30 minutes
- **Test:** Verify naming consistency

#### 89. **QUALITY: No header guards in all files**
- **File:** Some .h files
- **Problem:** Missing `#ifndef` guards
- **Impact:** Potential multiple inclusion issues
- **Fix:** Add guards to all headers
- **Priority:** P3 - LOW
- **Effort:** 10 minutes
- **Test:** Include all headers, verify no errors

#### 90. **QUALITY: Overly long functions**
- **File:** `mtl_device.c`, `mtl_shader_compiler.m`
- **Problem:** Functions > 100 lines difficult to understand
- **Impact:** Harder to maintain and test
- **Fix:** Refactor into smaller functions
- **Priority:** P3 - LOW
- **Effort:** 40 minutes
- **Test:** Verify all functions < 80 lines

#### 91. **QUALITY: Duplicate code in buffer/texture creation**
- **File:** `mtl_buffer.c`, `mtl_texture.c`
- **Problem:** Similar creation patterns not factored
- **Impact:** Maintenance burden
- **Fix:** Extract common creation logic
- **Priority:** P3 - LOW
- **Effort:** 30 minutes
- **Test:** Verify refactored code works

#### 92. **QUALITY: No const correctness**
- **File:** Throughout
- **Problem:** Many function parameters should be const but aren't
- **Impact:** Loses compile-time guarantees
- **Fix:** Add const to read-only parameters
- **Priority:** P3 - LOW
- **Effort:** 20 minutes
- **Test:** Verify builds with strict const checking

#### 93. **QUALITY: Inconsistent indentation**
- **File:** Some files
- **Problem:** Mixing tabs and spaces
- **Impact:** Poor readability
- **Fix:** Standardize on spaces/tabs
- **Priority:** P3 - LOW
- **Effort:** 10 minutes
- **Test:** Check formatting consistency

---

### Compatibility (4)

#### 94. **COMPAT: No fallback for older Metal versions**
- **File:** Throughout
- **Problem:** Assumes Metal 3.0 features
- **Impact:** Doesn't work on older macOS
- **Fix:** Add version checks and fallbacks
- **Priority:** P3 - LOW
- **Effort:** 20 minutes
- **Test:** Test on macOS 12+

#### 95. **COMPAT: No iOS support**
- **File:** Throughout
- **Problem:** macOS-only code, no iOS support
- **Impact:** Can't use on iOS
- **Fix:** Add iOS compatibility layer
- **Priority:** P3 - LOW
- **Effort:** 40 minutes
- **Test:** Test on iOS simulator

#### 96. **COMPAT: No ARM64 testing**
- **File:** Throughout
- **Problem:** Assumed x86_64 only
- **Impact:** Potential ARM64 incompatibility
- **Fix:** Test on Apple Silicon
- **Priority:** P3 - LOW
- **Effort:** 5 minutes (if already ARM64)
- **Test:** Run on M1/M2 Mac

#### 97. **COMPAT: No bitfield compatibility**
- **File:** Some structures
- **Problem:** Bitfields used without size guarantees
- **Impact:** Potential alignment issues
- **Fix:** Use explicit byte fields instead
- **Priority:** P3 - LOW
- **Effort:** 15 minutes
- **Test:** Verify structure sizes consistent

---

### Future Extensions (3)

#### 98. **EXTEND: No hot shader reload support**
- **File:** `mtl_shader_compiler.m`
- **Problem:** Shaders can't be reloaded at runtime
- **Impact:** Can't iterate on shaders
- **Fix:** Add file watching and reload API
- **Priority:** P3 - LOW
- **Effort:** 30 minutes
- **Test:** Edit shader file, verify reload

#### 99. **EXTEND: No GPU debugging hooks**
- **File:** Throughout
- **Problem:** Can't attach GPU debuggers
- **Impact:** Hard to debug GPU issues
- **Fix:** Add Xcode GPU frame capture support
- **Priority:** P3 - LOW
- **Effort:** 10 minutes
- **Test:** Capture GPU frame in Xcode

#### 100. **EXTEND: No performance monitoring**
- **File:** Throughout
- **Problem:** Can't monitor GPU performance
- **Impact:** Hard to optimize
- **Fix:** Add GPU performance counter API
- **Priority:** P3 - LOW
- **Effort:** 30 minutes
- **Test:** Read performance counters

---

## IMPLEMENTATION STRATEGY

### Phase 1: Critical Crash Fixes (2-3 days)
**Issues:** 1-11 (11 items)
- Fix void function return issue (#1)
- Add null checks (#2, 7)
- Fix fence management (#3, 11)
- Fix listener lifetime (#4)
- Fix realloc (#5)
- Fix uninitialized formats (#6)
- Fix command buffer state (#8)
- Fix shader cache collision (#9)
- Fix device array leak (#10)

**Test:** Basic rendering without crashes

### Phase 2: Freeze/Hang Fixes (2-3 days)
**Issues:** 12-25 (14 items)
- Add timeouts (#12-14, 16, 22)
- Add polling sleep (#15)
- Fix drawable acquisition (#17)
- Add completion callbacks (#18)
- Release locks earlier (#19)
- Add resource creation timeout (#20)
- Signal listeners (#21)
- Initialize render pass (#23)
- Add memory barriers (#24)
- Check GPU status (#25)

**Test:** Rendering without hangs or CPU spinning

### Phase 3: Memory Leak Fixes (2-3 days)
**Issues:** 26-37 (12 items)
- Fix pool saturation (#26)
- Clean up resource cleanup (#27-37)

**Test:** Memory stable over 1000+ frames

### Phase 4: Synchronization & Error Handling (3-4 days)
**Issues:** 38-60 (23 items)
- Add mutexes for shared data (#38-49)
- Add error propagation (#50-60)

**Test:** Parallel rendering without corruption

### Phase 5: Performance & Quality (2-3 days)
**Issues:** 61-100 (40 items)
- Performance optimizations (#61-68)
- Configuration options (#69-74)
- Diagnostics (#75-79)
- Documentation (#80-85)
- Code quality (#86-100)

**Test:** Full test suite passing

---

## TESTING CHECKLIST

After each fix:
- [ ] Compiles without warnings
- [ ] No AddressSanitizer errors
- [ ] No ThreadSanitizer errors
- [ ] Metal API validation passes
- [ ] Runs 1000+ frames without crash
- [ ] CPU usage < 50% idle
- [ ] Memory stable (no growth)
- [ ] Frame rate consistent (no hangs)

---

## SUCCESS CRITERIA

✅ Phase 1 complete when:
- All 100 issues fixed
- 1000+ frame test passes
- No memory leaks detected
- No race conditions
- CPU <50%, Memory stable
- Frame time < 16.7ms (60 FPS)
- All error codes propagate correctly
- Full test coverage

---

**Status:** Ready for Fixes
**Priority:** CRITICAL - Address before Phase 2 begins
**Estimated Time:** 12-16 days with focused team
