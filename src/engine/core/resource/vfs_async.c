// VFS async operations implementation
#include <core/logger.h>
#include <thread/job.h>
#include <vfs/vfs.h>
#include <stdlib.h>
#include <string.h>
#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

// Async operation worker function
typedef struct {
  VFS *vfs;
  char path[256];
  VFSCallback callback;
  void *user_data;
} VFSAsyncReadJob;

static void vfs_async_read_worker(void *data) {
  VFSAsyncReadJob *job = (VFSAsyncReadJob *)data;
  if (!job || !job->vfs) {
    free(job);
    return;
  }

  // Perform synchronous read
  VFSFile *file = vfs_open(job->vfs, job->path, VFS_MODE_READ_BINARY);
  void *buffer = NULL;
  u64 bytes_read = 0;
  bool success = false;

  if (file) {
    u64 size = vfs_size(file);
    if (size > 0) {
      buffer = malloc(size);
      if (buffer) {
        bytes_read = vfs_read(file, buffer, size);
        success = (bytes_read == size);
      }
    }
    vfs_close(file);
  }

  // Queue result for main thread processing
#ifndef PLATFORM_WEB
  pthread_mutex_lock(&job->vfs->async_lock);
#endif

  // Expand completed ops array if needed
  if (job->vfs->completed_count >= job->vfs->completed_capacity) {
    u32 new_capacity = job->vfs->completed_capacity == 0
                           ? 8
                           : job->vfs->completed_capacity * 2;
    VFSAsyncOp *new_ops = (VFSAsyncOp *)realloc(
        job->vfs->completed_ops, sizeof(VFSAsyncOp) * new_capacity);
    if (new_ops) {
      job->vfs->completed_ops = new_ops;
      job->vfs->completed_capacity = new_capacity;
    }
  }

  // Add to completed queue
  if (job->vfs->completed_count < job->vfs->completed_capacity) {
    VFSAsyncOp *op = &job->vfs->completed_ops[job->vfs->completed_count++];
    strncpy(op->path, job->path, sizeof(op->path) - 1);
    op->path[sizeof(op->path) - 1] = '\0';
    op->buffer = buffer;
    op->bytes_read = bytes_read;
    op->success = success;
    op->callback = job->callback;
    op->user_data = job->user_data;
  } else {
    // Failed to queue, cleanup
    if (buffer)
      free(buffer);
  }

#ifndef PLATFORM_WEB
  pthread_mutex_unlock(&job->vfs->async_lock);
#endif

  free(job);
}

void vfs_read_async(VFS *vfs, const char *path, VFSCallback callback,
                    void *user_data) {
  if (!vfs || !path || !callback) {
    return;
  }

  // Create async job
  VFSAsyncReadJob *job = (VFSAsyncReadJob *)malloc(sizeof(VFSAsyncReadJob));
  if (!job) {
    LOG_ERROR("VFS: Failed to allocate async read job for: %s", path);
    return;
  }

  job->vfs = vfs;
  strncpy(job->path, path, sizeof(job->path) - 1);
  job->path[sizeof(job->path) - 1] = '\0';
  job->callback = callback;
  job->user_data = user_data;

  // Submit to thread pool
  extern ThreadPool *g_thread_pool;
  if (g_thread_pool) {
    thread_pool_submit(g_thread_pool, vfs_async_read_worker, job, 1);
  } else {
    LOG_WARN("VFS: Thread pool not available, performing synchronous read");
    vfs_async_read_worker(job);
  }
}

void vfs_update(VFS *vfs) {
  if (!vfs) {
    return;
  }

#ifndef PLATFORM_WEB
  pthread_mutex_lock(&vfs->async_lock);
#endif

  // Process all completed operations
  for (u32 i = 0; i < vfs->completed_count; i++) {
    VFSAsyncOp *op = &vfs->completed_ops[i];
    if (op->callback) {
      op->callback(op->user_data, op->buffer, op->bytes_read, op->success);
    }
    // Note: callback is responsible for freeing buffer if needed
  }

  // Clear completed queue
  vfs->completed_count = 0;

#ifndef PLATFORM_WEB
  pthread_mutex_unlock(&vfs->async_lock);
#endif
}
