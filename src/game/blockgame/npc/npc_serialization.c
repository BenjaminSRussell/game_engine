// NPC serialization for save/load.
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <ecs/ecs.h>
#include <errno.h>
#include <npc/npc_serialization.h>
#include <save/save.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define NPC_SAVE_MAGIC 0x4E504353u // "NPCS"
#define NPC_SAVE_VERSION 2u        // Incremented for new fields
#define NPC_SAVE_ENDIANNESS 0x01020304u
#define NPC_PATH_CAPACITY 32u
#define NPC_ENTRY_U32_FIELDS 8u // Added behavior_flags and flee_target
#define NPC_ENTRY_F32_FIELDS (NPC_PATH_CAPACITY * 3u + 11u) // Added 4 timers
#define NPC_ENTRY_SIZE                                                         \
  (NPC_ENTRY_U32_FIELDS * sizeof(u32) + NPC_ENTRY_F32_FIELDS * sizeof(f32))

typedef struct {
  u32 magic;
  u32 version;
  u32 endianness;
  u32 count;
} NPCSaveHeader;

static void get_npcs_path(SaveSystem *save, const char *world_name, char *path,
                          size_t path_size) {
  snprintf(path, path_size, "%s/%s/npcs.dat", save->save_directory, world_name);
}

static bool ensure_directory(const char *path) {
#ifdef _WIN32
  if (_mkdir(path) == 0)
    return true;
  if (errno == EEXIST)
    return true;
#else
  if (mkdir(path, 0755) == 0)
    return true;
  if (errno == EEXIST)
    return true;
#endif
  return false;
}

static bool ensure_world_dir(SaveSystem *save, const char *world_name) {
  char world_path[512];
  snprintf(world_path, sizeof(world_path), "%s/%s", save->save_directory,
           world_name);
  return ensure_directory(save->save_directory) && ensure_directory(world_path);
}

static bool write_bytes(FILE *file, const void *data, size_t size) {
  return fwrite(data, size, 1, file) == 1;
}

static bool write_u32(FILE *file, u32 value) {
  return write_bytes(file, &value, sizeof(value));
}

static bool write_f32(FILE *file, f32 value) {
  return write_bytes(file, &value, sizeof(value));
}

static bool write_vec3(FILE *file, Vec3 value) {
  return write_f32(file, value.x) && write_f32(file, value.y) &&
         write_f32(file, value.z);
}

static bool write_quat(FILE *file, Quat value) {
  return write_f32(file, value.w) && write_f32(file, value.x) &&
         write_f32(file, value.y) && write_f32(file, value.z);
}

static bool read_bytes(FILE *file, void *data, size_t size) {
  return fread(data, size, 1, file) == 1;
}

static bool read_u32(FILE *file, u32 *value) {
  return read_bytes(file, value, sizeof(*value));
}

static bool read_f32(FILE *file, f32 *value) {
  return read_bytes(file, value, sizeof(*value));
}

static bool read_vec3(FILE *file, Vec3 *value) {
  return read_f32(file, &value->x) && read_f32(file, &value->y) &&
         read_f32(file, &value->z);
}

static bool read_quat(FILE *file, Quat *value) {
  return read_f32(file, &value->w) && read_f32(file, &value->x) &&
         read_f32(file, &value->y) && read_f32(file, &value->z);
}

static bool close_and_commit(FILE *file, const char *temp_path,
                             const char *final_path) {
  bool ok = (fflush(file) == 0);
#ifdef _WIN32
  if (ok) {
    _commit(_fileno(file));
  }
#else
  if (ok) {
    int fd = fileno(file);
    if (fd >= 0) {
      fsync(fd);
    }
  }
#endif
  if (fclose(file) != 0) {
    ok = false;
  }
  if (!ok) {
    remove(temp_path);
    return false;
  }
#ifdef _WIN32
  remove(final_path);
#endif
  if (rename(temp_path, final_path) != 0) {
    remove(temp_path);
    return false;
  }
  return true;
}

static bool get_file_size(FILE *file, size_t *out_size) {
  if (fseek(file, 0, SEEK_END) != 0)
    return false;
  long size = ftell(file);
  if (size < 0)
    return false;
  if (fseek(file, 0, SEEK_SET) != 0)
    return false;
  *out_size = (size_t)size;
  return true;
}

static bool write_npc_entry(FILE *file, EntityID entity,
                            const NPCComponent *npc,
                            const TransformComponent *transform) {
  u32 type = (u32)npc->type;
  u32 state = (u32)npc->state;
  u32 target = (u32)npc->target;
  u32 path_length = npc->path_length > NPC_PATH_CAPACITY ? NPC_PATH_CAPACITY
                                                         : npc->path_length;
  u32 path_index = npc->current_path_index > path_length
                       ? path_length
                       : npc->current_path_index;
  u32 behavior_flags = npc->behavior_flags;
  u32 flee_target = (u32)npc->flee_target;

  // Write u32 fields
  if (!write_u32(file, entity))
    return false;
  if (!write_u32(file, type))
    return false;
  if (!write_u32(file, state))
    return false;
  if (!write_u32(file, target))
    return false;
  if (!write_u32(file, path_length))
    return false;
  if (!write_u32(file, path_index))
    return false;
  if (!write_u32(file, behavior_flags))
    return false;
  if (!write_u32(file, flee_target))
    return false;

  // Write path
  for (u32 i = 0; i < NPC_PATH_CAPACITY; i++) {
    Vec3 node = i < path_length ? npc->path[i] : vec3(0.0f, 0.0f, 0.0f);
    if (!write_vec3(file, node))
      return false;
  }

  // Write transform
  if (!write_vec3(file, transform->position))
    return false;
  if (!write_quat(file, transform->rotation))
    return false;

  // Write behavior timers
  if (!write_f32(file, npc->behavior_timer))
    return false;
  if (!write_f32(file, npc->breed_cooldown))
    return false;
  if (!write_f32(file, npc->panic_timer))
    return false;
  if (!write_f32(file, npc->growth_timer))
    return false;

  return true;
}

bool save_npcs(struct SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system) {
  char npcs_path[512];
  char temp_path[520];
  get_npcs_path(save, world_name, npcs_path, sizeof(npcs_path));
  snprintf(temp_path, sizeof(temp_path), "%s.tmp", npcs_path);

  if (!ensure_world_dir(save, world_name))
    return false;

  FILE *file = fopen(temp_path, "wb");
  if (!file)
    return false;

  QueryDesc desc = {0};
  ComponentTypeID components[] = {NPC_COMPONENT_ID, TRANSFORM_COMPONENT_ID};
  desc.all_components = components;
  desc.all_count = 2;
  Query *query = ecs_query_create((World *)npc_system->ecs, &desc);

  NPCSaveHeader header = {.magic = NPC_SAVE_MAGIC,
                          .version = NPC_SAVE_VERSION,
                          .endianness = NPC_SAVE_ENDIANNESS,
                          .count = ecs_query_count(query)};

  bool ok = write_bytes(file, &header, sizeof(header));
  if (ok) {
    Entity entity;
    void *comps[2];
    while (ecs_query_next(query, &entity, comps)) {
      NPCComponent *npc = (NPCComponent *)comps[0];
      TransformComponent *transform = (TransformComponent *)comps[1];
      if (!write_npc_entry(file, entity.id, npc, transform)) {
        ok = false;
        break;
      }
    }
  }

  ecs_query_destroy((World *)npc_system->ecs, query);

  if (!ok) {
    fclose(file);
    remove(temp_path);
    return false;
  }

  return close_and_commit(file, temp_path, npcs_path);
}

bool load_npcs(struct SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system, struct World *ecs,
               EntityID *id_map) {
  char npcs_path[512];
  get_npcs_path(save, world_name, npcs_path, sizeof(npcs_path));

  FILE *file = fopen(npcs_path, "rb");
  if (!file)
    return true; // It's ok if the file doesn't exist

  size_t file_size = 0;
  if (!get_file_size(file, &file_size) || file_size < sizeof(NPCSaveHeader)) {
    fclose(file);
    return false;
  }

  NPCSaveHeader header;
  if (!read_bytes(file, &header, sizeof(header))) {
    fclose(file);
    return false;
  }
  if (header.magic != NPC_SAVE_MAGIC || header.version != NPC_SAVE_VERSION ||
      header.endianness != NPC_SAVE_ENDIANNESS) {
    fclose(file);
    return false;
  }
  if (header.count > MAX_ENTITIES) {
    fclose(file);
    return false;
  }

  size_t expected_size = sizeof(NPCSaveHeader) + header.count * NPC_ENTRY_SIZE;
  if (file_size < expected_size) {
    fclose(file);
    return false;
  }

  bool *used = calloc(ecs->config.max_entities, sizeof(bool));
  if (!used) {
    fclose(file);
    return false;
  }

  for (u32 i = 0; i < header.count; i++) {
    u32 saved_id = 0;
    u32 type = 0;
    u32 state = 0;
    u32 target = 0;
    u32 path_length = 0;
    u32 path_index = 0;
    u32 behavior_flags = 0;
    u32 flee_target = 0;
    Vec3 path[NPC_PATH_CAPACITY];
    TransformComponent transform = {0};
    f32 behavior_timer = 0.0f;
    f32 breed_cooldown = 0.0f;
    f32 panic_timer = 0.0f;
    f32 growth_timer = 0.0f;

    // Read u32 fields
    if (!read_u32(file, &saved_id) || !read_u32(file, &type) ||
        !read_u32(file, &state) || !read_u32(file, &target) ||
        !read_u32(file, &path_length) || !read_u32(file, &path_index)) {
      free(used);
      fclose(file);
      return false;
    }

    // Handle version differences
    if (header.version >= 2) {
      if (!read_u32(file, &behavior_flags) || !read_u32(file, &flee_target)) {
        free(used);
        fclose(file);
        return false;
      }
    }

    // Read path
    for (u32 j = 0; j < NPC_PATH_CAPACITY; j++) {
      if (!read_vec3(file, &path[j])) {
        free(used);
        fclose(file);
        return false;
      }
    }

    // Read transform
    if (!read_vec3(file, &transform.position) ||
        !read_quat(file, &transform.rotation)) {
      free(used);
      fclose(file);
      return false;
    }

    // Read timers if version 2+
    if (header.version >= 2) {
      if (!read_f32(file, &behavior_timer) ||
          !read_f32(file, &breed_cooldown) || !read_f32(file, &panic_timer) ||
          !read_f32(file, &growth_timer)) {
        free(used);
        fclose(file);
        return false;
      }
    }

    EntityID new_entity_id = 0;
    if (saved_id < ecs->config.max_entities) {
      new_entity_id = id_map[saved_id];
    }
    if (new_entity_id == 0 || new_entity_id >= ecs->config.max_entities ||
        used[new_entity_id]) {
      continue;
    }
    used[new_entity_id] = true;

    NPCComponent *npc = (NPCComponent *)ecs_add_component(
        ecs, (Entity){new_entity_id, 0}, NPC_COMPONENT_ID, NULL);
    TransformComponent *transform_comp =
        (TransformComponent *)ecs_add_component(ecs, (Entity){new_entity_id, 0},
                                                TRANSFORM_COMPONENT_ID, NULL);
    if (!npc || !transform_comp) {
      LOG_WARN("Failed to create NPC or components during load at index %u", i);
      continue;
    }

    npc->type = (type < NPC_TYPE_COUNT) ? (NPCType)type : NPC_TYPE_VILLAGER;
    npc->state = (state < NPC_STATE_COUNT) ? (NPCState)state : NPC_STATE_IDLE;
    npc->target = (target < ecs->config.max_entities) ? id_map[target] : 0;
    npc->path_length =
        path_length > NPC_PATH_CAPACITY ? NPC_PATH_CAPACITY : path_length;
    npc->current_path_index =
        path_index > npc->path_length ? npc->path_length : path_index;
    npc->behavior_flags = behavior_flags;
    npc->flee_target =
        (flee_target < ecs->config.max_entities) ? id_map[flee_target] : 0;
    npc->behavior_timer = behavior_timer;
    npc->breed_cooldown = breed_cooldown;
    npc->panic_timer = panic_timer;
    npc->growth_timer = growth_timer;

    for (u32 j = 0; j < npc->path_length; j++) {
      npc->path[j] = path[j];
    }
    for (u32 j = npc->path_length; j < NPC_PATH_CAPACITY; j++) {
      npc->path[j] = vec3(0.0f, 0.0f, 0.0f);
    }

    *transform_comp = transform;
  }

  free(used);
  fclose(file);
  return true;
}
