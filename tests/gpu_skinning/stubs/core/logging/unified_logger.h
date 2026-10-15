// Test-only stand-in for core/logging/unified_logger.h. gpu_skinning.c passes a
// category as the first argument (LOG_INFO(LOG_CAT_RENDERER, "...")), which the
// real one-format macros reject. The test discards all log output.
#ifndef GPU_SKINNING_TEST_UNIFIED_LOGGER_H
#define GPU_SKINNING_TEST_UNIFIED_LOGGER_H

#define LOG_TRACE(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_FATAL(...) ((void)0)

#endif
