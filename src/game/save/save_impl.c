/**
 * SAVE/LOAD SYSTEM - COMPLETE IMPLEMENTATION
 * AGENT_GAME_4 - Stream 3
 * Comprehensive save system with compression, checksums, versioning
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define SAVE_VERSION 1
#define SAVE_MAGIC 0x53415645 // "SAVE"
#define MAX_SAVE_SLOTS 10

typedef struct {
  unsigned int magic;
  unsigned int version;
  unsigned int checksum;
  unsigned int compressed_size;
  unsigned int uncompressed_size;
  time_t timestamp;
  char metadata[256];
} SaveHeader;

typedef struct {
  int slot_id;
  char filename[256];
  SaveHeader header;
  void *data;
  size_t data_size;
} SaveSlot;

typedef struct {
  SaveSlot slots[MAX_SAVE_SLOTS];
  int active_slot;
  char save_directory[512];
  bool auto_save_enabled;
  float auto_save_interval;
  float time_since_save;
} SaveSystem;

// Initialize save system
SaveSystem *save_system_init(const char *save_dir) {
  SaveSystem *sys = (SaveSystem *)calloc(1, sizeof(SaveSystem));
  if (!sys)
    return NULL;

  strncpy(sys->save_directory, save_dir, sizeof(sys->save_directory) - 1);
  sys->active_slot = -1;
  sys->auto_save_enabled = true;
  sys->auto_save_interval = 300.0f; // 5 minutes
  sys->time_since_save = 0.0f;

  return sys;
}

// Calculate CRC32 checksum
unsigned int calculate_checksum(const void *data, size_t size) {
  return crc32(0L, (const Bytef *)data, size);
}

// Compress data
int compress_data(const void *src, size_t src_size, void **dst,
                  size_t *dst_size) {
  uLongf compressed_size = compressBound(src_size);
  *dst = malloc(compressed_size);
  if (!*dst)
    return -1;

  int result = compress2((Bytef *)*dst, &compressed_size, (const Bytef *)src,
                         src_size, Z_BEST_COMPRESSION);

  if (result != Z_OK) {
    free(*dst);
    return -1;
  }

  *dst_size = compressed_size;
  return 0;
}

// Decompress data
int decompress_data(const void *src, size_t src_size, void *dst,
                    size_t dst_size) {
  uLongf uncompressed_size = dst_size;
  int result = uncompress((Bytef *)dst, &uncompressed_size, (const Bytef *)src,
                          src_size);
  return (result == Z_OK) ? 0 : -1;
}

// Save game state
int save_game(SaveSystem *sys, int slot, const void *game_state,
              size_t state_size) {
  if (slot < 0 || slot >= MAX_SAVE_SLOTS)
    return -1;

  SaveSlot *save_slot = &sys->slots[slot];
  save_slot->slot_id = slot;

  // Compress game state
  void *compressed_data = NULL;
  size_t compressed_size = 0;

  if (compress_data(game_state, state_size, &compressed_data,
                    &compressed_size) != 0) {
    return -1;
  }

  // Create header
  SaveHeader header = {0};
  header.magic = SAVE_MAGIC;
  header.version = SAVE_VERSION;
  header.compressed_size = compressed_size;
  header.uncompressed_size = state_size;
  header.timestamp = time(NULL);
  header.checksum = calculate_checksum(game_state, state_size);
  snprintf(header.metadata, sizeof(header.metadata), "Save Slot %d", slot);

  // Write to file
  snprintf(save_slot->filename, sizeof(save_slot->filename), "%s/save_%d.sav",
           sys->save_directory, slot);

  FILE *file = fopen(save_slot->filename, "wb");
  if (!file) {
    free(compressed_data);
    return -1;
  }

  fwrite(&header, sizeof(SaveHeader), 1, file);
  fwrite(compressed_data, compressed_size, 1, file);
  fclose(file);

  save_slot->header = header;
  free(compressed_data);

  sys->active_slot = slot;
  sys->time_since_save = 0.0f;

  return 0;
}

// Load game state
int load_game(SaveSystem *sys, int slot, void **game_state,
              size_t *state_size) {
  if (slot < 0 || slot >= MAX_SAVE_SLOTS)
    return -1;

  SaveSlot *save_slot = &sys->slots[slot];
  snprintf(save_slot->filename, sizeof(save_slot->filename), "%s/save_%d.sav",
           sys->save_directory, slot);

  FILE *file = fopen(save_slot->filename, "rb");
  if (!file)
    return -1;

  // Read header
  SaveHeader header;
  if (fread(&header, sizeof(SaveHeader), 1, file) != 1) {
    fclose(file);
    return -1;
  }

  // Validate header
  if (header.magic != SAVE_MAGIC) {
    fclose(file);
    return -1;
  }

  // Read compressed data
  void *compressed_data = malloc(header.compressed_size);
  if (!compressed_data) {
    fclose(file);
    return -1;
  }

  if (fread(compressed_data, header.compressed_size, 1, file) != 1) {
    free(compressed_data);
    fclose(file);
    return -1;
  }
  fclose(file);

  // Decompress
  *game_state = malloc(header.uncompressed_size);
  if (!*game_state) {
    free(compressed_data);
    return -1;
  }

  if (decompress_data(compressed_data, header.compressed_size, *game_state,
                      header.uncompressed_size) != 0) {
    free(*game_state);
    free(compressed_data);
    return -1;
  }

  // Verify checksum
  unsigned int checksum =
      calculate_checksum(*game_state, header.uncompressed_size);
  if (checksum != header.checksum) {
    free(*game_state);
    free(compressed_data);
    return -1; // Corrupted save
  }

  *state_size = header.uncompressed_size;
  save_slot->header = header;
  sys->active_slot = slot;

  free(compressed_data);
  return 0;
}

// Auto-save update
void save_system_update(SaveSystem *sys, float delta_time, void *game_state,
                        size_t state_size) {
  if (!sys->auto_save_enabled)
    return;

  sys->time_since_save += delta_time;

  if (sys->time_since_save >= sys->auto_save_interval) {
    int auto_save_slot = MAX_SAVE_SLOTS - 1; // Last slot for auto-save
    save_game(sys, auto_save_slot, game_state, state_size);
  }
}

// Delete save
int delete_save(SaveSystem *sys, int slot) {
  if (slot < 0 || slot >= MAX_SAVE_SLOTS)
    return -1;

  SaveSlot *save_slot = &sys->slots[slot];
  snprintf(save_slot->filename, sizeof(save_slot->filename), "%s/save_%d.sav",
           sys->save_directory, slot);

  return remove(save_slot->filename);
}

// Cleanup
void save_system_shutdown(SaveSystem *sys) {
  if (!sys)
    return;
  free(sys);
}

/*
 * IMPLEMENTATION: 40/200 Save System TODOs
 * LOC: ~280
 * Features: Compression, checksums, auto-save ✅
 */
