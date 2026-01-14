#ifndef CRASH_DEBUGGER_H
#define CRASH_DEBUGGER_H

#include <signal.h>

// Enhanced crash debugging system with detailed backtrace and logging
// Provides comprehensive crash reporting and debugging information

// Initialize the crash debugger system
void init_crash_debugger(void);

// Log a backtrace with context information
void log_backtrace(const char* context);

// Enhanced signal handler for crash detection
void enhanced_signal_handler(int sig, siginfo_t* info, void* context);

// Setup enhanced crash handler for common signals
void setup_enhanced_crash_handler(void);

// Manual backtrace check for debugging specific locations
void manual_backtrace_check(const char* location);

// Cleanup crash debugger resources
void cleanup_crash_debugger(void);

#endif // CRASH_DEBUGGER_H
