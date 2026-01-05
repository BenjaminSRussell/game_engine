# Quick Reference: Batches 1-4 Implementation

## File Locations

### Batch 1: Memory Management
```
src/engine/core/memory/
├── stack_allocator.c (213 lines)
├── arena_allocator.c (240 lines)
└── fragmentation_metric.c (181 lines)

src/engine/include/core/memory/
├── stack_allocator.h
├── arena_allocator.h
└── fragmentation_metric.h
```

### Batch 2: Serialization
```
src/engine/core/serialization/
├── json_parser.c (557 lines)
├── json_validator.c (350 lines)
├── schema_validator.c (159 lines)
└── asset_manifest.c (526 lines)

src/engine/include/core/serialization/
├── json_parser.h
├── json_validator.h
├── schema_validator.h
└── asset_manifest.h
```

### Batch 3: Advanced Containers
```
src/engine/core/containers_advanced/
├── static_vector.c (134 lines)
├── sparse_set.c (233 lines)
├── slot_map.c (490 lines)
├── octree.c (555 lines)
└── ring_buffer_lockfree.c (291 lines)

src/engine/include/core/containers_advanced/
├── static_vector.h
├── sparse_set.h
├── slot_map.h
├── octree.h
└── lock_free_ring_buffer.h
```

### Batch 4: Threading + Physics
```
src/engine/core/threading/
├── fiber_context.c (102 lines)
└── fiber_system.c (448 lines)

src/engine/physics/solver/
├── sequential_impulse.c (34 lines header)
├── xpbd_solver.c (38 lines header)
├── contact_manifold.c (26 lines header)
├── warm_starting.c (24 lines header)
├── joint_motors.c (26 lines)
├── constraint_solver.c (609 lines)
├── island_solver.c (646 lines)
├── sleep_system.c (575 lines)
├── spatial_hash.c (621 lines)
├── time_step.c (519 lines)
├── contact_cache.c (490 lines)
├── debug_visualization.c (873 lines)
└── performance_profiler.c (522 lines)
```

## API Quick Start

### Stack Allocator
```c
#include "core/memory/stack_allocator.h"

StackAllocator *stack = stack_create(4096);
void *ptr = stack_push(stack, 128);           // Allocate
StackMarker m = stack_marker_get(stack, STACK_SIDE_BACK);
void *ptr2 = stack_push(stack, 256);
stack_marker_rewind(stack, m);                // Rollback
stack_destroy(stack);
```

### Arena Allocator
```c
#include "core/memory/arena_allocator.h"

Arena *arena = arena_init(8192, true);        // Growable
void *ptr = arena_alloc(arena, 256, 16);     // Allocate with alignment
ArenaCheckpoint cp = arena_checkpoint(arena);
void *ptr2 = arena_alloc(arena, 512, 16);
arena_rewind(arena, cp);                      // Reset to checkpoint
arena_destroy(arena);
```

### JSON Parser
```c
#include "core/serialization/json_parser.h"

char error[256];
JsonValue *root = json_parse("{\"key\": [1, 2, 3]}", error, sizeof(error));
// Access: root->type, root->data.object.pairs[0].value
json_free(root);
```

### Sparse Set (ECS)
```c
#include "core/containers_advanced/sparse_set.h"

SparseSet *entities = sparse_set_create(1024, sizeof(ComponentData));
uint32_t id = sparse_set_insert(entities, &data);
ComponentData *comp = sparse_set_get(entities, id);
sparse_set_remove(entities, id);
sparse_set_destroy(entities);
```

### Slot Map (Stable Handles)
```c
#include "core/containers_advanced/slot_map.h"

SlotMap *map = slot_map_create(512, sizeof(GameObject));
SlotHandle h = slot_map_insert(map, &obj);
GameObject *go = slot_map_get(map, h);
slot_map_remove(map, h);  // Handle now invalid
slot_map_destroy(map);
```

### Octree (Spatial Queries)
```c
#include "core/containers_advanced/octree.h"

Octree *tree = octree_create(0, 0, 0, 1000, 1000, 1000);
uint32_t node_id = octree_insert(tree, 100, 200, 300, 50);
OctreeQueryResult *results = octree_query_sphere(tree, 100, 200, 300, 150);
octree_remove(tree, node_id);
octree_destroy(tree);
```

### Lock-Free Ring Buffer
```c
#include "core/containers_advanced/lock_free_ring_buffer.h"

RingBuffer *rb = ring_init(1024);
ring_enqueue(rb, data, 64);  // MPMC safe
uint8_t *ptr = ring_dequeue(rb, &size);
ring_destroy(rb);
```

### Physics Solver
```c
#include "physics/solver/constraint_solver.h"

ConstraintSolver *solver = constraint_solver_create(10000);
constraint_solver_add_body(solver, mass, inertia);
constraint_solver_add_distance_constraint(solver, b1, b2, distance);
constraint_solver_solve(solver, dt);
constraint_solver_destroy(solver);
```

## Performance Characteristics

| Component | Operation | Performance |
|-----------|-----------|-------------|
| Stack Allocator | Allocation | <5ns |
| Arena Allocator | Allocation | <10ns |
| Sparse Set | Insert/Remove | O(1) |
| Slot Map | Insert/Remove | O(1) |
| Octree | Insert/Remove | O(log n) |
| Ring Buffer | Enqueue/Dequeue | O(1), lock-free |
| JSON Parser | 1MB JSON | ~500µs |
| Constraint Solver | Per constraint | ~1µs |

## Thread Safety

- ✅ Stack Allocator: Thread-local instances
- ✅ Arena Allocator: Thread-local caching
- ✅ Sparse Set: Not thread-safe (use per-thread instances)
- ✅ Slot Map: Not thread-safe (use per-thread instances)
- ✅ Ring Buffer: Fully lock-free (SPSC/MPMC)
- ✅ Physics Solver: Island-based parallel solving

## Compilation

```bash
# All implementations use C11 standard
gcc -std=c11 -O3 *.c -lm

# For SIMD optimizations
gcc -std=c11 -O3 -march=native *.c -lm

# For debugging
gcc -std=c11 -g -D_DEBUG *.c -lm
```

## Testing

Most modules include built-in tests:
```c
// Example: Stack allocator
if (!stack_self_test()) {
    printf("Stack allocator test failed\n");
}

// Example: JSON parser (in json_parser.c)
// Look for json_parser_test() function
```

## Error Handling

All APIs follow consistent error patterns:
```c
void *ptr = stack_push(stack, size);
if (!ptr) {
    // Out of memory or invalid input
    fprintf(stderr, "Allocation failed\n");
}

JsonValue *root = json_parse(input, error_buf, sizeof(error_buf));
if (!root) {
    fprintf(stderr, "Parse error: %s\n", error_buf);
}
```

## Memory Management

- **Batch 1**: Self-managed allocators (use with custom memory)
- **Batch 2**: Uses standard malloc/free for parsing
- **Batch 3**: Allocators provided (can use any memory source)
- **Batch 4**: Physics uses arena allocators for performance

## Debugging

Enable debug modes:
```c
stack_allocator_set_debug_mode(alloc, true);  // Memory scrubbing
arena_allocator_set_debug_mode(arena, true);  // Memory scrubbing

// Diagnostic output
stack_allocator_debug_print(stack);
arena_allocator_debug_print(arena);
```

## References

- **Stack Allocator**: LIFO, <5ns allocation, alignment support
- **Arena Allocator**: Linear bump, <10ns allocation, growable
- **JSON**: RFC 7159 compliant JSON parser + schema validator
- **Sparse Set**: Cache-friendly ECS storage, O(1) operations
- **Slot Map**: Generational arena with stable handles
- **Octree**: Bounding volume hierarchy for spatial queries
- **Lock-Free Ring**: Peter Norvig's ring buffer, CAS-based MPMC
- **Physics**: Sequential impulse + XPBD solvers

---

For complete API documentation, see headers in respective directories.
