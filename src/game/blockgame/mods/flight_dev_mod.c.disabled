#include "../include/modding/mod_api.h>
#include "../include/core/logger.h>
#include "../include/player/player.h>
#include "../include/math/vec3.h>
#include <string.h>

static ModInfo mod_info = {
    .name = "FlightDevMod",
    .description = "Developer mod for flying and exploring the world",
    .version_major = 1,
    .version_minor = 0,
    .version_patch = 0,
    .author = "Developer Tools",
    .on_init = NULL,
    .on_shutdown = NULL,
    .on_update = NULL
};

typedef struct {
    PlayerSystem *player_system;
    bool flight_enabled;
    f32 flight_speed;
} FlightModState;

static FlightModState flight_state = {
    .player_system = NULL,
    .flight_enabled = false,
    .flight_speed = 15.0f
};

void mod_on_init(Mod *mod) {
    LOG_INFO("FlightDevMod initialized - Press SPACE to toggle flight");
    flight_state.player_system = (PlayerSystem *)mod->user_data;
    flight_state.flight_enabled = false;
}

void mod_on_shutdown(Mod *mod) {
    LOG_INFO("FlightDevMod shutting down");
    if (flight_state.flight_enabled && flight_state.player_system) {
        player_fly(flight_state.player_system, false);
    }
}

bool mod_on_update(Mod *mod, f32 delta_time) {
    PlayerSystem *sys = flight_state.player_system;
    if (!sys || !sys->player) return true;
    
    PlayerComponent *p = sys->player;
    
    if (flight_state.flight_enabled) {
        p->fly_speed = flight_state.flight_speed;
        
        if (sys->camera) {
            Vec3 movement = vec3_zero();
            
            if (sys->input) {
                Vec3 forward = sys->camera->front;
                Vec3 right = sys->camera->right;
                Vec3 up = vec3(0.0f, 1.0f, 0.0f);
                
                f32 speed = delta_time * p->fly_speed;
                
                bool moved = false;
                
                if (p->on_ground && sys->input) {
                    if (sys->input->keys[KEY_W]) {
                        movement = vec3_add(movement, vec3_mul(forward, speed));
                        moved = true;
                    }
                    if (sys->input->keys[KEY_S]) {
                        movement = vec3_sub(movement, vec3_mul(forward, speed));
                        moved = true;
                    }
                    if (sys->input->keys[KEY_A]) {
                        movement = vec3_sub(movement, vec3_mul(right, speed));
                        moved = true;
                    }
                    if (sys->input->keys[KEY_D]) {
                        movement = vec3_add(movement, vec3_mul(right, speed));
                        moved = true;
                    }
                    
                    if (moved) {
                        p->position = vec3_add(p->position, movement);
                    }
                }
                
                if (p->is_jumping) {
                    p->position = vec3_add(p->position, vec3_mul(up, speed));
                    p->is_jumping = false;
                }
                
                if (p->is_crouching) {
                    p->position = vec3_sub(p->position, vec3_mul(up, speed));
                    p->is_crouching = false;
                }
            }
        }
    }
    
    return true;
}

ModInfo *get_mod_info(void) {
    mod_info.on_init = mod_on_init;
    mod_info.on_shutdown = mod_on_shutdown;
    mod_info.on_update = mod_on_update;
    return &mod_info;
}
