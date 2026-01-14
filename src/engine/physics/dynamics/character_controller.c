#include <physics/core/physics_types.h>
#include <physics/physics.h>
#include <stddef.h>
#include <stdio.h> // for NULL if needed, or stddef

CharacterController *character_controller_create(PhysicsWorld *world,
                                                 Vec3 position) {
  return NULL; // stub
}

void character_controller_destroy(CharacterController *controller) {}

void character_controller_set_move_speed(CharacterController *controller,
                                         f32 speed) {}
void character_controller_set_jump_height(CharacterController *controller,
                                          f32 height) {}
void character_controller_set_step_height(CharacterController *controller,
                                          f32 height) {}
void character_controller_set_slope_limit(CharacterController *controller,
                                          f32 angle_degrees) {}
void character_controller_set_air_control(CharacterController *controller,
                                          f32 air_control) {}

bool character_controller_is_grounded(CharacterController *controller) {
  return false;
}
Vec3 character_controller_get_velocity(CharacterController *controller) {
  return (Vec3){0};
}
Vec3 character_controller_get_position(CharacterController *controller) {
  return (Vec3){0};
}

void character_controller_move(CharacterController *controller,
                               PhysicsWorld *world, Vec3 direction,
                               f32 delta_time) {}
void character_controller_jump(CharacterController *controller,
                               PhysicsWorld *world) {}
void character_controller_update(CharacterController *controller,
                                 PhysicsWorld *world, f32 delta_time) {}

void character_controller_teleport(CharacterController *controller,
                                   Vec3 position) {}
void character_controller_reset(CharacterController *controller) {}

void character_controller_init_system(void) {}
void character_controller_cleanup_system(void) {}
void character_controller_update_all(PhysicsWorld *world, f32 delta_time) {}
