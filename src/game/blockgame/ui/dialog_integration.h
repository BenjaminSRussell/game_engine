/*
 * dialog_integration.h
 * Game-Specific Dialog Integration Helpers
 * Convenience wrappers for common game dialogs
 *
 * Part of the blockgame UI subsystem
 */

#ifndef BLOCKGAME_DIALOG_INTEGRATION_H
#define BLOCKGAME_DIALOG_INTEGRATION_H

#include "engine/ui/widgets/dialog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GAME-SPECIFIC DIALOG TYPES
 * ============================================================================ */

/**
 * Show a pause menu confirmation dialog
 * "Are you sure you want to exit?"
 *
 * @param on_confirm  Callback if user confirms (non-NULL)
 * @param on_cancel   Callback if user cancels (non-NULL)
 * @return            Created dialog
 */
UIDialog* game_dialog_exit_confirm(
    void (*on_confirm)(void),
    void (*on_cancel)(void)
);

/**
 * Show a save game dialog
 *
 * @param slot_name   Current save slot name
 * @param on_confirm  Callback if user confirms
 * @param on_cancel   Callback if user cancels
 * @return            Created dialog
 */
UIDialog* game_dialog_save_game(
    const char* slot_name,
    void (*on_confirm)(const char* slot_name),
    void (*on_cancel)(void)
);

/**
 * Show a load game dialog
 *
 * @param slot_name   Current save slot name
 * @param on_confirm  Callback if user confirms
 * @param on_cancel   Callback if user cancels
 * @return            Created dialog
 */
UIDialog* game_dialog_load_game(
    const char* slot_name,
    void (*on_confirm)(const char* slot_name),
    void (*on_cancel)(void)
);

/**
 * Show a settings confirmation dialog
 *
 * @param on_apply    Callback if user applies changes
 * @param on_cancel   Callback if user cancels
 * @return            Created dialog
 */
UIDialog* game_dialog_settings_apply(
    void (*on_apply)(void),
    void (*on_cancel)(void)
);

/**
 * Show a crafting confirmation dialog
 *
 * @param item_name   Name of item being crafted
 * @param quantity    Quantity to craft
 * @param on_craft    Callback if user confirms craft
 * @param on_cancel   Callback if user cancels
 * @return            Created dialog
 */
UIDialog* game_dialog_craft_confirm(
    const char* item_name,
    uint32_t quantity,
    void (*on_craft)(void),
    void (*on_cancel)(void)
);

/**
 * Show a death notification dialog
 *
 * @param killer_name  Name of what killed the player (or NULL)
 * @param on_respawn   Callback when player respawns
 * @return             Created dialog
 */
UIDialog* game_dialog_death_notification(
    const char* killer_name,
    void (*on_respawn)(void)
);

/**
 * Show a trade/merchant dialog
 *
 * @param npc_name    Name of NPC merchant
 * @param on_confirm  Callback if trade confirmed
 * @param on_cancel   Callback if trade cancelled
 * @return            Created dialog
 */
UIDialog* game_dialog_trade_confirm(
    const char* npc_name,
    void (*on_confirm)(void),
    void (*on_cancel)(void)
);

/**
 * Show a quest acceptance dialog
 *
 * @param quest_name  Name of the quest
 * @param quest_desc  Quest description/objectives
 * @param on_accept   Callback if player accepts quest
 * @param on_decline  Callback if player declines quest
 * @return            Created dialog
 */
UIDialog* game_dialog_quest_prompt(
    const char* quest_name,
    const char* quest_desc,
    void (*on_accept)(void),
    void (*on_decline)(void)
);

/**
 * Show a dialogue choice dialog
 *
 * @param npc_name    Name of NPC speaking
 * @param dialogue    Dialogue text to display
 * @param choice1     First choice text
 * @param choice2     Second choice text
 * @param on_choice1  Callback for first choice
 * @param on_choice2  Callback for second choice
 * @return            Created dialog
 */
UIDialog* game_dialog_dialogue_choice(
    const char* npc_name,
    const char* dialogue,
    const char* choice1,
    const char* choice2,
    void (*on_choice1)(void),
    void (*on_choice2)(void)
);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * Pause the game and show a dialog
 * Unpauses when dialog closes
 *
 * @param dialog  Dialog to show
 */
void game_dialog_show_paused(UIDialog* dialog);

/**
 * Check if a modal dialog is currently active
 *
 * @return  True if any modal dialog is open
 */
bool game_dialog_is_active(void);

/**
 * Get the currently active modal dialog
 *
 * @return  Pointer to active dialog, or NULL if none
 */
UIDialog* game_dialog_get_active(void);

/**
 * Close all active dialogs
 */
void game_dialog_close_all(void);

#ifdef __cplusplus
}
#endif

#endif /* BLOCKGAME_DIALOG_INTEGRATION_H */
