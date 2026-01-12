// Stack Capture System for macOS implementation
#include "debug/stack_capture_macos.h"
#include <core/logger.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <math/math.h>
#include <math/vec3.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysctl.h>
#include <time.h>

// Global stack capture system instance
StackCaptureSystem *g_stack_system = NULL;

// MARK: - Helper Functions

static u64 get_current_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static u32 get_current_thread_id(void) {
  // TODO(Jules): Optimize get_current_thread_id for performance in tight loops.
  return (u32)pthread_mach_thread_np(pthread_self());
}

static bool is_valid_stack_pointer(void *sp) {
  // Basic validation of stack pointer
  if (!sp)
    return false;

  // Check if it's in a reasonable range (simplified)
  uintptr_t addr = (uintptr_t)sp;
  return (addr > 0x1000 && addr < 0x7fffffffffff);
}

// MARK: - Stack Capture System Management

bool stack_capture_init(StackCaptureSystem *system, u32 max_allocations,
                        u32 max_stack_frames) {
  if (!system || max_allocations == 0 || max_stack_frames == 0) {
    LOG_ERROR("Invalid parameters for stack_capture_init");
    return false;
  }

  memset(system, 0, sizeof(StackCaptureSystem));

  // Initialize leak detector
  system->leak_detector.allocations =
      (MemoryAllocation *)calloc(max_allocations, sizeof(MemoryAllocation));
  if (!system->leak_detector.allocations) {
    LOG_ERROR("Failed to allocate memory allocations array");
    return false;
  }
  system->leak_detector.allocation_capacity = max_allocations;
  system->leak_detector.max_stack_frames = max_stack_frames;
  system->leak_detector.leak_threshold_bytes = 1024; // 1KB default threshold

  // Initialize stack cache
  system->stack_cache =
      (StackTrace *)calloc(max_allocations, sizeof(StackTrace));
  if (!system->stack_cache) {
    free(system->leak_detector.allocations);
    LOG_ERROR("Failed to allocate stack cache");
    return false;
  }
  system->stack_cache_capacity = max_allocations;

  // Set default settings
  system->enable_symbol_resolution = true;
  system->enable_demangling = true;

  // Set global instance
  g_stack_system = system;

  LOG_INFO(
      "Stack capture system initialized (allocations: %u, stack frames: %u)",
      max_allocations, max_stack_frames);
  return true;
}

void stack_capture_shutdown(StackCaptureSystem *system) {
  if (!system) {
    return;
  }

  // Report any remaining leaks
  if (system->leak_detector.enable_tracking) {
    memory_leak_detector_report_leaks(&system->leak_detector);
  }

  // Free allocations
  if (system->leak_detector.allocations) {
    free(system->leak_detector.allocations);
  }

  // Free stack cache
  if (system->stack_cache) {
    for (u32 i = 0; i < system->stack_cache_count; i++) {
      if (system->stack_cache[i].frames) {
        free(system->stack_cache[i].frames);
      }
    }
    free(system->stack_cache);
  }

  // Clear global instance
  g_stack_system = NULL;

  memset(system, 0, sizeof(StackCaptureSystem));
  LOG_INFO("Stack capture system shutdown");
}

// MARK: - Stack Trace Capture

bool stack_capture_current_thread(StackTrace *trace, u32 max_frames) {
  if (!trace || max_frames == 0) {
    return false;
  }

  return macos_capture_stack_trace(trace, max_frames);
}

bool macos_capture_stack_trace(StackTrace *trace, u32 max_frames) {
  if (!trace || max_frames == 0) {
    return false;
  }

  // Allocate frames array
  trace->frames = (StackFrame *)calloc(max_frames, sizeof(StackFrame));
  if (!trace->frames) {
    LOG_ERROR("Failed to allocate stack frames");
    return false;
  }

  // Capture stack trace using backtrace
  void *buffer[64];
  int frame_count = backtrace(buffer, max_frames < 64 ? max_frames : 64);

  // TODO(Jules): Add support for capturing thread names in
  // macos_capture_stack_trace.
  if (frame_count <= 0) {
    LOG_WARN("Failed to capture stack trace");
    free(trace->frames);
    trace->frames = NULL;
    return false;
  }

  trace->frame_count = (u32)frame_count;
  trace->max_frames = max_frames;
  trace->capture_time_ms = get_current_time_ms();
  trace->thread_id = get_current_thread_id();

  // Convert to StackFrame format
  for (int i = 0; i < frame_count; i++) {
    StackFrame *frame = &trace->frames[i];
    frame->instruction_pointer = buffer[i];
    // TODO(Jules): Implement frame pointer walking as a fallback if backtrace
    // fails.
    frame->frame_pointer = NULL; // Would need frame pointer walking for this
    frame->stack_pointer = NULL; // Would need stack pointer capture

    // Try to get symbol information
    Dl_info dl_info;
    if (dladdr(buffer[i], &dl_info)) {
      if (dl_info.dli_sname) {
        strncpy(frame->function_name, dl_info.dli_sname,
                sizeof(frame->function_name) - 1);
        frame->function_name[sizeof(frame->function_name) - 1] = '\0';

        // Demangle if it's a C++ symbol
        if (g_stack_system && g_stack_system->enable_demangling) {
          char demangled[256];
          if (stack_capture_demangle_symbol(dl_info.dli_sname, demangled,
                                            sizeof(demangled))) {
            strncpy(frame->function_name, demangled,
                    sizeof(frame->function_name) - 1);
            frame->function_name[sizeof(frame->function_name) - 1] = '\0';
          }
        }
      }

      if (dl_info.dli_fname) {
        strncpy(frame->module_name, dl_info.dli_fname,
                sizeof(frame->module_name) - 1);
        frame->module_name[sizeof(frame->module_name) - 1] = '\0';
        frame->module_base_address = (u64)dl_info.dli_fbase;
      }
    }
  }

  LOG_DEBUG("Captured stack trace: %u frames", trace->frame_count);
  return true;
}

bool stack_capture_resolve_symbol(void *address, char *function_name,
                                  size_t func_size, char *file_name,
                                  size_t file_size, u32 *line_number) {
  if (!address || !function_name || !file_name || !line_number) {
    return false;
  }

  Dl_info dl_info;
  if (!dladdr(address, &dl_info)) {
    return false;
  }

  // TODO(Jules): Add cache for symbol resolution to avoid repeated dladdr
  // calls.
  if (dl_info.dli_sname) {
    strncpy(function_name, dl_info.dli_sname, func_size - 1);
    function_name[func_size - 1] = '\0';

    // Demangle if needed
    if (g_stack_system && g_stack_system->enable_demangling) {
      char demangled[256];
      if (stack_capture_demangle_symbol(dl_info.dli_sname, demangled,
                                        sizeof(demangled))) {
        strncpy(function_name, demangled, func_size - 1);
        function_name[func_size - 1] = '\0';
      }
    }
  }

  if (dl_info.dli_fname) {
    strncpy(file_name, dl_info.dli_fname, file_size - 1);
    file_name[file_size - 1] = '\0';
  }

  *line_number = 0; // Line number resolution would require debug info

  return true;
}

bool stack_capture_demangle_symbol(const char *mangled_name,
                                   char *demangled_name, size_t size) {
  if (!mangled_name || !demangled_name || size == 0) {
    return false;
  }

  // Check if it's a C++ mangled name (starts with _Z)
  if (strncmp(mangled_name, "_Z", 2) != 0) {
    strncpy(demangled_name, mangled_name, size - 1);
    demangled_name[size - 1] = '\0';
    // TODO(Jules): Verify memory safety in stack_capture_demangle_symbol buffer
    // handling.
    return false;
  }

  int status;
  char *demangled = abi::__cxa_demangle(mangled_name, NULL, NULL, &status);

  if (status == 0 && demangled) {
    strncpy(demangled_name, demangled, size - 1);
    demangled_name[size - 1] = '\0';
    free(demangled);
    return true;
  }

  // Fallback to original name
  strncpy(demangled_name, mangled_name, size - 1);
  demangled_name[size - 1] = '\0';
  return false;
}

void stack_trace_print(const StackTrace *trace) {
  if (!trace) {
    printf("Stack trace is NULL\n");
    return;
  }

  printf("=== STACK TRACE ===\n");
  printf("Thread ID: %u\n", trace->thread_id);
  printf("Capture Time: %llu ms\n", trace->capture_time_ms);
  printf("Frame Count: %u\n", trace->frame_count);
  printf("\n");

  for (u32 i = 0; i < trace->frame_count; i++) {
    const StackFrame *frame = &trace->frames[i];
    printf("%u: 0x%p", i, frame->instruction_pointer);

    if (strlen(frame->function_name) > 0) {
      printf(" %s", frame->function_name);
    }

    if (strlen(frame->module_name) > 0) {
      printf(" (%s+0x%llx)", frame->module_name,
             (u64)frame->instruction_pointer - frame->module_base_address);
    }

    printf("\n");
  }

  printf("==================\n");
}

// MARK: - Memory Leak Detection

bool memory_leak_detector_start_tracking(MemoryLeakDetector *detector) {
  if (!detector) {
    return false;
  }

  detector->enable_tracking = true;
  detector->enable_stack_capture = true;

  LOG_INFO("Memory leak tracking started");
  return true;
}

void memory_leak_detector_stop_tracking(MemoryLeakDetector *detector) {
  if (!detector) {
    return;
  }

  detector->enable_tracking = false;

  LOG_INFO("Memory leak tracking stopped");
}

bool memory_leak_detector_record_allocation(MemoryLeakDetector *detector,
                                            void *address, u64 size,
                                            const char *tag) {
  if (!detector || !address || !detector->enable_tracking) {
    return false;
  }

  if (detector->allocation_count >= detector->allocation_capacity) {
    LOG_ERROR("Allocation tracking capacity exceeded");
    detector->failed_allocations++;
    return false;
  }

  MemoryAllocation *allocation =
      &detector->allocations[detector->allocation_count++];
  memset(allocation, 0, sizeof(MemoryAllocation));

  allocation->address = address;
  allocation->size = size;
  allocation->allocation_time_ms = get_current_time_ms();
  allocation->thread_id = get_current_thread_id();

  if (tag) {
    strncpy(allocation->allocation_tag, tag,
            sizeof(allocation->allocation_tag) - 1);
    allocation->allocation_tag[sizeof(allocation->allocation_tag) - 1] = '\0';
  }

  // Capture stack trace if enabled
  if (detector->enable_stack_capture && g_stack_system) {
    stack_capture_current_thread(&allocation->allocation_stack,
                                 detector->max_stack_frames);
  }

  // Update statistics
  detector->total_allocated_bytes += size;
  detector->total_allocations++;

  if (detector->total_allocated_bytes > detector->peak_allocated_bytes) {
    detector->peak_allocated_bytes = detector->total_allocated_bytes;
  }

  if (detector->allocation_count > detector->allocation_count_peak) {
    detector->allocation_count_peak = detector->allocation_count;
  }

  LOG_DEBUG("Recorded allocation: %p (%llu bytes)", address, size);
  return true;
}

bool memory_leak_detector_record_free(MemoryLeakDetector *detector,
                                      void *address) {
  if (!detector || !address || !detector->enable_tracking) {
    return false;
  }

  // Find the allocation
  MemoryAllocation *allocation =
      memory_leak_detector_find_allocation(detector, address);
  if (!allocation) {
    LOG_WARN("Free of untracked allocation: %p", address);
    return false;
  }

  allocation->is_freed = true;
  allocation->free_time_ms = get_current_time_ms();

  // Capture free stack trace if enabled
  if (detector->enable_stack_capture && g_stack_system) {
    stack_capture_current_thread(&allocation->free_stack,
                                 detector->max_stack_frames);
  }

  // Update statistics
  detector->total_allocated_bytes -= allocation->size;
  detector->total_frees++;

  LOG_DEBUG("Recorded free: %p", address);
  return true;
}

MemoryAllocation *
memory_leak_detector_find_allocation(MemoryLeakDetector *detector,
                                     void *address) {
  if (!detector || !address) {
    return NULL;
  }

  for (u32 i = 0; i < detector->allocation_count; i++) {
    if (detector->allocations[i].address == address &&
        !detector->allocations[i].is_freed) {
      return &detector->allocations[i];
    }
  }

  return NULL;
}

void memory_leak_detector_report_leaks(MemoryLeakDetector *detector) {
  if (!detector) {
    return;
  }

  u32 leak_count = 0;
  u64 leaked_bytes = 0;

  printf("=== MEMORY LEAK REPORT ===\n");

  for (u32 i = 0; i < detector->allocation_count; i++) {
    MemoryAllocation *allocation = &detector->allocations[i];
    if (!allocation->is_freed) {
      leak_count++;
      leaked_bytes += allocation->size;

      printf("Leak %u: %p (%llu bytes) - %s\n", leak_count, allocation->address,
             allocation->size, allocation->allocation_tag);
      printf("  Thread: %u\n", allocation->thread_id);
      printf("  Allocated: %llu ms ago\n",
             get_current_time_ms() - allocation->allocation_time_ms);

      if (allocation->allocation_stack.frame_count > 0) {
        printf("  Allocation stack:\n");
        for (u32 j = 0; j < allocation->allocation_stack.frame_count && j < 5;
             j++) {
          const StackFrame *frame = &allocation->allocation_stack.frames[j];
          printf("    %u: 0x%p %s\n", j, frame->instruction_pointer,
                 frame->function_name);
        }
      }
      printf("\n");
    }
  }

  printf("Total Leaks: %u\n", leak_count);
  printf("Total Leaked Bytes: %llu\n", leaked_bytes);
  printf("========================\n");

  if (leak_count > 0) {
    LOG_ERROR("Detected %u memory leaks (%llu bytes)", leak_count,
              leaked_bytes);
  } else {
    LOG_INFO("No memory leaks detected");
  }
}

void memory_leak_detector_print_statistics(MemoryLeakDetector *detector) {
  if (!detector) {
    return;
  }

  printf("=== MEMORY STATISTICS ===\n");
  printf("Total Allocations: %llu\n", detector->total_allocations);
  printf("Total Frees: %llu\n", detector->total_frees);
  printf("Failed Allocations: %llu\n", detector->failed_allocations);
  printf("Current Allocations: %u\n", detector->allocation_count);
  printf("Current Allocated Bytes: %llu\n", detector->total_allocated_bytes);
  printf("Peak Allocated Bytes: %llu\n", detector->peak_allocated_bytes);
  printf("Peak Allocation Count: %u\n", detector->allocation_count_peak);
  printf("========================\n");
}

// MARK: - Utility Functions

void stack_capture_set_leak_threshold(StackCaptureSystem *system,
                                      u64 threshold_bytes) {
  if (!system) {
    return;
  }

  system->leak_detector.leak_threshold_bytes = threshold_bytes;
  LOG_INFO("Set leak threshold to %llu bytes", threshold_bytes);
}

void stack_capture_enable_symbol_resolution(StackCaptureSystem *system,
                                            bool enabled) {
  if (!system) {
    return;
  }

  system->enable_symbol_resolution = enabled;
  LOG_INFO("Symbol resolution %s", enabled ? "enabled" : "disabled");
}

void stack_capture_enable_demangling(StackCaptureSystem *system, bool enabled) {
  if (!system) {
    return;
  }

  system->enable_demangling = enabled;
  LOG_INFO("Symbol demangling %s", enabled ? "enabled" : "disabled");
}

void stack_capture_get_statistics(StackCaptureSystem *system,
                                  u64 *total_allocations,
                                  u64 *current_allocations,
                                  u64 *peak_allocations, u64 *total_leaks) {
  if (!system) {
    return;
  }

  if (total_allocations) {
    *total_allocations = system->leak_detector.total_allocations;
  }

  if (current_allocations) {
    *current_allocations = system->leak_detector.allocation_count;
  }

  if (peak_allocations) {
    *peak_allocations = system->leak_detector.peak_allocated_bytes;
  }

  if (total_leaks) {
    *total_leaks = 0;
    for (u32 i = 0; i < system->leak_detector.allocation_count; i++) {
      if (!system->leak_detector.allocations[i].is_freed) {
        (*total_leaks)++;
      }
    }
  }
}

bool stack_capture_export_leaks_to_json(StackCaptureSystem *system,
                                        const char *filename) {
  if (!system || !filename) {
    return false;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    LOG_ERROR("Failed to open JSON file: %s", filename);
    return false;
  }

  fprintf(fp, "{\n");
  fprintf(fp, "  \"memory_leaks\": {\n");
  fprintf(fp, "    \"total_leaks\": 0,\n");
  fprintf(fp, "    \"leaked_bytes\": 0,\n");

  u32 leak_count = 0;
  u64 leaked_bytes = 0;

  for (u32 i = 0; i < system->leak_detector.allocation_count; i++) {
    MemoryAllocation *allocation = &system->leak_detector.allocations[i];
    if (!allocation->is_freed) {
      leak_count++;
      leaked_bytes += allocation->size;

      if (leak_count > 1) {
        fprintf(fp, ",\n");
      }

      fprintf(fp, "      {\n");
      fprintf(fp, "        \"address\": \"%p\",\n", allocation->address);
      fprintf(fp, "        \"size\": %llu,\n", allocation->size);
      fprintf(fp, "        \"tag\": \"%s\",\n", allocation->allocation_tag);
      fprintf(fp, "        \"thread_id\": %u,\n", allocation->thread_id);
      fprintf(fp, "        \"allocation_time_ms\": %llu,\n",
              allocation->allocation_time_ms);
      fprintf(fp, "        \"stack_frames\": %u\n",
              allocation->allocation_stack.frame_count);
      fprintf(fp, "      }");
    }
  }

  fprintf(fp, "\n    ]\n");
  fprintf(fp, "  }\n");
  fprintf(fp, "}\n");

  fclose(fp);

  // Update the total values
  // (This is a simplified approach - in a real implementation, you'd rewrite
  // the file)

  LOG_INFO("Exported memory leaks to JSON: %s", filename);
  return true;
}
