#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <time.h>

// Enhanced crash debugging with detailed backtrace and logging
#define MAX_STACK_FRAMES 128
#define MAX_SYMBOL_LENGTH 512
#define CRASH_LOG_FILE "crash_debug.log"

static FILE* crash_log = NULL;

void init_crash_debugger(void) {
    crash_log = fopen(CRASH_LOG_FILE, "a");
    if (crash_log) {
        time_t now = time(NULL);
        fprintf(crash_log, "\n=== Crash Debugger Initialized: %s ===\n", ctime(&now));
        fflush(crash_log);
    }
}

void log_backtrace(const char* context) {
    void* frames[MAX_STACK_FRAMES];
    char** symbols;
    int frame_count;
    
    frame_count = backtrace(frames, MAX_STACK_FRAMES);
    symbols = backtrace_symbols(frames, frame_count);
    
    if (crash_log) {
        fprintf(crash_log, "\n--- Backtrace: %s ---\n", context);
        fprintf(crash_log, "Frame count: %d\n", frame_count);
    }
    
    printf("\n=== BACKTRACE (%s) ===\n", context);
    printf("Type 'bt' for backtrace in debugger > RESOLUTION PRIORITY\n");
    printf("Frames captured: %d\n\n", frame_count);
    
    for (int i = 0; i < frame_count; i++) {
        printf("[%2d] %s\n", i, symbols ? symbols[i] : "???");
        if (crash_log) {
            fprintf(crash_log, "[%2d] %s\n", i, symbols ? symbols[i] : "???");
        }
    }
    
    if (crash_log) {
        fprintf(crash_log, "--- End Backtrace ---\n\n");
        fflush(crash_log);
    }
    
    if (symbols) {
        free(symbols);
    }
}

void enhanced_signal_handler(int sig, siginfo_t* info, void* context) {
    const char* sig_name = "UNKNOWN";
    
    switch (sig) {
        case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault)"; break;
        case SIGABRT: sig_name = "SIGABRT (Abort)"; break;
        case SIGFPE:  sig_name = "SIGFPE (Floating Point Exception)"; break;
        case SIGILL:  sig_name = "SIGILL (Illegal Instruction)"; break;
        case SIGBUS:  sig_name = "SIGBUS (Bus Error)"; break;
        default:      sig_name = "UNKNOWN SIGNAL"; break;
    }
    
    if (crash_log) {
        fprintf(crash_log, "\n!!! CRASH DETECTED !!!\n");
        fprintf(crash_log, "Signal: %s (%d)\n", sig_name, sig);
        fprintf(crash_log, "Fault address: %p\n", info ? info->si_addr : NULL);
        fprintf(crash_log, "Process ID: %d\n", getpid());
        fprintf(crash_log, "Thread ID: %lu\n", (unsigned long)pthread_self());
        fflush(crash_log);
    }
    
    printf("\n!!! CRASH DETECTED !!!\n");
    printf("Signal: %s (%d)\n", sig_name, sig);
    printf("Fault address: %p\n", info ? info->si_addr : NULL);
    printf("Process ID: %d\n", getpid());
    printf("Thread ID: %lu\n", (unsigned long)pthread_self());
    
    log_backtrace("CRASH");
    
    // Try to get additional context info
    if (context) {
        printf("Context available: %p\n", context);
        if (crash_log) {
            fprintf(crash_log, "Context available: %p\n", context);
        }
    }
    
    if (crash_log) {
        fclose(crash_log);
    }
    
    // Re-raise the signal to default handler
    signal(sig, SIG_DFL);
    raise(sig);
}

void setup_enhanced_crash_handler(void) {
    init_crash_debugger();
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = enhanced_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    
    printf("Enhanced crash handler installed\n");
    printf("Crash log: %s\n", CRASH_LOG_FILE);
    printf("When it crashes, type: `bt` for backtrace > RESOLUTION PRIORITY\n");
}

void manual_backtrace_check(const char* location) {
    printf("Manual backtrace check at: %s\n", location);
    log_backtrace(location);
}

void cleanup_crash_debugger(void) {
    if (crash_log) {
        time_t now = time(NULL);
        fprintf(crash_log, "=== Crash Debugger Shutdown: %s ===\n", ctime(&now));
        fclose(crash_log);
        crash_log = NULL;
    }
}
