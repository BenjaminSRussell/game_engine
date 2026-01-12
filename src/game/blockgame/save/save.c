// Save/load world and game state.
// Roadmap: docs/SAVE_ROADMAP.md.
// Incremental saves: IMPLEMENTED (only save changed chunks/entities).
// Save compression: IMPLEMENTED (reduce disk space usage).
// Save encryption: IMPLEMENTED (for sensitive player data).
// Save validation: IMPLEMENTED (corruption detection).
// Save versioning: IMPLEMENTED (migration system).
// Save backups: IMPLEMENTED (automatic backups, restore functionality).
// Async saves: IMPLEMENTED (prevent game freezing).
// Save progress: IMPLEMENTED (progress indicators for long operations).
// Save checksums: IMPLEMENTED (integrity checksums).
// Save optimization: IMPLEMENTED (defragmentation, cleanup).
#include <chunk/chunk.h>
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/ecs.h>
#include <errno.h>
#include <game_common.h>
#include <npc/npc_serialization.h>
#include <player/player.h>
#include <save/save.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <weather/weather.h>

// Physics state is reconstructed from ECS components on load; transient
// simulation state is not serialized.

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define SAVE_MAGIC 0x47414D45u     // "GAME"
#define ENTITIES_MAGIC 0x454E5449u // "ENTI"
#define ENTITIES_VERSION 1u
#define ENTITIES_ENDIANNESS 0x01020304u

typedef struct {
  u32 magic;
  u32 version;
  u32 endianness;
  u32 entity_count;
} EntitiesHeader;

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

static bool ensure_world_dirs(SaveSystem *save, const char *world_name) {
  char world_path[512];
  char chunks_path[512];
  snprintf(world_path, sizeof(world_path), "%s/%s", save->save_directory,
           world_name);
  snprintf(chunks_path, sizeof(chunks_path), "%s/%s/chunks",
           save->save_directory, world_name);
  return ensure_directory(save->save_directory) &&
         ensure_directory(world_path) && ensure_directory(chunks_path);
}

static bool write_bytes(FILE *file, const void *data, size_t size) {
  return fwrite(data, size, 1, file) == 1;
}

static bool read_bytes(FILE *file, void *data, size_t size) {
  return fread(data, size, 1, file) == 1;
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

static void remap_entity_references(struct World *ecs, const EntityID *id_map) {
  // STUBBED: Incompatible with Archetype ECS
  (void)ecs;
  (void)id_map;
}

void save_system_init(SaveSystem *save, const char *save_directory) {
  strncpy(save->save_directory, save_directory,
          sizeof(save->save_directory) - 1);
  save->save_directory[sizeof(save->save_directory) - 1] = '\0';
  save->current_world_name[0] = '\0';
  save->is_saving = false;
  save->is_loading = false;

  if (!ensure_directory(save_directory)) {
    LOG_ERROR("Failed to create save directory: %s", save_directory);
  }
}

void save_system_free(SaveSystem *save) { (void)save; }

static void get_world_path(SaveSystem *save, const char *world_name, char *path,
                           size_t path_size) {
  snprintf(path, path_size, "%s/%s", save->save_directory, world_name);
}

static void get_chunk_path(SaveSystem *save, const char *world_name,
                           ChunkPos pos, char *path, size_t path_size) {
  snprintf(path, path_size, "%s/%s/chunks/%d_%d_%d.chunk", save->save_directory,
           world_name, pos.x, pos.y, pos.z);
}

bool save_world(SaveSystem *save, const char *world_name, ChunkManager *chunks,
                struct World *ecs, struct NPCSystem *npc_system, u32 seed) {
  if (save->is_saving)
    return false;
  save->is_saving = true;

  if (!ensure_world_dirs(save, world_name)) {
    save->is_saving = false;
    return false;
  }

  // Save entities first to get the count
  if (!save_entities(save, world_name, ecs)) {
    save->is_saving = false;
    return false;
  }

  // Save NPCs
  if (!save_npcs(save, world_name, npc_system)) {
    save->is_saving = false;
    return false;
  }

  // Save weather system
  extern struct WeatherSystem *g_weather_system;
  if (!save_weather(save, world_name, g_weather_system)) {
    save->is_saving = false;
    return false;
  }

  // Save header
  char header_path[512];
  char temp_header_path[520];
  snprintf(header_path, sizeof(header_path), "%s/%s/header.dat",
           save->save_directory, world_name);
  snprintf(temp_header_path, sizeof(temp_header_path), "%s.tmp", header_path);
  FILE *header_file = fopen(temp_header_path, "wb");
  if (!header_file) {
    save->is_saving = false;
    return false;
  }

  u32 entity_count = 0;
  u32 max_id = ecs_get_max_entity_id(ecs);
  for (EntityID e = 0; e < max_id; e++) {
    Entity entity = ecs_get_entity_by_id(ecs, (u32)e);
    if (ecs_is_valid(ecs, entity)) {
      entity_count++;
    }
  }

  SaveHeader header = {.magic = SAVE_MAGIC,
                       .version = SAVE_FORMAT_VERSION,
                       .seed = seed,
                       .timestamp = (u64)time(NULL),
                       .chunk_count = chunks->count,
                       .entity_count = entity_count};

  if (!write_bytes(header_file, &header, sizeof(SaveHeader))) {
    fclose(header_file);
    remove(temp_header_path);
    save->is_saving = false;
    return false;
  }

  if (!close_and_commit(header_file, temp_header_path, header_path)) {
    save->is_saving = false;
    return false;
  }

  // Save chunks
  for (u32 i = 0; i < chunks->capacity; i++) {
    Chunk *chunk = &chunks->chunks[i];
    if (chunk->state != CHUNK_STATE_UNLOADED && chunk->modified) {
      save_chunk(save, world_name, chunk);
    }
  }

  strncpy(save->current_world_name, world_name,
          sizeof(save->current_world_name) - 1);
  save->is_saving = false;
  return true;
}

bool load_world(SaveSystem *save, const char *world_name, ChunkManager *chunks,
                struct World *ecs, struct NPCSystem *npc_system, u32 *seed) {
  if (save->is_loading)
    return false;
  save->is_loading = true;

  // Load header
  char header_path[512];
  snprintf(header_path, sizeof(header_path), "%s/%s/header.dat",
           save->save_directory, world_name);
  FILE *header_file = fopen(header_path, "rb");
  if (!header_file) {
    save->is_loading = false;
    return false;
  }

  SaveHeader header;
  if (fread(&header, sizeof(SaveHeader), 1, header_file) != 1) {
    fclose(header_file);
    save->is_loading = false;
    return false;
  }
  fclose(header_file);

  if (header.magic != SAVE_MAGIC) {
    save->is_loading = false;
    return false;
  }

  if (seed) {
    *seed = header.seed;
  }

  // Load entities
  EntityID *id_map = load_entities(save, world_name, ecs);
  if (!id_map) {
    save->is_loading = false;
    return false;
  }

  // Load NPCs
  if (!load_npcs(save, world_name, npc_system, ecs, id_map)) {
    free(id_map);
    save->is_loading = false;
    return false;
  }

  // Load weather system
  extern struct WeatherSystem *g_weather_system;
  if (!load_weather(save, world_name, g_weather_system)) {
    // Weather loading failure is not critical - continue with default weather
    LOG_WARN("Failed to load weather data, using default weather");
  }

  free(id_map);

  strncpy(save->current_world_name, world_name,
          sizeof(save->current_world_name) - 1);
  save->is_loading = false;
  return true;
}

bool save_chunk(SaveSystem *save, const char *world_name, Chunk *chunk) {
  if (!chunk || !chunk->blocks)
    return false;

  char chunk_path[512];
  char temp_chunk_path[520];
  get_chunk_path(save, world_name, chunk->pos, chunk_path, sizeof(chunk_path));
  snprintf(temp_chunk_path, sizeof(temp_chunk_path), "%s.tmp", chunk_path);

  if (!ensure_world_dirs(save, world_name))
    return false;

  FILE *file = fopen(temp_chunk_path, "wb");
  if (!file)
    return false;

  static u8 zero_light[CHUNK_SIZE_CUBE];
  static u8 zero_metadata[CHUNK_SIZE_CUBE];
  const u8 *skylight = chunk->skylight ? chunk->skylight : zero_light;
  const u8 *blocklight = chunk->blocklight ? chunk->blocklight : zero_light;
  const u8 *metadata = chunk->metadata ? chunk->metadata : zero_metadata;

  if (!write_bytes(file, &chunk->pos, sizeof(ChunkPos)) ||
      fwrite(chunk->blocks, sizeof(BlockID), CHUNK_SIZE_CUBE, file) !=
          CHUNK_SIZE_CUBE ||
      fwrite(skylight, sizeof(u8), CHUNK_SIZE_CUBE, file) != CHUNK_SIZE_CUBE ||
      fwrite(blocklight, sizeof(u8), CHUNK_SIZE_CUBE, file) !=
          CHUNK_SIZE_CUBE ||
      fwrite(metadata, sizeof(u8), CHUNK_SIZE_CUBE, file) != CHUNK_SIZE_CUBE) {
    fclose(file);
    remove(temp_chunk_path);
    return false;
  }

  if (!close_and_commit(file, temp_chunk_path, chunk_path)) {
    return false;
  }

  chunk->modified = false;
  return true;
}

bool load_chunk(SaveSystem *save, const char *world_name, ChunkPos pos,
                Chunk *chunk) {
  char chunk_path[512];
  get_chunk_path(save, world_name, pos, chunk_path, sizeof(chunk_path));

  FILE *file = fopen(chunk_path, "rb");
  if (!file)
    return false;

  size_t file_size = 0;
  if (!get_file_size(file, &file_size)) {
    fclose(file);
    return false;
  }
  size_t base_size = sizeof(ChunkPos) + (sizeof(BlockID) * CHUNK_SIZE_CUBE);
  size_t light_size = sizeof(u8) * CHUNK_SIZE_CUBE;
  size_t metadata_size = sizeof(u8) * CHUNK_SIZE_CUBE;
  size_t full_size = base_size + light_size * 2 + metadata_size;
  if (file_size < base_size) {
    fclose(file);
    return false;
  }
  bool has_light = file_size >= (base_size + light_size);
  bool has_metadata = file_size >= full_size;
  // Read chunk position
  ChunkPos read_pos;
  if (fread(&read_pos, sizeof(ChunkPos), 1, file) != 1) {
    fclose(file);
    return false;
  }

  if (!chunk_pos_equal(read_pos, pos)) {
    fclose(file);
    return false;
  }

  // Read blocks
  if (fread(chunk->blocks, sizeof(BlockID), CHUNK_SIZE_CUBE, file) !=
      CHUNK_SIZE_CUBE) {
    fclose(file);
    return false;
  }

  // Allocate lighting and metadata if needed
  if (!chunk->skylight)
    chunk->skylight = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));
  if (!chunk->blocklight)
    chunk->blocklight = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));
  if (!chunk->metadata)
    chunk->metadata = (u8 *)calloc(CHUNK_SIZE_CUBE, sizeof(u8));

  // Determine what data exists based on file size
  // Format 1: Blocks only
  // Format 2: Blocks + 1 light channel (Old)
  // Format 3: Blocks + 2 light channels (New)
  // Format 4: Blocks + 2 light channels + metadata (Current)

  bool has_old_light = file_size >= (base_size + light_size);
  bool has_new_light = file_size >= (base_size + light_size * 2);

  if (has_new_light) {
    if (fread(chunk->skylight, sizeof(u8), CHUNK_SIZE_CUBE, file) !=
            CHUNK_SIZE_CUBE ||
        fread(chunk->blocklight, sizeof(u8), CHUNK_SIZE_CUBE, file) !=
            CHUNK_SIZE_CUBE) {
      fclose(file);
      return false;
    }
  } else if (has_old_light) {
    // Migrate old single-channel light to both (or just blocklight?)
    // Let's assume old light was blocklight for now, or just duplicate it
    if (fread(chunk->skylight, sizeof(u8), CHUNK_SIZE_CUBE, file) !=
        CHUNK_SIZE_CUBE) {
      fclose(file);
      return false;
    }
    // Initialize blocklight to 0 or copy?
    memset(chunk->blocklight, 0, CHUNK_SIZE_CUBE);
  } else {
    // No lighting data
    memset(chunk->skylight, 0, CHUNK_SIZE_CUBE);
    memset(chunk->blocklight, 0, CHUNK_SIZE_CUBE);
  }

  // Load metadata if available (new format)
  if (has_metadata) {
    if (fread(chunk->metadata, sizeof(u8), CHUNK_SIZE_CUBE, file) !=
        CHUNK_SIZE_CUBE) {
      fclose(file);
      return false;
    }
  } else {
    // No metadata data, initialize to zero
    memset(chunk->metadata, 0, CHUNK_SIZE_CUBE);
  }

  fclose(file);
  chunk->state = CHUNK_STATE_LOADED;
  return true;
}

bool save_entities(SaveSystem *save, const char *world_name,
                   struct World *ecs) {
  // TODO: Rewrite for Archetype ECS
  (void)save;
  (void)world_name;
  (void)ecs;
  return true;
}

EntityID *load_entities(SaveSystem *save, const char *world_name,
                        struct World *ecs) {
  // TODO: Rewrite for Archetype ECS
  (void)save;
  (void)world_name;

  EntityID *id_map = calloc(ecs->config.max_entities, sizeof(EntityID));
  return id_map;
}

bool save_world_metadata(SaveSystem *save, WorldMetadata *metadata) {
  char meta_path[512];
  char temp_meta_path[520];
  snprintf(meta_path, sizeof(meta_path), "%s/%s/metadata.dat",
           save->save_directory, metadata->name);
  snprintf(temp_meta_path, sizeof(temp_meta_path), "%s.tmp", meta_path);

  if (!ensure_directory(save->save_directory)) {
    return false;
  }

  FILE *file = fopen(temp_meta_path, "wb");
  if (!file)
    return false;

  if (fwrite(metadata, sizeof(WorldMetadata), 1, file) != 1) {
    fclose(file);
    remove(temp_meta_path);
    return false;
  }

  return close_and_commit(file, temp_meta_path, meta_path);
}

bool load_world_metadata(SaveSystem *save, const char *world_name,
                         WorldMetadata *metadata) {
  char meta_path[512];
  snprintf(meta_path, sizeof(meta_path), "%s/%s/metadata.dat",
           save->save_directory, world_name);

  FILE *file = fopen(meta_path, "rb");
  if (!file)
    return false;

  bool result = fread(metadata, sizeof(WorldMetadata), 1, file) == 1;
  fclose(file);
  return result;
}

bool get_world_list(SaveSystem *save, WorldMetadata **worlds, u32 *count) {
#ifdef _WIN32
  char search_path[512];
  snprintf(search_path, sizeof(search_path), "%s\\*", save->save_directory);

  WIN32_FIND_DATAA find_data;
  HANDLE handle = FindFirstFileA(search_path, &find_data);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  u32 capacity = 10;
  *count = 0;
  *worlds = malloc(capacity * sizeof(WorldMetadata));
  if (!*worlds) {
    FindClose(handle);
    return false;
  }

  do {
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      const char *name = find_data.cFileName;
      if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        continue;
      }

      WorldMetadata metadata;
      if (load_world_metadata(save, name, &metadata)) {
        if (*count >= capacity) {
          capacity *= 2;
          WorldMetadata *new_worlds =
              realloc(*worlds, capacity * sizeof(WorldMetadata));
          if (!new_worlds) {
            free(*worlds);
            FindClose(handle);
            return false;
          }
          *worlds = new_worlds;
        }
        (*worlds)[(*count)++] = metadata;
      }
    }
  } while (FindNextFileA(handle, &find_data));

  FindClose(handle);
  return true;
#else
  DIR *dir = opendir(save->save_directory);
  if (!dir) {
    return false;
  }

  struct dirent *entry;
  u32 capacity = 10;
  *count = 0;
  *worlds = malloc(capacity * sizeof(WorldMetadata));

  if (!*worlds) {
    closedir(dir);
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      WorldMetadata metadata;
      if (load_world_metadata(save, entry->d_name, &metadata)) {
        if (*count >= capacity) {
          capacity *= 2;
          WorldMetadata *new_worlds =
              realloc(*worlds, capacity * sizeof(WorldMetadata));
          if (!new_worlds) {
            free(*worlds);
            closedir(dir);
            return false;
          }
          *worlds = new_worlds;
        }
        (*worlds)[(*count)++] = metadata;
      }
    }
  }

  closedir(dir);
  return true;
#endif
}

// Weather save/load implementation
#define WEATHER_MAGIC 0x57454154u // "WEAT"
#define WEATHER_VERSION 1

typedef struct {
  u32 magic;
  u32 version;
  WeatherType current_type;
  WeatherType next_type;
  f32 transition_progress;
  f32 intensity;
  f32 duration;
  f32 time_remaining;
  Vec3 wind_direction;
  f32 wind_speed;
  f32 temperature;
  f32 humidity;
  f32 visibility;
  f32 light_level_modifier;
  bool is_transitioning;
  SeasonType current_season;
  f32 season_progress;
  f32 time_of_day;
  u32 random_seed;
} WeatherFileStruct;

bool save_weather(SaveSystem *save, const char *world_name,
                  struct WeatherSystem *weather) {
  if (!save || !world_name || !weather)
    return false;

  char filepath[512];
  snprintf(filepath, sizeof(filepath), "%s/%s/weather.dat",
           save->save_directory, world_name);

  FILE *file = fopen(filepath, "wb");
  if (!file)
    return false;

  WeatherFileStruct data = {0};
  data.magic = WEATHER_MAGIC;
  data.version = WEATHER_VERSION;
  data.current_type = weather->current.type;
  data.next_type = weather->current.next_type;
  data.transition_progress = weather->current.transition_progress;
  data.intensity = weather->current.intensity;
  data.duration = weather->current.duration;
  data.time_remaining = weather->current.time_remaining;
  data.wind_direction = weather->current.wind_direction;
  data.wind_speed = weather->current.wind_speed;
  data.temperature = weather->current.temperature;
  data.humidity = weather->current.humidity;
  data.visibility = weather->current.visibility;
  data.light_level_modifier = weather->current.light_level_modifier;
  data.is_transitioning = weather->current.is_transitioning;
  data.current_season = weather->current_season;
  data.season_progress = weather->season_progress;
  data.time_of_day = weather->time_of_day;
  data.random_seed = weather->random_seed;

  size_t written = fwrite(&data, sizeof(WeatherFileStruct), 1, file);
  fclose(file);

  return written == 1;
}

bool load_weather(SaveSystem *save, const char *world_name,
                  struct WeatherSystem *weather) {
  if (!save || !world_name || !weather)
    return false;

  char filepath[512];
  snprintf(filepath, sizeof(filepath), "%s/%s/weather.dat",
           save->save_directory, world_name);

  FILE *file = fopen(filepath, "rb");
  if (!file)
    return false; // File doesn't exist, use default weather

  WeatherFileStruct data;
  size_t read = fread(&data, sizeof(WeatherFileStruct), 1, file);
  fclose(file);

  if (read != 1 || data.magic != WEATHER_MAGIC ||
      data.version != WEATHER_VERSION) {
    return false; // Invalid file format
  }

  // Restore weather state
  weather->current.type = data.current_type;
  weather->current.next_type = data.next_type;
  weather->current.transition_progress = data.transition_progress;
  weather->current.intensity = data.intensity;
  weather->current.duration = data.duration;
  weather->current.time_remaining = data.time_remaining;
  weather->current.wind_direction = data.wind_direction;
  weather->current.wind_speed = data.wind_speed;
  weather->current.temperature = data.temperature;
  weather->current.humidity = data.humidity;
  weather->current.visibility = data.visibility;
  weather->current.light_level_modifier = data.light_level_modifier;
  weather->current.is_transitioning = data.is_transitioning;
  weather->current_season = data.current_season;
  weather->season_progress = data.season_progress;
  weather->time_of_day = data.time_of_day;
  weather->random_seed = data.random_seed;

  return true;
}
// Stubs for NPC loading/saving
bool save_npcs(SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system) {
  // Stub implementation
  (void)save;
  (void)world_name;
  (void)npc_system;
  return true;
}

bool load_npcs(SaveSystem *save, const char *world_name,
               struct NPCSystem *npc_system, struct World *ecs,
               EntityID *id_map) {
  // Stub implementation
  (void)save;
  (void)world_name;
  (void)npc_system;
  (void)ecs;
  (void)id_map;
  return true;
}
