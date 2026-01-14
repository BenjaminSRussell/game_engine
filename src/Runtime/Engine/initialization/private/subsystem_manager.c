#include "../public/subsystem_registry.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBSYSTEMS 64

typedef struct {
  SubsystemDescriptor descriptor;
  ISubsystem *instance;
  bool initialized;
  bool registered;
} SubsystemEntry;

static SubsystemEntry g_subsystems[MAX_SUBSYSTEMS];
static uint32_t g_subsystem_count = 0;

bool subsystem_registry_register(const SubsystemDescriptor *descriptor) {
  if (!descriptor || !descriptor->name || !descriptor->create_func) {
    LOG_ERROR("Invalid subsystem descriptor");
    return false;
  }

  if (g_subsystem_count >= MAX_SUBSYSTEMS) {
    LOG_ERROR("Max subsystems reached");
    return false;
  }

  // Check duplicates
  if (subsystem_registry_get(descriptor->name)) {
    LOG_WARN("Subsystem %s already registered", descriptor->name);
    return true; // Already registered is not a failure?
  }

  SubsystemEntry *entry = &g_subsystems[g_subsystem_count++];
  entry->descriptor = *descriptor;
  entry->instance = NULL;
  entry->initialized = false;
  entry->registered = true;

  LOG_INFO("Registered subsystem: %s", descriptor->name);
  return true;
}

// Simple Bubble Sort for priority (Higher priority first)
static void sort_subsystems(void) {
  for (uint32_t i = 0; i < g_subsystem_count - 1; i++) {
    for (uint32_t j = 0; j < g_subsystem_count - i - 1; j++) {
      if (g_subsystems[j].descriptor.priority <
          g_subsystems[j + 1].descriptor.priority) {
        SubsystemEntry temp = g_subsystems[j];
        g_subsystems[j] = g_subsystems[j + 1];
        g_subsystems[j + 1] = temp;
      }
    }
  }
}

bool subsystem_registry_initialize_all(const EngineConfig *config) {
  sort_subsystems();

  LOG_INFO("Initializing %d subsystems...", g_subsystem_count);

  for (uint32_t i = 0; i < g_subsystem_count; i++) {
    SubsystemEntry *entry = &g_subsystems[i];

    LOG_INFO("Creating subsystem: %s", entry->descriptor.name);
    entry->instance = entry->descriptor.create_func();
    if (!entry->instance) {
      LOG_FATAL("Failed to create subsystem: %s", entry->descriptor.name);
      return false;
    }

    entry->instance->name = entry->descriptor.name;
    entry->instance->id = i;

    if (entry->instance->vtable && entry->instance->vtable->initialize) {
      LOG_INFO("Initializing subsystem: %s", entry->descriptor.name);
      if (!entry->instance->vtable->initialize(entry->instance, config)) {
        LOG_FATAL("Failed to initialize subsystem: %s", entry->descriptor.name);
        return false;
      }
    }

    entry->initialized = true;
  }

  return true;
}

void subsystem_registry_shutdown_all(void) {
  // Shutdown in reverse order
  for (int32_t i = (int32_t)g_subsystem_count - 1; i >= 0; i--) {
    SubsystemEntry *entry = &g_subsystems[i];
    if (entry->initialized && entry->instance) {
      LOG_INFO("Shutting down subsystem: %s", entry->descriptor.name);
      if (entry->instance->vtable && entry->instance->vtable->shutdown) {
        entry->instance->vtable->shutdown(entry->instance);
      }

      if (entry->instance->vtable && entry->instance->vtable->destroy) {
        entry->instance->vtable->destroy(entry->instance);
      }

      entry->instance = NULL;
      entry->initialized = false;
    }
  }
}

void subsystem_registry_update_all(float delta_time) {
  for (uint32_t i = 0; i < g_subsystem_count; i++) {
    SubsystemEntry *entry = &g_subsystems[i];
    if (entry->initialized && entry->instance && entry->instance->vtable &&
        entry->instance->vtable->tick) {
      entry->instance->vtable->tick(entry->instance, delta_time);
    }
  }
}

ISubsystem *subsystem_registry_get(const char *name) {
  if (!name)
    return NULL;
  for (uint32_t i = 0; i < g_subsystem_count; i++) {
    if (strcmp(g_subsystems[i].descriptor.name, name) == 0) {
      return g_subsystems[i].instance;
    }
  }
  return NULL;
}
