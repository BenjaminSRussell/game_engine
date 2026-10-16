/* Unit test for src/engine/backend/metal/mtl_transfer.c (issue #303).
 *
 * On Apple the root CMakeLists compiles the backend/metal C files as
 * Objective-C with ARC, so the unit's Metal branches are the ones under test.
 * The test passes Objective-C fakes through the C API. Each fake records the
 * messages the unit sends, and the test runs completion handlers when it
 * chooses, so no GPU or Metal device is needed.
 *
 * Two helpers the unit calls from other files, metal_get_device and
 * metal_pixel_format_bytes_per_pixel, are stubbed here. Engine does not build
 * on main, so the test does not link it.
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "backend/metal/mtl_transfer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* STAGING_BUFFER_SIZE in mtl_transfer.c: 16 MiB. */
#define EXPECTED_STAGING_BYTES (16 * 1024 * 1024)

static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* ---- Fake Metal objects: interfaces ------------------------------------- */

/* Base for every fake. Covers the setLabel: calls the unit makes. */
@interface FakeObject : NSObject
@property(nonatomic, copy) NSString *label;
@end

@interface FakeDevice : FakeObject
- (id)newCommandQueue;
- (id)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options;
@end

@interface FakeQueue : FakeObject
- (id)commandBuffer;
@end

@interface FakeCommandBuffer : FakeObject
@property(nonatomic, strong) NSMutableArray *completionHandlers;
@property(nonatomic) BOOL committed;
@property(nonatomic) BOOL completed;
- (id)blitCommandEncoder;
- (void)addCompletedHandler:(MTLCommandBufferHandler)handler;
- (void)commit;
- (void)waitUntilCompleted;
@end

@interface FakeBlitEncoder : FakeObject
- (void)copyFromBuffer:(id)src
          sourceOffset:(NSUInteger)srcOffset
              toBuffer:(id)dst
     destinationOffset:(NSUInteger)dstOffset
                  size:(NSUInteger)size;
- (void)copyFromBuffer:(id)src
          sourceOffset:(NSUInteger)srcOffset
     sourceBytesPerRow:(NSUInteger)bytesPerRow
   sourceBytesPerImage:(NSUInteger)bytesPerImage
            sourceSize:(MTLSize)size
             toTexture:(id)dst
      destinationSlice:(NSUInteger)slice
      destinationLevel:(NSUInteger)level
     destinationOrigin:(MTLOrigin)origin;
- (void)copyFromTexture:(id)src
            sourceSlice:(NSUInteger)srcSlice
            sourceLevel:(NSUInteger)srcLevel
           sourceOrigin:(MTLOrigin)srcOrigin
             sourceSize:(MTLSize)size
              toTexture:(id)dst
       destinationSlice:(NSUInteger)dstSlice
       destinationLevel:(NSUInteger)dstLevel
      destinationOrigin:(MTLOrigin)dstOrigin;
- (void)copyFromTexture:(id)src
            sourceSlice:(NSUInteger)srcSlice
            sourceLevel:(NSUInteger)srcLevel
           sourceOrigin:(MTLOrigin)srcOrigin
             sourceSize:(MTLSize)size
               toBuffer:(id)dst
      destinationOffset:(NSUInteger)dstOffset
 destinationBytesPerRow:(NSUInteger)bytesPerRow
destinationBytesPerImage:(NSUInteger)bytesPerImage;
- (void)endEncoding;
@end

@interface FakeBuffer : FakeObject
@property(nonatomic) void *storage;
- (void *)contents;
@end

@interface FakeTexture : FakeObject
@end

/* ---- Recorded state, reset before each test ----------------------------- */

typedef enum {
  COPY_BUFFER_TO_BUFFER,
  COPY_BUFFER_TO_TEXTURE,
  COPY_TEXTURE_TO_BUFFER,
  COPY_TEXTURE_TO_TEXTURE
} CopyKind;

typedef struct {
  CopyKind kind;
  const void *src; /* id<MTLBuffer> or id<MTLTexture> */
  const void *dst;
  NSUInteger src_offset; /* buffer side, when the source is a buffer */
  NSUInteger dst_offset; /* buffer side, when the destination is a buffer */
  NSUInteger bytes;      /* buffer-to-buffer copies */
  NSUInteger bytes_per_row;
  NSUInteger bytes_per_image;
  MTLSize size;
  MTLOrigin src_origin;
  MTLOrigin dst_origin;
  NSUInteger src_slice;
  NSUInteger src_level;
  NSUInteger dst_slice;
  NSUInteger dst_level;
} CopyRecord;

#define MAX_COPIES 16
static CopyRecord g_copies[MAX_COPIES];
static int g_copy_count = 0;

static int g_commits = 0;       /* commit calls on any command buffer */
static int g_end_encodings = 0; /* endEncoding calls on blit encoders */
static int g_waits = 0;         /* waitUntilCompleted calls */
static int g_queues_created = 0;
static int g_staging_creations = 0;
static NSUInteger g_staging_length = 0;
static MTLResourceOptions g_staging_options = 0;
static FakeBuffer *g_staging = nil;             /* the staging buffer */
static FakeQueue *g_queue = nil;                /* the transfer queue */
static NSMutableArray *g_command_buffers = nil; /* every command buffer, in order */
static FakeDevice *g_device = nil;

static void reset_recorder(void) {
  memset(g_copies, 0, sizeof g_copies);
  g_copy_count = 0;
  g_commits = 0;
  g_end_encodings = 0;
  g_waits = 0;
  g_queues_created = 0;
  g_staging_creations = 0;
  g_staging_length = 0;
  g_staging_options = 0;
  g_staging = nil;
  g_queue = nil;
  g_command_buffers = [NSMutableArray array];
}

static CopyRecord *new_copy(CopyKind kind, const void *src, const void *dst) {
  if (g_copy_count >= MAX_COPIES) {
    fprintf(stderr, "FAIL: more than %d copies recorded\n", MAX_COPIES);
    g_failed++;
    return &g_copies[MAX_COPIES - 1];
  }
  CopyRecord *r = &g_copies[g_copy_count++];
  memset(r, 0, sizeof *r);
  r->kind = kind;
  r->src = src;
  r->dst = dst;
  return r;
}

static int size_is(MTLSize s, NSUInteger w, NSUInteger h, NSUInteger d) {
  return s.width == w && s.height == h && s.depth == d;
}

static int origin_is(MTLOrigin o, NSUInteger x, NSUInteger y, NSUInteger z) {
  return o.x == x && o.y == y && o.z == z;
}

/* ---- Fake Metal objects: implementations -------------------------------- */

@implementation FakeObject
@end

@implementation FakeDevice
- (id)newCommandQueue {
  g_queues_created++;
  g_queue = [FakeQueue new];
  return g_queue;
}

- (id)newBufferWithLength:(NSUInteger)length options:(MTLResourceOptions)options {
  g_staging_creations++;
  g_staging_length = length;
  g_staging_options = options;
  FakeBuffer *buffer = [FakeBuffer new];
  buffer.storage = calloc(1, length);
  g_staging = buffer;
  return buffer;
}
@end

@implementation FakeQueue
- (id)commandBuffer {
  FakeCommandBuffer *buffer = [FakeCommandBuffer new];
  [g_command_buffers addObject:buffer];
  return buffer;
}
@end

@implementation FakeCommandBuffer
- (id)blitCommandEncoder {
  return [FakeBlitEncoder new];
}

- (void)addCompletedHandler:(MTLCommandBufferHandler)handler {
  if (!self.completionHandlers) {
    self.completionHandlers = [NSMutableArray array];
  }
  [self.completionHandlers addObject:handler];
}

- (void)commit {
  g_commits++;
  self.committed = YES;
}

- (void)waitUntilCompleted {
  g_waits++;
}
@end

@implementation FakeBlitEncoder
- (void)copyFromBuffer:(id)src
          sourceOffset:(NSUInteger)srcOffset
              toBuffer:(id)dst
     destinationOffset:(NSUInteger)dstOffset
                  size:(NSUInteger)size {
  CopyRecord *r = new_copy(COPY_BUFFER_TO_BUFFER, (__bridge const void *)src,
                           (__bridge const void *)dst);
  r->src_offset = srcOffset;
  r->dst_offset = dstOffset;
  r->bytes = size;
}

- (void)copyFromBuffer:(id)src
          sourceOffset:(NSUInteger)srcOffset
     sourceBytesPerRow:(NSUInteger)bytesPerRow
   sourceBytesPerImage:(NSUInteger)bytesPerImage
            sourceSize:(MTLSize)size
             toTexture:(id)dst
      destinationSlice:(NSUInteger)slice
      destinationLevel:(NSUInteger)level
     destinationOrigin:(MTLOrigin)origin {
  CopyRecord *r = new_copy(COPY_BUFFER_TO_TEXTURE, (__bridge const void *)src,
                           (__bridge const void *)dst);
  r->src_offset = srcOffset;
  r->bytes_per_row = bytesPerRow;
  r->bytes_per_image = bytesPerImage;
  r->size = size;
  r->dst_slice = slice;
  r->dst_level = level;
  r->dst_origin = origin;
}

- (void)copyFromTexture:(id)src
            sourceSlice:(NSUInteger)srcSlice
            sourceLevel:(NSUInteger)srcLevel
           sourceOrigin:(MTLOrigin)srcOrigin
             sourceSize:(MTLSize)size
              toTexture:(id)dst
       destinationSlice:(NSUInteger)dstSlice
       destinationLevel:(NSUInteger)dstLevel
      destinationOrigin:(MTLOrigin)dstOrigin {
  CopyRecord *r = new_copy(COPY_TEXTURE_TO_TEXTURE, (__bridge const void *)src,
                           (__bridge const void *)dst);
  r->src_slice = srcSlice;
  r->src_level = srcLevel;
  r->src_origin = srcOrigin;
  r->size = size;
  r->dst_slice = dstSlice;
  r->dst_level = dstLevel;
  r->dst_origin = dstOrigin;
}

- (void)copyFromTexture:(id)src
            sourceSlice:(NSUInteger)srcSlice
            sourceLevel:(NSUInteger)srcLevel
           sourceOrigin:(MTLOrigin)srcOrigin
             sourceSize:(MTLSize)size
               toBuffer:(id)dst
      destinationOffset:(NSUInteger)dstOffset
 destinationBytesPerRow:(NSUInteger)bytesPerRow
destinationBytesPerImage:(NSUInteger)bytesPerImage {
  CopyRecord *r = new_copy(COPY_TEXTURE_TO_BUFFER, (__bridge const void *)src,
                           (__bridge const void *)dst);
  r->src_slice = srcSlice;
  r->src_level = srcLevel;
  r->src_origin = srcOrigin;
  r->size = size;
  r->dst_offset = dstOffset;
  r->bytes_per_row = bytesPerRow;
  r->bytes_per_image = bytesPerImage;
}

- (void)endEncoding {
  g_end_encodings++;
}
@end

@implementation FakeBuffer
- (void *)contents {
  return self.storage;
}

- (void)dealloc {
  free(self.storage);
}
@end

@implementation FakeTexture
@end

/* ---- Stubs for helpers defined in other units --------------------------- */

static char g_device_token; /* opaque metal_device_t handle for the tests */
#define TEST_DEVICE ((metal_device_t *)&g_device_token)

void *metal_get_device(metal_device_t *dev) {
  return dev == TEST_DEVICE ? (__bridge void *)g_device : NULL;
}

size_t metal_pixel_format_bytes_per_pixel(metal_pixel_format_t format) {
  (void)format;
  return 4; /* every format these tests use is four bytes per pixel */
}

/* ---- Helpers ------------------------------------------------------------- */

static void count_callback(void *user_data) {
  (*(int *)user_data)++;
}

/* Runs the completion handlers of each committed command buffer that has not
 * completed yet, which is what the GPU does once the work is finished. */
static void complete_committed_buffers(void) {
  for (NSUInteger i = 0; i < g_command_buffers.count; i++) {
    FakeCommandBuffer *buffer = g_command_buffers[i];
    if (!buffer.committed || buffer.completed) {
      continue;
    }
    buffer.completed = YES;
    for (NSUInteger h = 0; h < buffer.completionHandlers.count; h++) {
      MTLCommandBufferHandler handler =
          (MTLCommandBufferHandler)buffer.completionHandlers[h];
      handler((id)buffer);
    }
  }
}

/* ---- Tests ---------------------------------------------------------------- */

static void test_lifecycle_and_null_device(void) {
  reset_recorder();
  CHECK(metal_transfer_manager_create(NULL) == NULL);

  metal_transfer_stats_t none = metal_transfer_manager_get_stats(NULL);
  CHECK(none.total_transfers == 0 && none.staging_buffer_size == 0);

  metal_transfer_manager_destroy(NULL);
  CHECK(g_waits == 0);

  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  CHECK(mgr != NULL);
  CHECK(g_queues_created == 1);
  CHECK([g_queue.label isEqualToString:@"Transfer Queue"]);
  CHECK(g_staging_creations == 0); /* staging is created by the first staged upload */

  metal_transfer_stats_t stats = metal_transfer_manager_get_stats(mgr);
  CHECK(stats.staging_buffer_size == EXPECTED_STAGING_BYTES);
  CHECK(stats.total_transfers == 0 && stats.active_transfers == 0);

  metal_transfer_manager_destroy(mgr);
  CHECK(g_waits == 1); /* destroy waits for the transfer queue to go idle */
}

static void test_null_and_empty_requests_complete_without_gpu_work(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  metal_buffer_t buf = {0};
  metal_texture_t tex = {0};
  int done = 0;

  metal_transfer_upload_buffer_staged(NULL, &buf, "x", 1, 0, count_callback, &done);
  metal_transfer_upload_buffer_staged(mgr, NULL, "x", 1, 0, count_callback, &done);
  metal_transfer_upload_buffer_staged(mgr, &buf, NULL, 1, 0, count_callback, &done);
  metal_transfer_upload_buffer_staged(mgr, &buf, "x", 0, 0, count_callback, &done);
  CHECK(done == 4);

  metal_transfer_copy_buffer(NULL, &buf, &buf, 0, 0, 1, count_callback, &done);
  metal_transfer_copy_buffer(mgr, NULL, &buf, 0, 0, 1, count_callback, &done);
  metal_transfer_copy_buffer(mgr, &buf, &buf, 0, 0, 0, count_callback, &done);
  CHECK(done == 7);

  metal_transfer_upload_texture_staged(mgr, &tex, NULL, NULL, count_callback, &done);
  metal_transfer_blit_texture(mgr, NULL, &tex, NULL, NULL, count_callback, &done);
  metal_transfer_buffer_to_texture(mgr, NULL, &tex, 0, NULL, 0, count_callback, &done);
  metal_transfer_texture_to_buffer(mgr, &tex, NULL, NULL, 0, 0, count_callback, &done);
  CHECK(done == 11);

  CHECK(g_copy_count == 0);
  CHECK(g_command_buffers.count == 0);
  CHECK(g_staging_creations == 0);
  metal_transfer_manager_destroy(mgr);
}

static void test_upload_to_shared_buffer_copies_in_place(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  uint8_t backing[64] = {0};
  metal_buffer_t dst = {0};
  dst.storage_mode = METAL_STORAGE_SHARED;
  dst.mapped_ptr = backing;
  const uint8_t payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  int done = 0;

  metal_transfer_upload_buffer_staged(mgr, &dst, payload, sizeof payload, 16,
                                      count_callback, &done);

  CHECK(memcmp(backing + 16, payload, sizeof payload) == 0);
  CHECK(done == 1); /* shared storage needs no GPU work, so the callback runs at once */
  CHECK(g_copy_count == 0);
  CHECK(g_command_buffers.count == 0);
  CHECK(g_staging_creations == 0);
  metal_transfer_manager_destroy(mgr);
}

static void test_private_upload_goes_through_staging_and_completes_once(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  FakeBuffer *dst_buffer = [FakeBuffer new];
  metal_buffer_t dst = {0};
  dst.buffer = (id<MTLBuffer>)dst_buffer;
  dst.storage_mode = METAL_STORAGE_PRIVATE;
  uint8_t payload[100];
  for (int i = 0; i < 100; i++) {
    payload[i] = (uint8_t)i;
  }
  int done = 0;

  metal_transfer_upload_buffer_staged(mgr, &dst, payload, sizeof payload, 32,
                                      count_callback, &done);

  CHECK(g_staging_creations == 1);
  CHECK(g_staging_length == EXPECTED_STAGING_BYTES);
  CHECK(g_staging_options == MTLResourceStorageModeShared);
  CHECK(memcmp([g_staging contents], payload, sizeof payload) == 0);
  CHECK(g_copy_count == 1);
  CopyRecord *r = &g_copies[0];
  CHECK(r->kind == COPY_BUFFER_TO_BUFFER);
  CHECK(r->src == (__bridge const void *)g_staging);
  CHECK(r->dst == (__bridge const void *)dst_buffer);
  CHECK(r->src_offset == 0 && r->dst_offset == 32 && r->bytes == 100);

  metal_transfer_stats_t stats = metal_transfer_manager_get_stats(mgr);
  CHECK(stats.total_transfers == 1);
  CHECK(stats.total_bytes_transferred == 100);
  CHECK(stats.active_transfers == 1);
  CHECK(done == 0); /* the callback waits for the GPU */

  metal_transfer_flush(mgr);
  CHECK(g_end_encodings == 1);
  CHECK(g_commits == 1);
  CHECK(done == 0);

  complete_committed_buffers();
  CHECK(done == 1);
  CHECK(metal_transfer_manager_get_stats(mgr).active_transfers == 0);
  metal_transfer_manager_destroy(mgr);
}

static void test_staging_offsets_are_256_byte_aligned(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  metal_buffer_t dst = {0};
  dst.buffer = (id<MTLBuffer>)[FakeBuffer new];
  dst.storage_mode = METAL_STORAGE_PRIVATE;
  uint8_t payload[100] = {0};
  int done = 0;

  metal_transfer_upload_buffer_staged(mgr, &dst, payload, sizeof payload, 0,
                                      count_callback, &done);
  metal_transfer_upload_buffer_staged(mgr, &dst, payload, sizeof payload, 0,
                                      count_callback, &done);

  CHECK(g_copy_count == 2);
  CHECK(g_copies[0].src_offset == 0);
  CHECK(g_copies[1].src_offset == 256); /* 100 bytes round up to one 256-byte slot */
  CHECK(g_staging_creations == 1);      /* the staging buffer is reused */
  CHECK(metal_transfer_manager_get_stats(mgr).staging_buffer_used == 512);

  metal_transfer_flush(mgr);
  complete_committed_buffers();
  CHECK(done == 2);
  metal_transfer_manager_destroy(mgr);
}

static void test_full_staging_buffer_is_flushed_and_rewound(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  metal_buffer_t dst = {0};
  dst.buffer = (id<MTLBuffer>)[FakeBuffer new];
  dst.storage_mode = METAL_STORAGE_PRIVATE;
  /* Two of these do not fit in the 16 MiB staging buffer. */
  size_t big = 10 * 1024 * 1024;
  void *payload = calloc(1, big);
  int done = 0;

  metal_transfer_upload_buffer_staged(mgr, &dst, payload, big, 0, count_callback, &done);
  CHECK(g_commits == 0);
  metal_transfer_upload_buffer_staged(mgr, &dst, payload, big, 0, count_callback, &done);

  /* The second upload commits the first command buffer, then waits on a fence. */
  CHECK(g_commits == 2);
  CHECK(g_waits == 1);
  CHECK(g_copy_count == 2);
  CHECK(g_copies[1].src_offset == 0); /* the staging cursor was rewound */
  CHECK(g_staging_creations == 1);

  complete_committed_buffers();
  CHECK(done == 1);
  metal_transfer_flush(mgr);
  complete_committed_buffers();
  CHECK(done == 2);
  CHECK(metal_transfer_manager_get_stats(mgr).active_transfers == 0);

  free(payload);
  metal_transfer_manager_destroy(mgr);
}

static void test_copy_buffer_encodes_offsets_and_completes(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  FakeBuffer *src_buffer = [FakeBuffer new];
  FakeBuffer *dst_buffer = [FakeBuffer new];
  metal_buffer_t src = {0};
  metal_buffer_t dst = {0};
  src.buffer = (id<MTLBuffer>)src_buffer;
  dst.buffer = (id<MTLBuffer>)dst_buffer;
  int done = 0;

  metal_transfer_copy_buffer(mgr, &src, &dst, 8, 16, 64, count_callback, &done);

  CHECK(g_copy_count == 1);
  CopyRecord *r = &g_copies[0];
  CHECK(r->kind == COPY_BUFFER_TO_BUFFER);
  CHECK(r->src == (__bridge const void *)src_buffer);
  CHECK(r->dst == (__bridge const void *)dst_buffer);
  CHECK(r->src_offset == 8 && r->dst_offset == 16 && r->bytes == 64);

  metal_transfer_stats_t stats = metal_transfer_manager_get_stats(mgr);
  CHECK(stats.total_transfers == 1);
  CHECK(stats.total_bytes_transferred == 64);
  CHECK(stats.active_transfers == 1);

  metal_transfer_flush(mgr);
  CHECK(done == 0);
  complete_committed_buffers();
  CHECK(done == 1);
  CHECK(metal_transfer_manager_get_stats(mgr).active_transfers == 0);
  metal_transfer_manager_destroy(mgr);
}

static void test_texture_upload_full_and_region_layouts(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  FakeTexture *tex_object = [FakeTexture new];
  metal_texture_t tex = {0};
  tex.texture = (id<MTLTexture>)tex_object;
  tex.format = METAL_PIXEL_FORMAT_RGBA8_UNORM;
  tex.type = METAL_TEXTURE_TYPE_2D;
  tex.width = 8;
  tex.height = 4;
  tex.depth = 1;
  uint8_t pixels[8 * 4 * 4] = {0};
  int done = 0;

  /* Whole texture: 8 pixels of 4 bytes per row, 4 rows. */
  metal_transfer_upload_texture_staged(mgr, &tex, pixels, NULL, count_callback, &done);
  CHECK(g_copy_count == 1);
  CopyRecord *r = &g_copies[0];
  CHECK(r->kind == COPY_BUFFER_TO_TEXTURE);
  CHECK(r->dst == (__bridge const void *)tex_object);
  CHECK(r->bytes_per_row == 32 && r->bytes_per_image == 128);
  CHECK(size_is(r->size, 8, 4, 1));
  CHECK(origin_is(r->dst_origin, 0, 0, 0));
  CHECK(r->dst_slice == 0 && r->dst_level == 0);
  CHECK(metal_transfer_manager_get_stats(mgr).total_bytes_transferred == 128);

  /* Sub-region: 4x2 pixels at (2,1) on mip 1. */
  metal_texture_region_t region = {.x = 2, .y = 1, .z = 0, .width = 4,
                                   .height = 2, .depth = 1, .mip_level = 1,
                                   .array_slice = 0};
  metal_transfer_upload_texture_staged(mgr, &tex, pixels, &region, count_callback, &done);
  CHECK(g_copy_count == 2);
  r = &g_copies[1];
  CHECK(r->bytes_per_row == 16 && r->bytes_per_image == 32);
  CHECK(size_is(r->size, 4, 2, 1));
  CHECK(origin_is(r->dst_origin, 2, 1, 0));
  CHECK(r->dst_level == 1);

  metal_transfer_flush(mgr);
  complete_committed_buffers();
  CHECK(done == 2);
  metal_transfer_manager_destroy(mgr);
}

static void test_blit_texture_defaults_and_regions(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  FakeTexture *src_object = [FakeTexture new];
  FakeTexture *dst_object = [FakeTexture new];
  metal_texture_t src = {0};
  metal_texture_t dst = {0};
  src.texture = (id<MTLTexture>)src_object;
  src.width = 8;
  src.height = 4;
  dst.texture = (id<MTLTexture>)dst_object;
  dst.width = 8;
  dst.height = 4;
  int done = 0;

  metal_transfer_blit_texture(mgr, &src, &dst, NULL, NULL, count_callback, &done);
  CHECK(g_copy_count == 1);
  CopyRecord *r = &g_copies[0];
  CHECK(r->kind == COPY_TEXTURE_TO_TEXTURE);
  CHECK(r->src == (__bridge const void *)src_object);
  CHECK(r->dst == (__bridge const void *)dst_object);
  CHECK(size_is(r->size, 8, 4, 1));
  CHECK(origin_is(r->src_origin, 0, 0, 0) && origin_is(r->dst_origin, 0, 0, 0));
  CHECK(r->src_level == 0 && r->dst_level == 0);

  metal_texture_region_t src_region = {.x = 1, .y = 1, .z = 0, .width = 2,
                                       .height = 2, .depth = 1, .mip_level = 0,
                                       .array_slice = 0};
  metal_texture_region_t dst_region = {.x = 3, .y = 0, .z = 0, .width = 2,
                                       .height = 2, .depth = 1, .mip_level = 2,
                                       .array_slice = 0};
  metal_transfer_blit_texture(mgr, &src, &dst, &src_region, &dst_region,
                              count_callback, &done);
  CHECK(g_copy_count == 2);
  r = &g_copies[1];
  CHECK(size_is(r->size, 2, 2, 1));
  CHECK(origin_is(r->src_origin, 1, 1, 0));
  CHECK(origin_is(r->dst_origin, 3, 0, 0));
  CHECK(r->dst_level == 2);

  metal_transfer_flush(mgr);
  complete_committed_buffers();
  CHECK(done == 2);
  metal_transfer_manager_destroy(mgr);
}

static void test_buffer_texture_copies_use_region_and_row_pitch(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);
  FakeBuffer *buf_object = [FakeBuffer new];
  FakeTexture *tex_object = [FakeTexture new];
  metal_buffer_t buf = {0};
  metal_texture_t tex = {0};
  buf.buffer = (id<MTLBuffer>)buf_object;
  tex.texture = (id<MTLTexture>)tex_object;
  tex.width = 8;
  tex.height = 4;
  tex.depth = 1;
  metal_texture_region_t region = {.x = 1, .y = 0, .z = 0, .width = 4,
                                   .height = 2, .depth = 1, .mip_level = 0,
                                   .array_slice = 0};
  int done = 0;

  /* Buffer to texture: 16-byte rows, 2 rows, read from buffer offset 64. */
  metal_transfer_buffer_to_texture(mgr, &buf, &tex, 64, &region, 16,
                                   count_callback, &done);
  CHECK(g_copy_count == 1);
  CopyRecord *r = &g_copies[0];
  CHECK(r->kind == COPY_BUFFER_TO_TEXTURE);
  CHECK(r->src == (__bridge const void *)buf_object);
  CHECK(r->dst == (__bridge const void *)tex_object);
  CHECK(r->src_offset == 64 && r->bytes_per_row == 16 && r->bytes_per_image == 32);
  CHECK(size_is(r->size, 4, 2, 1));
  CHECK(origin_is(r->dst_origin, 1, 0, 0));

  /* Texture to buffer, whole texture: 32-byte rows, written at buffer offset 128. */
  metal_transfer_texture_to_buffer(mgr, &tex, &buf, NULL, 128, 32,
                                   count_callback, &done);
  CHECK(g_copy_count == 2);
  r = &g_copies[1];
  CHECK(r->kind == COPY_TEXTURE_TO_BUFFER);
  CHECK(r->src == (__bridge const void *)tex_object);
  CHECK(r->dst == (__bridge const void *)buf_object);
  CHECK(r->dst_offset == 128 && r->bytes_per_row == 32 && r->bytes_per_image == 128);
  CHECK(size_is(r->size, 8, 4, 1));
  CHECK(origin_is(r->src_origin, 0, 0, 0));

  metal_transfer_flush(mgr);
  complete_committed_buffers();
  CHECK(done == 2);
  metal_transfer_manager_destroy(mgr);
}

static void test_flush_commits_once_and_wait_idle_fences(void) {
  reset_recorder();
  metal_transfer_manager_t *mgr = metal_transfer_manager_create(TEST_DEVICE);

  metal_transfer_flush(mgr); /* nothing recorded yet, so nothing to commit */
  CHECK(g_commits == 0);
  CHECK(g_end_encodings == 0);

  metal_buffer_t src = {0};
  metal_buffer_t dst = {0};
  src.buffer = (id<MTLBuffer>)[FakeBuffer new];
  dst.buffer = (id<MTLBuffer>)[FakeBuffer new];
  metal_transfer_copy_buffer(mgr, &src, &dst, 0, 0, 16, NULL, NULL);

  metal_transfer_flush(mgr);
  CHECK(g_end_encodings == 1);
  CHECK(g_commits == 1);
  metal_transfer_flush(mgr); /* already flushed: no second commit */
  CHECK(g_commits == 1);
  CHECK(g_end_encodings == 1);

  metal_transfer_wait_idle(mgr); /* fence buffer: commit, then wait */
  CHECK(g_commits == 2);
  CHECK(g_waits == 1);

  metal_transfer_flush(NULL);
  metal_transfer_wait_idle(NULL);
  CHECK(g_commits == 2);
  metal_transfer_manager_destroy(mgr);
}

int main(void) {
  @autoreleasepool {
    g_device = [FakeDevice new];
    test_lifecycle_and_null_device();
    test_null_and_empty_requests_complete_without_gpu_work();
    test_upload_to_shared_buffer_copies_in_place();
    test_private_upload_goes_through_staging_and_completes_once();
    test_staging_offsets_are_256_byte_aligned();
    test_full_staging_buffer_is_flushed_and_rewound();
    test_copy_buffer_encodes_offsets_and_completes();
    test_texture_upload_full_and_region_layouts();
    test_blit_texture_defaults_and_regions();
    test_buffer_texture_copies_use_region_and_row_pitch();
    test_flush_commits_once_and_wait_idle_fences();
  }

  if (g_failed != 0) {
    fprintf(stderr, "test_mtl_transfer: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("test_mtl_transfer: all checks passed\n");
  return 0;
}
