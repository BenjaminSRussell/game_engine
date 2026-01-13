// NPC scheduling implementation
#include <ai/npc_schedule.h>
#include <ai/npc.h>
#include <ecs/components/npc.h>
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

extern f32 g_game_time;

void npc_schedule_init(NPCScheduleSystem *system, struct World *ecs, NPCSystem *npc_system) {
  if (!system || !ecs || !npc_system) return;
  
  memset(system, 0, sizeof(NPCScheduleSystem));
  system->ecs = ecs;
  system->npc_system = npc_system;
  system->schedules = malloc(sizeof(NPCDailySchedule) * 256);
  system->max_schedules = 256;
  system->current_time.hour = 6.0f; // Start at 6 AM
  system->time_scale = 1.0f;
  system->day_length = 24.0f;
  
  npc_schedule_load_default_templates(system);
  system->is_initialized = true;
  
  LOG_INFO("NPC scheduling system initialized");
}

void npc_schedule_free(NPCScheduleSystem *system) {
  if (!system) return;
  
  free(system->schedules);
  memset(system, 0, sizeof(NPCScheduleSystem));
  LOG_INFO("NPC scheduling system freed");
}

void npc_schedule_update(NPCScheduleSystem *system, f32 delta_time) {
  if (!system || !system->is_initialized) return;
  
  // Update global time
  system->current_time.total_hours += delta_time * system->time_scale / 3600.0f;
  system->current_time.hour = fmod(system->current_time.total_hours, system->day_length);
  system->current_time.minute = fmod(system->current_time.hour * 60.0f, 60.0f);
  
  // Update all schedules
  for (u32 i = 0; i < system->active_count; i++) {
    NPCDailySchedule *schedule = &system->schedules[i];
    
    // Check if current activity is complete
    if (schedule->current_activity && 
        system->current_time.hour >= schedule->current_activity->end_time) {
      npc_schedule_complete_activity(system, schedule->npc_entity, 
                                   schedule->current_activity);
    }
    
    // Check if we should start next activity
    if (!schedule->current_activity && schedule->next_activity &&
        npc_schedule_is_time_for_activity(system, schedule->next_activity)) {
      npc_schedule_start_activity(system, schedule->npc_entity, 
                               schedule->next_activity);
    }
  }
}

void npc_schedule_create_schedule(NPCScheduleSystem *system, Entity entity, NPCType type) {
  if (!system || system->active_count >= system->max_schedules) return;
  
  NPCDailySchedule *schedule = &system->schedules[system->active_count++];
  memset(schedule, 0, sizeof(NPCDailySchedule));
  
  schedule->npc_entity = entity;
  schedule->npc_type = type;
  schedule->work_ethic = 0.7f; // Default work ethic
  schedule->social_tendency = 0.5f; // Default social
  
  // Create type-specific schedule
  switch (type) {
  case NPC_TYPE_VILLAGER:
    npc_schedule_create_villager_schedule(system, entity);
    break;
  case NPC_TYPE_ZOMBIE:
  case NPC_TYPE_SKELETON:
    // Hostile mobs don't have schedules
    break;
  default:
    npc_schedule_create_villager_schedule(system, entity);
    break;
  }
}

void npc_schedule_create_villager_schedule(NPCScheduleSystem *system, Entity entity) {
  NPCDailySchedule *schedule = npc_schedule_get_schedule(system, entity);
  if (!schedule) return;
  
  // Morning: Wake up and work
  NPCActivity work = {
    .type = NPC_ACTIVITY_WORKING,
    .priority = NPC_PRIORITY_NORMAL,
    .start_time = 6.0f,
    .end_time = 12.0f,
    .location_type = NPC_LOCATION_WORKPLACE,
    .is_recurring = true,
    .requires_daylight = true
  };
  npc_schedule_add_activity(system, entity, &work);
  
  // Afternoon: Lunch and social time
  NPCActivity lunch = {
    .type = NPC_ACTIVITY_EATING,
    .priority = NPC_PRIORITY_NORMAL,
    .start_time = 12.0f,
    .end_time = 13.0f,
    .location_type = NPC_LOCATION_HOME,
    .is_recurring = true
  };
  npc_schedule_add_activity(system, entity, &lunch);
  
  // Evening: Leisure and social
  NPCActivity leisure = {
    .type = NPC_ACTIVITY_SOCIALIZING,
    .priority = NPC_PRIORITY_LOW,
    .start_time = 18.0f,
    .end_time = 21.0f,
    .location_type = NPC_LOCATION_TAVERN,
    .is_recurring = true,
    .requires_company = true
  };
  npc_schedule_add_activity(system, entity, &leisure);
  
  // Night: Sleep
  NPCActivity sleep = {
    .type = NPC_ACTIVITY_SLEEPING,
    .priority = NPC_PRIORITY_HIGH,
    .start_time = 21.0f,
    .end_time = 6.0f,
    .location_type = NPC_LOCATION_HOME,
    .is_recurring = true
  };
  npc_schedule_add_activity(system, entity, &sleep);
}

bool npc_schedule_add_activity(NPCScheduleSystem *system, Entity entity, 
                             const NPCActivity *activity) {
  NPCDailySchedule *schedule = npc_schedule_get_schedule(system, entity);
  if (!schedule || schedule->activity_count >= 16) return false;
  
  schedule->activities[schedule->activity_count] = *activity;
  schedule->activities[schedule->activity_count].activity_id = schedule->activity_count;
  schedule->activity_count++;
  
  // Sort activities by start time
  for (u32 i = schedule->activity_count - 1; i > 0; i--) {
    if (schedule->activities[i].start_time < schedule->activities[i-1].start_time) {
      NPCActivity temp = schedule->activities[i];
      schedule->activities[i] = schedule->activities[i-1];
      schedule->activities[i-1] = temp;
    }
  }
  
  return true;
}

bool npc_schedule_is_time_for_activity(NPCScheduleSystem *system, const NPCActivity *activity) {
  if (!activity) return false;
  
  f32 current_hour = system->current_time.hour;
  
  // Handle activities that span midnight
  if (activity->start_time > activity->end_time) {
    return current_hour >= activity->start_time || current_hour < activity->end_time;
  } else {
    return current_hour >= activity->start_time && current_hour < activity->end_time;
  }
}

void npc_schedule_start_activity(NPCScheduleSystem *system, Entity entity, 
                              NPCActivity *activity) {
  NPCDailySchedule *schedule = npc_schedule_get_schedule(system, entity);
  if (!schedule || !activity) return;
  
  schedule->current_activity = activity;
  schedule->current_activity_start_time = system->current_time.total_hours;
  schedule->current_activity->is_active = true;
  schedule->current_activity->start_time_actual = system->current_time.total_hours;
  
  // Update NPC state
  NPCComponent *npc = (NPCComponent *)world_get_component(
      system->ecs, entity, NPC_COMPONENT_ID);
  if (npc) {
    npc->current_activity = activity->type;
  }
  
  LOG_DEBUG("NPC %u started activity %s", entity.id, activity->description);
}

void npc_schedule_complete_activity(NPCScheduleSystem *system, Entity entity, 
                                NPCActivity *activity) {
  NPCDailySchedule *schedule = npc_schedule_get_schedule(system, entity);
  if (!schedule || !activity) return;
  
  activity->is_active = false;
  activity->is_completed = true;
  
  // Add to completed activities
  if (schedule->completed_count < 8) {
    schedule->completed_activities[schedule->completed_count++] = activity;
  }
  
  schedule->current_activity = NULL;
  system->total_activities_completed++;
  
  LOG_DEBUG("NPC %u completed activity %s", entity.id, activity->description);
}

NPCDailySchedule *npc_schedule_get_schedule(NPCScheduleSystem *system, Entity entity) {
  if (!system) return NULL;
  
  for (u32 i = 0; i < system->active_count; i++) {
    if (system->schedules[i].npc_entity.id == entity.id) {
      return &system->schedules[i];
    }
  }
  return NULL;
}

void npc_schedule_load_default_templates(NPCScheduleSystem *system) {
  // Load default activity templates
  system->templates_loaded = true;
  LOG_DEBUG("Loaded default schedule templates");
}

u32 npc_schedule_get_active_schedule_count(NPCScheduleSystem *system) {
  return system ? system->active_count : 0;
}
