#pragma once

#include "include/core/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct FiberContext FiberContext;
typedef struct FiberScheduler FiberScheduler;

// Architecture-specific types (x64 and ARM64 support)
#if defined(__x86_64__) || defined(_M_X64)
    #define FIBER_ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define FIBER_ARCH_ARM64 1
#else
    #error "Unsupported architecture for fiber context"
#endif

// Register state for context switching
#ifdef FIBER_ARCH_X64
    typedef struct {
        u64 rax, rbx, rcx, rdx;
        u64 rsi, rdi, rbp, rsp;
        u64 r8, r9, r10, r11;
        u64 r12, r13, r14, r15;
        u64 rip;
        u64 xmm[16];  // SSE/AVX registers
    } RegisterState;
#elif FIBER_ARCH_ARM64
    typedef struct {
        u64 x[31];
        u64 sp;
        u64 pc;
        u64 q[32];    // NEON registers
    } RegisterState;
#endif

// Fiber function type
typedef void (*FiberFn)(void *arg);

// Fiber states
typedef enum {
    FIBER_STATE_CREATED,
    FIBER_STATE_READY,
    FIBER_STATE_RUNNING,
    FIBER_STATE_SUSPENDED,
    FIBER_STATE_WAITING,
    FIBER_STATE_COMPLETED,
    FIBER_STATE_FAILED
} FiberState;

// Context configuration
typedef struct {
    u32 stack_size;
    bool enable_valgrind;
    bool enable_exception_handling;
    bool enable_sanitizers;
    u32 stack_alignment;
} FiberContextConfig;

// Synchronization primitive for fiber coordination
typedef struct {
    volatile bool signaled;
    u32 waiting_fiber_count;
} FiberEvent;

// Fiber mutex for synchronization
typedef struct {
    volatile u32 owner_id;
    u32 lock_count;
} FiberMutex;

/**
 * FiberContext: Lightweight user-space threading with assembly-level context switching
 *
 * Properties:
 *   - x64 and ARM64 architecture support
 *   - Assembly-optimized context switching
 *   - Stack sanity checking
 *   - Valgrind integration for leak detection
 *   - Exception handling support
 *   - Cache-friendly fiber scheduling
 *
 * Use cases:
 *   - Lightweight concurrency without OS threads
 *   - Real-time systems
 *   - Game engines (fibers for game logic)
 *   - Server systems (thousands of concurrent tasks)
 */

// ============================================================================
// Fiber Lifecycle
// ============================================================================

/**
 * Create a new fiber context
 *
 * Args:
 *   stack_size: Stack size in bytes (must be page-aligned)
 *   fn: Function to execute when fiber runs
 *   arg: Argument passed to function
 *   config: Configuration options
 *
 * Returns:
 *   New FiberContext (must be freed with fiber_context_free)
 */
FiberContext *fiber_context_create(u32 stack_size, FiberFn fn, void *arg, FiberContextConfig config);

/**
 * Create fiber with default configuration
 *
 * Args:
 *   stack_size: Stack size in bytes
 *   fn: Function to execute
 *   arg: Argument
 *
 * Returns:
 *   New FiberContext
 */
FiberContext *fiber_context_create_default(u32 stack_size, FiberFn fn, void *arg);

/**
 * Free a fiber context
 *
 * Args:
 *   ctx: FiberContext to free (NULL-safe)
 */
void fiber_context_free(FiberContext *ctx);

/**
 * Get current running fiber
 *
 * Returns:
 *   Pointer to current FiberContext or NULL if none
 */
FiberContext *fiber_context_current(void);

/**
 * Get fiber state
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Current FiberState
 */
FiberState fiber_context_state(FiberContext *ctx);

/**
 * Get fiber ID
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Unique fiber ID
 */
u32 fiber_context_id(FiberContext *ctx);

// ============================================================================
// Context Switching (Assembly Level)
// ============================================================================

/**
 * Save current context (before switching)
 *
 * Args:
 *   ctx: FiberContext to save state to
 *
 * Returns:
 *   0 on initial call, non-zero after restore
 *
 * Note: Implemented in x64 or ARM64 assembly
 */
u32 fiber_context_save(FiberContext *ctx);

/**
 * Restore context (resume fiber execution)
 *
 * Args:
 *   ctx: FiberContext to restore from
 *
 * Note: Never returns - transfers control to fiber
 * Implemented in assembly for direct register manipulation
 */
void fiber_context_restore(FiberContext *ctx) __attribute__((noreturn));

/**
 * Switch from current fiber to target fiber
 *
 * Args:
 *   from: Current fiber
 *   to: Target fiber to switch to
 *
 * Returns:
 *   0 if switch successful
 *
 * Note: Handles all context saving/restoring
 */
u32 fiber_context_switch(FiberContext *from, FiberContext *to);

/**
 * Get register state of fiber
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Pointer to RegisterState structure
 */
RegisterState *fiber_context_get_registers(FiberContext *ctx);

/**
 * Set register state of fiber
 *
 * Args:
 *   ctx: FiberContext
 *   state: RegisterState to set
 */
void fiber_context_set_registers(FiberContext *ctx, RegisterState *state);

// ============================================================================
// Stack Management
// ============================================================================

/**
 * Get stack base address
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Pointer to stack base
 */
void *fiber_context_stack_base(FiberContext *ctx);

/**
 * Get stack size
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Stack size in bytes
 */
u32 fiber_context_stack_size(FiberContext *ctx);

/**
 * Get current stack pointer
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Current stack pointer value
 */
void *fiber_context_stack_pointer(FiberContext *ctx);

/**
 * Check stack integrity (guard pages, bounds)
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   true if stack is valid
 */
bool fiber_context_validate_stack(FiberContext *ctx);

/**
 * Check for stack overflow
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   true if stack pointer approaching limit
 */
bool fiber_context_check_stack_overflow(FiberContext *ctx);

/**
 * Get available stack space
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Bytes remaining on stack
 */
u32 fiber_context_available_stack(FiberContext *ctx);

// ============================================================================
// Sanity Checks
// ============================================================================

/**
 * Validate fiber context integrity
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   true if context is valid
 */
bool fiber_context_validate(FiberContext *ctx);

/**
 * Check if context can be safely switched to
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   true if context is in valid state
 */
bool fiber_context_can_switch(FiberContext *ctx);

/**
 * Detect stack corruption
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   true if corruption detected
 */
bool fiber_context_detect_corruption(FiberContext *ctx);

// ============================================================================
// Valgrind Integration
// ============================================================================

/**
 * Register fiber stack with Valgrind
 *
 * Args:
 *   ctx: FiberContext
 *
 * Note: Enables accurate memory debugging in Valgrind
 */
void fiber_context_valgrind_register(FiberContext *ctx);

/**
 * Unregister fiber stack with Valgrind
 *
 * Args:
 *   ctx: FiberContext
 */
void fiber_context_valgrind_unregister(FiberContext *ctx);

/**
 * Notify Valgrind of stack switch
 *
 * Args:
 *   old_ctx: Fiber being suspended
 *   new_ctx: Fiber being resumed
 */
void fiber_context_valgrind_notify_switch(FiberContext *old_ctx, FiberContext *new_ctx);

// ============================================================================
// Exception Handling
// ============================================================================

/**
 * Enable exception handling for fiber
 *
 * Args:
 *   ctx: FiberContext
 *   enable: Whether to enable handling
 */
void fiber_context_set_exception_handler(FiberContext *ctx, bool enable);

/**
 * Set custom exception handler
 *
 * Args:
 *   ctx: FiberContext
 *   handler: Exception handler function
 *
 * Signature: int handler(FiberContext *ctx, void *exception_info)
 */
typedef int (*FiberExceptionHandler)(FiberContext *ctx, void *exception_info);
void fiber_context_set_custom_handler(FiberContext *ctx, FiberExceptionHandler handler);

/**
 * Catch exception in fiber execution
 *
 * Args:
 *   ctx: FiberContext
 *   exception_info: Exception information (platform-specific)
 *
 * Returns:
 *   true if exception was handled
 */
bool fiber_context_catch_exception(FiberContext *ctx, void *exception_info);

/**
 * Get last exception
 *
 * Args:
 *   ctx: FiberContext
 *
 * Returns:
 *   Last exception info or NULL
 */
void *fiber_context_get_last_exception(FiberContext *ctx);

// ============================================================================
// Synchronization Primitives
// ============================================================================

/**
 * Create fiber event (manual reset)
 *
 * Args:
 *   initial_state: true if initially signaled
 *
 * Returns:
 *   New FiberEvent
 */
FiberEvent *fiber_event_create(bool initial_state);

/**
 * Signal event
 *
 * Args:
 *   event: FiberEvent
 */
void fiber_event_signal(FiberEvent *event);

/**
 * Reset event
 *
 * Args:
 *   event: FiberEvent
 */
void fiber_event_reset(FiberEvent *event);

/**
 * Wait for event (suspends fiber)
 *
 * Args:
 *   event: FiberEvent
 *   timeout_ms: Timeout in milliseconds (0 = infinite)
 *
 * Returns:
 *   true if event was signaled
 */
bool fiber_event_wait(FiberEvent *event, u32 timeout_ms);

/**
 * Create fiber mutex
 *
 * Returns:
 *   New FiberMutex
 */
FiberMutex *fiber_mutex_create(void);

/**
 * Acquire mutex (may suspend fiber)
 *
 * Args:
 *   mutex: FiberMutex
 *
 * Returns:
 *   true if acquired
 */
bool fiber_mutex_lock(FiberMutex *mutex);

/**
 * Try to acquire mutex (non-blocking)
 *
 * Args:
 *   mutex: FiberMutex
 *
 * Returns:
 *   true if acquired, false if locked
 */
bool fiber_mutex_try_lock(FiberMutex *mutex);

/**
 * Release mutex
 *
 * Args:
 *   mutex: FiberMutex
 */
void fiber_mutex_unlock(FiberMutex *mutex);

// ============================================================================
// Fiber Scheduler
// ============================================================================

/**
 * Create fiber scheduler
 *
 * Args:
 *   num_worker_threads: Number of OS threads for scheduling
 *
 * Returns:
 *   New FiberScheduler
 */
FiberScheduler *fiber_scheduler_create(u32 num_worker_threads);

/**
 * Destroy scheduler
 *
 * Args:
 *   scheduler: FiberScheduler to destroy
 */
void fiber_scheduler_destroy(FiberScheduler *scheduler);

/**
 * Schedule fiber for execution
 *
 * Args:
 *   scheduler: FiberScheduler
 *   ctx: FiberContext to schedule
 */
void fiber_scheduler_enqueue(FiberScheduler *scheduler, FiberContext *ctx);

/**
 * Run scheduler until all fibers complete
 *
 * Args:
 *   scheduler: FiberScheduler
 */
void fiber_scheduler_run(FiberScheduler *scheduler);

/**
 * Stop scheduler
 *
 * Args:
 *   scheduler: FiberScheduler
 */
void fiber_scheduler_stop(FiberScheduler *scheduler);

// ============================================================================
// Benchmarking
// ============================================================================

/**
 * Benchmark context switch latency
 *
 * Args:
 *   iterations: Number of switches to measure
 *
 * Returns:
 *   Average nanoseconds per context switch
 */
u64 fiber_context_benchmark_switch(u32 iterations);

/**
 * Benchmark fiber creation
 *
 * Args:
 *   iterations: Number of fibers to create
 *
 * Returns:
 *   Average nanoseconds per creation
 */
u64 fiber_context_benchmark_create(u32 iterations);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 fiber_context_run_tests(void);

/**
 * Stress test fiber system
 *
 * Args:
 *   num_fibers: Number of fibers to create
 *   duration_ms: Test duration
 *
 * Returns:
 *   true if stress test passed
 */
bool fiber_context_stress_test(u32 num_fibers, u32 duration_ms);

#ifdef __cplusplus
}
#endif

