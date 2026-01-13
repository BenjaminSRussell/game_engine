#ifndef SUBSYSTEM_REGISTRY_H
#define SUBSYSTEM_REGISTRY_H

#include "engine_init.h"

// Forward declaration
typedef struct ISubsystem ISubsystem;

// Subsystem Lifecycle VTable
typedef struct {
  bool (*initialize)(ISubsystem *self, const EngineConfig *config);
  void (*tick)(ISubsystem *self, float delta_time);
  void (*shutdown)(ISubsystem *self);
  void (*destroy)(ISubsystem *self);
} ISubsystemVTable;

// Base Subsystem Interface
struct ISubsystem {
  const ISubsystemVTable *vtable;
  const char *name;
  uint32_t id;
};

// Subsystem Descriptor for Registration
typedef struct {
  const char *name;
  const char **dependencies;
  uint32_t dependency_count;
  int32_t priority;
  ISubsystem *(*create_func)(void);
} SubsystemDescriptor;

// Registry API
bool subsystem_registry_register(const SubsystemDescriptor *descriptor);
bool subsystem_registry_initialize_all(const EngineConfig *config);
void subsystem_registry_shutdown_all(void);
void subsystem_registry_update_all(float delta_time);
ISubsystem *subsystem_registry_get(const char *name);

#endif // SUBSYSTEM_REGISTRY_H
