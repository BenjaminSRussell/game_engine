/*
 * phase1_test.m
 * Phase 1 Metal Core Infrastructure Tests
 *
 * Tests basic Metal device initialization, buffers, textures, and capabilities
 */

#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_command_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_texture.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdbool.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg)                                                 \
  if (!(cond)) {                                                               \
    printf("FAIL: %s\n", msg);                                                 \
    return false;                                                              \
  }

#define TEST_PASS(msg) printf("PASS: %s\n", msg)

// Test 1: Device Creation
bool test_device_creation() {
  printf("\n=== Test 1: Device Creation ===\n");

  metal_device_t *device = metal_device_create_system_default();
  TEST_ASSERT(device != NULL, "Device creation failed");
  TEST_PASS("Device created successfully");

  // Verify singleton behavior
  metal_device_t *device2 = metal_device_get_default();
  TEST_ASSERT(device == device2, "Singleton pattern broken");
  TEST_PASS("Singleton pattern verified");

  return true;
}

// Test 2: Capability Detection
bool test_capability_detection() {
  printf("\n=== Test 2: Capability Detection ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  const mtl_device_caps_t *caps = metal_device_get_caps(device);
  TEST_ASSERT(caps != NULL, "Capability query failed");

  printf("  GPU Family: %d\n", caps->gpu_family);
  printf("  Max Texture Size: %d\n", caps->max_texture_size);
  printf("  Max Buffer Length: %llu\n", caps->max_buffer_length);
  printf("  Supports Ray Tracing: %s\n",
         caps->supports_raytracing ? "Yes" : "No");
  TEST_PASS("Capability queries working");

  // Test raytracing support function
  bool rt_support = metal_device_supports_raytracing(device);
  printf("  Ray tracing support: %s\n", rt_support ? "Yes" : "No");
  TEST_PASS("Ray tracing support queried");

  return true;
}

// Test 3: Memory Info
bool test_memory_info() {
  printf("\n=== Test 3: Memory Info ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  mtl_memory_info_t mem_info;
  metal_device_get_memory_info(device, &mem_info);

  printf("  Shared Memory Size: %llu MB\n",
         mem_info.shared_memory_size / (1024 * 1024));
  printf("  Private Memory Size: %llu MB\n",
         mem_info.private_memory_size / (1024 * 1024));
  printf("  Recommended Working Set: %llu MB\n",
         mem_info.recommended_working_set_size / (1024 * 1024));
  TEST_PASS("Memory info retrieved");

  return true;
}

// Test 4: Command Buffer Creation
bool test_command_buffer_creation() {
  printf("\n=== Test 4: Command Buffer Creation ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  mtl_command_buffer_t cmd_buffer = metal_command_buffer_create(device);
  TEST_ASSERT(cmd_buffer != NULL, "Command buffer creation failed");
  TEST_PASS("Command buffer created");

  // Set label
  metal_command_buffer_set_label(cmd_buffer, "Phase1TestCmdBuffer");
  TEST_PASS("Command buffer label set");

  // Check status
  mtl_command_buffer_status_t status =
      metal_command_buffer_get_status(cmd_buffer);
  TEST_ASSERT(status == MTL_COMMAND_BUFFER_STATUS_NOT_ENQUEUED,
              "Unexpected initial status");
  TEST_PASS("Command buffer status verified");

  // Commit and wait
  metal_command_buffer_commit(cmd_buffer);
  metal_command_buffer_wait_until_completed(cmd_buffer);

  status = metal_command_buffer_get_status(cmd_buffer);
  TEST_ASSERT(status == MTL_COMMAND_BUFFER_STATUS_COMPLETED,
              "Command buffer did not complete");
  TEST_PASS("Command buffer executed successfully");

  return true;
}

// Test 5: Buffer Creation and Mapping
bool test_buffer_creation() {
  printf("\n=== Test 5: Buffer Creation ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  // Create a shared buffer
  metal_buffer_desc_t desc = {.size = 4096,
                              .storage_mode = METAL_STORAGE_SHARED,
                              .usage = METAL_BUFFER_USAGE_UNIFORM,
                              .initial_data = NULL,
                              .label = "TestBuffer"};

  metal_buffer_t *buffer = metal_buffer_create(device, &desc);
  TEST_ASSERT(buffer != NULL, "Buffer creation failed");
  TEST_ASSERT(buffer->size == 4096, "Buffer size mismatch");
  TEST_PASS("Buffer created successfully");

  // Map and write data
  void *mapped = metal_buffer_map(buffer);
  TEST_ASSERT(mapped != NULL, "Buffer mapping failed");

  float test_data[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  memcpy(mapped, test_data, sizeof(test_data));
  TEST_PASS("Data written to buffer");

  // Verify data
  float *read_back = (float *)mapped;
  TEST_ASSERT(read_back[0] == 1.0f && read_back[1] == 2.0f,
              "Data verification failed");
  TEST_PASS("Data verified");

  metal_buffer_unmap(buffer);
  metal_buffer_destroy(buffer);
  TEST_PASS("Buffer destroyed");

  return true;
}

// Test 6: Texture Creation
bool test_texture_creation() {
  printf("\n=== Test 6: Texture Creation ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  // Create a 2D texture
  metal_texture_desc_t desc = {.type = METAL_TEXTURE_TYPE_2D,
                               .format = METAL_PIXEL_FORMAT_RGBA8_UNORM,
                               .width = 256,
                               .height = 256,
                               .depth = 1,
                               .array_length = 1,
                               .mip_levels = 1,
                               .usage = METAL_TEXTURE_USAGE_SHADER_READ |
                                        METAL_TEXTURE_USAGE_RENDER_TARGET,
                               .storage_mode = METAL_TEXTURE_STORAGE_PRIVATE,
                               .generate_mipmaps = false,
                               .label = "TestTexture"};

  metal_texture_t *texture = metal_texture_create(device, &desc);
  TEST_ASSERT(texture != NULL, "Texture creation failed");
  TEST_ASSERT(texture->width == 256 && texture->height == 256,
              "Texture dimensions mismatch");
  TEST_ASSERT(texture->format == METAL_PIXEL_FORMAT_RGBA8_UNORM,
              "Texture format mismatch");
  TEST_PASS("Texture created successfully");

  metal_texture_destroy(texture);
  TEST_PASS("Texture destroyed");

  return true;
}

// Test 7: Ring Buffer
bool test_ring_buffer() {
  printf("\n=== Test 7: Ring Buffer ===\n");

  metal_device_t *device = metal_device_get_default();
  TEST_ASSERT(device != NULL, "Device not initialized");

  // Create ring buffer for 3 frames
  const size_t frame_size = 4096;
  const uint32_t frame_count = 3;

  metal_ring_buffer_t *ring =
      metal_ring_buffer_create(device, frame_size, frame_count);
  TEST_ASSERT(ring != NULL, "Ring buffer creation failed");
  TEST_ASSERT(ring->capacity == frame_size * frame_count,
              "Ring buffer capacity mismatch");
  TEST_PASS("Ring buffer created");

  // Allocate some data
  size_t offset = 0;
  void *alloc = metal_ring_buffer_alloc(ring, 256, 16, &offset);
  TEST_ASSERT(alloc != NULL, "Ring buffer allocation failed");
  TEST_PASS("Ring buffer allocation succeeded");

  // Advance frame
  metal_ring_buffer_next_frame(ring);
  TEST_PASS("Advanced to next frame");

  metal_ring_buffer_destroy(ring);
  TEST_PASS("Ring buffer destroyed");

  return true;
}

// Main test runner
int main(int argc, const char *argv[]) {
  @autoreleasepool {
    printf("========================================\n");
    printf("Phase 1: Metal Core Infrastructure Tests\n");
    printf("========================================\n");

    bool all_passed = true;

    all_passed &= test_device_creation();
    all_passed &= test_capability_detection();
    all_passed &= test_memory_info();
    all_passed &= test_command_buffer_creation();
    all_passed &= test_buffer_creation();
    all_passed &= test_texture_creation();
    all_passed &= test_ring_buffer();

    printf("\n========================================\n");
    if (all_passed) {
      printf("All Phase 1 tests PASSED!\n");
      printf("========================================\n");
      return 0;
    } else {
      printf("Some Phase 1 tests FAILED!\n");
      printf("========================================\n");
      return 1;
    }
  }
}
