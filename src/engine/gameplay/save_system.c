// save_system.c - Implementation
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define SAVE_VERSION 1
#define SAVE_MAGIC "VFGMSAVE"
#define MAX_SAVE_SLOTS 10
#define SAVE_BUFFER_SIZE (1024 * 1024) // 1MB

typedef struct {
  char magic[8];
  u32 version;
  u32 timestamp;
  u32 checksum;
  u32 compressed_size;
  u32 uncompressed_size;
} SaveHeader;

typedef struct {
  char filename[256];
  SaveHeader header;
  bool is_valid;
} SaveSlot;

static SaveSlot g_save_slots[MAX_SAVE_SLOTS];
static u8 g_save_buffer[SAVE_BUFFER_SIZE];

bool save_init(void) {
  memset(g_save_slots, 0, sizeof(g_save_slots));
  
  // Scan for existing save files
  for (u32 i = 0; i < MAX_SAVE_SLOTS; i++) {
    snprintf(g_save_slots[i].filename, sizeof(g_save_slots[i].filename), 
             "save_slot_%d.sav", i);
    
    FILE *file = fopen(g_save_slots[i].filename, "rb");
    if (file) {
      if (fread(&g_save_slots[i].header, sizeof(SaveHeader), 1, file) == 1) {
        if (memcmp(g_save_slots[i].header.magic, SAVE_MAGIC, 8) == 0 &&
            g_save_slots[i].header.version == SAVE_VERSION) {
          g_save_slots[i].is_valid = true;
        }
      }
      fclose(file);
    }
  }
  
  LOG_INFO("Save system initialized, found %d valid save slots", 
           save_get_slot_count());
  return true;
}

void save_shutdown(void) {
  memset(g_save_slots, 0, sizeof(g_save_slots));
  LOG_INFO("Save system shutdown");
}

u32 save_get_slot_count(void) {
  u32 count = 0;
  for (u32 i = 0; i < MAX_SAVE_SLOTS; i++) {
    if (g_save_slots[i].is_valid) count++;
  }
  return count;
}

bool save_slot_exists(u32 slot) {
  return slot < MAX_SAVE_SLOTS && g_save_slots[slot].is_valid;
}

const char* save_get_slot_name(u32 slot) {
  if (slot >= MAX_SAVE_SLOTS || !g_save_slots[slot].is_valid) {
    return NULL;
  }
  
  static char name_buffer[64];
  time_t timestamp = g_save_slots[slot].header.timestamp;
  struct tm *tm_info = localtime(&timestamp);
  
  strftime(name_buffer, sizeof(name_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
  return name_buffer;
}

u32 save_get_slot_timestamp(u32 slot) {
  if (slot >= MAX_SAVE_SLOTS || !g_save_slots[slot].is_valid) {
    return 0;
  }
  
  return g_save_slots[slot].header.timestamp;
}

bool save_game(u32 slot, const void *data, u32 size) {
  if (slot >= MAX_SAVE_SLOTS || !data || size == 0 || size > SAVE_BUFFER_SIZE) {
    LOG_ERROR("Invalid save parameters");
    return false;
  }
  
  if (size > SAVE_BUFFER_SIZE) {
    LOG_ERROR("Save data too large: %d bytes (max: %d)", size, SAVE_BUFFER_SIZE);
    return false;
  }
  
  // Copy data to buffer
  memcpy(g_save_buffer, data, size);
  
  // Calculate simple checksum
  u32 checksum = 0;
  for (u32 i = 0; i < size; i++) {
    checksum = ((checksum << 1) | (checksum >> 31)) ^ g_save_buffer[i];
  }
  
  // Prepare header
  SaveHeader header = {0};
  memcpy(header.magic, SAVE_MAGIC, 8);
  header.version = SAVE_VERSION;
  header.timestamp = time(NULL);
  header.checksum = checksum;
  header.uncompressed_size = size;
  header.compressed_size = size; // No compression for now
  
  // Write save file
  FILE *file = fopen(g_save_slots[slot].filename, "wb");
  if (!file) {
    LOG_ERROR("Failed to open save file: %s", g_save_slots[slot].filename);
    return false;
  }
  
  bool success = false;
  if (fwrite(&header, sizeof(SaveHeader), 1, file) == 1 &&
      fwrite(g_save_buffer, size, 1, file) == 1) {
    success = true;
    
    // Update slot info
    g_save_slots[slot].header = header;
    g_save_slots[slot].is_valid = true;
    
    LOG_INFO("Game saved to slot %d (%d bytes)", slot, size);
  } else {
    LOG_ERROR("Failed to write save file");
  }
  
  fclose(file);
  return success;
}

bool load_game(u32 slot, void *out_data, u32 max_size, u32 *out_size) {
  if (slot >= MAX_SAVE_SLOTS || !out_data || !g_save_slots[slot].is_valid) {
    LOG_ERROR("Invalid load parameters");
    return false;
  }
  
  FILE *file = fopen(g_save_slots[slot].filename, "rb");
  if (!file) {
    LOG_ERROR("Failed to open save file: %s", g_save_slots[slot].filename);
    return false;
  }
  
  bool success = false;
  
  // Read header
  SaveHeader header;
  if (fread(&header, sizeof(SaveHeader), 1, file) == 1) {
    // Validate header
    if (memcmp(header.magic, SAVE_MAGIC, 8) == 0 &&
        header.version == SAVE_VERSION &&
        header.uncompressed_size <= max_size) {
      
      // Read data
      if (fread(g_save_buffer, header.uncompressed_size, 1, file) == 1) {
        // Verify checksum
        u32 checksum = 0;
        for (u32 i = 0; i < header.uncompressed_size; i++) {
          checksum = ((checksum << 1) | (checksum >> 31)) ^ g_save_buffer[i];
        }
        
        if (checksum == header.checksum) {
          memcpy(out_data, g_save_buffer, header.uncompressed_size);
          if (out_size) *out_size = header.uncompressed_size;
          success = true;
          
          LOG_INFO("Game loaded from slot %d (%d bytes)", slot, 
                   header.uncompressed_size);
        } else {
          LOG_ERROR("Save file checksum mismatch");
        }
      } else {
        LOG_ERROR("Failed to read save data");
      }
    } else {
      LOG_ERROR("Invalid save file header");
    }
  } else {
    LOG_ERROR("Failed to read save header");
  }
  
  fclose(file);
  return success;
}

bool delete_save(u32 slot) {
  if (slot >= MAX_SAVE_SLOTS) {
    LOG_ERROR("Invalid save slot: %d", slot);
    return false;
  }
  
  if (remove(g_save_slots[slot].filename) == 0) {
    memset(&g_save_slots[slot], 0, sizeof(SaveSlot));
    LOG_INFO("Save slot %d deleted", slot);
    return true;
  } else {
    LOG_ERROR("Failed to delete save slot %d", slot);
    return false;
  }
}

bool save_export(u32 slot, const char *filename) {
  if (slot >= MAX_SAVE_SLOTS || !g_save_slots[slot].is_valid || !filename) {
    return false;
  }
  
  // Copy save file to export location
  FILE *src = fopen(g_save_slots[slot].filename, "rb");
  FILE *dst = fopen(filename, "wb");
  
  if (!src || !dst) {
    if (src) fclose(src);
    if (dst) fclose(dst);
    return false;
  }
  
  char buffer[4096];
  size_t bytes_read;
  bool success = true;
  
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
      success = false;
      break;
    }
  }
  
  fclose(src);
  fclose(dst);
  
  if (success) {
    LOG_INFO("Save exported to: %s", filename);
  } else {
    LOG_ERROR("Failed to export save");
  }
  
  return success;
}

bool save_import(const char *filename, u32 slot) {
  if (!filename || slot >= MAX_SAVE_SLOTS) {
    return false;
  }
  
  // Validate imported file
  FILE *file = fopen(filename, "rb");
  if (!file) {
    LOG_ERROR("Failed to open import file: %s", filename);
    return false;
  }
  
  SaveHeader header;
  bool valid = false;
  
  if (fread(&header, sizeof(SaveHeader), 1, file) == 1) {
    if (memcmp(header.magic, SAVE_MAGIC, 8) == 0 &&
        header.version == SAVE_VERSION) {
      valid = true;
    }
  }
  
  fclose(file);
  
  if (!valid) {
    LOG_ERROR("Invalid save file format");
    return false;
  }
  
  // Copy to destination slot
  FILE *src = fopen(filename, "rb");
  FILE *dst = fopen(g_save_slots[slot].filename, "wb");
  
  if (!src || !dst) {
    if (src) fclose(src);
    if (dst) fclose(dst);
    return false;
  }
  
  char buffer[4096];
  size_t bytes_read;
  bool success = true;
  
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
      success = false;
      break;
    }
  }
  
  fclose(src);
  fclose(dst);
  
  if (success) {
    // Update slot info
    g_save_slots[slot].header = header;
    g_save_slots[slot].is_valid = true;
    
    LOG_INFO("Save imported to slot %d from: %s", slot, filename);
  } else {
    LOG_ERROR("Failed to import save");
  }
  
  return success;
}

void save_get_info(u32 slot, char *out_info, u32 info_size) {
  if (!out_info || info_size == 0) return;
  
  if (slot >= MAX_SAVE_SLOTS || !g_save_slots[slot].is_valid) {
    snprintf(out_info, info_size, "Slot %d: Empty", slot);
    return;
  }
  
  time_t timestamp = g_save_slots[slot].header.timestamp;
  struct tm *tm_info = localtime(&timestamp);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
  
  snprintf(out_info, info_size, 
           "Slot %d: %s (%d bytes, v%d)", 
           slot, time_str, g_save_slots[slot].header.uncompressed_size,
           g_save_slots[slot].header.version);
}

bool save_verify(u32 slot) {
  if (slot >= MAX_SAVE_SLOTS || !g_save_slots[slot].is_valid) {
    return false;
  }
  
  FILE *file = fopen(g_save_slots[slot].filename, "rb");
  if (!file) {
    g_save_slots[slot].is_valid = false;
    return false;
  }
  
  SaveHeader header;
  bool valid = false;
  
  if (fread(&header, sizeof(SaveHeader), 1, file) == 1) {
    if (memcmp(header.magic, SAVE_MAGIC, 8) == 0 &&
        header.version == SAVE_VERSION &&
        header.uncompressed_size <= SAVE_BUFFER_SIZE) {
      
      // Read and verify data
      if (fread(g_save_buffer, header.uncompressed_size, 1, file) == 1) {
        u32 checksum = 0;
        for (u32 i = 0; i < header.uncompressed_size; i++) {
          checksum = ((checksum << 1) | (checksum >> 31)) ^ g_save_buffer[i];
        }
        
        valid = (checksum == header.checksum);
      }
    }
  }
  
  fclose(file);
  
  if (!valid) {
    g_save_slots[slot].is_valid = false;
    LOG_WARN("Save slot %d verification failed", slot);
  }
  
  return valid;
}

void save_cleanup(void) {
  for (u32 i = 0; i < MAX_SAVE_SLOTS; i++) {
    if (!save_verify(i)) {
      delete_save(i);
    }
  }
  
  LOG_INFO("Save cleanup completed");
}
