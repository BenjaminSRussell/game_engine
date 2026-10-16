/* Unit test for src/engine/backend/metal/mtl_command_graph.c (issue #287).
 *
 * On Apple the root CMakeLists compiles the backend/metal C sources as
 * Objective-C with -fobjc-arc, so the shipped build takes the __OBJC__ branch of
 * metal_command_dependency_wait(), which calls waitUntilCompleted on each
 * unsatisfied dependency. This test is compiled the same way. The command
 * buffers are Objective-C fakes that record waitUntilCompleted, so no GPU or
 * Metal device is needed. Engine does not build on main, so the unit is
 * compiled directly rather than linked.
 */

#import <Foundation/Foundation.h>

#include "backend/metal/mtl_command_graph.h"

#include <stdio.h>

static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* Stands in for id<MTLCommandBuffer>. */
@interface FakeCommandBuffer : NSObject
@property(nonatomic) int waitCalls;
@end

@implementation FakeCommandBuffer
- (void)waitUntilCompleted {
  self.waitCalls++;
}
@end

/* The tracker stores raw handles and does not retain them. objc_precise_lifetime
 * keeps each fake alive to the end of its scope, so ARC cannot release a buffer
 * that a later wait still dereferences. */
#define LIVE_FAKE(name)                                                        \
  FakeCommandBuffer *name __attribute__((objc_precise_lifetime)) = [FakeCommandBuffer new]

#define HANDLE(obj) ((__bridge mtl_command_buffer_t)(obj))

/* An empty tracker never blocks, and NULL arguments count as "no dependencies". */
static void test_new_tracker_never_blocks(void) {
  LIVE_FAKE(a);
  mtl_command_dependency_t tracker = metal_command_dependency_create(4);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(a)));
  CHECK(metal_command_dependency_can_submit(NULL, HANDLE(a)));
  CHECK(metal_command_dependency_can_submit(tracker, NULL));
  metal_command_dependency_destroy(tracker);
  metal_command_dependency_destroy(NULL);
}

static void test_add_rejects_null_arguments(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  mtl_command_dependency_t tracker = metal_command_dependency_create(2);
  CHECK(tracker != NULL);
  CHECK(!metal_command_dependency_add(NULL, HANDLE(b), HANDLE(a),
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_add(tracker, NULL, HANDLE(a),
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_add(tracker, HANDLE(b), NULL,
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  /* The rejected calls recorded nothing, so b is still unblocked. */
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));
  metal_command_dependency_destroy(tracker);
}

static void test_add_fails_when_full(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(c);
  LIVE_FAKE(d);
  mtl_command_dependency_t tracker = metal_command_dependency_create(2);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_MEMORY));
  CHECK(!metal_command_dependency_add(tracker, HANDLE(d), HANDLE(b),
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  /* The rejected edge was not recorded, so d is not blocked by it. */
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(d)));
  metal_command_dependency_destroy(tracker);
}

static void test_submit_blocked_until_dependency_completes(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  mtl_command_dependency_t tracker = metal_command_dependency_create(4);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(b)));
  /* A dependency gates its dependent, never the dependency itself. */
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(a)));

  /* Completing the dependent does not satisfy its own dependency. */
  metal_command_dependency_mark_completed(tracker, HANDLE(b));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(b)));

  metal_command_dependency_mark_completed(tracker, HANDLE(a));
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));

  /* Completing the same dependency again changes nothing. */
  metal_command_dependency_mark_completed(tracker, HANDLE(a));
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));
  metal_command_dependency_destroy(tracker);
}

static void test_submit_waits_for_every_dependency(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(c);
  LIVE_FAKE(d);
  LIVE_FAKE(x);
  mtl_command_dependency_t tracker = metal_command_dependency_create(4);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(b),
                                     MTL_DEPENDENCY_TYPE_MEMORY));
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(d),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(c)));

  metal_command_dependency_mark_completed(tracker, HANDLE(a));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(c)));
  metal_command_dependency_mark_completed(tracker, HANDLE(d));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(c)));
  metal_command_dependency_mark_completed(tracker, HANDLE(b));
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(c)));

  /* A buffer with no edges was never blocked. */
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(x)));
  metal_command_dependency_destroy(tracker);
}

static void test_mark_completed_ignores_null_and_unknown(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(unknown);
  mtl_command_dependency_t tracker = metal_command_dependency_create(4);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  metal_command_dependency_mark_completed(NULL, HANDLE(a));
  metal_command_dependency_mark_completed(tracker, NULL);
  metal_command_dependency_mark_completed(tracker, HANDLE(unknown));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(b)));
  metal_command_dependency_destroy(tracker);
}

static void test_reset_clears_edges_and_capacity(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(c);
  LIVE_FAKE(d);
  mtl_command_dependency_t tracker = metal_command_dependency_create(2);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_add(tracker, HANDLE(d), HANDLE(a),
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  metal_command_dependency_mark_completed(tracker, HANDLE(a));

  metal_command_dependency_reset(tracker);
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));
  /* Reset dropped the edges, so waiting on b must not touch a. */
  metal_command_dependency_wait(tracker, HANDLE(b));
  CHECK(a.waitCalls == 0);

  /* Capacity is back. A new edge starts unsatisfied even though a completed
   * before the reset. */
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(metal_command_dependency_add(tracker, HANDLE(c), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_add(tracker, HANDLE(d), HANDLE(a),
                                      MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(b)));

  metal_command_dependency_reset(NULL);
  metal_command_dependency_destroy(tracker);
}

/* Objective-C branch of metal_command_dependency_wait(): it waits on the
 * unsatisfied dependencies of the given dependent only, and marks each one
 * satisfied. */
static void test_wait_waits_only_on_unsatisfied_edges_of_dependent(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(c);
  LIVE_FAKE(d);
  mtl_command_dependency_t tracker = metal_command_dependency_create(4);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(c),
                                     MTL_DEPENDENCY_TYPE_MEMORY));
  CHECK(metal_command_dependency_add(tracker, HANDLE(d), HANDLE(c),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));

  metal_command_dependency_wait(tracker, HANDLE(b));
  CHECK(a.waitCalls == 1);
  /* Only b's edge on c is waited on. d's edge on c is not. */
  CHECK(c.waitCalls == 1);
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));
  /* Waiting on b does not satisfy d's edge. */
  CHECK(!metal_command_dependency_can_submit(tracker, HANDLE(d)));

  metal_command_dependency_wait(tracker, HANDLE(d));
  CHECK(c.waitCalls == 2);
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(d)));

  /* Satisfied edges are not waited on again. */
  metal_command_dependency_wait(tracker, HANDLE(b));
  CHECK(a.waitCalls == 1);
  CHECK(c.waitCalls == 2);
  metal_command_dependency_destroy(tracker);
}

static void test_wait_skips_satisfied_and_null_arguments(void) {
  LIVE_FAKE(a);
  LIVE_FAKE(b);
  LIVE_FAKE(x);
  mtl_command_dependency_t tracker = metal_command_dependency_create(2);
  CHECK(tracker != NULL);
  CHECK(metal_command_dependency_add(tracker, HANDLE(b), HANDLE(a),
                                     MTL_DEPENDENCY_TYPE_EXECUTION));
  metal_command_dependency_mark_completed(tracker, HANDLE(a));

  /* The edge is already satisfied, so nothing is waited on. */
  metal_command_dependency_wait(tracker, HANDLE(b));
  CHECK(a.waitCalls == 0);
  CHECK(metal_command_dependency_can_submit(tracker, HANDLE(b)));

  /* No edges, and NULL arguments, must return without waiting or crashing. */
  metal_command_dependency_wait(tracker, HANDLE(x));
  metal_command_dependency_wait(NULL, HANDLE(b));
  metal_command_dependency_wait(tracker, NULL);
  CHECK(a.waitCalls == 0);
  CHECK(b.waitCalls == 0);
  metal_command_dependency_destroy(tracker);
}

int main(void) {
  test_new_tracker_never_blocks();
  test_add_rejects_null_arguments();
  test_add_fails_when_full();
  test_submit_blocked_until_dependency_completes();
  test_submit_waits_for_every_dependency();
  test_mark_completed_ignores_null_and_unknown();
  test_reset_clears_edges_and_capacity();
  test_wait_waits_only_on_unsatisfied_edges_of_dependent();
  test_wait_skips_satisfied_and_null_arguments();

  if (g_failed != 0) {
    fprintf(stderr, "test_mtl_command_graph: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("test_mtl_command_graph: all checks passed\n");
  return 0;
}
