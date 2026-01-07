// Swift Bridge Wrapper - Maps Swift-expected names to actual bridge functions
// This allows Swift to call engine_init() which internally calls
// swiftbridge_init()

#include "voxelforge_bridge.h"

// Forward declare the actual bridge functions
extern void swiftbridge_init(void (*on_entity_created)(uint64_t),
                             void (*on_entity_deleted)(uint64_t),
                             void (*on_entity_modified)(uint64_t),
                             void (*on_log_message)(const char *, int32_t,
                                                    const char *),
                             void (*on_scene_loaded)(const char *));
extern void swiftbridge_shutdown(void);
extern void swiftbridge_update(float delta_time);
extern void swiftbridge_get_physics(uint64_t uuid, PhysicsData_C *out_data);

// Wrapper functions with names Swift expects
static inline void
engine_init_swift(void (*on_entity_created)(uint64_t),
                  void (*on_entity_deleted)(uint64_t),
                  void (*on_entity_modified)(uint64_t),
                  void (*on_log_message)(const char *, int32_t, const char *),
                  void (*on_scene_loaded)(const char *)) {
  swiftbridge_init(on_entity_created, on_entity_deleted, on_entity_modified,
                   on_log_message, on_scene_loaded);
}

static inline void engine_shutdown_swift(void) { swiftbridge_shutdown(); }

// engine_update wrapper removed - Use swiftbridge_update directly
#define engine_get_physics engine_get_physics_swift
