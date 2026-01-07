// include/ui/menu.h
//
// Purpose: Defines the public API and data structures for the game's interactive
// menu system. This header provides a comprehensive framework for managing various
// menu states (e.g., main menu, world selection, settings, paused menu) and the
// user interface elements within them. It enables navigation between menus,
// configuration of game settings, and management of saved worlds.
//
// Public APIs:
// - `MenuState`: Enumeration defining the different active states of the menu system.
// - `MenuButton`: Structure representing a clickable button in a menu, including its
//   position, size, text, hover/click states, and a callback function for actions.
// - `MenuPanel`: Structure representing a logical grouping of UI elements (buttons)
//   within a menu, controlling its visibility and layout.
// - `MenuSystem`: The main structure encapsulating the entire menu system's state,
//   including the current `state`, a collection of `MenuPanel`s, and specific
//   substructures for `world_select` (listing saved worlds), `world_create` (parameters
//   for new worlds), and `settings` (configurable game options).
// - `menu_init`: Initializes the menu system, setting up default states and panels.
// - `menu_free`: Frees resources associated with the menu system.
// - `menu_update`: Updates the menu system's state each frame, handling animations and transitions.
// - `menu_render`: Renders the currently active menu panel and its elements.
// - `menu_set_state`, `menu_get_state`: Functions for navigating between different menu states.
// - World management functions (`menu_load_worlds`, `menu_create_world`, `menu_delete_world`,
//   `menu_select_world`) integrate with the `SaveSystem` to manage game worlds.
// - Input handling functions (`menu_handle_mouse_move`, `menu_handle_mouse_click`, `menu_handle_key`)
//   process user input for menu interaction.
//
// Ownership: The `MenuSystem` manages the lifecycle of `MenuPanel`s and `MenuButton`s.
// It interacts with the `SaveSystem` (forward declared) but does not own it.
//
// Invariants:
// - A `MenuSystem` must be initialized with `menu_init` before use.
// - `menu_update` and `menu_render` should be called sequentially in the game loop.
// - Callback functions for `MenuButton`s must be valid.
// - World management functions rely on a properly initialized `SaveSystem`.
//
#ifndef MENU_H
#define MENU_H


#include "../game_common.h"
#include <math/vec2.h>

typedef struct InputProfiles InputProfiles;
typedef struct InputState InputState;

// Menu state
typedef enum {
    MENU_STATE_MAIN,
    MENU_STATE_WORLD_SELECT,
    MENU_STATE_WORLD_CREATE,
    MENU_STATE_SETTINGS,
    MENU_STATE_PAUSED,
    MENU_STATE_IN_GAME
} MenuState;

// Menu button
typedef struct {
    Vec2 position;
    Vec2 size;
    const char *text;
    bool hovered;
    bool clicked;
    void (*on_click)(void *user_data);
    void *user_data;
} MenuButton;

// Menu panel
typedef struct {
    Vec2 position;
    Vec2 size;
    bool visible;
    MenuButton *buttons;
    u32 button_count;
} MenuPanel;

// Animation types for menu transitions
typedef enum {
    ANIM_NONE,
    ANIM_FADE_IN,
    ANIM_FADE_OUT,
    ANIM_SLIDE_LEFT,
    ANIM_SLIDE_RIGHT,
    ANIM_SLIDE_UP,
    ANIM_SLIDE_DOWN
} MenuAnimationType;

// Animation state
typedef struct {
    MenuAnimationType type;
    f32 progress;        // 0.0 to 1.0
    f32 duration;        // Animation duration in seconds
    f32 elapsed;        // Time elapsed since animation start
    bool active;         // Is animation currently playing
} MenuAnimation;

// Main menu
typedef struct {
    MenuState state;
    MenuPanel *panels;
    u32 panel_count;
    
    // Animation system
    MenuAnimation animation;
    MenuState previous_state;  // State we're transitioning from
    
    // World selection
    struct {
        char **world_names;
        u32 *world_seeds;
        u32 count;
        u32 selected_index;
        bool creating_new;
    } world_select;
    
    // World creation
    struct {
        char name[64];
        u32 seed;
        u32 theme;
        u32 difficulty;
        bool creative_mode;
    } world_create;
    
    // Settings
    struct {
        f32 master_volume;
        f32 music_volume;
        f32 sfx_volume;
        u32 render_distance;
        bool vsync;
        bool fullscreen;
    } settings;

    // Controls profiles
    struct {
        InputProfiles *profiles;
        InputState *input_state;
        const char *profiles_path;
        u32 selected_index;
        f32 message_timer;
        char message[64];
    } controls;
} MenuSystem;

// Initialize menu system
void menu_init(MenuSystem *menu);
void menu_free(MenuSystem *menu);

// Update menu
void menu_update(MenuSystem *menu, f32 delta_time);
void menu_render(MenuSystem *menu);

// Menu navigation
void menu_set_state(MenuSystem *menu, MenuState state);
MenuState menu_get_state(MenuSystem *menu);

// Animation system
void menu_start_animation(MenuSystem *menu, MenuAnimationType type, f32 duration);
void menu_update_animation(MenuSystem *menu, f32 delta_time);
bool menu_is_animating(const MenuSystem *menu);
f32 menu_get_animation_progress(const MenuSystem *menu);

struct SaveSystem;

// World selection
void menu_load_worlds(MenuSystem *menu, struct SaveSystem *save_system);
void menu_create_world(MenuSystem *menu, const char *name, u32 seed, u32 theme, u32 difficulty, bool creative);
void menu_delete_world(MenuSystem *menu, u32 index);
void menu_select_world(MenuSystem *menu, u32 index);

// Controls profiles
void menu_set_input_profiles(MenuSystem *menu, InputProfiles *profiles,
                             InputState *input_state,
                             const char *profiles_path);

// Input handling
void menu_handle_mouse_move(MenuSystem *menu, f32 x, f32 y);
void menu_handle_mouse_click(MenuSystem *menu, f32 x, f32 y);
void menu_handle_key(MenuSystem *menu, u32 key, bool pressed);

#endif // MENU_H
