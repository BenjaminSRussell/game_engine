/* Unit test for src/engine/backend/metal/mtl_parallel_encoder.c (issue #295).
 *
 * On Apple the root CMakeLists compiles the backend/metal C files as
 * Objective-C, so the unit's Metal branch is the one under test. The test
 * passes Objective-C fake objects through the C API. Each fake records the
 * messages the unit sends and returns a sentinel object, so no GPU or Metal
 * device is needed.
 */

#import <Foundation/Foundation.h>

#include "backend/metal/mtl_parallel_encoder.h"

#include <stdio.h>

static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* Stands in for id<MTLParallelRenderCommandEncoder>. */
@interface FakeParallelEncoder : NSObject
@property(nonatomic, strong) id renderEncoder;
@property(nonatomic) int renderCommandCalls;
@property(nonatomic) int endEncodingCalls;
@end

@implementation FakeParallelEncoder
- (id)renderCommandEncoder {
  self.renderCommandCalls++;
  return self.renderEncoder;
}

- (void)endEncoding {
  self.endEncodingCalls++;
}
@end

/* Stands in for id<MTLCommandBuffer>. */
@interface FakeCommandBuffer : NSObject
@property(nonatomic, strong) FakeParallelEncoder *parallelEncoder;
@property(nonatomic, strong) id lastDescriptor;
@property(nonatomic) int createCalls;
@end

@implementation FakeCommandBuffer
- (id)parallelRenderCommandEncoderWithDescriptor:(id)descriptor {
  self.createCalls++;
  self.lastDescriptor = descriptor;
  return self.parallelEncoder;
}
@end

static void test_create_rejects_null_arguments(void) {
  FakeCommandBuffer *buffer = [FakeCommandBuffer new];
  NSObject *descriptor = [NSObject new];
  /* Non-nil encoder, so a missing guard shows up as a non-NULL return. */
  buffer.parallelEncoder = [FakeParallelEncoder new];

  CHECK(metal_parallel_render_command_encoder_create(NULL, (__bridge void *)descriptor) == NULL);
  CHECK(metal_parallel_render_command_encoder_create((__bridge void *)buffer, NULL) == NULL);
  /* The NULL-descriptor guard must stop the call before it reaches the buffer. */
  CHECK(buffer.createCalls == 0);
}

static void test_create_forwards_descriptor_and_returns_encoder(void) {
  FakeCommandBuffer *buffer = [FakeCommandBuffer new];
  FakeParallelEncoder *parallel = [FakeParallelEncoder new];
  NSObject *descriptor = [NSObject new];
  buffer.parallelEncoder = parallel;

  mtl_parallel_render_command_encoder_t encoder = metal_parallel_render_command_encoder_create(
      (__bridge void *)buffer, (__bridge void *)descriptor);

  CHECK(encoder == (__bridge void *)parallel);
  CHECK(buffer.createCalls == 1);
  CHECK(buffer.lastDescriptor == descriptor);
}

static void test_create_command_encoder_forwards_render_encoder(void) {
  FakeParallelEncoder *parallel = [FakeParallelEncoder new];
  NSObject *render = [NSObject new];
  parallel.renderEncoder = render;

  mtl_render_command_encoder_t encoder =
      metal_parallel_render_encoder_create_command_encoder((__bridge void *)parallel);

  CHECK(encoder == (__bridge void *)render);
  CHECK(parallel.renderCommandCalls == 1);
  CHECK(metal_parallel_render_encoder_create_command_encoder(NULL) == NULL);
}

static void test_end_encoding_ends_once_and_ignores_null(void) {
  FakeParallelEncoder *parallel = [FakeParallelEncoder new];

  metal_parallel_render_encoder_end_encoding((__bridge void *)parallel);
  CHECK(parallel.endEncodingCalls == 1);

  /* A NULL handle must return without crashing or ending anything else. */
  metal_parallel_render_encoder_end_encoding(NULL);
  CHECK(parallel.endEncodingCalls == 1);
}

int main(void) {
  test_create_rejects_null_arguments();
  test_create_forwards_descriptor_and_returns_encoder();
  test_create_command_encoder_forwards_render_encoder();
  test_end_encoding_ends_once_and_ignores_null();

  if (g_failed != 0) {
    fprintf(stderr, "test_mtl_parallel_encoder: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("test_mtl_parallel_encoder: all checks passed\n");
  return 0;
}
