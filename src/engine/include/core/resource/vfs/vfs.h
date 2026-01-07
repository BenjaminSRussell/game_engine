// include/vfs/vfs.h
//
// Purpose: Defines the public API for a Virtual File System (VFS). This system
// provides an abstraction layer over the native filesystem, allowing the game
// to access resources from various sources (physical directories, archives)
// through a unified virtual path structure. It supports mounting different
// storage locations, resolving paths, and performing standard file operations.
//
// Public APIs:
// - `VFSFile`: Opaque structure representing an open VFS file handle.
// - `VFSMode`: Enumeration defining various modes for opening files (read,
// write, append, binary).
// - `VFSMount`: Structure representing a mount point, linking a `virtual_path`
// to a
//   `physical_path` or `archive_handle`, along with a flag for archive status.
// - `VFS`: The main structure encapsulating the VFS state, including a list of
//   `mounts` and their count.
// - `vfs_init`: Initializes the VFS system.
// - `vfs_free`: Frees all resources held by the VFS.
// - Mount operations: `vfs_mount` (for directories), `vfs_mount_archive` (for
// archives),
//   `vfs_unmount` for managing virtual filesystem hierarchy.
// - File operations: `vfs_open`, `vfs_close`, `vfs_read`, `vfs_write`,
// `vfs_seek`,
//   `vfs_tell`, `vfs_size`, `vfs_eof` provide standard file I/O.
// - File info: `vfs_exists`, `vfs_is_file`, `vfs_is_directory`, `vfs_file_size`
// for querying file/directory properties.
// - Directory operations: `vfs_create_directory`, `vfs_list_directory` for
// managing directories.
// - Path operations: `vfs_normalize_path`, `vfs_join_path`, `vfs_get_filename`,
// `vfs_get_extension` for path manipulation.
// - Archive support: `vfs_create_archive`, `vfs_extract_archive` for creating
// and extracting archive files.
//
// Ownership: The `VFS` instance owns its internal `VFSMount` array and manages
// the lifecycle of `VFSFile` handles and archive handles.
//
// Invariants:
// - A `VFS` must be initialized with `vfs_init` before use and freed with
// `vfs_free`.
// - Virtual paths are resolved against the mounted points to determine the
// actual physical location.
// - File operations on `VFSFile` handles must be performed between `vfs_open`
// and `vfs_close`.
// - `VFSMode` must be respected for file access.
//
#ifndef VFS_H
#define VFS_H

#include <common.h>

#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

// VFS file handle
typedef struct VFSFile VFSFile;

// Async operation callback
typedef void (*VFSCallback)(void *user_data, void *buffer, u64 bytes_read,
                            bool success);

// Async operation structure
typedef struct {
  char path[256];
  void *buffer;
  u64 bytes_read;
  bool success;
  VFSCallback callback;
  void *user_data;
} VFSAsyncOp;

// VFS file modes
typedef enum {
  VFS_MODE_READ,
  VFS_MODE_WRITE,
  VFS_MODE_APPEND,
  VFS_MODE_READ_BINARY,
  VFS_MODE_WRITE_BINARY
} VFSMode;

typedef struct {
  u64 original_size;
  u64 compressed_size;
  u32 compression_type;
  bool is_archive;
} VFSFileInfo;

// VFS mount point
typedef struct {
  char virtual_path[256];
  char physical_path[512];
  bool is_archive;
  void *archive_handle;
} VFSMount;

// VFS structure
struct VFS {
  VFSMount *mounts;
  u32 mount_count;
  u32 mount_capacity;

  // Async operation support
#ifndef PLATFORM_WEB
  pthread_mutex_t async_lock;
#endif
  VFSAsyncOp *completed_ops;
  u32 completed_count;
  u32 completed_capacity;
};
typedef struct VFS VFS;

// Initialize VFS
void vfs_init(VFS *vfs);
void vfs_free(VFS *vfs);

// Mount operations
bool vfs_mount(VFS *vfs, const char *virtual_path, const char *physical_path);
bool vfs_mount_archive(VFS *vfs, const char *virtual_path,
                       const char *archive_path);
void vfs_unmount(VFS *vfs, const char *virtual_path);

// File operations
VFSFile *vfs_open(VFS *vfs, const char *path, VFSMode mode);
void vfs_close(VFSFile *file);
u64 vfs_read(VFSFile *file, void *buffer, u64 size);
u64 vfs_write(VFSFile *file, const void *buffer, u64 size);
u64 vfs_seek(VFSFile *file, i64 offset, int whence);
u64 vfs_tell(VFSFile *file);
u64 vfs_size(VFSFile *file);
bool vfs_eof(VFSFile *file);

// File info
bool vfs_exists(VFS *vfs, const char *path);
bool vfs_is_file(VFS *vfs, const char *path);
bool vfs_is_directory(VFS *vfs, const char *path);
u64 vfs_file_size(VFS *vfs, const char *path);
bool vfs_get_info(VFSFile *file, VFSFileInfo *info);

// Directory operations
bool vfs_create_directory(VFS *vfs, const char *path);
bool vfs_list_directory(VFS *vfs, const char *path, char **files, u32 *count);

// Path operations
void vfs_normalize_path(char *path);
void vfs_join_path(char *result, const char *base, const char *path);
const char *vfs_get_filename(const char *path);
const char *vfs_get_extension(const char *path);

// Archive support
bool vfs_create_archive(const char *archive_path, const char *source_dir);
bool vfs_extract_archive(const char *archive_path, const char *dest_dir);

// Security
// Enable/disable checksum verification for reads (best-effort;
// implementation-defined).
void vfs_set_checksum_verification_enabled(bool enabled);
bool vfs_get_checksum_verification_enabled(void);

// Async operations (processed on main thread via vfs_update)
void vfs_read_async(VFS *vfs, const char *path, VFSCallback callback,
                    void *user_data);
void vfs_update(VFS *vfs);

#endif // VFS_H
