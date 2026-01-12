#include "quest_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default configuration
quest_ui_config_t quest_ui_get_default_config(void) {
    quest_ui_config_t config = {0};
    config.notification_position = (Vec2){10.0f, 100.0f};
    config.notification_size = (Vec2){400.0f, 80.0f};
    config.notification_duration = 5.0f;
    config.quest_log_position = (Vec2){50.0f, 50.0f};
    config.quest_log_size = (Vec2){800.0f, 600.0f};
    config.show_quest_log = false;
    config.max_notifications = 5;
    return config;
}

// Quest UI creation
quest_ui_context_t* quest_ui_create(quest_manager_t* quest_manager, UIManager* ui_manager) {
    quest_ui_context_t* context = malloc(sizeof(quest_ui_context_t));
    if (!context) return NULL;
    
    memset(context, 0, sizeof(quest_ui_context_t));
    
    context->quest_manager = quest_manager;
    context->ui_manager = ui_manager;
    context->config = quest_ui_get_default_config();
    context->current_player_id = 1; // Default player
    context->tracked_quest_id = -1;
    context->show_objectives_on_hud = true;
    
    // Initialize notifications
    context->notification_capacity = context->config.max_notifications;
    context->notifications = malloc(sizeof(quest_notification_t) * context->notification_capacity);
    context->notification_count = 0;
    
    // Create UI elements
    context->quest_log_window = quest_ui_create_quest_log_window(context);
    context->notification_container = ui_manager->create_element(ui_manager, &(UIElementDesc){
        .type = UI_ELEMENT_PANEL,
        .id = "quest_notification_container",
        .position = context->config.notification_position,
        .size = (Vec2){context->config.notification_size.x, 0},
        .visible = true,
        .enabled = true
    });
    
    return context;
}

void quest_ui_destroy(quest_ui_context_t* context) {
    if (!context) return;
    
    if (context->notifications) {
        free(context->notifications);
    }
    
    // UI elements are managed by the UI manager, so we don't need to destroy them here
    
    free(context);
}

void quest_ui_update(quest_ui_context_t* context, f32 delta_time) {
    if (!context) return;
    
    // Update notifications
    for (int i = context->notification_count - 1; i >= 0; i--) {
        quest_notification_t* notification = &context->notifications[i];
        
        if (notification->is_active) {
            notification->display_timer -= delta_time;
            
            if (notification->display_timer <= 0.0f) {
                notification->is_active = false;
                // Remove notification by shifting remaining elements
                for (int j = i; j < context->notification_count - 1; j++) {
                    context->notifications[j] = context->notifications[j + 1];
                }
                context->notification_count--;
            }
        }
    }
    
    // Update notification container height
    f32 total_height = context->notification_count * (context->config.notification_size.y + 10.0f);
    context->notification_container->size.y = total_height;
    
    // Refresh quest list if quest log is visible
    if (context->config.show_quest_log) {
        quest_ui_refresh_quest_list(context);
    }
}

void quest_ui_render(quest_ui_context_t* context) {
    if (!context) return;
    
    // Render notifications
    for (int i = 0; i < context->notification_count; i++) {
        quest_notification_t* notification = &context->notifications[i];
        if (notification->is_active) {
            // Notification rendering is handled by the UI manager
        }
    }
}

// Quest log management
void quest_ui_show_quest_log(quest_ui_context_t* context, bool show) {
    if (!context || !context->quest_log_window) return;
    
    context->config.show_quest_log = show;
    context->quest_log_window->visible = show;
    
    if (show) {
        quest_ui_refresh_quest_list(context);
    }
}

void quest_ui_refresh_quest_list(quest_ui_context_t* context) {
    if (!context || !context->quest_list_container) return;
    
    // Clear existing quest items
    // Note: In a real implementation, you'd need to properly destroy child elements
    
    int active_count = 0;
    player_quest_t* active_quests = quest_manager_get_active_quests(context->quest_manager, 
                                                                   context->current_player_id, &active_count);
    
    int completed_count = 0;
    player_quest_t* completed_quests = NULL;
    // Note: You'd need a function to get completed quests
    
    // Add active quests
    for (int i = 0; i < active_count; i++) {
        quest_t* quest = quest_manager_get_quest(context->quest_manager, active_quests[i].quest_id);
        if (quest) {
            UIElement* quest_item = quest_ui_create_quest_list_item(context, &active_quests[i], quest);
            if (quest_item) {
                // Add to quest list container
                // ui_manager->add_child(context->ui_manager, context->quest_list_container, quest_item);
            }
        }
    }
    
    free(active_quests);
    free(completed_quests);
}

void quest_ui_show_quest_details(quest_ui_context_t* context, int quest_id) {
    if (!context) return;
    
    quest_t* quest = quest_manager_get_quest(context->quest_manager, quest_id);
    if (!quest) return;
    
    // Find player quest data
    player_quest_t* player_quest = NULL;
    for (int i = 0; i < context->quest_manager->player_quest_count; i++) {
        if (context->quest_manager->player_quests[i].quest_id == quest_id) {
            player_quest = &context->quest_manager->player_quests[i];
            break;
        }
    }
    
    if (context->quest_details_panel) {
        // Update existing details panel
        // In a real implementation, you'd update the panel content
    } else {
        // Create new details panel
        context->quest_details_panel = quest_ui_create_quest_details_panel(context, quest, player_quest);
    }
}

void quest_ui_track_quest(quest_ui_context_t* context, int quest_id) {
    if (!context) return;
    
    context->tracked_quest_id = quest_id;
    
    // Update quest tracking in the quest manager
    for (int i = 0; i < context->quest_manager->player_quest_count; i++) {
        if (context->quest_manager->player_quests[i].quest_id == quest_id) {
            context->quest_manager->player_quests[i].is_tracked = true;
        } else {
            context->quest_manager->player_quests[i].is_tracked = false;
        }
    }
}

// Notification system
void quest_ui_show_notification(quest_ui_context_t* context, quest_notification_type_t type, 
                               int quest_id, int objective_index, const char* custom_message) {
    if (!context || context->notification_count >= context->notification_capacity) return;
    
    quest_notification_t* notification = &context->notifications[context->notification_count];
    memset(notification, 0, sizeof(quest_notification_t));
    
    notification->type = type;
    notification->quest_id = quest_id;
    notification->objective_index = objective_index;
    notification->timestamp = time(NULL);
    notification->is_active = true;
    notification->display_timer = context->config.notification_duration;
    
    quest_t* quest = quest_manager_get_quest(context->quest_manager, quest_id);
    quest_objective_t* objective = (quest && objective_index >= 0 && objective_index < quest->objective_count) 
                                  ? &quest->objectives[objective_index] : NULL;
    
    const char* notification_text = quest_ui_get_notification_text(type, quest, objective, custom_message);
    strncpy(notification->message, notification_text, sizeof(notification->message) - 1);
    
    // Create notification UI element
    UIElement* notification_element = quest_ui_create_notification_element(context, notification);
    if (notification_element) {
        // Position notification
        Vec2 position = context->config.notification_position;
        position.y += context->notification_count * (context->config.notification_size.y + 10.0f);
        notification_element->position = position;
        
        // Add to notification container
        // ui_manager->add_child(context->ui_manager, context->notification_container, notification_element);
    }
    
    context->notification_count++;
}

void quest_ui_clear_notifications(quest_ui_context_t* context) {
    if (!context) return;
    
    for (int i = 0; i < context->notification_count; i++) {
        context->notifications[i].is_active = false;
    }
    context->notification_count = 0;
}

// Event handlers
void quest_ui_on_quest_started(quest_ui_context_t* context, int player_id, int quest_id) {
    if (!context || player_id != context->current_player_id) return;
    
    quest_ui_show_notification(context, QUEST_NOTIFICATION_STARTED, quest_id, -1, NULL);
    
    if (context->config.show_quest_log) {
        quest_ui_refresh_quest_list(context);
    }
}

void quest_ui_on_quest_completed(quest_ui_context_t* context, int player_id, int quest_id) {
    if (!context || player_id != context->current_player_id) return;
    
    quest_ui_show_notification(context, QUEST_NOTIFICATION_COMPLETED, quest_id, -1, NULL);
    
    if (context->config.show_quest_log) {
        quest_ui_refresh_quest_list(context);
    }
}

void quest_ui_on_quest_failed(quest_ui_context_t* context, int player_id, int quest_id) {
    if (!context || player_id != context->current_player_id) return;
    
    quest_ui_show_notification(context, QUEST_NOTIFICATION_FAILED, quest_id, -1, NULL);
    
    if (context->config.show_quest_log) {
        quest_ui_refresh_quest_list(context);
    }
}

void quest_ui_on_objective_updated(quest_ui_context_t* context, int player_id, int quest_id, 
                                   int objective_index, int progress) {
    if (!context || player_id != context->current_player_id) return;
    
    quest_t* quest = quest_manager_get_quest(context->quest_manager, quest_id);
    if (quest && objective_index >= 0 && objective_index < quest->objective_count) {
        quest_objective_t* objective = &quest->objectives[objective_index];
        
        if (progress >= objective->required_amount) {
            quest_ui_on_objective_completed(context, player_id, quest_id, objective_index);
        } else {
            quest_ui_show_notification(context, QUEST_NOTIFICATION_OBJECTIVE_UPDATED, 
                                     quest_id, objective_index, NULL);
        }
    }
}

void quest_ui_on_objective_completed(quest_ui_context_t* context, int player_id, int quest_id, 
                                     int objective_index) {
    if (!context || player_id != context->current_player_id) return;
    
    quest_ui_show_notification(context, QUEST_NOTIFICATION_OBJECTIVE_COMPLETED, 
                             quest_id, objective_index, NULL);
}

// UI creation helpers
UIElement* quest_ui_create_notification_element(quest_ui_context_t* context, const quest_notification_t* notification) {
    if (!context || !notification || !context->ui_manager) return NULL;
    
    Vec4 color = quest_ui_get_notification_color(notification->type);
    
    UIElementDesc desc = {
        .type = UI_ELEMENT_PANEL,
        .id = "quest_notification",
        .position = {0, 0},
        .size = context->config.notification_size,
        .background_color = color,
        .visible = true,
        .enabled = true
    };
    
    UIElement* element = context->ui_manager->create_element(context->ui_manager, &desc);
    
    // Add text label
    UIElementDesc text_desc = {
        .type = UI_ELEMENT_LABEL,
        .id = "notification_text",
        .position = {10.0f, 10.0f},
        .size = {context->config.notification_size.x - 20.0f, 60.0f},
        .text = notification->message,
        .color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* text_element = context->ui_manager->create_element(context->ui_manager, &text_desc);
    // ui_manager->add_child(context->ui_manager, element, text_element);
    
    return element;
}

UIElement* quest_ui_create_quest_log_window(quest_ui_context_t* context) {
    if (!context || !context->ui_manager) return NULL;
    
    UIElementDesc desc = {
        .type = UI_ELEMENT_PANEL,
        .id = "quest_log_window",
        .position = context->config.quest_log_position,
        .size = context->config.quest_log_size,
        .background_color = (Vec4){0.1f, 0.1f, 0.1f, 0.9f},
        .visible = context->config.show_quest_log,
        .enabled = true
    };
    
    UIElement* window = context->ui_manager->create_element(context->ui_manager, &desc);
    
    // Create title
    UIElementDesc title_desc = {
        .type = UI_ELEMENT_LABEL,
        .id = "quest_log_title",
        .position = {10.0f, 10.0f},
        .size = {200.0f, 30.0f},
        .text = "Quest Log",
        .color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* title = context->ui_manager->create_element(context->ui_manager, &title_desc);
    // ui_manager->add_child(context->ui_manager, window, title);
    
    // Create quest list container
    UIElementDesc list_desc = {
        .type = UI_ELEMENT_SCROLL_VIEW,
        .id = "quest_list_container",
        .position = {10.0f, 50.0f},
        .size = {context->config.quest_log_size.x - 20.0f, context->config.quest_log_size.y - 60.0f},
        .visible = true,
        .enabled = true
    };
    
    context->quest_list_container = context->ui_manager->create_element(context->ui_manager, &list_desc);
    // ui_manager->add_child(context->ui_manager, window, context->quest_list_container);
    
    return window;
}

UIElement* quest_ui_create_quest_list_item(quest_ui_context_t* context, const player_quest_t* player_quest, const quest_t* quest) {
    if (!context || !player_quest || !quest || !context->ui_manager) return NULL;
    
    UIElementDesc desc = {
        .type = UI_ELEMENT_BUTTON,
        .id = "quest_item",
        .position = {0, 0},
        .size = {context->config.quest_log_size.x - 40.0f, 80.0f},
        .background_color = (Vec4){0.2f, 0.2f, 0.2f, 0.8f},
        .visible = true,
        .enabled = true,
        .user_data = (void*)(intptr_t)quest->id
    };
    
    UIElement* item = context->ui_manager->create_element(context->ui_manager, &desc);
    
    // Add quest title
    char title_text[512];
    snprintf(title_text, sizeof(title_text), "%s - %s", quest->title, quest_status_to_string(player_quest->status));
    
    UIElementDesc title_desc = {
        .type = UI_ELEMENT_LABEL,
        .id = "quest_title",
        .position = {10.0f, 5.0f},
        .size = {300.0f, 25.0f},
        .text = title_text,
        .color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* title = context->ui_manager->create_element(context->ui_manager, &title_desc);
    // ui_manager->add_child(context->ui_manager, item, title);
    
    // Add progress bar if quest is active
    if (player_quest->status == QUEST_STATUS_active) {
        f32 progress = quest_get_completion_percentage(player_quest, quest);
        
        UIElementDesc progress_desc = {
            .type = UI_ELEMENT_PROGRESS_BAR,
            .id = "quest_progress",
            .position = {10.0f, 35.0f},
            .size = {200.0f, 10.0f},
            .color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f},
            .visible = true,
            .enabled = true
        };
        
        UIElement* progress_bar = context->ui_manager->create_element(context->ui_manager, &progress_desc);
        // ui_manager->add_child(context->ui_manager, item, progress_bar);
    }
    
    return item;
}

UIElement* quest_ui_create_quest_details_panel(quest_ui_context_t* context, const quest_t* quest, const player_quest_t* player_quest) {
    if (!context || !quest || !context->ui_manager) return NULL;
    
    UIElementDesc desc = {
        .type = UI_ELEMENT_PANEL,
        .id = "quest_details_panel",
        .position = {context->config.quest_log_position.x + context->config.quest_log_size.x + 20.0f, 
                    context->config.quest_log_position.y},
        .size = {400.0f, 600.0f},
        .background_color = (Vec4){0.15f, 0.15f, 0.15f, 0.9f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* panel = context->ui_manager->create_element(context->ui_manager, &desc);
    
    // Add quest title
    UIElementDesc title_desc = {
        .type = UI_ELEMENT_LABEL,
        .id = "quest_details_title",
        .position = {10.0f, 10.0f},
        .size = {380.0f, 30.0f},
        .text = quest->title,
        .color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* title = context->ui_manager->create_element(context->ui_manager, &title_desc);
    // ui_manager->add_child(context->ui_manager, panel, title);
    
    // Add quest description
    UIElementDesc desc_desc = {
        .type = UI_ELEMENT_LABEL,
        .id = "quest_description",
        .position = {10.0f, 50.0f},
        .size = {380.0f, 100.0f},
        .text = quest->description,
        .color = (Vec4){0.8f, 0.8f, 0.8f, 1.0f},
        .visible = true,
        .enabled = true
    };
    
    UIElement* description = context->ui_manager->create_element(context->ui_manager, &desc_desc);
    // ui_manager->add_child(context->ui_manager, panel, description);
    
    // Add objectives
    // (In a real implementation, you'd iterate through objectives and create UI elements for each)
    
    return panel;
}

// Utility functions
const char* quest_ui_get_notification_text(quest_notification_type_t type, const quest_t* quest, 
                                          const quest_objective_t* objective, const char* custom_message) {
    static char buffer[512];
    
    if (custom_message) {
        strncpy(buffer, custom_message, sizeof(buffer) - 1);
        return buffer;
    }
    
    switch (type) {
        case QUEST_NOTIFICATION_STARTED:
            if (quest) {
                snprintf(buffer, sizeof(buffer), "Quest Started: %s", quest->title);
            } else {
                strcpy(buffer, "Quest Started");
            }
            break;
            
        case QUEST_NOTIFICATION_COMPLETED:
            if (quest) {
                snprintf(buffer, sizeof(buffer), "Quest Completed: %s", quest->title);
            } else {
                strcpy(buffer, "Quest Completed");
            }
            break;
            
        case QUEST_NOTIFICATION_FAILED:
            if (quest) {
                snprintf(buffer, sizeof(buffer), "Quest Failed: %s", quest->title);
            } else {
                strcpy(buffer, "Quest Failed");
            }
            break;
            
        case QUEST_NOTIFICATION_OBJECTIVE_UPDATED:
            if (quest && objective) {
                snprintf(buffer, sizeof(buffer), "%s: Progress updated", objective->description);
            } else {
                strcpy(buffer, "Objective Progress Updated");
            }
            break;
            
        case QUEST_NOTIFICATION_OBJECTIVE_COMPLETED:
            if (quest && objective) {
                snprintf(buffer, sizeof(buffer), "%s: Completed!", objective->description);
            } else {
                strcpy(buffer, "Objective Completed");
            }
            break;
            
        default:
            strcpy(buffer, "Quest Notification");
            break;
    }
    
    return buffer;
}

Vec4 quest_ui_get_notification_color(quest_notification_type_t type) {
    switch (type) {
        case QUEST_NOTIFICATION_STARTED:
            return (Vec4){0.0f, 0.6f, 1.0f, 0.9f}; // Blue
        case QUEST_NOTIFICATION_COMPLETED:
            return (Vec4){0.0f, 1.0f, 0.0f, 0.9f}; // Green
        case QUEST_NOTIFICATION_FAILED:
            return (Vec4){1.0f, 0.0f, 0.0f, 0.9f}; // Red
        case QUEST_NOTIFICATION_OBJECTIVE_UPDATED:
            return (Vec4){1.0f, 1.0f, 0.0f, 0.9f}; // Yellow
        case QUEST_NOTIFICATION_OBJECTIVE_COMPLETED:
            return (Vec4){0.0f, 1.0f, 0.8f, 0.9f}; // Cyan
        default:
            return (Vec4){0.5f, 0.5f, 0.5f, 0.9f}; // Gray
    }
}

void quest_ui_set_player(quest_ui_context_t* context, int player_id) {
    if (!context) return;
    
    context->current_player_id = player_id;
    quest_ui_refresh_quest_list(context);
}

void quest_ui_set_config(quest_ui_context_t* context, const quest_ui_config_t* config) {
    if (!context || !config) return;
    
    context->config = *config;
}
