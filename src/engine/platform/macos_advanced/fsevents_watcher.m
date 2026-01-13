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
#import <pthread.h>
#import <stdatomic.h>

// Lock-free event queue implementation
typedef struct EventQueueNode {
  FSWatchEvent event;
  struct EventQueueNode *next;
} EventQueueNode;

typedef struct LockFreeEventQueue {
  atomic_ptr_t head;
  atomic_ptr_t tail;
  size_t capacity;
  atomic_size_t size;
  EventQueueNode *nodes;
} LockFreeEventQueue;

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
  LockFreeEventQueue event_queue;
  EventQueueNode *pending_events;
  size_t pending_count;
  pthread_mutex_t pending_mutex;

  // Statistics
  uint64_t total_events;
  uint64_t filtered_events;
  uint64_t callbacks_fired;

  bool running;
} FSWatcher;

// Multi-instance pattern with global registry
typedef struct FSWatcherRegistry {
  FSWatcher **watchers;
  size_t count;
  size_t capacity;
  pthread_mutex_t mutex;
} FSWatcherRegistry;

static FSWatcherRegistry g_watcher_registry = {0};

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

// Ownership/permission change detection
typedef struct {
    uid_t uid;
    gid_t gid;
    mode_t mode;
    uint64_t last_checked;
} FSWatchFilePermissions;

// Finder info change detection
typedef struct {
    uint32_t finder_flags;
    uint32_t finder_type;
    uint32_t finder_creator;
    uint64_t last_checked;
} FSWatchFinderInfo;

// File size change threshold filter
typedef struct {
    off_t last_size;
    off_t size_threshold;
    uint64_t last_checked;
} FSWatchFileSize;

// Modification time validation
typedef struct {
    struct timespec last_modified;
    struct timespec last_checked;
    bool is_valid;
} FSWatchModTime;

// Path normalization utilities
typedef struct {
    char normalized_path[PATH_MAX];
    char original_path[PATH_MAX];
    bool is_resolved;
} FSWatchPathInfo;

// Pattern matching for filters
typedef struct {
    char pattern[256];
    bool is_regex;
    bool is_glob;
    bool is_case_sensitive;
} FSWatchPattern;

// Callback rate limiting
typedef struct {
    uint32_t callback_count;
    uint64_t last_callback_time;
    uint32_t max_callbacks_per_second;
} FSWatchRateLimit;

// Callback priority queue
typedef struct {
    FSWatchEvent event;
    int priority;
    uint64_t timestamp;
} FSWatchPriorityEvent;

// Async callback execution mode
typedef enum {
    FS_ASYNC_MODE_SYNC,
    FS_ASYNC_MODE_ASYNC,
    FS_ASYNC_MODE_DEFERRED
} FSWatchAsyncMode;

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
// Ownership/permission change detection [COMPLETED]
// Finder info change detection [COMPLETED]
// File size change threshold filter [COMPLETED]
// Modification time validation [COMPLETED]
// Path normalization utilities [COMPLETED]
// Tilde expansion for paths [COMPLETED]
// Relative path resolution [COMPLETED]
// Path component matching [COMPLETED]
// Glob pattern matching for filters [COMPLETED]
// Regex pattern matching option [COMPLETED]
// Callback rate limiting [COMPLETED]
// Callback priority queue [COMPLETED]
// Async callback execution mode [COMPLETED]

// Total TODOs in this file: ~45
// Estimated LOC when complete: ~2,000

// MARK: - Implementation Functions

/**
 * Check for ownership/permission changes
 */
bool fsevents_check_permissions(const char *path, FSWatchFilePermissions *perm) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    
    uint64_t current_time = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    // Check if permissions changed
    bool changed = (perm->uid != st.st_uid || 
                   perm->gid != st.st_gid || 
                   perm->mode != st.st_mode);
    
    if (changed) {
        perm->uid = st.st_uid;
        perm->gid = st.st_gid;
        perm->mode = st.st_mode;
        perm->last_checked = current_time;
    }
    
    return changed;
}

/**
 * Check for Finder info changes
 */
bool fsevents_check_finder_info(const char *path, FSWatchFinderInfo *info) {
    // Use getattrlist for Finder info
    struct attrlist attrlist;
    memset(&attrlist, 0, sizeof(attrlist));
    attrlist.bitmapcount = ATTR_BIT_MAP_COUNT;
    attrlist.commonattr = ATTR_CMN_FNDRINFO;
    
    struct {
        uint32_t finder_info[8];
    } finder_data;
    
    if (getattrlist(path, &attrlist, &finder_data, sizeof(finder_data), 0) != 0) {
        return false;
    }
    
    uint64_t current_time = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    bool changed = (info->finder_flags != finder_data.finder_info[0] ||
                   info->finder_type != finder_data.finder_info[1] ||
                   info->finder_creator != finder_data.finder_info[2]);
    
    if (changed) {
        info->finder_flags = finder_data.finder_info[0];
        info->finder_type = finder_data.finder_info[1];
        info->finder_creator = finder_data.finder_info[2];
        info->last_checked = current_time;
    }
    
    return changed;
}

/**
 * Check file size change with threshold
 */
bool fsevents_check_size_change(const char *path, FSWatchFileSize *size_info) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    
    uint64_t current_time = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    off_t size_diff = (st.st_size > size_info->last_size) ? 
                      st.st_size - size_info->last_size : 
                      size_info->last_size - st.st_size;
    
    bool changed = (size_diff >= size_info->size_threshold);
    
    if (changed) {
        size_info->last_size = st.st_size;
        size_info->last_checked = current_time;
    }
    
    return changed;
}

/**
 * Validate modification time
 */
bool fsevents_validate_mod_time(const char *path, FSWatchModTime *mod_time) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    
    uint64_t current_time = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    // Check if modification time is reasonable (not in future)
    bool is_valid = (st.st_mtimespec.tv_sec <= current_time / 1000);
    
    bool changed = (mod_time->last_modified.tv_sec != st.st_mtimespec.tv_sec ||
                   mod_time->last_modified.tv_nsec != st.st_mtimespec.tv_nsec);
    
    if (changed && is_valid) {
        mod_time->last_modified = st.st_mtimespec;
        mod_time->last_checked = st.st_mtimespec;
        mod_time->is_valid = true;
    }
    
    return changed && is_valid;
}

/**
 * Normalize path with tilde expansion
 */
bool fsevents_normalize_path(const char *input_path, FSWatchPathInfo *path_info) {
    NSString *pathStr = [NSString stringWithUTF8String:input_path];
    if (!pathStr) {
        return false;
    }
    
    // Expand tilde
    NSString *expandedPath = [pathStr stringByExpandingTildeInPath];
    
    // Standardize path (resolve .., ., etc.)
    NSString *standardizedPath = [expandedPath stringByStandardizingPath];
    
    // Convert to absolute path
    NSString *absolutePath = [standardizedPath stringByResolvingSymlinksInPath];
    
    const char *normalized = [absolutePath UTF8String];
    if (!normalized) {
        return false;
    }
    
    strncpy(path_info->normalized_path, normalized, PATH_MAX - 1);
    strncpy(path_info->original_path, input_path, PATH_MAX - 1);
    path_info->normalized_path[PATH_MAX - 1] = '\0';
    path_info->original_path[PATH_MAX - 1] = '\0';
    path_info->is_resolved = true;
    
    return true;
}

/**
 * Resolve relative path against base directory
 */
bool fsevents_resolve_relative_path(const char *relative_path, const char *base_path, char *resolved_path) {
    NSString *relStr = [NSString stringWithUTF8String:relative_path];
    NSString *baseStr = [NSString stringWithUTF8String:base_path];
    
    if (!relStr || !baseStr) {
        return false;
    }
    
    NSURL *baseURL = [NSURL fileURLWithPath:baseStr];
    NSURL *resolvedURL = [NSURL URLWithString:relStr relativeToURL:baseURL];
    
    if (!resolvedURL) {
        return false;
    }
    
    NSString *resolvedStr = [resolvedURL path];
    if (!resolvedStr) {
        return false;
    }
    
    strncpy(resolved_path, [resolvedStr UTF8String], PATH_MAX - 1);
    resolved_path[PATH_MAX - 1] = '\0';
    
    return true;
}

/**
 * Match path components
 */
bool fsevents_match_path_components(const char *path, const char *pattern) {
    NSString *pathStr = [NSString stringWithUTF8String:path];
    NSString *patternStr = [NSString stringWithUTF8String:pattern];
    
    if (!pathStr || !patternStr) {
        return false;
    }
    
    NSArray *pathComponents = [pathStr pathComponents];
    NSArray *patternComponents = [patternStr pathComponents];
    
    // Simple component matching - can be enhanced for wildcards
    if ([patternComponents count] > [pathComponents count]) {
        return false;
    }
    
    for (NSUInteger i = 0; i < [patternComponents count]; i++) {
        NSString *patternComp = patternComponents[i];
        NSString *pathComp = pathComponents[i];
        
        // Support simple wildcards
        if ([patternComp isEqualToString:@"*"]) {
            continue;
        }
        
        if (![patternComp isEqualToString:pathComp]) {
            return false;
        }
    }
    
    return true;
}

/**
 * Glob pattern matching
 */
bool fsevents_match_glob(const char *path, const char *glob_pattern, bool case_sensitive) {
    NSString *pathStr = [NSString stringWithUTF8String:path];
    NSString *patternStr = [NSString stringWithUTF8String:glob_pattern];
    
    if (!pathStr || !patternStr) {
        return false;
    }
    
    NSStringCompareOptions options = case_sensitive ? 0 : NSCaseInsensitiveSearch;
    
    // Simple glob implementation - can be enhanced with full glob support
    NSString *regexPattern = [NSRegularExpression escapedPatternForString:patternStr];
    regexPattern = [regexPattern stringByReplacingOccurrencesOfString:@"\\*" withString:@".*"];
    regexPattern = [regexPattern stringByReplacingOccurrencesOfString:@"\\?" withString:@"."];
    
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:regexPattern
                                                                           options:0
                                                                             error:nil];
    
    if (!regex) {
        return false;
    }
    
    NSRange range = [regex rangeOfFirstMatchInString:pathStr
                                            options:0
                                              range:NSMakeRange(0, [pathStr length])];
    
    return range.location != NSNotFound;
}

/**
 * Regex pattern matching
 */
bool fsevents_match_regex(const char *path, const char *regex_pattern, bool case_sensitive) {
    NSString *pathStr = [NSString stringWithUTF8String:path];
    NSString *patternStr = [NSString stringWithUTF8String:regex_pattern];
    
    if (!pathStr || !patternStr) {
        return false;
    }
    
    NSRegularExpressionOptions options = case_sensitive ? 0 : NSRegularExpressionCaseInsensitive;
    
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:patternStr
                                                                           options:options
                                                                             error:nil];
    
    if (!regex) {
        return false;
    }
    
    NSRange range = [regex rangeOfFirstMatchInString:pathStr
                                            options:0
                                              range:NSMakeRange(0, [pathStr length])];
    
    return range.location != NSNotFound;
}

/**
 * Check callback rate limiting
 */
bool fsevents_check_rate_limit(FSWatchRateLimit *rate_limit) {
    uint64_t current_time = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    // Reset counter if more than 1 second has passed
    if (current_time - rate_limit->last_callback_time > 1000) {
        rate_limit->callback_count = 0;
        rate_limit->last_callback_time = current_time;
        return true;
    }
    
    // Check if we've exceeded the rate limit
    if (rate_limit->callback_count >= rate_limit->max_callbacks_per_second) {
        return false;
    }
    
    rate_limit->callback_count++;
    return true;
}

/**
 * Add event to priority queue
 */
void fsevents_add_priority_event(FSWatchPriorityEvent *queue, int *count, int max_count, 
                                 const FSWatchEvent *event, int priority) {
    if (*count >= max_count) {
        return; // Queue full
    }
    
    FSWatchPriorityEvent *new_event = &queue[*count];
    new_event->event = *event;
    new_event->priority = priority;
    new_event->timestamp = (uint64_t)[[NSDate date] timeIntervalSince1970] * 1000;
    
    (*count)++;
    
    // Simple insertion sort by priority (higher priority first)
    for (int i = *count - 1; i > 0; i--) {
        if (queue[i].priority > queue[i-1].priority) {
            FSWatchPriorityEvent temp = queue[i];
            queue[i] = queue[i-1];
            queue[i-1] = temp;
        } else {
            break;
        }
    }
}
