// include/ai/npc_schedule.h
//
// Purpose: Defines the NPC scheduling system that manages daily routines,
// time-based activities, and work schedules for NPCs. This system allows
// NPCs to have realistic daily patterns and behaviors based on time of day.
//
// Public APIs:
// - `NPCScheduleSystem`: Main scheduling system for all NPCs
// - `npc_schedule_init`: Initialize scheduling system
// - `npc_schedule_update`: Update all NPC schedules
// - `npc_schedule_add_activity`: Add scheduled activity
// - `npc_schedule_get_current_activity`: Get current NPC activity
// - `npc_schedule_is_time_for_activity`: Check if activity should run
//
// Ownership: The NPCScheduleSystem manages schedule data for all NPCs
// but does not own the entities themselves.
//
// Invariants:
// - Scheduling system must be initialized before use
// - Time values are in 24-hour format (0.0 to 24.0)
// - Activities are evaluated in chronological order
// - Schedule transitions are smooth and interruptible
//
#ifndef AI_NPC_SCHEDULE_H
#define AI_NPC_SCHEDULE_H

#include <ai/npc.h>
#include <ai/npc_types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct World;

// Time of day structure
typedef struct {
  f32 hour;        // 0.0 to 24.0
  f32 minute;      // 0.0 to 60.0
  f32 total_hours;  // Total hours since midnight
} GameTime;

// Activity types for NPC scheduling
typedef enum {
  NPC_ACTIVITY_NONE,
  NPC_ACTIVITY_SLEEPING,        // Sleeping at home
  NPC_ACTIVITY_WORKING,         // At workplace
  NPC_ACTIVITY_EATING,          // Having meal
  NPC_ACTIVITY_SOCIALIZING,      // Talking with others
  NPC_ACTIVITY_SHOPPING,        // Visiting market
  NPC_ACTIVITY_WORSHIP,         // Religious activities
  NPC_ACTIVITY_EXERCISE,        // Physical training
  NPC_ACTIVITY_LEISURE,         // Free time activities
  NPC_ACTIVITY_PATROLLING,      // Guard patrol duty
  NPC_ACTIVITY_FARMING,         // Agricultural work
  NPC_ACTIVITY_MINING,          // Resource extraction
  NPC_ACTIVITY_CRAFTING,       // Creating items
  NPC_ACTIVITY_TEACHING,        // Educational activities
  NPC_ACTIVITY_HEALING,        // Medical activities
  NPC_ACTIVITY_EXPLORING,      // Discovering new areas
  NPC_ACTIVITY_GUARDING,        // Standing guard
  NPC_ACTIVITY_TRAVELING,       // Moving between locations
  NPC_ACTIVITY_CELEBRATING,     // Festivities
  NPC_ACTIVITY_MOURNING,        // Funeral/memorial activities
  NPC_ACTIVITY_COUNT
} NPCActivityType;

// Activity priority levels
typedef enum {
  NPC_PRIORITY_LOW,      // Can be interrupted easily
  NPC_PRIORITY_NORMAL,   // Normal priority
  NPC_PRIORITY_HIGH,     // Important activity
  NPC_PRIORITY_CRITICAL,  // Cannot be interrupted
  NPC_PRIORITY_COUNT
} NPCActivityPriority;

// Location types for activities
typedef enum {
  NPC_LOCATION_NONE,
  NPC_LOCATION_HOME,        // Residence
  NPC_LOCATION_WORKPLACE,   // Job site
  NPC_LOCATION_MARKET,      // Shopping area
  NPC_LOCATION_TAVERN,      // Social gathering
  NPC_LOCATION_TEMPLE,      // Religious site
  NPC_LOCATION_TRAINING,    // Practice area
  NPC_LOCATION_FARM,        // Agricultural field
  NPC_LOCATION_MINE,        // Mining site
  NPC_LOCATION_GUARD_POST,  // Security position
  NPC_LOCATION_CUSTOM,      // User-defined location
  NPC_LOCATION_COUNT
} NPCLocationType;

// Individual activity definition
typedef struct {
  u32 activity_id;
  NPCActivityType type;
  NPCActivityPriority priority;
  
  // Timing
  f32 start_time;     // Hours since midnight (0.0 to 24.0)
  f32 end_time;       // Hours since midnight (0.0 to 24.0)
  f32 duration;       // Activity duration in hours
  
  // Location
  NPCLocationType location_type;
  Vec3 location;
  Entity location_entity; // Specific building/area
  
  // Activity data
  char description[128];
  bool is_recurring;    // Repeats daily
  u32 days_of_week;    // Bitmask for days (bit 0 = Monday, etc.)
  
  // Requirements and conditions
  bool requires_weather_clear;
  bool requires_daylight;
  i32 min_temperature;
  i32 max_temperature;
  
  // Social requirements
  bool requires_company;
  u32 min_participants;
  u32 max_participants;
  
  // Equipment requirements
  bool requires_tools;
  u32 required_tool_type;
  
  // State
  bool is_active;
  bool is_completed;
  f32 start_time_actual;  // When activity actually started
} NPCActivity;

// Daily schedule for an NPC
typedef struct {
  Entity npc_entity;
  NPCType npc_type;
  
  // Current schedule
  NPCActivity activities[16];  // Max 16 activities per day
  u32 activity_count;
  
  // Current state
  NPCActivity *current_activity;
  NPCActivity *next_activity;
  f32 current_activity_start_time;
  
  // Schedule preferences
  bool is_night_owl;      // Prefers night activities
  bool is_early_riser;     // Wakes up early
  f32 work_ethic;         // 0.0 (lazy) to 1.0 (hardworking)
  f32 social_tendency;     // 0.0 (introvert) to 1.0 (extrovert)
  
  // Schedule history
  NPCActivity *completed_activities[8];
  u32 completed_count;
  f32 last_schedule_update;
  
  // Interruption handling
  bool can_be_interrupted;
  NPCActivity *interrupted_activity;
  f32 interruption_time;
} NPCDailySchedule;

// Main scheduling system
typedef struct {
  struct World *ecs;
  NPCSystem *npc_system;
  
  // Active schedules
  NPCDailySchedule *schedules;
  u32 max_schedules;
  u32 active_count;
  
  // Global time tracking
  GameTime current_time;
  f32 time_scale;          // How fast time passes
  f32 day_length;          // Hours in a game day
  
  // Schedule templates
  NPCActivity default_activities[NPC_ACTIVITY_COUNT];
  bool templates_loaded;
  
  // Statistics
  u32 total_activities_completed;
  u32 total_interruptions;
  f32 average_work_hours;
  
  bool is_initialized;
} NPCScheduleSystem;

// Schedule system management
void npc_schedule_init(NPCScheduleSystem *system, struct World *ecs, NPCSystem *npc_system);
void npc_schedule_free(NPCScheduleSystem *system);
void npc_schedule_update(NPCScheduleSystem *system, f32 delta_time);

// Schedule management
NPCDailySchedule *npc_schedule_get_schedule(NPCScheduleSystem *system, Entity entity);
void npc_schedule_create_schedule(NPCScheduleSystem *system, Entity entity, NPCType type);
void npc_schedule_remove_schedule(NPCScheduleSystem *system, Entity entity);

// Activity management
bool npc_schedule_add_activity(NPCScheduleSystem *system, Entity entity, 
                             const NPCActivity *activity);
bool npc_schedule_remove_activity(NPCScheduleSystem *system, Entity entity, u32 activity_id);
NPCActivity *npc_schedule_get_current_activity(NPCScheduleSystem *system, Entity entity);
NPCActivity *npc_schedule_get_next_activity(NPCScheduleSystem *system, Entity entity);

// Time and scheduling utilities
GameTime npc_schedule_get_current_time(NPCScheduleSystem *system);
void npc_schedule_set_time(NPCScheduleSystem *system, f32 hours);
bool npc_schedule_is_time_for_activity(NPCScheduleSystem *system, const NPCActivity *activity);
f32 npc_schedule_time_until_activity(NPCScheduleSystem *system, const NPCActivity *activity);

// Activity execution
void npc_schedule_start_activity(NPCScheduleSystem *system, Entity entity, 
                              NPCActivity *activity);
void npc_schedule_complete_activity(NPCScheduleSystem *system, Entity entity, 
                                NPCActivity *activity);
void npc_schedule_interrupt_activity(NPCScheduleSystem *system, Entity entity, 
                                 const char *reason);

// Schedule templates and presets
void npc_schedule_load_default_templates(NPCScheduleSystem *system);
void npc_schedule_create_villager_schedule(NPCScheduleSystem *system, Entity entity);
void npc_schedule_create_guard_schedule(NPCScheduleSystem *system, Entity entity);
void npc_schedule_create_merchant_schedule(NPCScheduleSystem *system, Entity entity);
void npc_schedule_create_farmer_schedule(NPCScheduleSystem *system, Entity entity);

// Weather and environmental integration
bool npc_schedule_check_weather_conditions(NPCScheduleSystem *system, 
                                        const NPCActivity *activity);
bool npc_schedule_check_time_conditions(NPCScheduleSystem *system, 
                                     const NPCActivity *activity);

// Debug and utilities
void npc_schedule_debug_print_schedule(NPCScheduleSystem *system, Entity entity);
void npc_schedule_debug_print_activity(const NPCActivity *activity);
u32 npc_schedule_get_active_schedule_count(NPCScheduleSystem *system);

#ifdef __cplusplus
}
#endif

#endif // AI_NPC_SCHEDULE_H
