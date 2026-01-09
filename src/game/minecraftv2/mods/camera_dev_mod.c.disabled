#include "../include/modding/mod_api.h>
#include "../include/core/logger.h>
#include "../include/player/player.h>
#include "../include/render/camera.h>
#include "../include/math/vec3.h>
#include <string.h>

static ModInfo mod_info = {
    .name = "CameraDevMod",
    .description = "Developer mod for free camera exploration",
    .version_major = 1,
    .version_minor = 0,
    .version_patch = 0,
    .author = "Developer Tools",
    .on_init = NULL,
    .on_shutdown = NULL,
    .on_update = NULL
};

typedef struct {
    Camera *camera;
    PlayerSystem *player_system;
    bool free_camera_enabled;
    f32 camera_speed;
    Vec3 camera_position;
} CameraModState;

static CameraModState camera_state = {
    .camera = NULL,
    .player_system = NULL,
    .free_camera_enabled = false,
    .camera_speed = 20.0f,
    .camera_position = {0, 0, 0}
};

void mod_on_init(Mod *mod) {
    LOG_INFO("CameraDevMod initialized - Press C to toggle free camera");
    camera_state.player_system = (PlayerSystem *)mod->user_data;
    if (camera_state.player_system && camera_state.player_system->camera) {
        camera_state.camera = camera_state.player_system->camera;
        camera_state.camera_position = camera_state.camera->position;
    }
}

void mod_on_shutdown(Mod *mod) {
    LOG_INFO("CameraDevMod shutting down");
}

bool mod_on_update(Mod *mod, f32 delta_time) {
    Camera *cam = camera_state.camera;
    PlayerSystem *sys = camera_state.player_system;
    
    if (!cam || !sys) return true;
    
    if (camera_state.free_camera_enabled) {
        Vec3 forward = cam->front;
        Vec3 right = cam->right;
        Vec3 up = vec3(0.0f, 1.0f, 0.0f);
        
        f32 speed = delta_time * camera_state.camera_speed;
        Vec3 movement = vec3_zero();
        
        if (sys->input) {
            if (sys->input->keys[KEY_W]) {
                movement = vec3_add(movement, vec3_mul(forward, speed));
            }
            if (sys->input->keys[KEY_S]) {
                movement = vec3_sub(movement, vec3_mul(forward, speed));
            }
            if (sys->input->keys[KEY_A]) {
                movement = vec3_sub(movement, vec3_mul(right, speed));
            }
            if (sys->input->keys[KEY_D]) {
                movement = vec3_add(movement, vec3_mul(right, speed));
            }
            if (sys->input->keys[KEY_SPACE]) {
                movement = vec3_add(movement, vec3_mul(up, speed));
            }
            if (sys->input->keys[KEY_LCTRL]) {
                movement = vec3_sub(movement, vec3_mul(up, speed));
            }
        }
        
        camera_state.camera_position = vec3_add(camera_state.camera_position, movement);
        cam->position = camera_state.camera_position;
    } else if (sys->player) {
        camera_state.camera_position = vec3_add(sys->player->position, sys->player->camera_offset);
        cam->position = camera_state.camera_position;
    }
    
    return true;
}

ModInfo *get_mod_info(void) {
    mod_info.on_init = mod_on_init;
    mod_info.on_shutdown = mod_on_shutdown;
    mod_info.on_update = mod_on_update;
    return &mod_info;
}
