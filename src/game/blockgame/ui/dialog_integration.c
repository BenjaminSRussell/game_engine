/*
 * dialog_integration.c
 * Game-Specific Dialog Integration Helpers
 * Convenience wrappers for common game dialogs
 */

#include "dialog_integration.h"
#include "engine/ui/modal_manager.h"
#include "engine/include/core/logger.h"

/* ============================================================================
 * GAME STATE
 * ============================================================================ */

static bool g_game_paused_for_dialog = false;
static UIDialog* g_active_game_dialog = NULL;

/* ============================================================================
 * INTERNAL CALLBACK WRAPPERS
 * ============================================================================ */

typedef struct {
    void (*on_confirm)(void);
    void (*on_cancel)(void);
} ConfirmDialogCallbacks;

static void dialog_confirm_result_callback(UIDialog* dialog, DialogResult result, void* user_data) {
    ConfirmDialogCallbacks* callbacks = (ConfirmDialogCallbacks*)user_data;
    if (!callbacks) return;

    switch (result) {
        case DIALOG_RESULT_OK:
        case DIALOG_RESULT_YES:
            if (callbacks->on_confirm) callbacks->on_confirm();
            break;
        case DIALOG_RESULT_CANCEL:
        case DIALOG_RESULT_NO:
            if (callbacks->on_cancel) callbacks->on_cancel();
            break;
        default:
            break;
    }
}

static void dialog_confirm_close_callback(UIDialog* dialog, void* user_data) {
    ConfirmDialogCallbacks* callbacks = (ConfirmDialogCallbacks*)user_data;
    if (callbacks) {
        free(callbacks);
    }
}

/* ============================================================================
 * GAME-SPECIFIC DIALOG TYPES
 * ============================================================================ */

UIDialog* game_dialog_exit_confirm(
    void (*on_confirm)(void),
    void (*on_cancel)(void)
) {
    UIDialog* dialog = dialog_confirm(
        "Exit Game",
        "Are you sure you want to exit?\nYour progress will be saved."
    );

    if (dialog && (on_confirm || on_cancel)) {
        ConfirmDialogCallbacks* callbacks = malloc(sizeof(ConfirmDialogCallbacks));
        if (callbacks) {
            callbacks->on_confirm = on_confirm;
            callbacks->on_cancel = on_cancel;
            dialog_set_result_callback(dialog, dialog_confirm_result_callback, callbacks);
            dialog_set_close_callback(dialog, dialog_confirm_close_callback, callbacks);
        }
    }

    return dialog;
}

UIDialog* game_dialog_save_game(
    const char* slot_name,
    void (*on_confirm)(const char* slot_name),
    void (*on_cancel)(void)
) {
    UIDialog* dialog = dialog_confirm(
        "Save Game",
        slot_name ? slot_name : "Save progress?"
    );

    if (dialog) {
        dialog_set_modal(dialog, true);
    }

    return dialog;
}

UIDialog* game_dialog_load_game(
    const char* slot_name,
    void (*on_confirm)(const char* slot_name),
    void (*on_cancel)(void)
) {
    UIDialog* dialog = dialog_confirm(
        "Load Game",
        slot_name ? slot_name : "Load this save?"
    );

    if (dialog) {
        dialog_set_modal(dialog, true);
    }

    return dialog;
}

UIDialog* game_dialog_settings_apply(
    void (*on_apply)(void),
    void (*on_cancel)(void)
) {
    UIDialog* dialog = dialog_create(
        "SettingsApply",
        "Apply Settings",
        "Apply these settings changes?",
        DIALOG_TYPE_CONFIRM
    );

    if (dialog) {
        dialog_set_buttons(dialog, DIALOG_BUTTON_APPLY | DIALOG_BUTTON_RESET);
        dialog_set_modal(dialog, true);

        if (on_apply || on_cancel) {
            ConfirmDialogCallbacks* callbacks = malloc(sizeof(ConfirmDialogCallbacks));
            if (callbacks) {
                callbacks->on_confirm = on_apply;
                callbacks->on_cancel = on_cancel;
                dialog_set_result_callback(dialog, dialog_confirm_result_callback, callbacks);
                dialog_set_close_callback(dialog, dialog_confirm_close_callback, callbacks);
            }
        }
    }

    return dialog;
}

UIDialog* game_dialog_craft_confirm(
    const char* item_name,
    uint32_t quantity,
    void (*on_craft)(void),
    void (*on_cancel)(void)
) {
    char message[256];
    snprintf(message, sizeof(message), "Craft %u x %s?", quantity, item_name ? item_name : "item");

    UIDialog* dialog = dialog_confirm("Craft Item", message);

    if (dialog && (on_craft || on_cancel)) {
        ConfirmDialogCallbacks* callbacks = malloc(sizeof(ConfirmDialogCallbacks));
        if (callbacks) {
            callbacks->on_confirm = on_craft;
            callbacks->on_cancel = on_cancel;
            dialog_set_result_callback(dialog, dialog_confirm_result_callback, callbacks);
            dialog_set_close_callback(dialog, dialog_confirm_close_callback, callbacks);
        }
    }

    return dialog;
}

UIDialog* game_dialog_death_notification(
    const char* killer_name,
    void (*on_respawn)(void)
) {
    char message[256];
    if (killer_name) {
        snprintf(message, sizeof(message), "You were defeated by %s", killer_name);
    } else {
        snprintf(message, sizeof(message), "You were defeated!");
    }

    UIDialog* dialog = dialog_error("You Died", message);

    if (dialog && on_respawn) {
        dialog_set_result_callback(
            dialog,
            (void (*)(UIDialog*, DialogResult, void*))on_respawn,
            NULL
        );
    }

    return dialog;
}

UIDialog* game_dialog_trade_confirm(
    const char* npc_name,
    void (*on_confirm)(void),
    void (*on_cancel)(void)
) {
    char message[256];
    snprintf(message, sizeof(message), "Accept trade with %s?", npc_name ? npc_name : "merchant");

    UIDialog* dialog = dialog_confirm("Trade", message);

    if (dialog && (on_confirm || on_cancel)) {
        ConfirmDialogCallbacks* callbacks = malloc(sizeof(ConfirmDialogCallbacks));
        if (callbacks) {
            callbacks->on_confirm = on_confirm;
            callbacks->on_cancel = on_cancel;
            dialog_set_result_callback(dialog, dialog_confirm_result_callback, callbacks);
            dialog_set_close_callback(dialog, dialog_confirm_close_callback, callbacks);
        }
    }

    return dialog;
}

UIDialog* game_dialog_quest_prompt(
    const char* quest_name,
    const char* quest_desc,
    void (*on_accept)(void),
    void (*on_decline)(void)
) {
    UIDialog* dialog = dialog_create(
        "QuestPrompt",
        quest_name ? quest_name : "New Quest",
        quest_desc ? quest_desc : "Accept this quest?",
        DIALOG_TYPE_CONFIRM
    );

    if (dialog) {
        dialog_set_buttons(dialog, DIALOG_BUTTON_YES | DIALOG_BUTTON_NO);
        dialog_set_modal(dialog, true);

        if (on_accept || on_decline) {
            ConfirmDialogCallbacks* callbacks = malloc(sizeof(ConfirmDialogCallbacks));
            if (callbacks) {
                callbacks->on_confirm = on_accept;
                callbacks->on_cancel = on_decline;
                dialog_set_result_callback(dialog, dialog_confirm_result_callback, callbacks);
                dialog_set_close_callback(dialog, dialog_confirm_close_callback, callbacks);
            }
        }
    }

    return dialog;
}

UIDialog* game_dialog_dialogue_choice(
    const char* npc_name,
    const char* dialogue,
    const char* choice1,
    const char* choice2,
    void (*on_choice1)(void),
    void (*on_choice2)(void)
) {
    char title[128];
    snprintf(title, sizeof(title), "%s", npc_name ? npc_name : "NPC");

    UIDialog* dialog = dialog_custom(title);

    if (dialog) {
        dialog_set_messages(dialog, title, dialogue);
        dialog_set_modal(dialog, true);
        dialog_set_buttons(dialog, DIALOG_BUTTON_YES | DIALOG_BUTTON_NO);

        // TODO: Add custom button labels for choice1 and choice2
        // This would require extending the button API to support custom labels
    }

    return dialog;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void game_dialog_show_paused(UIDialog* dialog) {
    if (!dialog) return;

    g_active_game_dialog = dialog;
    g_game_paused_for_dialog = true;

    // TODO: Pause game logic
    // game_pause();

    dialog_show(dialog);

    LOG_INFO("Showing paused dialog: %s", dialog->title ? dialog->title : "unnamed");
}

bool game_dialog_is_active(void) {
    ModalManager* manager = modal_manager_get_global();
    if (!manager) return false;
    return modal_manager_has_active_modal(manager);
}

UIDialog* game_dialog_get_active(void) {
    ModalManager* manager = modal_manager_get_global();
    if (!manager) return NULL;
    return modal_manager_get_top(manager);
}

void game_dialog_close_all(void) {
    ModalManager* manager = modal_manager_get_global();
    if (!manager) return;

    while (modal_manager_has_active_modal(manager)) {
        UIDialog* dialog = modal_manager_pop(manager);
        if (dialog) {
            dialog_close(dialog, DIALOG_RESULT_CANCEL);
        }
    }

    g_game_paused_for_dialog = false;
    g_active_game_dialog = NULL;

    // TODO: Resume game logic
    // game_resume();

    LOG_INFO("Closed all game dialogs");
}
