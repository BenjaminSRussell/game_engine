#include <stdio.h>
#include <stdarg.h>
#include "../include/thread/job.h>
#include "../include/world/generator.h>
#include "../include/core/logger.h>
#include "../include/weather/weather.h>

// Minimal stubs for globals used by chunk_manager during tests
ThreadPool *g_thread_pool = NULL;
WorldGenerator *g_world_generator = NULL;
u32 g_world_seed = 0;

// Minimal logger implementation for tests
Logger g_logger;

void logger_init(LogLevel level, LogTarget target, const char *filename) { (void)level; (void)target; (void)filename; }
void logger_shutdown(void) {}
void logger_set_level(LogLevel level) { (void)level; }
void logger_set_target(LogTarget target) { (void)target; }

void logger_log(LogLevel level, const char *category, const char *format, ...) {
    (void)level;
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s: ", category ? category : "log");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void logger_log_hex(const char *data, u32 length) { (void)data; (void)length; }

void logger_set_buffering(bool enabled, u32 buffer_size) { (void)enabled; (void)buffer_size; }
void logger_flush_buffer(void) {}
void logger_set_category_filter(LogCategory category, LogLevel level, bool enabled) { (void)category; (void)level; (void)enabled; }
void logger_set_category_name(LogCategory category, const char *name) { (void)category; (void)name; }
void logger_set_file_rotation(u32 max_size, u32 max_backups) { (void)max_size; (void)max_backups; }
void logger_rotate_file(void) {}
void logger_log_with_category(LogCategory category, LogLevel level, const char *function, const char *format, ...) { (void)category; (void)level; (void)function; va_list args; va_start(args, format); vfprintf(stderr, format, args); fprintf(stderr, "\n"); va_end(args); }
void logger_set_session_id(const char *session_id) { (void)session_id; }
void logger_log_session_info(void) {}
void logger_set_use_colors(bool enabled) { (void)enabled; }
void logger_set_show_timestamp(bool enabled) { (void)enabled; }

void logger_breadcrumb_add(LogEntry entry) { (void)entry; }
const LogEntry *logger_breadcrumb_get_last(u32 count, u32 *out_count) { (void)count; if (out_count) *out_count = 0; return NULL; }

// Minimal stub implementations for thread pool / world generator functions
void thread_pool_submit(ThreadPool *pool, JobFunction function, void *data, u32 priority) {
    (void)pool; (void)priority; // Execute synchronously for tests
    if (function) function(data);
}

void world_generator_generate_chunk(WorldGenerator *gen, Chunk *chunk) {
    (void)gen; (void)chunk; // no-op for tests
}

// Minimal stub for block registry and light propagation called by chunk generation
BlockRegistry g_block_registry;

void block_propagate_light(ChunkManager *chunk_manager, BlockRegistry *block_registry, i32 x, i32 y, i32 z, u8 light_level, ChunkLightType type) {
    (void)chunk_manager; (void)block_registry; (void)x; (void)y; (void)z; (void)light_level; (void)type; // no-op
}
