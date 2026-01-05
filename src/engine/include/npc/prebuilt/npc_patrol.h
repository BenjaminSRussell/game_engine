#pragma once

#include "common.h"
#include <stdint.h>
#include <stddef.h>

// Forward declarations
typedef struct PatrolRoute PatrolRoute;
typedef struct PatrolWaypoint PatrolWaypoint;

// API functions
PatrolRoute* patrol_route_create(const char* name);
void patrol_route_destroy(PatrolRoute* route);
int patrol_route_add_waypoint(PatrolRoute* route, const PatrolWaypoint* waypoint);
bool patrol_route_insert_waypoint(PatrolRoute* route, size_t index, const PatrolWaypoint* waypoint);
bool patrol_route_remove_waypoint(PatrolRoute* route, size_t index);
bool patrol_attach_to_entity(uint64_t entity_id, PatrolRoute* route);
void patrol_detach_from_entity(uint64_t entity_id);
void patrol_system_update(float delta_time);
void patrol_interrupt(uint64_t entity_id);
void patrol_resume(uint64_t entity_id);
bool patrol_route_serialize(const PatrolRoute* route, void** out_buffer, size_t* out_size);
PatrolRoute* patrol_route_deserialize(const void* buffer, size_t size);
