#ifndef QUEST_UI_H
#define QUEST_UI_H

#include "../assets/assets/code/todo_quests.c"
#include "engine/include/ui/ui_manager.h"
#include "engine/include/ui/ui_system.h"
#include <common.h>
#include <time.h>

// Quest UI configuration
typedef struct {
    Vec2 notification_position;
    Vec2 notification_size;
    f32 notification_duration;
    Vec2 quest_log_position;
    Vec2 quest_log_size;
    bool show_quest_log;
    int max_notifications;
} quest_ui_config_t;

// Quest notification types
typedef enum {
    QUEST_NOTIFICATION_STARTED,
    QUEST_NOTIFICATION_COMPLETED,
    QUEST_NOTIFICATION_FAILED,
    QUEST_NOTIFICATION_OBJECTIVE_UPDATED,
    QUEST_NOTIFICATION_OBJECTIVE_COMPLETED
} quest_notification_type_t;

// Quest notification data
typedef struct {
    quest_notification_type_t type;
    int quest_id;
    int objective_index;
    char message[512];
    time_t timestamp;
    bool is_active;
    f32 display_timer;
} quest_notification_t;

// Quest UI context
typedef struct {
    quest_manager_t* quest_manager;
    UIManager* ui_manager;
    quest_ui_config_t config;
    
    // UI elements
    UIElement* quest_log_window;
    UIElement* quest_list_container;
    UIElement* quest_details_panel;
    UIElement* notification_container;
    
    // Notifications
    quest_notification_t* notifications;
    int notification_count;
    int notification_capacity;
    
    // Current player
    int current_player_id;
    
    // Quest tracking
    int tracked_quest_id;
    bool show_objectives_on_hud;
} quest_ui_context_t;

// Quest UI lifecycle
quest_ui_context_t* quest_ui_create(quest_manager_t* quest_manager, UIManager* ui_manager);
void quest_ui_destroy(quest_ui_context_t* context);
void quest_ui_update(quest_ui_context_t* context, f32 delta_time);
void quest_ui_render(quest_ui_context_t* context);

// Quest log management
void quest_ui_show_quest_log(quest_ui_context_t* context, bool show);
void quest_ui_refresh_quest_list(quest_ui_context_t* context);
void quest_ui_show_quest_details(quest_ui_context_t* context, int quest_id);
void quest_ui_track_quest(quest_ui_context_t* context, int quest_id);

// Notification system
void quest_ui_show_notification(quest_ui_context_t* context, quest_notification_type_t type, 
                               int quest_id, int objective_index, const char* custom_message);
void quest_ui_clear_notifications(quest_ui_context_t* context);

// Event handlers (called by quest manager)
void quest_ui_on_quest_started(quest_ui_context_t* context, int player_id, int quest_id);
void quest_ui_on_quest_completed(quest_ui_context_t* context, int player_id, int quest_id);
void quest_ui_on_quest_failed(quest_ui_context_t* context, int player_id, int quest_id);
void quest_ui_on_objective_updated(quest_ui_context_t* context, int player_id, int quest_id, 
                                   int objective_index, int progress);
void quest_ui_on_objective_completed(quest_ui_context_t* context, int player_id, int quest_id, 
                                     int objective_index);

// UI creation helpers
UIElement* quest_ui_create_notification_element(quest_ui_context_t* context, const quest_notification_t* notification);
UIElement* quest_ui_create_quest_log_window(quest_ui_context_t* context);
UIElement* quest_ui_create_quest_list_item(quest_ui_context_t* context, const player_quest_t* player_quest, const quest_t* quest);
UIElement* quest_ui_create_quest_details_panel(quest_ui_context_t* context, const quest_t* quest, const player_quest_t* player_quest);
UIElement* quest_ui_create_objective_progress_bar(quest_ui_context_t* context, const quest_objective_t* objective, int current_progress);

// Utility functions
const char* quest_ui_get_notification_text(quest_notification_type_t type, const quest_t* quest, 
                                          const quest_objective_t* objective, const char* custom_message);
Vec4 quest_ui_get_notification_color(quest_notification_type_t type);
const char* quest_ui_format_time_remaining(time_t start_time, int time_limit_seconds);
void quest_ui_set_player(quest_ui_context_t* context, int player_id);

// Configuration
void quest_ui_set_config(quest_ui_context_t* context, const quest_ui_config_t* config);
quest_ui_config_t quest_ui_get_default_config(void);

#endif // QUEST_UI_H
