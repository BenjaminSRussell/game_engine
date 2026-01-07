#include "include/common.h"
#include "include/core/asset_compression.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Replicated from vfs.c for now - should be shared in real implementation
typedef struct {
  char path[256];
  u64 offset;
  u64 original_size;
  u64 compressed_size;
  u32 compression_type;
} VFSArchiveEntry;

void normalize_path(char *path) {
  for (char *p = path; *p; p++) {
    if (*p == '\\')
      *p = '/';
  }
}

void get_files_recursive(const char *base_path, const char *current_path,
                         char ***files, u32 *count, u32 *capacity) {
  char full_path[512];
  if (strlen(current_path) > 0) {
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, current_path);
  } else {
    snprintf(full_path, sizeof(full_path), "%s", base_path);
  }

  DIR *dir = opendir(full_path);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if (entry->d_name[0] == '.')
      continue; // Skip hidden files

    char entry_path[512];
    if (strlen(current_path) > 0) {
      snprintf(entry_path, sizeof(entry_path), "%s/%s", current_path,
               entry->d_name);
    } else {
      snprintf(entry_path, sizeof(entry_path), "%s", entry->d_name);
    }

    char full_entry_path[512];
    snprintf(full_entry_path, sizeof(full_entry_path), "%s/%s", base_path,
             entry_path);

    struct stat st;
    if (stat(full_entry_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        get_files_recursive(base_path, entry_path, files, count, capacity);
      } else if (S_ISREG(st.st_mode)) {
        if (*count >= *capacity) {
          *capacity *= 2;
          *files = realloc(*files, sizeof(char *) * (*capacity));
        }
        (*files)[*count] = strdup(entry_path);
        (*count)++;
      }
    }
  }
  closedir(dir);
}

int main(int argc, char **argv) {
  if (argc < 3) { // Expect ./asset_packer <source_dir> <output_pak>
    printf("Usage: %s <source_dir> <output_pak>\n", argv[0]);
    return 1;
  }

  const char *source_dir = argv[1];
  const char *output_pak = argv[2];

  printf("Packing assets from %s to %s\n", source_dir, output_pak);

  // 1. Collect all files
  u32 capacity = 16;
  u32 count = 0;
  char **files = malloc(sizeof(char *) * capacity);
  get_files_recursive(source_dir, "", &files, &count, &capacity);

  printf("Found %u files to pack.\n", count);

  // 2. Prepare header and entries
  VFSArchiveEntry *entries = malloc(sizeof(VFSArchiveEntry) * count);
  FILE *out = fopen(output_pak, "wb");
  if (!out) {
    printf("Failed to open output file: %s\n", output_pak);
    return 1;
  }

  // Write placeholder header
  u32 magic = 0x4B434150; // "PACK"
  u32 version = 1;
  fwrite(&magic, 1, 4, out);
  fwrite(&version, 1, 4, out);
  fwrite(&count, 1, 4, out);

  // Write placeholder entries
  long entries_start = ftell(out);
  fwrite(entries, sizeof(VFSArchiveEntry), count, out);

  long data_start = ftell(out);

  // 3. Process each file
  u64 current_offset = 0; // Relative to data_start

  void *compression_buffer = malloc(1024 * 1024 * 16); // 16MB buffer

  for (u32 i = 0; i < count; i++) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", source_dir, files[i]);

    strncpy(entries[i].path, files[i], 255);
    normalize_path(entries[i].path);

    FILE *in = fopen(full_path, "rb");
    if (!in) {
      printf("Failed to read file: %s\n", full_path);
      continue;
    }

    fseek(in, 0, SEEK_END);
    u64 original_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    void *data = malloc(original_size);
    fread(data, 1, original_size, in);
    fclose(in);

    // Try compress
    size_t bound = asset_compress_bound(COMPRESSION_LZ77, original_size);
    if (bound > 1024 * 1024 * 16) {
      // Buffer too small, realloc
      compression_buffer = realloc(compression_buffer, bound);
    }

    size_t compressed_size = asset_compress(
        COMPRESSION_LZ77, data, original_size, compression_buffer, bound);

    entries[i].original_size = original_size;
    entries[i].offset =
        current_offset +
        (data_start); // Absolute offset in file? No, vfs expects...
    // Wait, vfs.c: fseek(handle->archive->file, handle->entry.offset +
    // handle->pos, SEEK_SET); It treats entry.offset as absolute file offset.
    entries[i].offset = data_start + current_offset;

    if (compressed_size > 0 && compressed_size < original_size) {
      entries[i].compressed_size = compressed_size;
      entries[i].compression_type = COMPRESSION_LZ77;
      fwrite(compression_buffer, 1, compressed_size, out);
      current_offset += compressed_size;
      printf("Packed %s (Compressed: %llu -> %llu)\n", files[i],
             (unsigned long long)original_size,
             (unsigned long long)compressed_size);
    } else {
      entries[i].compressed_size = original_size;
      entries[i].compression_type = COMPRESSION_NONE;
      fwrite(data, 1, original_size, out);
      current_offset += original_size;
      printf("Packed %s (Stored: %llu)\n", files[i],
             (unsigned long long)original_size);
    }

    free(data);
    free(files[i]);
  }

  free(files);
  free(compression_buffer);

  // 4. Update entries table
  fseek(out, entries_start, SEEK_SET);
  fwrite(entries, sizeof(VFSArchiveEntry), count, out);

  fclose(out);
  free(entries);

  printf("Successfully created %s\n", output_pak);
  return 0;
}
