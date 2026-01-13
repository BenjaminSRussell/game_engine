# Dialog/Modal System - API Quick Reference

## Quick Start

### Basic Alert Dialog
```c
UIDialog* dialog = dialog_alert("Title", "Message");
dialog_show(dialog);
```

### Confirmation Dialog with Callback
```c
void on_result(UIDialog* dialog, DialogResult result, void* user_data) {
    if (result == DIALOG_RESULT_OK) {
        // User clicked OK
    } else if (result == DIALOG_RESULT_CANCEL) {
        // User clicked Cancel
    }
}

UIDialog* dialog = dialog_confirm("Confirm", "Are you sure?");
dialog_set_result_callback(dialog, on_result, NULL);
dialog_show(dialog);
```

### Builder Pattern (Recommended)
```c
UIDialog* dialog =
    dialog_builder_create()
        -> dialog_builder_title("Settings")
        -> dialog_builder_message("Apply settings?")
        -> dialog_builder_buttons(DIALOG_BUTTON_APPLY | DIALOG_BUTTON_RESET)
        -> dialog_builder_modal(true)
        -> dialog_builder_build();

dialog_show(dialog);
```

## Dialog Creation

### Quick Helper Functions
```c
UIDialog* dialog_alert(const char* title, const char* message);
UIDialog* dialog_confirm(const char* title, const char* message);
UIDialog* dialog_yes_no(const char* title, const char* message);
UIDialog* dialog_warning(const char* title, const char* message);
UIDialog* dialog_error(const char* title, const char* message);
UIDialog* dialog_prompt(const char* title, const char* message, const char* default_text);
UIDialog* dialog_custom(const char* title);
```

### Generic Creation
```c
UIDialog* dialog_create(
    const char* name,
    const char* title,
    const char* message,
    DialogType type
);
```

## Dialog Configuration

```c
// Modal behavior
void dialog_set_modal(UIDialog* dialog, bool modal);
void dialog_set_draggable(UIDialog* dialog, bool draggable);
void dialog_set_resizable(UIDialog* dialog, bool resizable);

// Close behavior
void dialog_set_close_behavior(
    UIDialog* dialog,
    bool close_on_backdrop,   // Close when clicking outside
    bool close_on_escape      // Close when pressing ESC
);

// Buttons
void dialog_set_buttons(UIDialog* dialog, uint32_t flags);
// Flags: DIALOG_BUTTON_OK, DIALOG_BUTTON_CANCEL, DIALOG_BUTTON_YES,
//        DIALOG_BUTTON_NO, DIALOG_BUTTON_APPLY, DIALOG_BUTTON_RESET

// Messages
void dialog_set_messages(
    UIDialog* dialog,
    const char* title,
    const char* message
);

// Content
void dialog_add_custom_widget(UIDialog* dialog, Widget* widget);

// Size constraints
void dialog_set_size_constraints(UIDialog* dialog, Vec2 min_size, Vec2 max_size);
```

## Dialog Display Control

```c
void dialog_show(UIDialog* dialog);      // Show with fade-in animation
void dialog_hide(UIDialog* dialog);      // Hide with fade-out animation
void dialog_close(UIDialog* dialog, DialogResult result);  // Close with result

bool dialog_is_visible(const UIDialog* dialog);
bool dialog_is_modal(const UIDialog* dialog);
```

## Callbacks

```c
// Result callback - called when dialog closes with a result
typedef void (*DialogResultCallback)(UIDialog* dialog, DialogResult result, void* user_data);

void dialog_set_result_callback(
    UIDialog* dialog,
    DialogResultCallback callback,
    void* user_data
);

// Close callback - called before dialog is destroyed
typedef void (*DialogCloseCallback)(UIDialog* dialog, void* user_data);

void dialog_set_close_callback(
    UIDialog* dialog,
    DialogCloseCallback callback,
    void* user_data
);
```

## Dialog Results

```c
typedef enum {
    DIALOG_RESULT_NONE,
    DIALOG_RESULT_OK,
    DIALOG_RESULT_CANCEL,
    DIALOG_RESULT_YES,
    DIALOG_RESULT_NO,
    DIALOG_RESULT_APPLY,
    DIALOG_RESULT_RESET,
    DIALOG_RESULT_CUSTOM
} DialogResult;

DialogResult dialog_get_result(const UIDialog* dialog);
const char* dialog_get_prompt_text(const UIDialog* dialog);  // For prompt dialogs
```

## Visual Customization

```c
void dialog_set_title_bg_color(UIDialog* dialog, Vec4 color);
void dialog_set_content_bg_color(UIDialog* dialog, Vec4 color);
void dialog_set_corner_radius(UIDialog* dialog, float radius);
void dialog_set_shadow(UIDialog* dialog, float offset, Vec4 color);
```

## Dialog Builder Pattern

```c
DialogBuilder* dialog_builder_create(void);

DialogBuilder* dialog_builder_title(DialogBuilder* builder, const char* title);
DialogBuilder* dialog_builder_message(DialogBuilder* builder, const char* message);
DialogBuilder* dialog_builder_type(DialogBuilder* builder, DialogType type);
DialogBuilder* dialog_builder_buttons(DialogBuilder* builder, uint32_t flags);
DialogBuilder* dialog_builder_modal(DialogBuilder* builder, bool modal);
DialogBuilder* dialog_builder_size(DialogBuilder* builder, Vec2 size);
DialogBuilder* dialog_builder_draggable(DialogBuilder* builder, bool draggable);
DialogBuilder* dialog_builder_resizable(DialogBuilder* builder, bool resizable);

UIDialog* dialog_builder_build(DialogBuilder* builder);
void dialog_builder_destroy(DialogBuilder* builder);
```

## Button Widget

```c
// Creation
UIButton* ui_button_create(const char* name, const char* text);

// Content
void ui_button_set_text(UIButton* button, const char* text);
const char* ui_button_get_text(const UIButton* button);
void ui_button_set_icon(UIButton* button, uint32_t icon_id);

// Styling
void ui_button_set_style(UIButton* button, ButtonStyle style);
void ui_button_set_size(UIButton* button, ButtonSize size);
void ui_button_set_corner_radius(UIButton* button, float radius);
void ui_button_set_colors(UIButton* button, Vec4 normal, Vec4 hover, Vec4 pressed, Vec4 disabled);
void ui_button_set_text_color(UIButton* button, Vec4 color);

// State
bool ui_button_is_pressed(const UIButton* button);
bool ui_button_is_hovered(const UIButton* button);

// Callbacks
void ui_button_set_on_click(UIButton* button, void (*callback)(UIButton*, void*), void* user_data);
void ui_button_set_on_press(UIButton* button, void (*callback)(UIButton*, void*), void* user_data);
void ui_button_set_on_release(UIButton* button, void (*callback)(UIButton*, void*), void* user_data);

// Utility
void ui_button_click(UIButton* button);  // Programmatic click
void ui_button_destroy(UIButton* button);
```

### Button Styles
```c
BUTTON_STYLE_DEFAULT   // Standard button
BUTTON_STYLE_PRIMARY   // Highlighted/primary action
BUTTON_STYLE_DANGER    // Warning/destructive action
BUTTON_STYLE_OUTLINE   // Outlined button
BUTTON_STYLE_FLAT      // No background
```

### Button Sizes
```c
BUTTON_SIZE_SMALL      // ~80x24
BUTTON_SIZE_MEDIUM     // ~100x32
BUTTON_SIZE_LARGE      // ~150x48
```

## Label Widget

```c
// Creation
UILabel* ui_label_create(const char* name, const char* text);

// Content
void ui_label_set_text(UILabel* label, const char* text);
const char* ui_label_get_text(const UILabel* label);
void ui_label_set_text_format(UILabel* label, const char* fmt, ...);

// Text properties
void ui_label_set_font_size(UILabel* label, float font_size);
void ui_label_set_text_color(UILabel* label, Vec4 color);
void ui_label_set_alignment(UILabel* label, LabelAlignment alignment);
void ui_label_set_vertical_alignment(UILabel* label, LabelVerticalAlignment v_alignment);
void ui_label_set_format(UILabel* label, LabelTextFormat format);

// Layout
void ui_label_set_word_wrap(UILabel* label, bool wrap);
void ui_label_set_ellipsis(UILabel* label, bool ellipsis);
void ui_label_set_max_lines(UILabel* label, uint32_t max_lines);
uint32_t ui_label_get_line_count(const UILabel* label);

// Effects
void ui_label_set_shadow(UILabel* label, float offset_x, float offset_y, Vec4 color, bool show_shadow);
void ui_label_disable_shadow(UILabel* label);

// Utility
void ui_label_destroy(UILabel* label);
```

### Label Alignments
```c
LABEL_ALIGN_LEFT
LABEL_ALIGN_CENTER
LABEL_ALIGN_RIGHT

LABEL_VALIGN_TOP
LABEL_VALIGN_CENTER
LABEL_VALIGN_BOTTOM
```

### Label Text Formats
```c
LABEL_FORMAT_PLAIN      // Plain text
LABEL_FORMAT_BOLD       // Bold text
LABEL_FORMAT_ITALIC     // Italic text
LABEL_FORMAT_MONOSPACE  // Monospace font
```

## Modal Manager

```c
// Global access
ModalManager* modal_manager_get_global(void);
void modal_manager_init_global(void);
void modal_manager_shutdown_global(void);

// Stack management
void modal_manager_push(ModalManager* manager, UIDialog* dialog);
UIDialog* modal_manager_pop(ModalManager* manager);
UIDialog* modal_manager_get_top(const ModalManager* manager);
uint32_t modal_manager_get_count(const ModalManager* manager);
bool modal_manager_has_active_modal(const ModalManager* manager);

// Backdrop
void modal_manager_set_backdrop_color(ModalManager* manager, Vec4 color);
void modal_manager_set_backdrop_opacity(ModalManager* manager, float opacity);
void modal_manager_set_backdrop_visible(ModalManager* manager, bool visible);

// Z-index
uint32_t modal_manager_allocate_z_index(ModalManager* manager);
void modal_manager_set_base_z_index(ModalManager* manager, uint32_t base_index);
void modal_manager_set_z_index_step(ModalManager* manager, uint32_t step);

// Updates
void modal_manager_update(ModalManager* manager, float dt);

// Query
bool modal_manager_is_blocking_input(const ModalManager* manager);
float modal_manager_get_backdrop_fade(const ModalManager* manager);
```

## Game-Specific Dialogs

```c
UIDialog* game_dialog_exit_confirm(void (*on_confirm)(void), void (*on_cancel)(void));
UIDialog* game_dialog_save_game(const char* slot_name, void (*on_confirm)(const char*), void (*on_cancel)(void));
UIDialog* game_dialog_load_game(const char* slot_name, void (*on_confirm)(const char*), void (*on_cancel)(void));
UIDialog* game_dialog_settings_apply(void (*on_apply)(void), void (*on_cancel)(void));
UIDialog* game_dialog_craft_confirm(const char* item_name, uint32_t quantity, void (*on_craft)(void), void (*on_cancel)(void));
UIDialog* game_dialog_death_notification(const char* killer_name, void (*on_respawn)(void));
UIDialog* game_dialog_trade_confirm(const char* npc_name, void (*on_confirm)(void), void (*on_cancel)(void));
UIDialog* game_dialog_quest_prompt(const char* quest_name, const char* quest_desc, void (*on_accept)(void), void (*on_decline)(void));
UIDialog* game_dialog_dialogue_choice(const char* npc_name, const char* dialogue, const char* choice1, const char* choice2, void (*on_choice1)(void), void (*on_choice2)(void));

// Utilities
void game_dialog_show_paused(UIDialog* dialog);
bool game_dialog_is_active(void);
UIDialog* game_dialog_get_active(void);
void game_dialog_close_all(void);
```

## Complete Usage Example

```c
// Create a confirmation dialog
UIDialog* dialog = dialog_confirm("Save Progress", "Do you want to save?");

// Configure it
dialog_set_modal(dialog, true);
dialog_set_close_behavior(dialog, true, true);  // Click outside or ESC to close

// Set callbacks
void on_dialog_result(UIDialog* d, DialogResult result, void* user_data) {
    if (result == DIALOG_RESULT_OK) {
        printf("Saving game...\n");
        // Save game logic here
    } else {
        printf("Save cancelled\n");
    }
}

dialog_set_result_callback(dialog, on_dialog_result, NULL);

// Show the dialog
dialog_show(dialog);

// In your main loop, update the modal manager
void game_update(float dt) {
    ModalManager* manager = modal_manager_get_global();
    modal_manager_update(manager, dt);

    // Game logic here
    // Modal dialogs block input automatically
}

// In your render loop
void game_render(void) {
    // Render game scene

    // Render dialogs (called automatically by widget system)
    ModalManager* manager = modal_manager_get_global();
    modal_manager_render_backdrop(manager, RENDER_LAYER_DIALOG_BACKDROP);
    modal_manager_render_dialogs(manager);
}

// When done with dialog
// dialog_close(dialog, DIALOG_RESULT_OK);  // or dialog_destroy(dialog);
```

## File Locations

- Dialog Widget: `src/engine/ui/widgets/dialog.h/c`
- Button Widget: `src/engine/ui/widgets/ui_button.h/c`
- Label Widget: `src/engine/ui/widgets/ui_label.h/c`
- Modal Manager: `src/engine/ui/modal_manager.h/c`
- Game Integration: `src/game/blockgame/ui/dialog_integration.h/c`

## Memory Management

- Always pair `dialog_create()` with `dialog_destroy()` or let callbacks manage cleanup
- Buttons and labels created by dialog are automatically cleaned up
- Modal manager tracks dialogs but doesn't own them
- Callbacks can safely delete dialogs

## Threading Notes

- Dialog system is not thread-safe by design
- Always call dialog functions from the main/game thread
- Modal manager updates should happen in main loop

---

For complete documentation, see: `DIALOG_MODAL_IMPLEMENTATION_SUMMARY.md`
