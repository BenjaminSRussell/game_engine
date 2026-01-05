/**
 * =================================================================================================
 *                          FSEVENTS DIRECTORY WATCHER
 *                        macOS Native File System Monitoring
 * =================================================================================================
 *
 * This file implements FSEvents-based directory watching for hot-reload
 * functionality. FSEvents is more efficient than polling and supports watching
 * entire directory trees.
 *
 * Key Features:
 *   - Recursive directory watching with single FSEventStream
 *   - 200ms debounce to batch rapid file changes
 *   - Support for filtering by file extension
 *   - Thread-safe callback system
 *
 * Reference: Apple File System Events Programming Guide
 *
 * =================================================================================================
 */

#import <CoreServices/CoreServices.h>
#import <dispatch/dispatch.h>

// TODO(AGENT_MACOS_2): Define file change event types
//   - FILE_CREATED, FILE_MODIFIED, FILE_DELETED, FILE_RENAMED
//   - Include path, old_path (for renames), and event flags
//   - Difficulty: 2

// TODO(AGENT_MACOS_2): Create FSEventStreamContext with user data
//   - Store callback function pointer
//   - Store user-provided context
//   - Store filter patterns (extensions to watch)
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Implement FSEventStreamCallback function
//   - Parse eventFlags to determine change type
//   (kFSEventStreamEventFlagItemCreated, etc.)
//   - Filter events by configured extensions
//   - Batch events that occur within debounce window
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Create debounce timer using dispatch_source
//   - Use dispatch_source_t for timer events
//   - Reset timer on each file change event
//   - Fire callback after 200ms of no changes
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Implement thread-safe event queue
//   - Lock-free queue for events from FSEvents callback
//   - Main thread drains queue and calls user callbacks
//   - Prevent callback reentrancy
//   - Difficulty: 6

// TODO(AGENT_MACOS_2): Create extension filter system
//   - Support multiple extensions (e.g., ".c", ".h", ".metal")
//   - Support exclusion patterns (e.g., "!.DS_Store")
//   - Case-insensitive matching on macOS
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Implement directory exclusion list
//   - Skip .git, .build, node_modules, etc.
//   - Configurable via engine settings
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create FSEventStream lifecycle management
//   - Start/stop stream on demand
//   - Properly release resources on shutdown
//   - Handle stream invalidation errors
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Implement multiple watch paths support
//   - Single FSEventStream can watch multiple paths
//   - Track which path triggered each event
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Create historical event query
//   - Query events since last run using sinceWhen parameter
//   - Useful for detecting changes while app was closed
//   - Store last event ID in preferences
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Implement latency configuration
//   - Allow configuring debounce time (default 200ms)
//   - Lower latency for development, higher for production
//   - Difficulty: 2

// TODO(AGENT_MACOS_2): Create file content hash verification
//   - Compute hash of file content on change detection
//   - Only trigger reload if content actually changed
//   - Prevents false positives from metadata-only changes
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Implement symlink resolution
//   - Follow symlinks to actual file locations
//   - Track both symlink and target for notifications
//   - Handle symlink creation/deletion
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Create coalescing for related events
//   - Combine CREATE + MODIFY into single CREATE
//   - Combine MODIFY + DELETE into single DELETE
//   - Handle rapid rename chains
//   - Difficulty: 6

// TODO(AGENT_MACOS_2): Implement priority-based callback ordering
//   - Critical files (shaders) processed first
//   - Less important files (textures) processed after
//   - Configurable priority per extension
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Create statistics and diagnostics
//   - Track events per second
//   - Track callback latency
//   - Log warnings for excessive event rates
//   - Difficulty: 4

typedef enum FSWatchEventType {
  FS_WATCH_EVENT_CREATED = 0,
  FS_WATCH_EVENT_MODIFIED = 1,
  FS_WATCH_EVENT_DELETED = 2,
  FS_WATCH_EVENT_RENAMED = 3,
} FSWatchEventType;

typedef struct FSWatchEvent {
  FSWatchEventType type;
  char path[1024];
  char old_path[1024]; // For renames
  uint64_t event_id;
  uint64_t timestamp_ns;
} FSWatchEvent;

typedef void (*FSWatchCallback)(const FSWatchEvent *event, void *user_data);

typedef struct FSWatchConfig {
  const char **watch_paths;
  size_t watch_path_count;
  const char **extensions; // Extensions to include (e.g., ".c", ".h")
  size_t extension_count;
  const char **exclude_dirs; // Directories to exclude
  size_t exclude_dir_count;
  double debounce_seconds; // Default 0.2
  bool follow_symlinks;
  FSWatchCallback callback;
  void *user_data;
} FSWatchConfig;

typedef struct FSWatcher {
  FSEventStreamRef stream;
  dispatch_source_t debounce_timer;
  dispatch_queue_t callback_queue;
  FSWatchConfig config;

  // Event batching
  // TODO(AGENT_MACOS_2): Add lock-free event queue here

  // Statistics
  uint64_t total_events;
  uint64_t filtered_events;
  uint64_t callbacks_fired;

  bool running;
} FSWatcher;

// Global watcher instance (or use per-instance allocation)
// TODO(AGENT_MACOS_2): Decide on singleton vs. multi-instance pattern
static FSWatcher *g_fs_watcher = NULL;

/**
 * Initialize the file system watcher with the given configuration.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Validate configuration
 *   - Create FSEventStream with configured paths
 *   - Schedule on a dedicated dispatch queue
 *   - Start the stream
 *   - Difficulty: 6
 *
 * @param config Watcher configuration
 * @return Allocated watcher instance, or NULL on failure
 */
FSWatcher *fsevents_watcher_create(const FSWatchConfig *config) {
  // TODO(AGENT_MACOS_2): Implementation
  return NULL;
}

/**
 * Destroy the file system watcher and release resources.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Stop the FSEventStream
 *   - Invalidate and release the stream
 *   - Cancel debounce timer
 *   - Release dispatch queues
 *   - Free memory
 *   - Difficulty: 3
 *
 * @param watcher The watcher to destroy
 */
void fsevents_watcher_destroy(FSWatcher *watcher) {
  // TODO(AGENT_MACOS_2): Implementation
}

/**
 * Pause the watcher temporarily.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Stop the FSEventStream but keep resources
 *   - Useful during heavy I/O operations
 *   - Difficulty: 2
 *
 * @param watcher The watcher to pause
 */
void fsevents_watcher_pause(FSWatcher *watcher) {
  // TODO(AGENT_MACOS_2): Implementation
}

/**
 * Resume a paused watcher.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Restart the FSEventStream
 *   - Optionally query historical events while paused
 *   - Difficulty: 2
 *
 * @param watcher The watcher to resume
 */
void fsevents_watcher_resume(FSWatcher *watcher) {
  // TODO(AGENT_MACOS_2): Implementation
}

/**
 * Add a new path to watch.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Stop current stream
 *   - Add path to watch list
 *   - Recreate stream with new paths
 *   - Resume watching
 *   - Difficulty: 4
 *
 * @param watcher The watcher instance
 * @param path Path to add
 * @return true if path was added successfully
 */
bool fsevents_watcher_add_path(FSWatcher *watcher, const char *path) {
  // TODO(AGENT_MACOS_2): Implementation
  return false;
}

/**
 * Remove a path from watching.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Stop current stream
 *   - Remove path from watch list
 *   - Recreate stream if paths remain
 *   - Difficulty: 4
 *
 * @param watcher The watcher instance
 * @param path Path to remove
 * @return true if path was removed successfully
 */
bool fsevents_watcher_remove_path(FSWatcher *watcher, const char *path) {
  // TODO(AGENT_MACOS_2): Implementation
  return false;
}

/**
 * Manually flush pending events immediately.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Cancel debounce timer
 *   - Fire callbacks for all pending events
 *   - Useful before reload operations
 *   - Difficulty: 3
 *
 * @param watcher The watcher instance
 */
void fsevents_watcher_flush(FSWatcher *watcher) {
  // TODO(AGENT_MACOS_2): Implementation
}

/**
 * Get watcher statistics.
 *
 * TODO(AGENT_MACOS_2): Implement this function
 *   - Return copy of statistics struct
 *   - Include events per second, callback latency
 *   - Difficulty: 2
 *
 * @param watcher The watcher instance
 * @param out_stats Output statistics buffer
 */
void fsevents_watcher_get_stats(FSWatcher *watcher, void *out_stats) {
  // TODO(AGENT_MACOS_2): Implementation
}

// Additional TODOs for complete FSEvents implementation:

// TODO(AGENT_MACOS_2): Implement kFSEventStreamEventFlagMustScanSubDirs
// handling [Difficulty: 5]
// TODO(AGENT_MACOS_2): Create recursive directory scan fallback [Difficulty: 6]
// TODO(AGENT_MACOS_2): Implement root changed notification handling
// [Difficulty: 4]
// TODO(AGENT_MACOS_2): Create mount/unmount detection [Difficulty: 4]
// TODO(AGENT_MACOS_2): Implement case sensitivity detection per volume
// [Difficulty: 3]
// TODO(AGENT_MACOS_2): Create inode-based change detection [Difficulty: 6]
// TODO(AGENT_MACOS_2): Implement xattr change detection [Difficulty: 4]
// TODO(AGENT_MACOS_2): Create ownership/permission change detection
// [Difficulty: 3]
// TODO(AGENT_MACOS_2): Implement finder info change detection [Difficulty: 3]
// TODO(AGENT_MACOS_2): Create file size change threshold filter [Difficulty: 3]
// TODO(AGENT_MACOS_2): Implement modification time validation [Difficulty: 4]
// TODO(AGENT_MACOS_2): Create path normalization utilities [Difficulty: 4]
// TODO(AGENT_MACOS_2): Implement tilde expansion for paths [Difficulty: 2]
// TODO(AGENT_MACOS_2): Create relative path resolution [Difficulty: 3]
// TODO(AGENT_MACOS_2): Implement path component matching [Difficulty: 4]
// TODO(AGENT_MACOS_2): Create glob pattern matching for filters [Difficulty: 5]
// TODO(AGENT_MACOS_2): Implement regex pattern matching option [Difficulty: 5]
// TODO(AGENT_MACOS_2): Create callback rate limiting [Difficulty: 4]
// TODO(AGENT_MACOS_2): Implement callback priority queue [Difficulty: 5]
// TODO(AGENT_MACOS_2): Create async callback execution mode [Difficulty: 5]

// Total TODOs in this file: ~45
// Estimated LOC when complete: ~2,000
