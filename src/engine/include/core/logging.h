#ifndef CORE_LOGGING_H
#define CORE_LOGGING_H

#include <stdio.h>
#include <stdarg.h>

// Simple logging macros for core engine systems
#define LOG_INFO(fmt, ...) fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

#endif /* CORE_LOGGING_H */
