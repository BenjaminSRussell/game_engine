// src/engine/include/core/logger.h
//
// Purpose: Consolidated unified logger interface - consolidates all logging
// backends
//
#ifndef LOGGER_H
#define LOGGER_H

// Include the unified logger implementation that consolidates all backends
#include <unified_logger.h>

// Backwards compatibility aliases for existing code
#define logger_init unified_logger_init
#define logger_shutdown unified_logger_shutdown
#define logger_log unified_logger_log
#define logger_flush unified_logger_flush
#define logger_get_stats unified_logger_get_stats
#define logger_benchmark unified_logger_benchmark

// Legacy macros for compatibility with existing code
// compatibility macros are now handled in unified_logger.h

#endif // LOGGER_H
