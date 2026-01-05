# Phase 1 & 2 Implementation Guide

## Overview

This guide helps you systematically implement the 269 filtered TODOs from Phase 1 (Core Infrastructure) and Phase 2 (Physics Engine).

**Total Scope:** ~750 developer-hours
**Recommended Team:** 2-3 developers
**Timeline:** 3-6 months (full-time)

---

## Part 1: Quick Win - Start with Stack Allocator (11 TODOs, ~30 hours)

### Why First?
- Most memory allocations are temporary (per-frame, per-task)
- Stack allocator is the simplest and most efficient
- Enables all other systems to allocate memory cheaply
- Low risk, high impact

### File: `src/engine/core/memory/stack_allocator.c`

### Implementation Steps

#### Step 1: Basic Structure (2 hours)
Create the header in the .c file:

```c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    void* buffer;
    size_t capacity;
    size_t offset;
    size_t peak;  // Track peak usage
    const char* name;
} StackAllocator;

// TODO(AGENT_CORE_1): Implement stack_create() [Difficulty: 3]
StackAllocator* stack_create(size_t capacity, const char* name);

// TODO(AGENT_CORE_1): Implement stack_push() [Difficulty: 3]
void* stack_push(StackAllocator* stack, size_t size);

// TODO(AGENT_CORE_1): Implement stack_pop() [Difficulty: 3]
void stack_pop(StackAllocator* stack, size_t size);

// TODO(AGENT_CORE_1): Implement stack_marker_get() [Difficulty: 2]
size_t stack_marker_get(StackAllocator* stack);

// TODO(AGENT_CORE_1): Implement stack_marker_rewind() [Difficulty: 2]
void stack_marker_rewind(StackAllocator* stack, size_t marker);

// (other functions...)
```

#### Step 2: Core Functions (4 hours)
Implement the most basic operations:

```c
StackAllocator* stack_create(size_t capacity, const char* name) {
    // TODO: Allocate allocator struct
    // TODO: Allocate capacity bytes from main allocator
    // TODO: Initialize fields
    // TODO: Return pointer
}

void* stack_push(StackAllocator* stack, size_t size) {
    // TODO: Check if offset + size > capacity
    // TODO: Return NULL on overflow
    // TODO: Save return pointer
    // TODO: Increment offset
    // TODO: Update peak if needed
    // TODO: Return saved pointer
}

void stack_pop(StackAllocator* stack, size_t size) {
    // TODO: Check if size > offset
    // TODO: Decrement offset by size
}

size_t stack_marker_get(StackAllocator* stack) {
    return stack->offset;
}

void stack_marker_rewind(StackAllocator* stack, size_t marker) {
    // TODO: Validate marker <= offset
    // TODO: Set offset = marker
}
```

#### Step 3: Advanced Features (4 hours)
```c
// TODO: Add boundary checks (canaries for overflow detection)
// TODO: Implement aligned allocations (useful for SIMD)
// TODO: Add debug memory viewing (dump allocation list)
// TODO: Implement thread-local instances (per-thread allocation)
```

#### Step 4: Testing (3 hours)
Create test file: `src/engine/test/unit/core/test_stack_allocator.c`

```c
#include "test_framework.h"
#include "memory/stack_allocator.h"

void test_stack_basic_push_pop() {
    // TODO: Create allocator
    // TODO: Push 100 bytes
    // TODO: Verify offset is correct
    // TODO: Pop 100 bytes
    // TODO: Verify offset reset
}

void test_stack_alignment() {
    // TODO: Test 4-byte alignment
    // TODO: Test 16-byte alignment (SIMD)
    // TODO: Test 64-byte alignment (cache line)
}

void test_stack_overflow() {
    // TODO: Try to allocate beyond capacity
    // TODO: Verify returns NULL
}

void test_stack_markers() {
    // TODO: Create allocator
    // TODO: Save marker 1
    // TODO: Allocate more
    // TODO: Rewind to marker 1
    // TODO: Verify state
}
```

#### Step 5: Benchmarking (2 hours)
Create: `src/engine/test/bench/bench_stack_allocator.c`

```c
// Compare with malloc:
// - malloc: ~200ns per allocation
// - stack: <10ns per allocation
// - stack (aligned): <50ns per allocation
```

### Acceptance Criteria
- [x] All 5 core functions implemented
- [x] All boundary checks in place
- [x] Unit tests pass (10+ test cases)
- [x] Performance: <20ns per allocation
- [x] Peak tracking accurate
- [x] Thread-local version working

---

## Part 2: Arena Allocator (8 TODOs, ~25 hours)

### Why After Stack?
- Uses same allocation model (pointer bump)
- But with reset/checkpoint/rewind
- Good for per-frame allocations
- Slightly more complex than stack

### Key Difference from Stack
- Stack: LIFO (Last In First Out)
- Arena: Can reset entire allocation at once
- No need to track exact pop sizes

### Implementation Checklist
- [ ] `arena_init()` - Difficulty: 3
- [ ] `arena_alloc()` - Difficulty: 2
- [ ] `arena_reset()` - Difficulty: 2
- [ ] `arena_checkpoint()` - Difficulty: 3
- [ ] `arena_rewind()` - Difficulty: 3
- [ ] Thread-local cache - Difficulty: 5
- [ ] Overflow protection - Difficulty: 4
- [ ] Benchmark vs malloc - Difficulty: 3

---

## Part 3: Sparse Set Container (13 TODOs, ~60 hours)

### Why Third in Phase 1?
- Core data structure for ECS systems
- Enables efficient component storage
- Will be heavily used by physics

### Difficulty Progression
1. Basic structure (create, insert, remove, has, get, clear) - Difficulty: 2-3 each
2. Iteration support - Difficulty: 3
3. Sorting - Difficulty: 5
4. SIMD iteration - Difficulty: 6
5. Parallel iteration - Difficulty: 6
6. Archetype chunking - Difficulty: 8
7. Versioning for stability - Difficulty: 7
8. Unit tests - Difficulty: 4

### Implementation Strategy
Start with minimal version:
- 3 arrays: indices, values, reverse indices
- Simple iteration
- Then add complexity incrementally

---

## Part 4: Physics Core - Sequential Impulse Solver (11 TODOs, ~70 hours)

### Why Jump to Physics?
- Once memory + containers work, physics can develop in parallel
- Most complex but most important system
- Critical path item

### Key Components
1. Contact detection (existing from narrowphase)
2. Constraint formulation (7-8 hours)
3. Velocity solver (8-10 hours)
4. Position solver (8-10 hours)
5. Warm starting (6-8 hours)
6. Sleeping/deactivation (5-6 hours)
7. SIMD optimization (10-15 hours)
8. Testing (10-15 hours)

### Milestone: Get One Stack of Boxes Stable
- Simple test: stack 10 boxes
- Target: stable for 10 seconds
- Success criterion: no excessive bouncing or sliding

---

## Part 5: Parallelization Strategy

Once foundations are solid, parallelize:

**Team Member 1: Memory + Containers**
- Complete stack allocator
- Complete arena allocator
- Get sparse set working
- Start slot map

**Team Member 2: Physics Core**
- Start sequential impulse solver
- Implement narrowphase collision
- Contact manifold management

**Team Member 3: Advanced Features**
- Start JSON parser/serializer
- Lock-free ring buffer
- Octree implementation

---

## Quality Checklist for Each TODO

Before marking a TODO as complete:

### Code Quality
- [ ] Function has clear documentation
- [ ] Edge cases handled
- [ ] No memory leaks (valgrind/asan)
- [ ] Follows code style

### Testing
- [ ] Unit tests written
- [ ] All test cases pass
- [ ] Edge cases tested
- [ ] Error conditions tested

### Performance
- [ ] Benchmarked against reference
- [ ] Performance target met
- [ ] Memory usage tracked
- [ ] Profile shows no bottlenecks

### Documentation
- [ ] Function signature documented
- [ ] Algorithm explained in comments
- [ ] Usage examples provided
- [ ] Limitations documented

---

## Tools & Testing Setup

### Unit Test Framework
Use existing: `src/engine/test/unit/`

Template:
```c
#include "test_framework.h"

TEST(StackAllocator, BasicPushPop) {
    // Arrange
    StackAllocator* stack = stack_create(1024, "test");

    // Act
    void* ptr = stack_push(stack, 100);
    size_t offset1 = stack->offset;
    stack_pop(stack, 100);
    size_t offset2 = stack->offset;

    // Assert
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(offset1, 100);
    EXPECT_EQ(offset2, 0);

    // Cleanup
    free(stack);
}
```

### Benchmarking
Use: `src/engine/test/bench/`

Template:
```c
BENCHMARK(StackAllocator, AllocSpeed) {
    StackAllocator* stack = stack_create(10 * 1024 * 1024, "bench");

    Timer timer = timer_start();
    for (int i = 0; i < 1000000; i++) {
        void* ptr = stack_push(stack, 64);
        stack_pop(stack, 64);
    }
    double elapsed = timer_elapsed(&timer);

    printf("1M push/pop cycles: %.2f ms (%.1f ns per)\n",
           elapsed, elapsed * 1000.0 / 1000000.0);
}
```

### Memory Checking
```bash
# Run with valgrind
valgrind --leak-check=full ./test_runner

# Or with AddressSanitizer
clang -fsanitize=address test_*.c
```

---

## Dependency Map

```
Stack Allocator
  ├→ Arena Allocator
  ├→ All other allocators
  └→ Container memory management

Memory ✓
  ├→ Sparse Set
  ├→ Slot Map
  ├→ Octree
  └→ Lock-Free Containers

Containers ✓
  ├→ ECS System (not in Phase 1-2)
  ├→ Physics Simulation
  └→ JSON Serialization

Physics Simulation
  ├→ Sequential Impulse Solver
  ├→ XPBD Solver
  ├→ Constraint Solver
  ├→ Broadphase (GPU or SAP)
  └→ Soft Body (depends on constraints)
```

---

## Risk Mitigation

### High Risk Items (Likely to have delays)
- GPU Broadphase (requires Vulkan/Compute knowledge)
- Lock-Free Ring Buffer (concurrency bugs hard to debug)
- FEM Deformable (complex math, numerical stability)
- SIMD optimization (platform-specific assembly)

### Mitigation Strategy
1. **CPU fallback first** - Implement serial version first
2. **Extensive testing** - 10x more tests for risky components
3. **Code review** - Have expert review before integration
4. **Gradual optimization** - Optimize after correctness proven
5. **Benchmark early** - Establish performance baseline immediately

---

## Tracking Progress

Use the JSON file: `PHASE_1_2_PROGRESS.json`

### Update After Each TODO
```json
{
  "id": "P1.1.1.1",
  "text": "Implement stack_create()",
  "status": "in_progress"  // or "completed"
}
```

### Weekly Review
- [ ] Count completed TODOs
- [ ] Estimate remaining work
- [ ] Identify blockers
- [ ] Adjust timeline

---

## Next Steps

1. **This Week:** Complete stack allocator (all 11 TODOs)
2. **Week 2-3:** Arena allocator
3. **Week 4-5:** JSON serialization basics
4. **Week 6-8:** Sparse set container
5. **Week 9+:** Physics core solver

Total: ~3-4 months for full Phase 1 + Phase 2

---

## Success Metrics

### Phase 1 Complete When:
- [ ] Zero memory leaks detected
- [ ] All allocators meet performance targets
- [ ] JSON serialization working
- [ ] All containers tested and benchmarked
- [ ] ECS-ready data structures in place

### Phase 2 Complete When:
- [ ] 10K rigid bodies stable (no jitter)
- [ ] Box stacks 20 high without collapse
- [ ] Performance target: solve 50K constraints/ms
- [ ] Broadphase handles 100K objects
- [ ] All collision shapes working
- [ ] Ragdoll character physics stable

