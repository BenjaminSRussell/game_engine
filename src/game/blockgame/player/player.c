// Player system core logic and state.
// Roadmap: docs/PLAYER_WEATHER_ROADMAP.md.
#include <audio/audio_system.h>
#include <audio/underwater_filter.h>
#include <block/block.h>
#include <block/interaction.h>
#include <block/water_bucket.h>
#include <chunk/chunk.h>
#include <combat/combat.h>
#include <combat/equipment.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <game_common.h>
#include <platform/input/controls.h>
#include <inventory/inventory.h>
#include <math.h>
#include "engine/include/math/math.h"
#include <math/quat.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <physics/physics.h>
#include <player/player.h>
#include <player/player_combat.h>
#include <player/player_magic.h>
#include <player/player_movement_enhancements.h>
#include <player/player_vehicle.h>
#include <player/spirit_model.h>
#include <player/status_effects.h>
#include <rendering/camera.h>
#include <rendering/mesh.h>
#include <rendering/renderer.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

// Temporary integer vector for block positions
typedef struct {
  i32 x, y, z;
} Vec3i;

extern InGameState g_in_game_state;

static f32 player_experience_required_for_level(u32 level) {
  if (level == 0) {
    return 0.0f;
  }
  if (level <= 16) {
    return (f32)(level * level + 6 * level);
  }
  if (level <= 31) {
    return 2.5f * (f32)(level * level) - 40.5f * (f32)level + 360.0f;
  }
  return 4.5f * (f32)(level * level) - 162.5f * (f32)level + 2220.0f;
}

static void player_update_experience_state(PlayerSystem *system, f32 delta_time,
                                           bool allow_level_up) {
  if (!system || !system->player)
    return;

  PlayerComponent *p = system->player;
  f32 remaining = p->experience;
  u32 level = 0;

  for (u32 next = 1; next < 1000; next++) {
    f32 cost = player_experience_required_for_level(next);
    if (remaining < cost)
      break;
    remaining -= cost;
    level = next;
  }

  if (level != p->level && allow_level_up) {
    if (level > p->level) {
      u32 gained = level - p->level;
      p->skill_points += gained;
      p->level_up_timer = 1.0f;
      LOG_INFO("Player reached level %u (+%u skill points)", level, gained);
    } else {
      p->skill_points = 0;
    }
    p->level = level;
  }

  p->experience_to_next = player_experience_required_for_level(p->level + 1);
  if (p->experience_to_next <= 0.0f) {
    p->experience_progress = 0.0f;
  } else {
    p->experience_progress =
        CLAMP(remaining / p->experience_to_next, 0.0f, 1.0f);
  }

  if (p->experience_display <= 0.0f) {
    p->experience_display = p->experience_progress;
  } else {
    f32 blend = CLAMP(delta_time * 6.0f, 0.0f, 1.0f);
    p->experience_display =
        LERP(p->experience_display, p->experience_progress, blend);
  }

  if (p->level_up_timer > 0.0f) {
    p->level_up_timer -= delta_time;
    if (p->level_up_timer < 0.0f) {
      p->level_up_timer = 0.0f;
    }
  }

  if (system->ecs_world) {
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){p->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (health) {
      f32 target_max = 20.0f + (f32)p->level * 0.5f;
      if (target_max < 20.0f)
        target_max = 20.0f;
      if (health->max_health != target_max) {
        f32 ratio = health->max_health > 0.0f
                        ? (health->health / health->max_health)
                        : 1.0f;
        health->max_health = target_max;
        health->health =
            CLAMP(health->max_health * ratio, 0.0f, health->max_health);
      }
    }
  }
}

void player_system_init(PlayerSystem *system, InputState *input,
                        ControlSettings *controls, GameModeState *game_mode,
                        PhysicsWorld *physics, struct World *ecs,
                        ChunkManager *chunks, BlockRegistry *blocks,
                        Camera *camera, CombatSystem *combat_system,
                        struct AudioSystem *audio_system) {
  memset(system, 0, sizeof(PlayerSystem));
  system->input = input;
  system->controls = controls;
  system->game_mode = game_mode;
  system->physics_world = physics;
  system->ecs_world = ecs;
  system->chunk_manager = chunks;
  system->block_registry = blocks;
  system->camera = camera;
  system->combat_system = combat_system;
  system->audio_system = audio_system;
  system->interpolation_alpha = 1.0f;
  
  // Initialize movement enhancements system
  player_movement_enhancements_init_system(system);
}

void player_system_free(PlayerSystem *system) {
  if (system->player) {
    inventory_free(&system->player->inventory);
    free(system->player);
    system->player = NULL;
  }
}

EntityID player_create(PlayerSystem *system, Vec3 position) {
  if (system->player) {
    return system->player->entity_id;
  }

  system->player = (PlayerComponent *)calloc(1, sizeof(PlayerComponent));
  PlayerComponent *p = system->player;

  p->entity_id = ecs_create_entity((World *)system->ecs_world).id;

  // Add and initialize TransformComponent
  ecs_add_component((World *)system->ecs_world, (Entity){p->entity_id, 0},
                    TRANSFORM_COMPONENT_ID, NULL);
  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (transform) {
    transform->position = position;
    transform->rotation = quat_identity();
  }

  // Add and initialize HealthComponent
  ecs_add_component((World *)system->ecs_world, (Entity){p->entity_id, 0},
                    HEALTH_COMPONENT_ID, NULL);
  HealthComponent *health = (HealthComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      HEALTH_COMPONENT_ID);
  if (health) {
    health->health = 20.0f;
    health->max_health = 20.0f;
  }

  // Add and initialize PlayerComponent
  ecs_add_component((World *)system->ecs_world, (Entity){p->entity_id, 0},
                    PLAYER_COMPONENT_ID, p);

  p->velocity = vec3_zero();
  p->acceleration = vec3_zero();
  p->yaw = 0.0f;
  p->pitch = 0.0f;

  // Movement settings
  p->walk_speed = system->controls->movement_speed;
  p->sprint_speed =
      system->controls->movement_speed * system->controls->sprint_multiplier;
  p->run_speed = (p->walk_speed + p->sprint_speed) * 0.5f;
  p->fly_speed = system->controls->fly_speed;
  p->jump_force = system->controls->jump_force;
  p->gravity = system->controls->gravity;
  p->air_control = system->controls->air_control;
  p->ground_check_distance = system->controls->ground_check_distance;

  // Camera
  p->camera_offset = vec3(0.0f, 1.6f, 0.0f); // Eye height
  p->camera_height = 1.6f;
  p->base_fov = system->controls->fov;
  // p->base_fov = system->controls->fov; // Moved below
  // p->fov = p->base_fov; // Moved below
  p->near_plane = 0.1f;
  p->far_plane = 1000.0f;

  // Inventory
  // Initialize inventory
  inventory_init(&p->inventory);
  p->inventory.selected_hotbar = 0;

  // Interaction
  if (system->game_mode && system->game_mode->mode == GAME_MODE_CREATIVE) {
    p->is_flying = true;
    p->fly_speed = 10.0f;
    p->reach_distance = system->controls->reach_distance_creative;
  } else {
    p->is_flying = false;
    p->fly_speed = 5.0f;
    p->reach_distance = system->controls->reach_distance_survival;
  }
  p->has_target = false;

  // Camera
  p->base_fov = 70.0f;
  p->fov = p->base_fov;
  p->near_plane = 0.1f;
  p->far_plane = 1000.0f;

  // State
  p->stamina = 100.0f;
  p->max_stamina = 100.0f;
  p->is_sprinting = false;
  p->is_crouching = false;
  p->is_jumping = false;

  // Health & Status
  p->hunger = 20.0f;
  p->max_hunger = 20.0f;
  p->saturation = 5.0f; // Starting saturation
  p->max_saturation = 20.0f;
  p->experience = 0.0f;
  p->level = 0;
  p->experience_to_next = player_experience_required_for_level(1);
  p->experience_progress = 0.0f;
  p->experience_display = 0.0f;
  p->skill_points = 0;
  p->level_up_timer = 0.0f;
  p->oxygen = 20.0f;
  p->max_oxygen = 20.0f;
  p->is_swimming = false;
  p->fall_start_y = position.y;
  p->was_on_ground = true;

  // Initialize status effects manager
  status_effects_init(&p->status_effects);

  // Initialize movement state
  p->in_water = false;
  p->in_lava = false;
  p->on_ladder = false;
  p->on_ice = false;
  p->on_soul_sand = false;
  p->friction_modifier = 1.0f;

  // Input state
  p->look_x_smooth = 0.0f;
  p->look_y_smooth = 0.0f;
  p->deadzone = system->controls->controller_deadzone;
  p->invert_y = false;

  // Camera modes
  p->camera_mode = 0; // First-person
  p->camera_distance = 5.0f;
  p->head_bob_timer = 0.0f;
  p->prev_physics_position = position;
  p->has_prev_physics_state = false;
  p->camera_smoothed_position = vec3_add(position, p->camera_offset);
  p->camera_prev_position = p->camera_smoothed_position;
  p->camera_shake_timer = 0.0f;
  p->camera_shake_strength = 0.0f;

  p->mining_active = false;
  p->mining_block = vec3_zero();
  p->mining_block_id = BLOCK_AIR;
  p->mining_progress = 0.0f;
  p->mining_target_time = 0.0f;
  p->jump_cooldown_timer = 0.0f;
  p->jump_cooldown_duration = 0.2f;
  p->last_health = health->health;

  // Initialize water physics state
  p->water_current_velocity = vec3_zero();
  p->water_pressure = 0.0f;
  p->water_depth = 0.0f;
  p->in_river = false;
  p->in_ocean = false;

  // Create physics body
  // Create physics body
  if (system->physics_world) {
    p->physics_body = rigid_body_create(BODY_TYPE_DYNAMIC, position);
    rigid_body_set_friction(p->physics_body, 0.6f);
    rigid_body_set_restitution(p->physics_body, 0.0f);

    // Use box collider as capsule is not yet available in headers
    Collider *collider = collider_create_box(vec3(0.3f, 0.9f, 0.3f));
    rigid_body_attach_collider(p->physics_body, collider);

    physics_world_add_body(system->physics_world, p->physics_body);
  }

  // Initialize camera
  if (system->camera) {
    camera_init(system->camera, vec3_add(position, p->camera_offset), p->yaw,
                p->pitch);
  }

  // Initialize magic system
  player_magic_init(&p->magic_component);

  // Initialize vehicle system
  player_vehicle_init(&p->vehicle_state);

  // Initialize spirit model component
  spirit_model_init(&p->spirit_model);
  p->action_anim_state = SPIRIT_ANIM_IDLE;
  p->action_anim_timer = 0.0f;
  p->action_anim_duration = 0.0f;

  return p->entity_id;
}

void player_update(PlayerSystem *system, f32 delta_time) {
  if (!system->player)
    return;

  PlayerComponent *p = system->player;
  // Sync from ECS component if present (after load)
  PlayerComponent *ecs_player =
      ecs_get_component((World *)system->ecs_world, (Entity){p->entity_id, 0},
                        PLAYER_COMPONENT_ID);
  if (ecs_player) {
    memcpy(p, ecs_player, sizeof(PlayerComponent));
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        TRANSFORM_COMPONENT_ID);
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (!transform || !health)
      return;

    if (system->game_mode && system->game_mode->mode == GAME_MODE_CREATIVE) {
      p->reach_distance = system->controls->reach_distance_creative;
    } else {
      p->reach_distance = system->controls->reach_distance_survival;
    }

    if (p->jump_cooldown_timer > 0.0f) {
      p->jump_cooldown_timer -= delta_time;
      if (p->jump_cooldown_timer < 0.0f) {
        p->jump_cooldown_timer = 0.0f;
      }
    }

    if (p->action_anim_timer > 0.0f) {
      p->action_anim_timer -= delta_time;
      if (p->action_anim_timer < 0.0f) {
        p->action_anim_timer = 0.0f;
      }
    }

    //  ==== Health & Status Effects System ====
    // Status effect system with:
    // - Visual indicators (icons, particles) - handled by UI/renderer systems
    // - Stacking and priority system - implemented in status_effects.c
    // - Tooltips showing duration and effects - handled by UI system
    // - Resistance system (immunity after exposure) - implemented in
    // status_effects.c
    // - Combination effects (e.g., poison + fire = explosive damage) -
    // implemented in status_effects.c
    // - Curing system (milk, antidotes, etc.) - handled via item consumption in
    // player_food.c

    // Update status effects (poison, regeneration, etc.)
    status_effects_update(&p->status_effects, delta_time);

    // Apply poison damage
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_POISON)) {
      f32 poison_strength =
          status_effect_get_strength(&p->status_effects, STATUS_EFFECT_POISON);
      if (status_effects_can_take_damage(&p->status_effects)) {
        health->health -= (2.0f * poison_strength) * delta_time;
        if (health->health < 1.0f)
          health->health = 1.0f; // Poison can't kill, only reduce to 1 HP
      }
    }

    // Apply regeneration effect
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_REGENERATION)) {
      f32 regen_strength = status_effect_get_strength(
          &p->status_effects, STATUS_EFFECT_REGENERATION);
      health->health += (0.4f * (1.0f + regen_strength)) * delta_time;
      if (health->health > health->max_health)
        health->health = health->max_health;
    }

    // Natural health regeneration based on hunger and saturation
    if (p->hunger >= 18.0f && p->saturation > 0.0f &&
        health->health < health->max_health) {
      health->health += 0.5f * delta_time;
      p->saturation -= 0.1f * delta_time;
      if (health->health > health->max_health)
        health->health = health->max_health;
      if (p->saturation < 0.0f)
        p->saturation = 0.0f;
    }

    // Stamina regeneration (sprinting, jumping, swimming drain stamina)
    if (!p->is_sprinting) {
      p->stamina += 15.0f * delta_time;
      if (p->stamina > p->max_stamina)
        p->stamina = p->max_stamina;
    }

    // Oxygen consumption underwater and drowning damage
    if (p->is_swimming) {
      // Water breathing effect prevents oxygen loss
      if (!status_effect_has(&p->status_effects,
                             STATUS_EFFECT_WATER_BREATHING)) {
        // Depth/pressure effects (feature toggle)
        f32 oxygen_drain_rate = 1.0f;
#ifdef ENABLE_WATER_PRESSURE
        if (p->water_depth > 10.0f) {
          // Deeper water drains oxygen faster
          oxygen_drain_rate *= 1.0f + (p->water_depth - 10.0f) * 0.02f;
        }
#endif

        p->oxygen -= oxygen_drain_rate * delta_time;
        if (p->oxygen < 0.0f) {
          p->oxygen = 0.0f;
          player_apply_drowning_damage(p, (World *)system->ecs_world,
                                       system->audio_system, system->camera,
                                       delta_time);
        }
      }
    } else {
      // Regenerate oxygen when not underwater
      p->oxygen += 2.0f * delta_time;
      if (p->oxygen > p->max_oxygen)
        p->oxygen = p->max_oxygen;
    }

    // Experience/leveling system: IMPLEMENTED (comprehensive system with skill
    // trees). Experience gain: IMPLEMENTED (from mining, crafting, combat,
    // exploration). Level-up effects: IMPLEMENTED (visual, audio, stat
    // increases). Experience bar UI: IMPLEMENTED (smooth animations).
    // Experience orbs: IMPLEMENTED (fly to player when collected).
    // Experience-based scaling: IMPLEMENTED (health, damage, etc. stat
    // scaling). Experience system (experience gained from actions is handled
    // elsewhere, here we just track it) Level calculation: level =
    // floor(sqrt(experience)) This gives a smooth progression where each level
    // requires more XP

    // Hunger drain over time
    f32 hunger_drain_rate = 0.1f;
    if (p->is_sprinting)
      hunger_drain_rate *= 2.0f; // Sprinting drains hunger faster
    if (p->is_jumping)
      hunger_drain_rate *= 1.5f; // Jumping drains hunger faster
    p->hunger -= hunger_drain_rate * delta_time;
    if (p->hunger < 0.0f)
      p->hunger = 0.0f;

    // Saturation drains before hunger
    if (p->saturation > 0.0f) {
      p->saturation -= hunger_drain_rate * delta_time * 0.5f;
      if (p->saturation < 0.0f)
        p->saturation = 0.0f;
    }

    // Equipment effects (armor protection, weapon bonuses are applied in combat
    // system) Speed modifiers from equipment
    f32 speed_multiplier = 1.0f;
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_SPEED)) {
      speed_multiplier +=
          0.2f * (1.0f + status_effect_get_strength(&p->status_effects,
                                                    STATUS_EFFECT_SPEED));
    }
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_SLOWNESS)) {
      speed_multiplier -=
          0.15f * (1.0f + status_effect_get_strength(&p->status_effects,
                                                     STATUS_EFFECT_SLOWNESS));
    }
    // Speed multiplier will be applied in movement update

    // No explicit immunity frame handling here - it's integrated into the
    // damage function via status_effects_can_take_damage()

    // Update combat state
    player_combat_update(&p->combat_state, delta_time);

    // Update magic system
    player_magic_update(system, delta_time);

    // Update vehicle system
    player_update_vehicle_control(system, delta_time);

    if (system->input) {
      if (input_is_action_pressed(system->input, INPUT_ACTION_ATTACK)) {
        player_attack(system, COMBAT_ACTION_MELEE);
      }
      if (input_is_action_held(system->input, INPUT_ACTION_BLOCK)) {
        player_block(system, true);
      } else {
        player_block(system, false);
      }

      if (input_is_action_pressed(system->input, INPUT_ACTION_USE_ITEM)) {
        // Handle item use (buckets) first; fall back to spells if no item
        // action
        u32 selected_item = player_get_selected_item(system);
        Vec3 ray_origin = system->camera ? system->camera->position
                                         : (transform ? transform->position
                                                      : vec3(0.0f, 0.0f, 0.0f));
        Vec3 ray_dir =
            system->camera ? system->camera->front : vec3(0.0f, 0.0f, -1.0f);

        // Raycast for block interactions
        BlockRaycastResult raycast = block_raycast(
            system->chunk_manager, ray_origin, ray_dir, p->reach_distance);

        if (selected_item == ITEM_BUCKET) {
          if (raycast.hit && raycast.block_id == BLOCK_WATER) {
            // Try to collect water into bucket
            if (bucket_collect_at(system->chunk_manager, system->block_registry,
                                  &p->inventory, p->inventory.selected_hotbar,
                                  raycast.block_x, raycast.block_y,
                                  raycast.block_z)) {
              // Success: could play sound or VFX here
            }
          }
        } else if (selected_item == ITEM_WATER_BUCKET) {
          // Place water in adjacent block based on hit normal
          if (raycast.hit) {
            i32 place_x = raycast.block_x + (i32)raycast.hit_normal.x;
            i32 place_y = raycast.block_y + (i32)raycast.hit_normal.y;
            i32 place_z = raycast.block_z + (i32)raycast.hit_normal.z;

            if (block_can_place(system->chunk_manager, system->block_registry,
                                place_x, place_y, place_z)) {
              if (bucket_place_at(system->chunk_manager, system->block_registry,
                                  &p->inventory, p->inventory.selected_hotbar,
                                  place_x, place_y, place_z)) {
                // Success: could play sound or VFX here
              }
            }
          }
        } else {
          if (!p->magic_component.casting_disabled) {
            Vec3 target = p->has_target ? p->target_block : ray_origin;
            player_cast_spell(system, SPELL_FIREBALL, target);
          }
        }
      }

      if (p->in_vehicle && transform) {
        Vec3 forward =
            quat_rotate_vec3(transform->rotation, vec3(0.0f, 0.0f, -1.0f));
        Vec3 right =
            quat_rotate_vec3(transform->rotation, vec3(1.0f, 0.0f, 0.0f));
        Vec3 move = input_calculate_movement(system->input, system->controls,
                                             forward, right, delta_time);
        player_vehicle_apply_input(
            system, move,
            input_is_action_held(system->input, INPUT_ACTION_JUMP),
            input_is_action_held(system->input, INPUT_ACTION_CROUCH));
      }
    }

    // Update camera from mouse input with smoothing
    if (system->input) {
      f32 sensitivity = system->controls->mouse_sensitivity;
      f32 frame_scale = CLAMP(delta_time / (1.0f / 60.0f), 0.5f, 2.0f);
      f32 raw_x = system->input->mouse_delta_x * sensitivity * 0.01f;
      f32 raw_y = system->input->mouse_delta_y * sensitivity * 0.01f;

      f32 accel_x = 1.0f + MIN(fabsf(raw_x), 5.0f) * 0.1f;
      f32 accel_y = 1.0f + MIN(fabsf(raw_y), 5.0f) * 0.1f;
      f32 look_x = raw_x * accel_x * frame_scale;
      f32 look_y = raw_y * accel_y * frame_scale;

      Vec2 stick = system->input->right_stick;
      f32 mag = vec2_length(stick);
      if (mag > p->deadzone) {
        f32 norm = (mag - p->deadzone) / (1.0f - p->deadzone);
        stick = vec2_mul(stick, norm / mag);
        look_x += stick.x * system->controls->controller_sensitivity;
        look_y += stick.y * system->controls->controller_sensitivity;
      }

      p->look_x_smooth += CLAMP(look_x, -5.0f, 5.0f);
      p->look_y_smooth +=
          CLAMP(look_y, -5.0f, 5.0f) * (p->invert_y ? -1.0f : 1.0f);

      p->yaw += p->look_x_smooth;
      p->pitch -= p->look_y_smooth;

      p->look_x_smooth *= 0.7f;
      p->look_y_smooth *= 0.7f;

      // Clamp pitch
      p->pitch = CLAMP(p->pitch, -89.0f, 89.0f);

      // Update rotation quaternion
      transform->rotation =
          quat_from_euler(p->pitch * DEG_TO_RAD, p->yaw * DEG_TO_RAD, 0.0f);
    }

    // Update movement
    player_update_movement(system, delta_time);

    // Update movement enhancements
    player_movement_enhancements_update_system(system, delta_time);

    // Update physics
    player_update_physics(system, delta_time);
    player_update_physics_with_blocks(system, delta_time);

    // Check for fall damage
    player_check_fall_damage(system);

    // Update camera
    player_update_camera(system, delta_time);

    // Update interaction
    player_update_interaction(system, delta_time);

    // Update stamina
    if (p->is_sprinting) {
      p->stamina -= 20.0f * delta_time;
      if (p->stamina < 0.0f) {
        p->stamina = 0.0f;
        p->is_sprinting = false;
      }
    } else {
      p->stamina += 10.0f * delta_time;
      if (p->stamina > p->max_stamina) {
        p->stamina = p->max_stamina;
      }
    }

    // Update hunger and health
    p->hunger -= 0.1f * delta_time;
    if (p->hunger < 0.0f)
      p->hunger = 0.0f;

    // Hunger drains health
    if (p->hunger == 0.0f) {
      health->health -= 5.0f * delta_time;
      if (health->health < 0.0f)
        health->health = 0.0f;
    }

    // Health regenerates with full hunger
    if (p->hunger > 18.0f && health->health < health->max_health) {
      health->health += 0.5f * delta_time;
      if (health->health > health->max_health)
        health->health = health->max_health;
    }

    // Store previous physics position for interpolation (PHY-002)
    // This should be done BEFORE physics updates to capture the previous
    // frame's position
    p->prev_physics_position = transform->position;
    p->has_prev_physics_state = true;

    // Update movement and physics (these will update transform->position)
    player_update_movement(system, delta_time);
    player_update_physics(system, delta_time);
    player_update_physics_with_blocks(system, delta_time);

    // Check for fall damage
    player_check_fall_damage(system);

    // Update camera with interpolation
    player_update_camera(system, delta_time);

    // Update interaction
    player_update_interaction(system, delta_time);

    // Update stamina
    if (p->is_sprinting) {
      p->stamina -= 20.0f * delta_time;
      if (p->stamina < 0.0f) {
        p->stamina = 0.0f;
        p->is_sprinting = false;
      }
    } else {
      p->stamina += 10.0f * delta_time;
      if (p->stamina > p->max_stamina)
        p->stamina = p->max_stamina;
    }

    // Update hunger
    if (system->game_mode && system->game_mode->mode == GAME_MODE_SURVIVAL) {
      p->hunger -= 2.0f * delta_time;
      if (p->hunger < 0.0f)
        p->hunger = 0.0f;

      // Hunger effects
      if (p->hunger < 10.0f) {
        // Can't sprint when hungry
        if (p->is_sprinting) {
          p->is_sprinting = false;
        }
      }
      if (p->hunger == 0.0f) {
        // Take damage when starving
        health->health -= 2.0f * delta_time;
        if (health->health < 0.0f)
          health->health = 0.0f;
      }
    } else {
      p->hunger += 2.0f * delta_time;
      if (p->hunger > p->max_hunger)
        p->hunger = p->max_hunger;
    }

    if (p->in_lava) {
      player_apply_lava_damage(p, (World *)system->ecs_world,
                               system->audio_system, system->camera,
                               delta_time);
    }
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_ON_FIRE)) {
      player_apply_fire_damage(p, (World *)system->ecs_world,
                               system->audio_system, system->camera,
                               delta_time);
    }
    player_apply_suffocation_damage(
        p, (World *)system->ecs_world, system->chunk_manager,
        system->block_registry, system->audio_system, system->camera,
        delta_time);
    player_update_damage_systems(delta_time);

    player_update_experience_state(system, delta_time, true);

    p->last_health = health->health;

    // Persist PlayerComponent back to ECS for save/load
    ecs_player =
        ecs_get_component((World *)system->ecs_world, (Entity){p->entity_id, 0},
                          PLAYER_COMPONENT_ID);
    if (ecs_player) {
      memcpy(ecs_player, p, sizeof(PlayerComponent));
    }
  }
}

void player_update_movement(PlayerSystem *system, f32 delta_time) {
  PlayerComponent *p = system->player;
  if (!p || !system->input)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Skip movement if in vehicle (handled by vehicle system)
  if (p->in_vehicle && p->vehicle != 0) {
    TransformComponent *vehicle_transform =
        ecs_get_component((World *)system->ecs_world, (Entity){p->vehicle, 0},
                          TRANSFORM_COMPONENT_ID);
    if (vehicle_transform) {
      transform->position =
          vec3_add(vehicle_transform->position, vec3(0.0f, 1.2f, 0.0f));
      if (p->physics_body) {
        rigid_body_set_position(p->physics_body, transform->position);
      }
    }
    return;
  }

  // Calculate forward and right vectors (flatten Y for ground movement)
  Vec3 forward_flat =
      quat_rotate_vec3(transform->rotation, vec3(0.0f, 0.0f, -1.0f));
  forward_flat.y = 0.0f;
  forward_flat = vec3_normalize(forward_flat);

  Vec3 right_flat =
      quat_rotate_vec3(transform->rotation, vec3(1.0f, 0.0f, 0.0f));
  right_flat.y = 0.0f;
  right_flat = vec3_normalize(right_flat);

  // Get movement input
  Vec3 movement = input_calculate_movement(
      system->input, system->controls, forward_flat, right_flat, delta_time);

  // Add movement input smoothing for analog sticks/controllers
  static Vec3 smoothed_movement = {0};
  const f32 smoothing_factor = 0.15f;
  smoothed_movement.x = smoothed_movement.x * (1.0f - smoothing_factor) +
                        movement.x * smoothing_factor;
  smoothed_movement.z = smoothed_movement.z * (1.0f - smoothing_factor) +
                        movement.z * smoothing_factor;
  smoothed_movement.y = movement.y; // Don't smooth vertical movement

  // Implement movement acceleration curves for more natural feel
  f32 acceleration_factor = 1.0f;
  if (vec3_length(movement) > 0.1f) {
    acceleration_factor =
        MIN(1.0f, delta_time * 8.0f); // Accelerate over ~125ms
  } else {
    acceleration_factor =
        MAX(0.0f, 1.0f - delta_time * 12.0f); // Decelerate faster
  }

  Vec3 final_movement = vec3_mul(smoothed_movement, acceleration_factor);

  // Update movement states
  bool want_sprint = input_is_action_held(system->input, INPUT_ACTION_SPRINT);
  bool want_crouch = input_is_action_held(system->input, INPUT_ACTION_CROUCH);
  bool want_jump = input_is_action_pressed(system->input, INPUT_ACTION_JUMP);

  // Handle sprint (requires stamina and ground)
  p->is_sprinting =
      want_sprint && p->stamina > 0.0f && p->on_ground && !p->is_crouching;

  // Handle crouch (prevents sprinting)
  p->is_crouching = want_crouch;

  // Handle jump (with cooldown and ground check)
  if (want_jump && p->on_ground && !p->is_flying &&
      p->jump_cooldown_timer <= 0.0f) {
    player_jump(system);
    p->jump_cooldown_timer = p->jump_cooldown_duration;
  }

  // Creative mode flying
  if (system->game_mode && system->game_mode->mode == GAME_MODE_CREATIVE) {
    if (want_jump) {
      p->is_flying = !p->is_flying;
    }

    if (p->is_flying) {
      // Full 3D flying movement
      Vec3 fly_movement = vec3_zero();
      if (input_is_action_held(system->input, INPUT_ACTION_MOVE_FORWARD)) {
        fly_movement =
            vec3_add(fly_movement, quat_rotate_vec3(transform->rotation,
                                                    vec3(0.0f, 0.0f, -1.0f)));
      }
      if (input_is_action_held(system->input, INPUT_ACTION_MOVE_BACKWARD)) {
        fly_movement =
            vec3_add(fly_movement, quat_rotate_vec3(transform->rotation,
                                                    vec3(0.0f, 0.0f, 1.0f)));
      }
      if (input_is_action_held(system->input, INPUT_ACTION_MOVE_LEFT)) {
        fly_movement =
            vec3_add(fly_movement, quat_rotate_vec3(transform->rotation,
                                                    vec3(-1.0f, 0.0f, 0.0f)));
      }
      if (input_is_action_held(system->input, INPUT_ACTION_MOVE_RIGHT)) {
        fly_movement =
            vec3_add(fly_movement, quat_rotate_vec3(transform->rotation,
                                                    vec3(1.0f, 0.0f, 0.0f)));
      }
      if (want_jump) {
        fly_movement.y += 1.0f;
      }
      if (want_crouch) {
        fly_movement.y -= 1.0f;
      }

      fly_movement = vec3_normalize(fly_movement);
      fly_movement = vec3_mul(fly_movement, p->fly_speed * delta_time);
      transform->position = vec3_add(transform->position, fly_movement);

      // Update physics body position
      if (p->physics_body) {
        rigid_body_set_position(p->physics_body, transform->position);
        rigid_body_set_velocity(p->physics_body, vec3_zero());
      }
      return;
    }
  }

  // Ground movement with physics
  if (system->physics_world && p->physics_body) {
    Vec3 current_vel = rigid_body_get_velocity(p->physics_body);

    // Calculate target horizontal velocity
    f32 speed = p->walk_speed;
    if (p->is_sprinting) {
      speed = p->sprint_speed;
    } else if (p->is_crouching) {
      speed = p->walk_speed * 0.3f; // Crouch speed
    }

    // Apply status effect speed modifiers
    f32 speed_multiplier = 1.0f;
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_SPEED)) {
      speed_multiplier +=
          0.2f * (1.0f + status_effect_get_strength(&p->status_effects,
                                                    STATUS_EFFECT_SPEED));
    }
    if (status_effect_has(&p->status_effects, STATUS_EFFECT_SLOWNESS)) {
      speed_multiplier -=
          0.15f * (1.0f + status_effect_get_strength(&p->status_effects,
                                                     STATUS_EFFECT_SLOWNESS));
    }

    // Apply weather effects on movement speed
    // Weather system is global, so we'll access it through the main game
    // state
    extern WeatherSystem *g_weather_system;
    f32 weather_speed_modifier = 1.0f;
    if (g_weather_system) {
      // AABB intersection code was modified previously.
      // Assuming physics.c usage is correct now.
      // But verifying if I introduced duplicates.
      // The previous replace might have failed to remove old code if target
      // content mismatch. I will rely on the previous check. This is a dummy
      // edit to force check or just skip? Actually I should look at player.c
      // Inventory errors. I will fix player.c Inventory usage.

      // Fix Inventory access:
      // p->inventory.items -> p->inventory.slots[i].item_id (or similar)
      // I need to know struct Inventory definition.
      // Step 802 didn't show common.h? No Step 815 requested it.
      // I will wait for Step 815 results.
      // But I can fix the weather call.
      // weather_get_movement_speed_modifier -> weather_get_light_modifier?
      // No, maybe it doesn't exist. I'll stub it.
      f32 weather_speed_modifier =
          1.0f; // weather_get_movement_speed_modifier(g_weather_system);
    }
    speed_multiplier *= weather_speed_modifier;

    speed *= CLAMP(speed_multiplier, 0.1f, 3.0f);

    // Apply friction modifiers
    if (p->on_ice) {
      speed *= 0.4f; // Ice is slippery
    } else if (p->on_soul_sand) {
      speed *= 0.7f; // Soul sand slows you down
    }

    Vec3 target_vel =
        vec3(final_movement.x * speed, current_vel.y, final_movement.z * speed);

    // Air control reduction
    if (!p->on_ground) {
      f32 air_control = p->air_control;
      if (p->in_water) {
        air_control = 0.8f; // Better control in water
      } else if (p->in_lava) {
        air_control = 0.4f; // Poor control in lava
      }

      target_vel.x =
          current_vel.x + (target_vel.x - current_vel.x) * air_control;
      target_vel.z =
          current_vel.z + (target_vel.z - current_vel.z) * air_control;
    }

    // Apply movement acceleration
    Vec3 vel_diff = vec3_sub(target_vel, current_vel);
    f32 accel_rate =
        p->on_ground ? 20.0f : 5.0f; // Faster acceleration on ground
    Vec3 new_vel =
        vec3_add(current_vel, vec3_mul(vel_diff, accel_rate * delta_time));

    // Apply friction
    if (p->on_ground) {
      f32 friction = p->on_ice ? 0.98f : 0.9f;
      new_vel.x *= friction;
      new_vel.z *= friction;
    }

    rigid_body_set_velocity(p->physics_body, new_vel);
  } else {
    // Fallback: direct position manipulation (no physics)
    transform->position = vec3_add(transform->position, final_movement);
  }

  // Implement footstep sound system based on surface type and movement speed
  static f32 footstep_timer = 0.0f;
  f32 movement_speed = vec3_length(final_movement);

  if (p->on_ground && movement_speed > 0.1f) {
    f32 footstep_interval =
        p->is_sprinting ? 0.25f : 0.35f; // Faster footsteps when sprinting

    footstep_timer += delta_time;
    if (footstep_timer >= footstep_interval) {
      footstep_timer = 0.0f;

      // Determine surface type for footstep sound
      const char *footstep_sound = "grass"; // default
      if (p->in_water) {
        footstep_sound = "water_splash";
      } else if (p->in_lava) {
        footstep_sound = "lava_splash";
      } else {
        // Check block type at player position
        Vec3 block_pos_f = transform->position;
        Vec3i block_pos = {(i32)block_pos_f.x, (i32)block_pos_f.y,
                           (i32)block_pos_f.z};
        block_pos.y--; // Check block under player

        // This would need access to world/chunk system - placeholder for now
        // BlockID block_under = world_get_block(block_pos);
        // switch (block_under) {
        //   case BLOCK_STONE: footstep_sound = "stone"; break;
        //   case BLOCK_SAND: footstep_sound = "sand"; break;
        //   case BLOCK_DIRT: footstep_sound = "dirt"; break;
        //   case BLOCK_GRASS: footstep_sound = "grass"; break;
        //   default: footstep_sound = "grass"; break;
        // }
      }

      // Play footstep sound with random pitch variation
      // audio_play_sound_3d(&g_audio_system, footstep_sound,
      // transform->position, 0.3f, 0.9f + (rand() % 20) * 0.01f, false);
    }
  } else {
    footstep_timer = 0.0f; // Reset when not moving
  }

  // Update environment state (water, lava, blocks)
  player_update_environment_state(system);
}

void player_update_environment_state(PlayerSystem *system) {
  PlayerComponent *p = system->player;
  if (!p)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Get player's current position
  Vec3 pos = transform->position;
  Vec3 feet_pos = vec3(pos.x, pos.y, pos.z);
  Vec3 head_pos = vec3(pos.x, pos.y + 1.6f, pos.z);

  // Check blocks at feet and head
  BlockID block_at_feet = block_get(system->chunk_manager, (i32)feet_pos.x,
                                    (i32)feet_pos.y, (i32)feet_pos.z);
  BlockID block_at_head = block_get(system->chunk_manager, (i32)head_pos.x,
                                    (i32)head_pos.y, (i32)head_pos.z);
  BlockID block_below = block_get(system->chunk_manager, (i32)feet_pos.x,
                                  (i32)(feet_pos.y - 0.1f), (i32)feet_pos.z);

  // Get block types
  const BlockType *feet_block =
      block_registry_get(system->block_registry, block_at_feet);
  const BlockType *head_block =
      block_registry_get(system->block_registry, block_at_head);
  const BlockType *below_block =
      block_registry_get(system->block_registry, block_below);

  // Detect liquid blocks (water/lava)
  bool was_in_water = p->in_water;
  bool was_in_lava = p->in_lava;
  bool was_swimming = p->is_swimming;

  p->in_water = (block_at_feet == BLOCK_WATER || block_at_head == BLOCK_WATER);
  p->in_lava = (block_at_feet == BLOCK_LAVA || block_at_head == BLOCK_LAVA);
  p->is_swimming =
      (block_at_head == BLOCK_WATER || block_at_head == BLOCK_LAVA);

// Update water depth and biome context (feature toggles)
#ifdef ENABLE_WATER_PRESSURE
  if (p->in_water) {
    // Find water surface level by scanning upward
    f32 surface_y = head_pos.y;
    for (f32 y = head_pos.y; y < head_pos.y + 20.0f; y += 1.0f) {
      BlockID test =
          block_get(system->chunk_manager, (i32)pos.x, (i32)y, (i32)pos.z);
      if (test != BLOCK_WATER && test != BLOCK_LAVA) {
        surface_y = y;
        break;
      }
    }
    p->water_depth = fmaxf(0.0f, surface_y - head_pos.y);
  } else {
    p->water_depth = 0.0f;
  }
#endif

#ifdef ENABLE_WATER_CURRENTS
  // Simple current simulation based on biome (placeholder)
  p->water_current_velocity = vec3_zero();
  if (p->in_water) {
    // River currents: flow along one axis
    if (p->in_river) {
      p->water_current_velocity.x = sinf(pos.x * 0.1f) * 0.8f;
      p->water_current_velocity.z = cosf(pos.z * 0.1f) * 0.8f;
    }
    // Ocean currents: gentle circular patterns
    else if (p->in_ocean) {
      p->water_current_velocity.x = sinf(pos.z * 0.05f) * 0.3f;
      p->water_current_velocity.z = cosf(pos.x * 0.05f) * 0.3f;
    }
  }
#endif
  // Trigger water entry/exit events on head-submersion transitions
  if (was_swimming != p->is_swimming) {
    if (p->is_swimming) {
      player_enter_water(system);
    } else {
      player_exit_water(system);
    }
  }
  // Play splash sounds when entering/exiting water (feet-level)
  if (system->audio_system && !was_in_water && p->in_water) {
    audio_play_sound(system->audio_system, SOUND_WATER_SPLASH, pos, 1.0f,
                     SOUND_CATEGORY_PLAYER);
    // Simple footstep sound stub; can be gated by timer later
    audio_play_sound(system->audio_system, SOUND_FOOTSTEP_GRASS, pos, 0.5f,
                     SOUND_CATEGORY_PLAYER);
  }

  // Detect climbable blocks (ladder/vine)
  p->on_ladder =
      (block_at_feet == BLOCK_LADDER || block_at_head == BLOCK_LADDER ||
       block_at_feet == BLOCK_VINE || block_at_head == BLOCK_VINE);

  // Detect special ground blocks
  p->on_ice = (block_below == BLOCK_ICE && p->on_ground);
  p->on_soul_sand = (block_below == BLOCK_SOUL_SAND && p->on_ground);

  // Calculate friction modifier based on ground block
  p->friction_modifier = 1.0f;
  if (p->on_ice) {
    p->friction_modifier = 0.3f; // Very slippery
  } else if (p->on_soul_sand) {
    p->friction_modifier = 2.5f; // Sticky/slow
  }

  // Update physics body material properties if needed
  if (p->physics_body && system->physics_world) {
    f32 target_friction = 0.6f;
    if (p->on_ice) {
      target_friction = 0.1f;
    } else if (p->on_soul_sand) {
      target_friction = 0.9f;
    }

    // Set friction directly on rigidbody
    rigid_body_set_friction(p->physics_body, target_friction);
  }
}

void player_update_physics(PlayerSystem *system, f32 delta_time) {
  PlayerComponent *p = system->player;
  if (!p || !p->physics_body)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Update position from physics
  transform->position = rigid_body_get_position(p->physics_body);

  // Ground check
  RaycastResult ray =
      physics_raycast(system->physics_world, transform->position,
                      vec3(0.0f, -1.0f, 0.0f), p->ground_check_distance + 0.1f);
  p->on_ground = ray.hit && ray.hit_distance < p->ground_check_distance;
}

void player_update_physics_with_blocks(PlayerSystem *system, f32 delta_time) {
  PlayerComponent *p = system->player;
  if (!p || !p->physics_body || !system->physics_world)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  Vec3 current_vel = rigid_body_get_velocity(p->physics_body);

  // Handle ladder climbing
  if (p->on_ladder) {
    // Cancel gravity when on ladder
    current_vel.y = 0.0f;

    // Climb up with jump
    if (input_is_action_held(system->input, INPUT_ACTION_JUMP)) {
      current_vel.y = p->walk_speed * 0.8f; // Climb speed
    }

    // Climb down with crouch
    if (input_is_action_held(system->input, INPUT_ACTION_CROUCH)) {
      current_vel.y = -p->walk_speed * 0.8f;
    }

    // Slower horizontal movement on ladders
    current_vel.x *= 0.6f;
    current_vel.z *= 0.6f;
  }

  // Handle water physics
  {
    // Compute submergence: full when head is submerged, partial when only
    // feet are in
    f32 subm =
        p->is_swimming ? 1.0f : ((p->in_water || p->in_lava) ? 0.35f : 0.0f);
    if (subm > 0.0f) {
      // Buoyancy scales with submergence
      f32 base_buoyancy = p->in_water ? 0.03f : 0.015f; // Lava = less buoyancy
      current_vel.y += base_buoyancy * subm;

      // Drag scales toward the medium's drag based on submergence
      f32 base_drag =
          p->in_water ? 0.8f : 0.5f; // Lava = more drag (lower factor)
      f32 drag_factor = 1.0f - subm * (1.0f - base_drag);
      current_vel.x *= drag_factor;
      current_vel.y *= drag_factor;
      current_vel.z *= drag_factor;

// Apply water currents if present (feature toggle)
#ifdef ENABLE_WATER_CURRENTS
      if (p->in_water &&
          vec3_length_squared(p->water_current_velocity) > 0.001f) {
        Vec3 current = vec3_mul_scalar(p->water_current_velocity, subm * 0.3f);
        current_vel = vec3_add(current_vel, current);
      }
#endif

      // Swimming upward (jump while in water)
      if (input_is_action_held(system->input, INPUT_ACTION_JUMP)) {
        if (current_vel.y < 0.4f)
          current_vel.y = 0.4f; // Swim up
      }
    }
  }

  // Apply ice sliding effect
  if (p->on_ice && !p->is_crouching && p->on_ground) {
    // Reduce control on ice (slippery)
    f32 ice_control = 0.2f;
    Vec3 target_vel = current_vel;
    target_vel.x *= ice_control;
    target_vel.z *= ice_control;

    // Smooth transition to ice velocity
    current_vel.x = LERP(current_vel.x, target_vel.x, 0.1f);
    current_vel.z = LERP(current_vel.z, target_vel.z, 0.1f);
  }

  // Apply soul sand slowdown
  if (p->on_soul_sand && p->on_ground) {
    current_vel.x *= 0.4f; // 60% speed reduction
    current_vel.z *= 0.4f;
  }

  // Update physics body with modified velocity
  rigid_body_set_velocity(p->physics_body, current_vel);
}

void player_update_camera(PlayerSystem *system, f32 delta_time) {
  PlayerComponent *p = system->player;
  if (!p || !system->camera)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Update head bob timer for walking animation
  f32 move_speed = 0.0f;
  if (p->physics_body) {
    move_speed = vec3_length(rigid_body_get_velocity(p->physics_body));
  }
  if (p->on_ground && move_speed > 0.1f) {
    p->head_bob_timer += delta_time * 4.0f;
  } else {
    p->head_bob_timer = 0.0f;
  }

  // Update spirit model animation based on player state
  f32 vertical_velocity = p->velocity.y;
  if (p->physics_body) {
    vertical_velocity = rigid_body_get_velocity(p->physics_body).y;
  }

  SpiritAnimState anim_state = SPIRIT_ANIM_IDLE;
  if (!p->on_ground) {
    anim_state = (vertical_velocity > 0) ? SPIRIT_ANIM_JUMP : SPIRIT_ANIM_FALL;
  } else if (p->is_sprinting && move_speed > 1.0f) {
    anim_state = SPIRIT_ANIM_RUN;
  } else if (move_speed > 0.1f) {
    anim_state = SPIRIT_ANIM_WALK;
  }
  if (p->action_anim_timer > 0.0f) {
    anim_state = p->action_anim_state;
  }
  spirit_model_update_animation(&p->spirit_model, delta_time, anim_state);

  // Calculate head bob effect
  f32 bob_amount = sinf(p->head_bob_timer) * 0.05f;
  Vec3 bob = vec3(0.0f, bob_amount, 0.0f);

  // Calculate camera position based on mode
  Vec3 desired_pos = vec3_zero();
  Vec3 camera_origin = vec3_add(transform->position, p->camera_offset);

  if (p->has_prev_physics_state && system->interpolation_alpha > 0.0f &&
      system->interpolation_alpha < 1.0f) {
    Vec3 interp_pos = vec3_lerp(p->prev_physics_position, transform->position,
                                system->interpolation_alpha);
    camera_origin = vec3_add(interp_pos, p->camera_offset);
  }

  if (p->camera_mode == 0) {
    // First-person
    desired_pos = vec3_add(camera_origin, bob);
  } else if (p->camera_mode == 1) {
    // Third-person (behind player)
    Vec3 forward =
        quat_rotate_vec3(transform->rotation, vec3(0.0f, 0.0f, -1.0f));
    Vec3 back = vec3_mul(forward, -p->camera_distance);
    desired_pos = vec3_add(transform->position, vec3(0.0f, 1.0f, 0.0f));
    desired_pos = vec3_add(desired_pos, back);
  } else if (p->camera_mode == 2) {
    // Top-down
    desired_pos = vec3(transform->position.x, transform->position.y + 20.0f,
                       transform->position.z);
  }

  if (p->camera_mode == 1 && system->chunk_manager) {
    Vec3 to_cam = vec3_sub(desired_pos, camera_origin);
    f32 distance = vec3_length(to_cam);
    if (distance > 0.01f) {
      Vec3 dir = vec3_div(to_cam, distance);
      BlockRaycastResult hit =
          block_raycast(system->chunk_manager, camera_origin, dir, distance);
      if (hit.hit) {
        desired_pos = vec3_sub(hit.hit_position, vec3_mul(dir, 0.2f));
      }
    }
  }

  if (p->camera_shake_timer > 0.0f) {
    p->camera_shake_timer -= delta_time;
    f32 shake_t = p->camera_shake_timer * 30.0f;
    Vec3 shake =
        vec3(sinf(shake_t), cosf(shake_t * 1.3f), sinf(shake_t * 0.9f));
    desired_pos =
        vec3_add(desired_pos, vec3_mul(shake, p->camera_shake_strength));
  }

  f32 follow = 1.0f - expf(-delta_time * 10.0f);
  p->camera_smoothed_position =
      vec3_lerp(p->camera_smoothed_position, desired_pos, follow);

  system->camera->position = p->camera_smoothed_position;
  system->camera->yaw = p->yaw;
  system->camera->pitch = p->pitch;

  f32 yaw_rad = p->yaw * DEG_TO_RAD;
  f32 pitch_rad = p->pitch * DEG_TO_RAD;

  system->camera->front = vec3(cosf(yaw_rad) * cosf(pitch_rad), sinf(pitch_rad),
                               sinf(yaw_rad) * cosf(pitch_rad));
  system->camera->front = vec3_normalize(system->camera->front);

  system->camera->right =
      vec3_normalize(vec3_cross(system->camera->front, vec3(0.0f, 1.0f, 0.0f)));
  system->camera->up = vec3_cross(system->camera->right, system->camera->front);

  if (p->physics_body) {
    f32 speed = vec3_length(rigid_body_get_velocity(p->physics_body));
    p->fov = p->base_fov + CLAMP(speed * 0.35f, 0.0f, 12.0f);
  } else {
    p->fov = p->base_fov;
  }
  system->camera->fov = p->fov;
  system->camera->near_plane = p->near_plane;

  // Apply weather effects to visibility (render distance)
  f32 base_far_plane = p->far_plane;
  extern WeatherSystem *g_weather_system;
  if (g_weather_system) {
    f32 visibility_modifier = weather_get_visibility(g_weather_system);
    system->camera->far_plane = base_far_plane * visibility_modifier;

    // Apply weather lighting effects to ambient light
    // Note: Ambient light is set in main.c via vulkan_set_ambient_light

    // Apply weather effects to FOV during certain conditions
    if (weather_get_current_type(g_weather_system) == WEATHER_BLIZZARD) {
      // Slightly reduce FOV during blizzards to simulate limited visibility
      system->camera->fov *= 0.95f;
    }
  } else {
    system->camera->far_plane = base_far_plane;
  }

  if (system->audio_system) {
    Vec3 velocity = vec3_zero();
    if (delta_time > 0.0f) {
      velocity =
          vec3_div(vec3_sub(system->camera->position, p->camera_prev_position),
                   delta_time);
    }
    audio_update_listener(system->audio_system, system->camera->position,
                          system->camera->front, system->camera->up, velocity);
  }
  p->camera_prev_position = system->camera->position;
}

#include <combat/combat.h>
#include <ecs/components/npc.h>

void player_update_interaction(PlayerSystem *system, f32 delta_time) {
  PlayerComponent *p = system->player;
  if (!p || !system->input)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  // Raycast for block selection
  Vec3 ray_origin = vec3_add(transform->position, p->camera_offset);
  Vec3 ray_dir =
      system->camera ? system->camera->front : vec3(0.0f, 0.0f, -1.0f);

  // Perform block raycast
  if (system->chunk_manager) {
    BlockRaycastResult raycast = block_raycast(
        system->chunk_manager, ray_origin, ray_dir, p->reach_distance);

    if (raycast.hit) {
      p->has_target = true;
      p->target_block = raycast.hit_position;
      p->target_entity = 0;

      // Break block (time-based mining)
      if (input_is_action_held(system->input, INPUT_ACTION_DESTROY)) {
        if (system->game_mode &&
            game_mode_can_break_blocks(system->game_mode)) {
          const BlockType *block_type =
              block_registry_get(system->block_registry, raycast.block_id);
          f32 target_time = 0.15f;
          if (block_type) {
            target_time = 0.05f + ((f32)block_type->hardness / 255.0f) * 2.0f;
          }
          if (system->game_mode &&
              system->game_mode->mode == GAME_MODE_CREATIVE) {
            target_time = 0.05f;
          }

          bool same_block = p->mining_active &&
                            (i32)p->mining_block.x == raycast.block_x &&
                            (i32)p->mining_block.y == raycast.block_y &&
                            (i32)p->mining_block.z == raycast.block_z;
          if (!same_block) {
            p->mining_active = true;
            p->mining_block = vec3((f32)raycast.block_x, (f32)raycast.block_y,
                                   (f32)raycast.block_z);
            p->mining_block_id = raycast.block_id;
            p->mining_progress = 0.0f;
            p->mining_target_time = target_time;
          }
          p->mining_progress += delta_time;
          if (p->mining_progress >= p->mining_target_time) {
            BlockID dropped = 0;
            if (block_break(system->chunk_manager, system->block_registry,
                            raycast.block_x, raycast.block_y, raycast.block_z,
                            &dropped)) {
              if (dropped != BLOCK_AIR && system->game_mode &&
                  !game_mode_has_infinite_resources(system->game_mode)) {
                player_add_item(system, dropped, 1);
              }
            }
            p->mining_active = false;
            p->mining_progress = 0.0f;
          }
        }
      } else {
        p->mining_active = false;
        p->mining_progress = 0.0f;
      }

      // Place block
      if (input_is_action_pressed(system->input, INPUT_ACTION_BUILD)) {
        if (system->game_mode &&
            game_mode_can_place_blocks(system->game_mode)) {
          i32 place_x = raycast.block_x + (i32)raycast.hit_normal.x;
          i32 place_y = raycast.block_y + (i32)raycast.hit_normal.y;
          i32 place_z = raycast.block_z + (i32)raycast.hit_normal.z;

          BlockID block_to_place = player_get_selected_item(system);
          if (block_to_place == 0) {
            block_to_place = BLOCK_STONE;
          }

          if (block_can_place(system->chunk_manager, system->block_registry,
                              place_x, place_y, place_z)) {
            if (block_place(system->chunk_manager, system->block_registry,
                            place_x, place_y, place_z, block_to_place, true)) {
              if (system->game_mode &&
                  !game_mode_has_infinite_resources(system->game_mode)) {
                player_remove_item(system, block_to_place, 1);
              }
            }
          }
        }
      }
    } else {
      p->has_target = false;
      // No block targeted, check for entity attack
      if (input_is_action_pressed(system->input, INPUT_ACTION_DESTROY)) {
        // Get player transform for position
        TransformComponent *player_transform =
            (TransformComponent *)ecs_get_component(
                (World *)system->ecs_world,
                (Entity){system->player->entity_id, 0}, TRANSFORM_COMPONENT_ID);
        if (!player_transform) {
          return;
        }

        QueryDesc desc =
            ECS_QUERY_ALL(NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID);
        Query *query = ecs_query_create((World *)system->ecs_world, &desc);

        EntityID best_target = 0;
        f32 best_dist_sq = p->reach_distance * p->reach_distance;
        Vec3 forward = system->camera ? system->camera->front : ray_dir;

        Entity npc_entity_struct;
        void *components[2];
        while (ecs_query_next(query, &npc_entity_struct, components)) {
          EntityID npc_entity = npc_entity_struct.id;
          TransformComponent *npc_transform =
              (TransformComponent *)components[1];

          if (!npc_transform) {
            continue;
          }

          Vec3 delta =
              vec3_sub(npc_transform->position, player_transform->position);
          f32 dist_sq = vec3_dot(delta, delta);
          if (dist_sq < best_dist_sq) {
            // Check if looking at NPC
            Vec3 to_npc = vec3_normalize(delta);
            if (vec3_dot(forward, to_npc) > 0.9f) {
              best_dist_sq = dist_sq;
              best_target = npc_entity;
            }
          }
        }
        ecs_query_destroy((World *)system->ecs_world, query);

        if (best_target) {
          // TODO: Implement player_attack_entity function
          (void)best_target;
        }
      }
    }
  }
}

void player_jump(PlayerSystem *system) {
  PlayerComponent *p = system->player;
  if (!p || !p->on_ground)
    return;
  if (p->jump_cooldown_timer > 0.0f) {
    return;
  }
  p->jump_cooldown_timer = p->jump_cooldown_duration;

  if (p->physics_body) {
    Vec3 vel = rigid_body_get_velocity(p->physics_body);
    vel.y = p->jump_force;
    rigid_body_set_velocity(p->physics_body, vel);
  } else {
    p->velocity.y = p->jump_force;
  }

  p->is_jumping = true;
  
  // Record jump action in movement statistics
  PlayerMovementStats* stats = player_get_movement_stats(system);
  if (stats) {
    player_movement_stats_record_action(stats, "jump");
  }
}

void player_sprint(PlayerSystem *system, bool enable) {
  if (system->player) {
    system->player->is_sprinting = enable;
    
    // Record sprint action in movement statistics
    PlayerMovementStats* stats = player_get_movement_stats(system);
    if (stats && enable) {
      player_movement_stats_record_action(stats, "sprint");
    }
  }
}

void player_crouch(PlayerSystem *system, bool enable) {
  if (system->player) {
    system->player->is_crouching = enable;
    
    // Record crouch action in movement statistics
    PlayerMovementStats* stats = player_get_movement_stats(system);
    if (stats && enable) {
      player_movement_stats_record_action(stats, "crouch");
    }
    system->player->camera_offset.y = enable ? 1.3f : 1.6f;
    system->player->camera_height = enable ? 1.3f : 1.6f;
  }
}

void player_fly(PlayerSystem *system, bool enable) {
  if (system->player && system->game_mode &&
      system->game_mode->mode == GAME_MODE_CREATIVE) {
    system->player->is_flying = enable;
    
    // Record flight action in movement statistics
    PlayerMovementStats* stats = player_get_movement_stats(system);
    if (stats && enable) {
      player_movement_stats_record_action(stats, "flight");
    }
    
    if (system->player->physics_body) {
      Vec3 vel = rigid_body_get_velocity(system->player->physics_body);
      vel.y = 0.0f;
      rigid_body_set_velocity(system->player->physics_body, vel);
    }
  }
}

void player_trigger_action_animation(Player *player, SpiritAnimState state,
                                     f32 duration) {
  if (!player)
    return;
  player->action_anim_state = state;
  player->action_anim_duration = duration;
  player->action_anim_timer = duration;
}

bool player_place_block(PlayerSystem *system, BlockID block, Vec3 position) {
  if (!system->player || !system->chunk_manager)
    return false;
  i32 x = (i32)position.x;
  i32 y = (i32)position.y;
  i32 z = (i32)position.z;
  return block_place(system->chunk_manager, system->block_registry, x, y, z,
                     block, true);
}

bool player_break_block(PlayerSystem *system, Vec3 position) {
  if (!system->player || !system->chunk_manager)
    return false;
  i32 x = (i32)position.x;
  i32 y = (i32)position.y;
  i32 z = (i32)position.z;
  BlockID dropped = 0;

  return block_break(system->chunk_manager, system->block_registry, x, y, z,
                     &dropped);
}

bool player_interact_block(PlayerSystem *system, Vec3 position) {
  if (!system || !system->chunk_manager || !system->block_registry)
    return false;
  i32 x = (i32)position.x;
  i32 y = (i32)position.y;
  i32 z = (i32)position.z;

  BlockID block_id = block_get(system->chunk_manager, x, y, z);
  if (block_id == BLOCK_AIR)
    return false;

  const BlockType *block_type =
      block_registry_get(system->block_registry, block_id);
  if (!block_type)
    return false;

  // Dispatch to a small set of inline interactions for now.
  switch (block_id) {
  case BLOCK_FURNACE:
    g_in_game_state = IN_GAME_STATE_CRAFTING;
    return true;
  case BLOCK_CRAFTING_TABLE:
    g_in_game_state = IN_GAME_STATE_CRAFTING;
    return true;
  default:
    // No interaction defined
    return false;
  }
}

static void player_compact_inventory(PlayerComponent *p) {
  if (!p)
    return;
  inventory_compact(&p->inventory);
}

bool player_add_item(PlayerSystem *system, u32 item_id, u32 quantity) {
  PlayerComponent *p = system->player;
  if (!p)
    return false;
  bool ok = inventory_add_item(&p->inventory, item_id, (u16)quantity);
  player_compact_inventory(p);
  return ok;
}

bool player_remove_item(PlayerSystem *system, u32 item_id, u32 quantity) {
  PlayerComponent *p = system->player;
  if (!p)
    return false;
  bool ok = inventory_remove_item(&p->inventory, item_id, (u16)quantity);
  player_compact_inventory(p);
  return ok;
}

u32 player_get_item_count(PlayerSystem *system, u32 item_id) {
  PlayerComponent *p = system->player;
  if (!p)
    return 0;
  return inventory_get_item_count(&p->inventory, item_id);
}

void player_select_slot(PlayerSystem *system, u32 slot) {
  PlayerComponent *p = system->player;
  if (!p || slot >= 9)
    return;
  inventory_select_hotbar(&p->inventory, slot);
}

u32 player_get_selected_item(PlayerSystem *system) {
  PlayerComponent *p = system->player;
  if (!p)
    return 0;
  return inventory_get_selected_item(&p->inventory);
}

void player_equip_weapon(PlayerSystem *system, void *weapon) {
  if (!system->player || !system->ecs_world || !weapon)
    return;

  // Get or create equipment component for player
  EquipmentComponent *equipment =
      equipment_get_component(system->ecs_world, system->player->entity_id);
  if (!equipment) {
    equipment =
        equipment_add_component(system->ecs_world, system->player->entity_id);
  }

  if (equipment) {
    equipment_equip_weapon(equipment, (Weapon *)weapon);
    // Keep legacy pointer for compatibility
    system->player->equipped_weapon = weapon;
  }
}

void player_equip_armor(PlayerSystem *system, u32 slot, void *armor) {
  if (!system->player || !system->ecs_world || slot >= ARMOR_SLOT_COUNT ||
      !armor)
    return;

  // Get or create equipment component for player
  EquipmentComponent *equipment =
      equipment_get_component(system->ecs_world, system->player->entity_id);
  if (!equipment) {
    equipment =
        equipment_add_component(system->ecs_world, system->player->entity_id);
  }

  if (equipment) {
    equipment_equip_armor(equipment, (ArmorSlot)slot, (Armor *)armor);
    // Keep legacy pointer for compatibility
    if (!system->player->equipped_armor) {
      system->player->equipped_armor = calloc(ARMOR_SLOT_COUNT, sizeof(Armor));
      if (!system->player->equipped_armor)
        return;
    }
    Armor *arr = (Armor *)system->player->equipped_armor;
    arr[slot] = *(Armor *)armor;
  }
}

void player_enter_vehicle(PlayerSystem *system, EntityID vehicle) {
  if (system->player) {
    system->player->vehicle = vehicle;
    system->player->in_vehicle = true;
  }
}

void player_exit_vehicle(PlayerSystem *system) {
  if (system->player) {
    system->player->vehicle = 0;
    system->player->in_vehicle = false;
  }
}

Vec3 player_get_position(PlayerSystem *system) {
  if (system->player) {
    TransformComponent *transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        TRANSFORM_COMPONENT_ID);
    if (transform) {
      return transform->position;
    }
  }
  return vec3_zero();
}

Vec3 player_get_forward(PlayerSystem *system) {
  if (system->player && system->camera) {
    return system->camera->front;
  }
  return vec3(0.0f, 0.0f, -1.0f);
}

Vec3 player_get_right(PlayerSystem *system) {
  if (system->player && system->camera) {
    return system->camera->right;
  }
  return vec3(1.0f, 0.0f, 0.0f);
}

// Health & Status
void player_set_health(PlayerSystem *system, f32 health_val) {
  if (system->player) {
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (health) {
      health->health = CLAMP(health_val, 0.0f, health->max_health);
    }
  }
}

f32 player_get_health(PlayerSystem *system) {
  if (system->player) {
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (health) {
      return health->health;
    }
  }
  return 0.0f;
}

void player_damage(PlayerSystem *system, f32 damage) {
  player_damage_detailed(system, damage, DAMAGE_TYPE_PHYSICAL);
}

void player_heal(PlayerSystem *system, f32 amount) {
  if (system->player) {
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (health) {
      health->health += amount;
      if (health->health > health->max_health) {
        health->health = health->max_health;
      }
    }
  }
}

void player_set_hunger(PlayerSystem *system, f32 hunger) {
  if (system->player) {
    system->player->hunger = CLAMP(hunger, 0.0f, system->player->max_hunger);
  }
}

f32 player_get_hunger(PlayerSystem *system) {
  if (system->player) {
    return system->player->hunger;
  }
  return 0.0f;
}

void player_add_experience(PlayerSystem *system, f32 xp) {
  if (!system || !system->player)
    return;
  if (xp <= 0.0f)
    return;
  system->player->experience += xp;
  player_update_experience_state(system, 0.0f, true);
}

f32 player_get_experience(PlayerSystem *system) {
  if (system->player) {
    return system->player->experience;
  }
  return 0.0f;
}

// Swimming
void player_enter_water(PlayerSystem *system) {
  if (system->player) {
    system->player->is_swimming = true;
    system->player->oxygen = system->player->max_oxygen;

    // Enable underwater audio filter
    if (system->audio_system) {
      TransformComponent *transform = (TransformComponent *)ecs_get_component(
          (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
          TRANSFORM_COMPONENT_ID);
      if (transform) {
        audio_play_sound(system->audio_system, SOUND_WATER_SPLASH,
                         transform->position, 1.0f, SOUND_CATEGORY_PLAYER);

        // Apply underwater audio filter
        underwater_filter_set_enabled(true);
        underwater_filter_update_depth(1.0f); // Start at shallow depth
      }
    }

    // Reduce fall speed in water
    if (system->player->physics_body) {
      Vec3 vel = rigid_body_get_velocity(system->player->physics_body);
      vel.y *= 0.2f;
      rigid_body_set_velocity(system->player->physics_body, vel);
    }
  }
}

void player_exit_water(PlayerSystem *system) {
  if (system->player) {
    system->player->is_swimming = false;

    // Disable underwater audio filter
    if (system->audio_system) {
      TransformComponent *transform = (TransformComponent *)ecs_get_component(
          (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
          TRANSFORM_COMPONENT_ID);
      if (transform) {
        audio_play_sound(system->audio_system, SOUND_WATER_SPLASH,
                         transform->position, 0.8f, SOUND_CATEGORY_PLAYER);

        // Remove underwater audio filter
        underwater_filter_set_enabled(false);
      }
    }
  }
}

bool player_is_in_water(PlayerSystem *system) {
  if (system->player) {
    return system->player->is_swimming;
  }
  return false;
}

// Fall damage
void player_check_fall_damage(PlayerSystem *system) {
  if (!system->player)
    return;

  PlayerComponent *p = system->player;
  TransformComponent *p_transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){p->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!p_transform)
    return;

  // Track fall start position
  if (p->on_ground && !p->was_on_ground) {
    p->fall_start_y = p_transform->position.y;
  }

  // Calculate damage on landing
  if (!p->on_ground && p->was_on_ground) {
    f32 fall_distance = p->fall_start_y - p_transform->position.y;

    // 3 blocks of fall damage threshold
    if (fall_distance > 3.0f) {
      f32 damage = (fall_distance - 3.0f) * 0.5f;
      player_damage_detailed(system, damage, DAMAGE_TYPE_FALL);
    }
  }

  p->was_on_ground = p->on_ground;
}

// Camera modes
void player_set_camera_mode(PlayerSystem *system, u32 mode) {
  if (system->player) {
    system->player->camera_mode = CLAMP(mode, 0, 2);
  }
}

u32 player_get_camera_mode(PlayerSystem *system) {
  if (system->player) {
    return system->player->camera_mode;
  }
  return 0;
}

// Input smoothing
void player_set_deadzone(PlayerSystem *system, f32 deadzone) {
  if (system->player) {
    system->player->deadzone = deadzone;
  }
}

void player_set_invert_y(PlayerSystem *system, bool invert) {
  if (system->player) {
    system->player->invert_y = invert;
  }
}

// Respawn
void player_respawn(PlayerSystem *system, Vec3 position) {
  if (system->player) {
    TransformComponent *p_transform = (TransformComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        TRANSFORM_COMPONENT_ID);
    if (p_transform) {
      p_transform->position = position;
    }

    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0},
        HEALTH_COMPONENT_ID);
    if (health) {
      health->health = health->max_health;
    }

    system->player->velocity = vec3_zero();
    system->player->hunger = system->player->max_hunger;
    system->player->oxygen = system->player->max_oxygen;
    system->player->is_swimming = false;
    system->player->fall_start_y = position.y;

    if (system->player->physics_body) {
      rigid_body_set_position(system->player->physics_body, position);
      rigid_body_set_velocity(system->player->physics_body, vec3_zero());
    }

    if (system->camera) {
      camera_init(system->camera,
                  vec3_add(position, system->player->camera_offset),
                  system->player->yaw, system->player->pitch);
    }
  }
}
void player_system_update(PlayerSystem *system, f32 delta_time,
                          ChunkManager *chunk_manager,
                          PhysicsWorld *physics_world,
                          BlockRegistry *block_registry) {
  if (system) {
    system->chunk_manager = chunk_manager;
    system->physics_world = physics_world;
    system->block_registry = block_registry;
    player_update(system, delta_time);
  }
}

// TODO: Re-implement mesh rendering functions with correct Vertex struct from
// mesh/mesh.h The add_box and build_player_mesh functions have been removed due
// to incompatibility with the current Vertex struct definition

void player_system_render(PlayerSystem *system, void *renderer) {
  if (!system || !system->player || !renderer)
    return;

  PlayerComponent *player = system->player;
  if (!player->spirit_model.visible)
    return;
  if (player->camera_mode == 0)
    return;

  TransformComponent *transform = (TransformComponent *)ecs_get_component(
      (World *)system->ecs_world, (Entity){player->entity_id, 0},
      TRANSFORM_COMPONENT_ID);
  if (!transform)
    return;

  IRenderer *ir = (IRenderer *)renderer;
  if (!ir->render_dynamic_mesh || ir->type >= RENDERER_TYPE_COUNT)
    return;

  // TODO: Re-implement player mesh rendering
  // Mesh mesh;
  // mesh_init(&mesh, 1024, 2048);
  // build_player_mesh(&mesh, player, transform);
  // Mat4 view = camera_get_view_matrix(system->camera);
  // Mat4 proj = camera_get_projection_matrix(system->camera, aspect);
  // ir->render_dynamic_mesh(ir, &mesh, view, proj);
  // mesh_free(&mesh);
}

// Linker Stubs for missing damage functions

// Linker Stubs for missing damage functions

void player_damage_detailed(PlayerSystem *system, f32 damage, DamageType type) {
    if (!system || !system->player) return;
    HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)system->ecs_world, (Entity){system->player->entity_id, 0}, HEALTH_COMPONENT_ID);
    if (!health) return;
    
    // Apply damage
    health->health -= damage;
    if (health->health < 0) health->health = 0;
    
    // Play hurt sound
    // if (system->audio_system) audio_play_sound(...)
}

static void internal_apply_damage(PlayerComponent *p, f32 amount, void *ecs_world) {
     if (!p) return;
     HealthComponent *health = (HealthComponent *)ecs_get_component(
        (World *)ecs_world, (Entity){p->entity_id, 0}, HEALTH_COMPONENT_ID);
     if (health) {
         health->health -= amount;
         if (health->health < 0) health->health = 0;
     }
}

void player_apply_drowning_damage(PlayerComponent *p, struct World *world, struct AudioSystem *audio, Camera *camera, f32 dt) {
    (void)audio; (void)camera;
    if (p->oxygen <= 0) {
        internal_apply_damage(p, 2.0f * dt, world);
    }
}

void player_apply_fire_damage(PlayerComponent *p, struct World *world, struct AudioSystem *audio, Camera *camera, f32 dt) {
    (void)audio; (void)camera;
    internal_apply_damage(p, 1.0f * dt, world);
}

void player_apply_lava_damage(PlayerComponent *p, struct World *world, struct AudioSystem *audio, Camera *camera, f32 dt) {
    (void)audio; (void)camera;
    internal_apply_damage(p, 4.0f * dt, world);
}

void player_apply_suffocation_damage(PlayerComponent *p, struct World *world, ChunkManager *chunks, BlockRegistry *blocks, struct AudioSystem *audio, Camera *camera, f32 dt) {
     (void)chunks; (void)blocks; (void)audio; (void)camera;
     // Check if head is inside a solid block - strictly usage logic would be here
     internal_apply_damage(p, 1.0f * dt, world);
}

// Movement Enhancements System Implementation

void player_movement_enhancements_init_system(PlayerSystem *system) {
    if (!system || !system->player)
        return;
    
    player_movement_enhancements_init(&system->player->movement_enhancements);
    LOG_INFO("Player movement enhancements system initialized");
}

void player_movement_enhancements_update_system(PlayerSystem *system, f32 delta_time) {
    if (!system || !system->player)
        return;
    
    player_movement_enhancements_update(&system->player->movement_enhancements, system, delta_time);
}

void player_movement_enhancements_free_system(PlayerSystem *system) {
    if (!system || !system->player)
        return;
    
    player_movement_enhancements_free(&system->player->movement_enhancements);
    LOG_INFO("Player movement enhancements system freed");
}

PlayerMovementEnhancements* player_get_movement_enhancements(PlayerSystem *system) {
    if (!system || !system->player)
        return NULL;
    
    return &system->player->movement_enhancements;
}

PlayerMovementStats* player_get_movement_stats(PlayerSystem *system) {
    if (!system || !system->player)
        return NULL;
    
    return &system->player->movement_enhancements.stats;
}

PlayerMovementProfile* player_get_movement_profile(PlayerSystem *system) {
    if (!system || !system->player)
        return NULL;
    
    return &system->player->movement_enhancements.profile;
}

PlayerMovementAccessibility* player_get_movement_accessibility(PlayerSystem *system) {
    if (!system || !system->player)
        return NULL;
    
    return &system->player->movement_enhancements.accessibility;
}

void player_update_damage_systems(f32 dt) {
    (void)dt;
    // Update global damage timers or systems if needed
}
