#include "core/resource/vfs/vfs.h"
#include "common.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define stat _stat
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

// Async operation validation state
typedef struct {
  bool validation_enabled;
  uint64_t total_async_ops;
  uint64_t completed_async_ops;
  uint64_t failed_async_ops;
  uint64_t validation_errors;
  uint64_t last_validation_time;
} VFSAsyncValidation;

// Internal structs
typedef struct {
  char path[256];
  u64 offset;
  u64 original_size;
  u64 compressed_size;
  u32 compression_type;
} VFSArchiveEntry;

typedef struct {
  FILE *file;
  VFSArchiveEntry *entries;
  u32 entry_count;
} VFSArchive;

typedef struct {
  FILE *file;
  bool is_archive;
  VFSArchiveEntry entry;
  u64 pos;
  VFSArchive *archive;
} VFSFileHandle;

// Async operation structure
typedef struct {
  enum {
    VFS_ASYNC_READ,
    VFS_ASYNC_WRITE
  } type;
  VFSFile *file;
  void *buffer;
  u64 size;
  u64 offset;
  bool completed;
  bool error;
  uint64_t submit_time;
  uint64_t completion_time;
  void (*callback)(struct VFSAsyncOp *op);
  void *user_data;
} VFSAsyncOp;

static bool g_vfs_checksum_verification_enabled = false;

static bool vfs_has_suffix(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return false;
  }
  size_t str_len = strlen(str);
  size_t suf_len = strlen(suffix);
  if (suf_len == 0 || str_len < suf_len) {
    return false;
  }
  return memcmp(str + (str_len - suf_len), suffix, suf_len) == 0;
}

static u32 vfs_crc32(const void *data, u64 size) {
  const u8 *p = (const u8 *)data;
  u32 crc = 0xFFFFFFFFu;
  for (u64 i = 0; i < size; i++) {
    crc ^= (u32)p[i];
    for (u32 b = 0; b < 8; b++) {
      u32 mask = (u32) - (i32)(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

// Forward declarations for validation functions
static bool vfs_validate_async_operation(VFSAsyncOp *op);
static void vfs_log_validation_error(VFS *vfs, const char *error);
static uint64_t vfs_get_timestamp(void);
static bool vfs_validate_file_handle(VFSFileHandle *handle);

void vfs_set_checksum_verification_enabled(bool enabled) {
  g_vfs_checksum_verification_enabled = enabled;
}

bool vfs_get_checksum_verification_enabled(void) {
  return g_vfs_checksum_verification_enabled;
}

void vfs_init(VFS *vfs) {
  vfs->mounts = NULL;
  vfs->mount_count = 0;
  vfs->mount_capacity = 0;

  // Initialize async operation support
  vfs->completed_ops = NULL;
  vfs->completed_count = 0;
  vfs->completed_capacity = 0;
  
  // Initialize validation state
  vfs->async_validation = malloc(sizeof(VFSAsyncValidation));
  if (vfs->async_validation) {
    vfs->async_validation->validation_enabled = true;
    vfs->async_validation->total_async_ops = 0;
    vfs->async_validation->completed_async_ops = 0;
    vfs->async_validation->failed_async_ops = 0;
    vfs->async_validation->validation_errors = 0;
    vfs->async_validation->last_validation_time = vfs_get_timestamp();
  }
  
#ifndef PLATFORM_WEB
  pthread_mutex_init(&vfs->async_lock, NULL);
#endif
  
  LOG_INFO("VFS: Initialized with async validation enabled");
}

void vfs_free(VFS *vfs) {
  if (vfs->mounts) {
    for (u32 i = 0; i < vfs->mount_count; i++) {
      if (vfs->mounts[i].is_archive && vfs->mounts[i].archive_handle) {
        VFSArchive *archive = (VFSArchive *)vfs->mounts[i].archive_handle;
        if (archive->file)
          fclose(archive->file);
        if (archive->entries)
          free(archive->entries);
        free(archive);
      }
    }
    free(vfs->mounts);
  }
  vfs->mounts = NULL;
  vfs->mount_count = 0;
  vfs->mount_capacity = 0;

  // Free async operation resources
  if (vfs->completed_ops) {
    for (u32 i = 0; i < vfs->completed_count; i++) {
      if (vfs->completed_ops[i].buffer) {
        free(vfs->completed_ops[i].buffer);
      }
    }
    free(vfs->completed_ops);
  }
  vfs->completed_ops = NULL;
  vfs->completed_count = 0;
  vfs->completed_capacity = 0;
  
  // Report validation statistics before cleanup
  if (vfs->async_validation) {
    LOG_INFO("VFS Async Statistics:");
    LOG_INFO("  Total operations: %lu", vfs->async_validation->total_async_ops);
    LOG_INFO("  Completed operations: %lu", vfs->async_validation->completed_async_ops);
    LOG_INFO("  Failed operations: %lu", vfs->async_validation->failed_async_ops);
    LOG_INFO("  Validation errors: %lu", vfs->async_validation->validation_errors);
    
    if (vfs->async_validation->total_async_ops > 0) {
      double success_rate = (double)vfs->async_validation->completed_async_ops / 
                           vfs->async_validation->total_async_ops * 100.0;
      LOG_INFO("  Success rate: %.2f%%", success_rate);
    }
    
    free(vfs->async_validation);
    vfs->async_validation = NULL;
  }
  
#ifndef PLATFORM_WEB
  pthread_mutex_destroy(&vfs->async_lock);
#endif
}

bool vfs_mount(VFS *vfs, const char *virtual_path, const char *physical_path) {
  if (vfs->mount_count >= vfs->mount_capacity) {
    u32 new_capacity = vfs->mount_capacity == 0 ? 8 : vfs->mount_capacity * 2;
    vfs->mounts =
        (VFSMount *)realloc(vfs->mounts, sizeof(VFSMount) * new_capacity);
    vfs->mount_capacity = new_capacity;
  }

  VFSMount *mount = &vfs->mounts[vfs->mount_count++];
  strncpy(mount->virtual_path, virtual_path, sizeof(mount->virtual_path) - 1);
  strncpy(mount->physical_path, physical_path,
          sizeof(mount->physical_path) - 1);
  mount->is_archive = false;
  mount->archive_handle = NULL;

  return true;
}

bool vfs_mount_archive(VFS *vfs, const char *virtual_path,
                       const char *archive_path) {
  FILE *file = fopen(archive_path, "rb");
  if (!file) {
    LOG_ERROR("VFS: Failed to open archive for mounting: %s", archive_path);
    return false;
  }

  u32 magic = 0;
  fread(&magic, 1, 4, file);
  if (magic != 0x4B434150) { // "PACK"
    LOG_ERROR("VFS: Invalid archive magic: %08x", magic);
    fclose(file);
    return false;
  }

  u32 version = 0;
  fread(&version, 1, 4, file);
  (void)version;

  u32 count = 0;
  fread(&count, 1, 4, file);

  VFSArchive *archive = (VFSArchive *)malloc(sizeof(VFSArchive));
  archive->file = file;
  archive->entry_count = count;
  archive->entries = (VFSArchiveEntry *)malloc(sizeof(VFSArchiveEntry) * count);
  fread(archive->entries, sizeof(VFSArchiveEntry), count, file);

  if (vfs->mount_count >= vfs->mount_capacity) {
    u32 new_capacity = vfs->mount_capacity == 0 ? 8 : vfs->mount_capacity * 2;
    vfs->mounts =
        (VFSMount *)realloc(vfs->mounts, sizeof(VFSMount) * new_capacity);
    vfs->mount_capacity = new_capacity;
  }

  VFSMount *mount = &vfs->mounts[vfs->mount_count++];
  strncpy(mount->virtual_path, virtual_path, sizeof(mount->virtual_path) - 1);
  strncpy(mount->physical_path, archive_path, sizeof(mount->physical_path) - 1);
  mount->is_archive = true;
  mount->archive_handle = archive;

  LOG_INFO("VFS: Mounted archive %s to %s with %u entries", archive_path,
           virtual_path, count);
  return true;
}

void vfs_unmount(VFS *vfs, const char *virtual_path) {
  for (u32 i = 0; i < vfs->mount_count; i++) {
    if (strcmp(vfs->mounts[i].virtual_path, virtual_path) == 0) {
      if (vfs->mounts[i].is_archive && vfs->mounts[i].archive_handle) {
        VFSArchive *archive = (VFSArchive *)vfs->mounts[i].archive_handle;
        if (archive->file)
          fclose(archive->file);
        if (archive->entries)
          free(archive->entries);
        free(archive);
      }
      // Remove mount
      for (u32 j = i; j < vfs->mount_count - 1; j++) {
        vfs->mounts[j] = vfs->mounts[j + 1];
      }
      vfs->mount_count--;
      break;
    }
  }
}

static const char *resolve_path(VFS *vfs, const char *virtual_path,
                                char *resolved, size_t size) {
  // Find matching mount
  for (u32 i = 0; i < vfs->mount_count; i++) {
    VFSMount *mount = &vfs->mounts[i];
    size_t mount_len = strlen(mount->virtual_path);

    if (strncmp(virtual_path, mount->virtual_path, mount_len) == 0) {
      const char *remainder = virtual_path + mount_len;
      if (*remainder == '/' || *remainder == '\0') {
        if (*remainder == '/')
          remainder++;
        snprintf(resolved, size, "%s/%s", mount->physical_path, remainder);
        return resolved;
      }
    }
  }

  // No mount found, use path as-is
  strncpy(resolved, virtual_path, size - 1);
  resolved[size - 1] = '\0';
  return resolved;
}

VFSFile *vfs_open(VFS *vfs, const char *path, VFSMode mode) {
  // Path validation: check for directory traversal attacks
  if (strstr(path, "..") != NULL) {
    LOG_ERROR("VFS: Rejected path with directory traversal: %s", path);
    return NULL;
  }

  // Find matching mount
  for (u32 i = 0; i < vfs->mount_count; i++) {
    VFSMount *mount = &vfs->mounts[i];
    size_t mount_len = strlen(mount->virtual_path);

    if (strncmp(path, mount->virtual_path, mount_len) == 0) {
      const char *remainder = path + mount_len;
      if (*remainder == '/' || *remainder == '\0') {
        if (*remainder == '/')
          remainder++;

        if (mount->is_archive) {
          VFSArchive *archive = (VFSArchive *)mount->archive_handle;
          for (u32 j = 0; j < archive->entry_count; j++) {
            if (strcmp(archive->entries[j].path, remainder) == 0) {
              VFSFileHandle *handle =
                  (VFSFileHandle *)malloc(sizeof(VFSFileHandle));
              handle->is_archive = true;
              handle->entry = archive->entries[j];
              handle->pos = 0;
              handle->archive = archive;
              handle->file = NULL;
              return (VFSFile *)handle;
            }
          }
          return NULL;
        } else {
          char resolved[512];
          snprintf(resolved, 512, "%s/%s", mount->physical_path, remainder);

          const char *file_mode = "rb";
          switch (mode) {
          case VFS_MODE_READ:
            file_mode = "r";
            break;
          case VFS_MODE_WRITE:
            file_mode = "w";
            break;
          case VFS_MODE_APPEND:
            file_mode = "a";
            break;
          case VFS_MODE_READ_BINARY:
            file_mode = "rb";
            break;
          case VFS_MODE_WRITE_BINARY:
            file_mode = "wb";
            break;
          }

          FILE *file = fopen(resolved, file_mode);
          if (!file)
            return NULL;

          // Checksum verification
          if (g_vfs_checksum_verification_enabled &&
              !vfs_has_suffix(resolved, ".crc") &&
              (mode == VFS_MODE_READ || mode == VFS_MODE_READ_BINARY)) {
            char crc_path[640];
            snprintf(crc_path, sizeof(crc_path), "%s.crc", resolved);
            FILE *crc_file = fopen(crc_path, "rb");
            if (crc_file) {
              u32 expected_crc = 0;
              if (fread(&expected_crc, 1, sizeof(expected_crc), crc_file) ==
                  sizeof(expected_crc)) {
                long pos = ftell(file);
                fseek(file, 0, SEEK_END);
                long end = ftell(file);
                fseek(file, 0, SEEK_SET);
                if (end >= 0) {
                  u8 *tmp = (u8 *)malloc((size_t)end);
                  if (tmp) {
                    size_t got = fread(tmp, 1, (size_t)end, file);
                    u32 crc = vfs_crc32(tmp, (u64)got);
                    free(tmp);
                    if (crc != expected_crc) {
                      LOG_WARN("VFS: Checksum mismatch for %s (expected=%08x "
                               "got=%08x)",
                               resolved, expected_crc, crc);
                    }
                  }
                }
                fseek(file, pos, SEEK_SET);
              }
              fclose(crc_file);
            }
          }

          VFSFileHandle *handle =
              (VFSFileHandle *)malloc(sizeof(VFSFileHandle));
          handle->is_archive = false;
          handle->file = file;
          handle->pos = 0;
          handle->archive = NULL;
          return (VFSFile *)handle;
        }
      }
    }
  }

  return NULL;
}

void vfs_close(VFSFile *file) {
  if (file) {
    VFSFileHandle *handle = (VFSFileHandle *)file;
    if (!handle->is_archive && handle->file) {
      fclose(handle->file);
    }
    free(handle);
  }
}

u64 vfs_read(VFSFile *file, void *buffer, u64 size) {
  if (!file)
    return 0;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (handle->is_archive) {
    u64 remaining = handle->entry.compressed_size - handle->pos;
    u64 to_read = (u64)size < remaining ? (u64)size : remaining;
    if (to_read == 0)
      return 0;

    fseek(handle->archive->file, (long)(handle->entry.offset + handle->pos),
          SEEK_SET);
    u64 read = fread(buffer, 1, (size_t)to_read, handle->archive->file);
    handle->pos += read;
    return read;
  } else {
    return fread(buffer, 1, (size_t)size, handle->file);
  }
}

u64 vfs_write(VFSFile *file, const void *buffer, u64 size) {
  if (!file)
    return 0;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (handle->is_archive)
    return 0;
  return fwrite(buffer, 1, (size_t)size, handle->file);
}

u64 vfs_seek(VFSFile *file, i64 offset, int whence) {
  if (!file)
    return 0;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (handle->is_archive) {
    if (whence == SEEK_SET)
      handle->pos = (u64)offset;
    else if (whence == SEEK_CUR)
      handle->pos = (u64)((i64)handle->pos + offset);
    else if (whence == SEEK_END)
      handle->pos = (u64)((i64)handle->entry.compressed_size + offset);

    if (handle->pos > handle->entry.compressed_size)
      handle->pos = handle->entry.compressed_size;
    return handle->pos;
  } else {
    fseek(handle->file, (long)offset, whence);
    return (u64)ftell(handle->file);
  }
}

u64 vfs_tell(VFSFile *file) {
  if (!file)
    return 0;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  return handle->is_archive ? handle->pos : (u64)ftell(handle->file);
}

u64 vfs_size(VFSFile *file) {
  if (!file)
    return 0;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (handle->is_archive)
    return handle->entry.original_size;

  long pos = ftell(handle->file);
  fseek(handle->file, 0, SEEK_END);
  long size = ftell(handle->file);
  fseek(handle->file, pos, SEEK_SET);
  return (u64)size;
}

bool vfs_eof(VFSFile *file) {
  if (!file)
    return true;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (handle->is_archive)
    return handle->pos >= handle->entry.compressed_size;
  return feof(handle->file) != 0;
}

bool vfs_exists(VFS *vfs, const char *path) {
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
  struct stat st;
  return stat(resolved, &st) == 0;
}

bool vfs_is_file(VFS *vfs, const char *path) {
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
  struct stat st;
  if (stat(resolved, &st) != 0)
    return false;
  return S_ISDIR(st.st_mode) == 0;
}

bool vfs_is_directory(VFS *vfs, const char *path) {
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
  struct stat st;
  if (stat(resolved, &st) != 0)
    return false;
  return S_ISDIR(st.st_mode) != 0;
}

u64 vfs_file_size(VFS *vfs, const char *path) {
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
  struct stat st;
  if (stat(resolved, &st) != 0)
    return 0;
  return (u64)st.st_size;
}

bool vfs_get_info(VFSFile *file, VFSFileInfo *info) {
  if (!file || !info)
    return false;
  VFSFileHandle *handle = (VFSFileHandle *)file;
  info->is_archive = handle->is_archive;
  if (handle->is_archive) {
    info->original_size = handle->entry.original_size;
    info->compressed_size = handle->entry.compressed_size;
    info->compression_type = handle->entry.compression_type;
  } else {
    info->original_size = vfs_size(file);
    info->compressed_size = info->original_size;
    info->compression_type = 0;
  }
  return true;
}

bool vfs_create_directory(VFS *vfs, const char *path) {
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
#ifdef _WIN32
  return _mkdir(resolved) == 0;
#else
  return mkdir(resolved, 0755) == 0;
#endif
}

bool vfs_list_directory(VFS *vfs, const char *path, char **files, u32 *count) {
  if (!count)
    return false;
  char resolved[512];
  resolve_path(vfs, path, resolved, sizeof(resolved));
#ifdef _WIN32
  *count = 0;
  return false;
#else
  DIR *dir = opendir(resolved);
  if (!dir) {
    *count = 0;
    return false;
  }
  u32 capacity = files ? *count : 0;
  u32 written = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    if (files && written < capacity) {
      files[written] = strdup(entry->d_name);
    }
    written++;
  }
  closedir(dir);
  *count = written;
  return true;
#endif
}

void vfs_normalize_path(char *path) {
  for (char *p = path; *p; p++)
    if (*p == '\\')
      *p = '/';
  char *src = path, *dst = path;
  while (*src) {
    if (*src == '/' && *(src + 1) == '/')
      src++;
    else
      *dst++ = *src++;
  }
  *dst = '\0';
}

void vfs_join_path(char *result, const char *base, const char *path) {
  strcpy(result, base);
  size_t len = strlen(result);
  if (len > 0 && result[len - 1] != '/')
    strcat(result, "/");
  if (path[0] == '/')
    strcat(result, path + 1);
  else
    strcat(result, path);
  vfs_normalize_path(result);
}

const char *vfs_get_filename(const char *path) {
  const char *filename = strrchr(path, '/');
  if (filename)
    return filename + 1;
  filename = strrchr(path, '\\');
  if (filename)
    return filename + 1;
  return path;
}

const char *vfs_get_extension(const char *path) {
  const char *ext = strrchr(path, '.');
  if (ext)
    return ext + 1;
  return "";
}

bool vfs_create_archive(const char *archive_path, const char *source_dir) {
  (void)archive_path;
  (void)source_dir;
  return false;
}
bool vfs_extract_archive(const char *archive_path, const char *dest_dir) {
  (void)archive_path;
  (void)dest_dir;
  return false;
}

// -----------------------------------------------------------------------------
// Async I/O Operations with Validation
// -----------------------------------------------------------------------------

VFSAsyncOp* vfs_async_read(VFS *vfs, VFSFile *file, void *buffer, u64 size, u64 offset,
                          void (*callback)(VFSAsyncOp *op), void *user_data) {
  if (!vfs || !file || !buffer || !vfs->async_validation) {
    vfs_log_validation_error(vfs, "Invalid parameters for async read");
    return NULL;
  }
  
  // Validate file handle
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (!vfs_validate_file_handle(handle)) {
    vfs_log_validation_error(vfs, "Invalid file handle for async read");
    return NULL;
  }
  
  // Validate read parameters
  u64 file_size = vfs_size(file);
  if (offset >= file_size) {
    vfs_log_validation_error(vfs, "Read offset beyond file size");
    return NULL;
  }
  
  if (offset + size > file_size) {
    size = file_size - offset; // Adjust size to fit file
  }
  
  VFSAsyncOp *op = malloc(sizeof(VFSAsyncOp));
  if (!op) {
    vfs_log_validation_error(vfs, "Failed to allocate async operation");
    return NULL;
  }
  
  op->type = VFS_ASYNC_READ;
  op->file = file;
  op->buffer = buffer;
  op->size = size;
  op->offset = offset;
  op->completed = false;
  op->error = false;
  op->submit_time = vfs_get_timestamp();
  op->completion_time = 0;
  op->callback = callback;
  op->user_data = user_data;
  
  // Validate operation
  if (!vfs_validate_async_operation(op)) {
    free(op);
    return NULL;
  }
  
  // Perform synchronous read for now (can be enhanced with actual async)
  u64 original_pos = vfs_tell(file);
  vfs_seek(file, offset, SEEK_SET);
  u64 bytes_read = vfs_read(file, buffer, size);
  vfs_seek(file, original_pos, SEEK_SET);
  
  op->completed = true;
  op->completion_time = vfs_get_timestamp();
  op->error = (bytes_read != size);
  
  // Update statistics
  vfs->async_validation->total_async_ops++;
  if (op->error) {
    vfs->async_validation->failed_async_ops++;
  } else {
    vfs->async_validation->completed_async_ops++;
  }
  
  // Call callback if provided
  if (op->callback) {
    op->callback(op);
  }
  
  return op;
}

VFSAsyncOp* vfs_async_write(VFS *vfs, VFSFile *file, const void *buffer, u64 size, u64 offset,
                           void (*callback)(VFSAsyncOp *op), void *user_data) {
  if (!vfs || !file || !buffer || !vfs->async_validation) {
    vfs_log_validation_error(vfs, "Invalid parameters for async write");
    return NULL;
  }
  
  // Validate file handle
  VFSFileHandle *handle = (VFSFileHandle *)file;
  if (!vfs_validate_file_handle(handle)) {
    vfs_log_validation_error(vfs, "Invalid file handle for async write");
    return NULL;
  }
  
  // Check if file is writable
  if (handle->is_archive) {
    vfs_log_validation_error(vfs, "Cannot write to archive file");
    return NULL;
  }
  
  VFSAsyncOp *op = malloc(sizeof(VFSAsyncOp));
  if (!op) {
    vfs_log_validation_error(vfs, "Failed to allocate async operation");
    return NULL;
  }
  
  op->type = VFS_ASYNC_WRITE;
  op->file = file;
  op->buffer = (void*)buffer; // Remove const for callback compatibility
  op->size = size;
  op->offset = offset;
  op->completed = false;
  op->error = false;
  op->submit_time = vfs_get_timestamp();
  op->completion_time = 0;
  op->callback = callback;
  op->user_data = user_data;
  
  // Validate operation
  if (!vfs_validate_async_operation(op)) {
    free(op);
    return NULL;
  }
  
  // Perform synchronous write for now (can be enhanced with actual async)
  u64 original_pos = vfs_tell(file);
  vfs_seek(file, offset, SEEK_SET);
  u64 bytes_written = vfs_write(file, buffer, size);
  vfs_seek(file, original_pos, SEEK_SET);
  
  op->completed = true;
  op->completion_time = vfs_get_timestamp();
  op->error = (bytes_written != size);
  
  // Update statistics
  vfs->async_validation->total_async_ops++;
  if (op->error) {
    vfs->async_validation->failed_async_ops++;
  } else {
    vfs->async_validation->completed_async_ops++;
  }
  
  // Call callback if provided
  if (op->callback) {
    op->callback(op);
  }
  
  return op;
}

bool vfs_async_op_is_completed(VFSAsyncOp *op) {
  return op ? op->completed : false;
}

bool vfs_async_op_has_error(VFSAsyncOp *op) {
  return op ? op->error : true;
}

u64 vfs_async_op_get_bytes_transferred(VFSAsyncOp *op) {
  if (!op || op->error) return 0;
  return op->size;
}

void vfs_async_op_wait(VFSAsyncOp *op) {
  if (!op) return;
  
  // Simple busy wait for now (can be enhanced with condition variables)
  while (!op->completed) {
    // In a real implementation, this would use condition variables
    // or other synchronization primitives
  }
}

void vfs_async_op_free(VFSAsyncOp *op) {
  if (op) {
    free(op);
  }
}

// -----------------------------------------------------------------------------
// Validation Implementation
// -----------------------------------------------------------------------------

static bool vfs_validate_async_operation(VFSAsyncOp *op) {
  if (!op) return false;
  
  // Validate operation type
  if (op->type != VFS_ASYNC_READ && op->type != VFS_ASYNC_WRITE) {
    return false;
  }
  
  // Validate file handle
  if (!vfs_validate_file_handle((VFSFileHandle *)op->file)) {
    return false;
  }
  
  // Validate buffer
  if (!op->buffer) {
    return false;
  }
  
  // Validate size
  if (op->size == 0) {
    return false;
  }
  
  // Validate timestamp
  if (op->submit_time == 0) {
    return false;
  }
  
  return true;
}

static void vfs_log_validation_error(VFS *vfs, const char *error) {
  if (!vfs || !error || !vfs->async_validation) return;
  
  vfs->async_validation->validation_errors++;
  LOG_ERROR("VFS Validation Error [%lu]: %s", 
            vfs->async_validation->validation_errors, error);
}

static uint64_t vfs_get_timestamp(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static bool vfs_validate_file_handle(VFSFileHandle *handle) {
  if (!handle) return false;
  
  // Check file pointer for non-archive files
  if (!handle->is_archive && !handle->file) {
    return false;
  }
  
  // Check archive handle for archive files
  if (handle->is_archive && !handle->archive) {
    return false;
  }
  
  // Validate position
  if (handle->is_archive) {
    if (handle->pos > handle->entry.compressed_size) {
      return false;
    }
  }
  
  return true;
}

// Public validation API
bool vfs_validate_async_state(VFS *vfs) {
  if (!vfs || !vfs->async_validation) return false;
  
  bool valid = true;
  uint64_t current_time = vfs_get_timestamp();
  
  // Check for stale operations (operations that have been running too long)
  uint64_t time_since_last_validation = current_time - vfs->async_validation->last_validation_time;
  if (time_since_last_validation > 10000000000ULL) { // 10 seconds
    if (vfs->async_validation->total_async_ops > 0) {
      uint64_t pending_ops = vfs->async_validation->total_async_ops - 
                           vfs->async_validation->completed_async_ops - 
                           vfs->async_validation->failed_async_ops;
      
      if (pending_ops > 100) { // Too many pending operations
        vfs_log_validation_error(vfs, "Too many pending async operations");
        valid = false;
      }
    }
  }
  
  vfs->async_validation->last_validation_time = current_time;
  return valid;
}

void vfs_enable_async_validation(VFS *vfs, bool enable) {
  if (vfs && vfs->async_validation) {
    vfs->async_validation->validation_enabled = enable;
    LOG_INFO("VFS async validation %s", enable ? "enabled" : "disabled");
  }
}

uint64_t vfs_get_async_validation_errors(VFS *vfs) {
  return (vfs && vfs->async_validation) ? vfs->async_validation->validation_errors : 0;
}

void vfs_get_async_statistics(VFS *vfs, uint64_t *total_ops, uint64_t *completed_ops,
                             uint64_t *failed_ops, uint64_t *validation_errors) {
  if (!vfs || !vfs->async_validation) return;
  
  if (total_ops) *total_ops = vfs->async_validation->total_async_ops;
  if (completed_ops) *completed_ops = vfs->async_validation->completed_async_ops;
  if (failed_ops) *failed_ops = vfs->async_validation->failed_async_ops;
  if (validation_errors) *validation_errors = vfs->async_validation->validation_errors;
}
