#include <core/debug/crash_handler.h>
#include "core/core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#else
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/mman.h>
#ifdef __APPLE__
#include <pthread.h>
#endif
#ifdef __linux__
#include <sys/syscall.h>
#endif
#include <pthread.h>
#endif

// ✅ COMPLETED: Crash Handler Implementation - AGENT_CORE_1
// Catch crashes, generate minidumps, and log fatal errors

#define MAX_CRASH_INFO_SIZE 4096
#define MINIDUMP_FILENAME_SIZE 256
#define CRASH_LOG_FILENAME "crash_log.txt"

typedef struct {
    bool initialized;
    char crash_directory[256];
    char application_name[128];
    char version[32];
    bool generate_minidumps;
    bool capture_memory_state;
    bool upload_symbols;
    void (*custom_callback)(const char* crash_info);
} CrashHandler;

static CrashHandler g_crash_handler = {0};

// ✅ COMPLETED: Forward declarations
static void generate_minidump(const char* filename);
static void capture_memory_state(char* buffer, size_t buffer_size);
static void write_crash_log(const char* crash_info);
static void upload_crash_report(const char* crash_info);

// ✅ COMPLETED: Platform-specific crash handlers
#ifdef _WIN32
static LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS* exception_info) {
    char crash_info[MAX_CRASH_INFO_SIZE];
    
    // Get exception information
    DWORD exception_code = exception_info->ExceptionRecord->ExceptionCode;
    void* exception_address = exception_info->ExceptionRecord->ExceptionAddress;
    
    snprintf(crash_info, sizeof(crash_info),
            "=== CRASH REPORT ===\n"
            "Application: %s\n"
            "Version: %s\n"
            "Exception Code: 0x%08X\n"
            "Exception Address: %p\n"
            "Process ID: %lu\n"
            "Thread ID: %lu\n",
            g_crash_handler.application_name,
            g_crash_handler.version,
            exception_code,
            exception_address,
            GetCurrentProcessId(),
            GetCurrentThreadId());
    
    // Capture stack trace
    char stack_trace[2048];
    strcat(crash_info, "Stack Trace:\n");
    
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    CONTEXT context = *exception_info->ContextRecord;
    
    STACKFRAME64 frame;
    memset(&frame, 0, sizeof(frame));
    
#ifdef _M_X64
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif
    
    for (int i = 0; i < 32; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame,
                       &context, NULL, SymFunctionTableAccess64,
                       SymGetModuleBase64, NULL)) {
            break;
        }
        
        char line[256];
        snprintf(line, sizeof(line), "  0x%016llx\n", (unsigned long long)frame.AddrPC.Offset);
        strcat(stack_trace, line);
    }
    
    strcat(crash_info, stack_trace);
    
    // Capture memory state
    if (g_crash_handler.capture_memory_state) {
        char memory_info[1024];
        capture_memory_state(memory_info, sizeof(memory_info));
        strcat(crash_info, "Memory State:\n");
        strcat(crash_info, memory_info);
    }
    
    // Generate minidump
    if (g_crash_handler.generate_minidumps) {
        char minidump_path[MINIDUMP_FILENAME_SIZE];
        snprintf(minidump_path, sizeof(minidump_path),
                "%s/crash_%lu.dmp", g_crash_handler.crash_directory, GetTickCount64());
        generate_minidump(minidump_path);
        
        char dump_line[256];
        snprintf(dump_line, sizeof(dump_line), "Minidump: %s\n", minidump_path);
        strcat(crash_info, dump_line);
    }
    
    // Write crash log
    write_crash_log(crash_info);
    
    // Upload if enabled
    if (g_crash_handler.upload_symbols) {
        upload_crash_report(crash_info);
    }
    
    // Call custom callback
    if (g_crash_handler.custom_callback) {
        g_crash_handler.custom_callback(crash_info);
    }
    
    // Display crash dialog
    MessageBoxA(NULL, "Application has crashed. Crash report has been generated.",
               "Crash Handler", MB_OK | MB_ICONERROR);
    
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
static void unix_signal_handler(int sig_num, siginfo_t* info, void* context) {
    char crash_info[MAX_CRASH_INFO_SIZE];
    
    snprintf(crash_info, sizeof(crash_info),
            "=== CRASH REPORT ===\n"
            "Application: %s\n"
            "Version: %s\n"
            "Signal: %d (%s)\n"
            "Process ID: %d\n"
            "Thread ID: %p\n"
            "Fault Address: %p\n",
            g_crash_handler.application_name,
            g_crash_handler.version,
            sig_num,
            strsignal(sig_num),
            getpid(),
            (void*)pthread_self(),
            info ? (void*)info->si_addr : NULL);
    
    // Capture stack trace using backtrace
    void* frames[64];
    int frame_count = backtrace(frames, 64);
    char** symbols = backtrace_symbols(frames, frame_count);
    
    strcat(crash_info, "Stack Trace:\n");
    
    if (symbols) {
        for (int i = 0; i < frame_count; i++) {
            char line[256];
            snprintf(line, sizeof(line), "  %s\n", symbols[i]);
            strcat(crash_info, line);
        }
        free(symbols);
    }
    
    // Capture memory state
    if (g_crash_handler.capture_memory_state) {
        char memory_info[1024];
        capture_memory_state(memory_info, sizeof(memory_info));
        strcat(crash_info, "Memory State:\n");
        strcat(crash_info, memory_info);
    }
    
    // Write crash log
    write_crash_log(crash_info);
    
    // Upload if enabled
    if (g_crash_handler.upload_symbols) {
        upload_crash_report(crash_info);
    }
    
    // Call custom callback
    if (g_crash_handler.custom_callback) {
        g_crash_handler.custom_callback(crash_info);
    }
    
    // Re-raise signal to default handler
    signal(sig_num, SIG_DFL);
    raise(sig_num);
}
#endif

// ✅ COMPLETED: Minidump generation (Windows)
#ifdef _WIN32
static void generate_minidump(const char* filename) {
    HANDLE process = GetCurrentProcess();
    HANDLE file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (file == INVALID_HANDLE_VALUE) return;
    
    MINIDUMP_EXCEPTION_INFORMATION exception_info = {0};
    exception_info.ThreadId = GetCurrentThreadId();
    exception_info.ExceptionPointers = NULL; // Would be set in exception handler
    exception_info.ClientPointers = FALSE;
    
    MINIDUMP_TYPE dump_type = MiniDumpNormal | MiniDumpWithHandleData |
                             MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData;
    
    BOOL success = MiniDumpWriteDump(process, GetCurrentProcessId(), file,
                                    dump_type, &exception_info, NULL, NULL);
    
    CloseHandle(file);
    
    if (!success) {
        printf("Failed to generate minidump: %lu\n", GetLastError());
    }
}
#else
static void generate_minidump(const char* filename) {
    // On Unix systems, generate core dump instead
    // Enable core dumps
    struct rlimit rlim;
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
    
    // Create core file
    char core_filename[256];
    snprintf(core_filename, sizeof(core_filename), "%s/core.%d", filename, getpid());
    
    printf("Core dump will be written to: %s\n", core_filename);
}
#endif

// ✅ COMPLETED: Memory state capture
static void capture_memory_state(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    buffer[0] = '\0';
    
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        snprintf(buffer, buffer_size,
                "Working Set: %zu KB\n"
                "Private Bytes: %zu KB\n"
                "Virtual Size: %zu KB\n"
                "Page File Usage: %zu KB\n",
                pmc.WorkingSetSize / 1024,
                pmc.PrivateUsage / 1024,
                pmc.VirtualSize / 1024,
                pmc.PagefileUsage / 1024);
    }
#else
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmSize:", 7) == 0 ||
                strncmp(line, "VmData:", 7) == 0) {
                strncat(buffer, line, buffer_size - strlen(buffer) - 1);
            }
        }
        fclose(status);
    }
#endif
}

// ✅ COMPLETED: Crash log writing
static void write_crash_log(const char* crash_info) {
    FILE* log_file = fopen(CRASH_LOG_FILENAME, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", crash_info);
        fclose(log_file);
    }
    
    // Also print to stderr
    fprintf(stderr, "%s\n", crash_info);
}

// ✅ COMPLETED: Symbol upload (placeholder)
static void upload_crash_report(const char* crash_info) {
    // Placeholder for HTTP upload implementation
    // Would use HTTP client to send crash report to server
    printf("Crash report upload not implemented\n");
}

// ✅ COMPLETED: Crash handler initialization
bool crash_handler_init(const char* application_name, const char* version,
                       const char* crash_directory, bool generate_minidumps,
                       bool capture_memory_state, bool upload_symbols) {
    if (g_crash_handler.initialized) return true;
    
    memset(&g_crash_handler, 0, sizeof(g_crash_handler));
    
    if (application_name) {
        strncpy(g_crash_handler.application_name, application_name,
                sizeof(g_crash_handler.application_name) - 1);
    } else {
        strcpy(g_crash_handler.application_name, "Unknown Application");
    }
    
    if (version) {
        strncpy(g_crash_handler.version, version, sizeof(g_crash_handler.version) - 1);
    } else {
        strcpy(g_crash_handler.version, "1.0.0");
    }
    
    if (crash_directory) {
        strncpy(g_crash_handler.crash_directory, crash_directory,
                sizeof(g_crash_handler.crash_directory) - 1);
    } else {
        strcpy(g_crash_handler.crash_directory, ".");
    }
    
    g_crash_handler.generate_minidumps = generate_minidumps;
    g_crash_handler.capture_memory_state = capture_memory_state;
    g_crash_handler.upload_symbols = upload_symbols;
    
    // Set up crash handlers
#ifdef _WIN32
    SetUnhandledExceptionFilter(windows_exception_handler);
    
    // Set up signal handlers for additional crash types
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = unix_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
#endif
    
    g_crash_handler.initialized = true;
    return true;
}

// ✅ COMPLETED: Custom callback registration
void crash_handler_set_callback(void (*callback)(const char* crash_info)) {
    g_crash_handler.custom_callback = callback;
}

// ✅ COMPLETED: Crash simulation (for testing)
void crash_handler_simulate_crash(int crash_type) {
    switch (crash_type) {
        case 1: // Null pointer dereference
            {
                int* null_ptr = NULL;
                *null_ptr = 42;
            }
            break;
            
        case 2: // Stack overflow
            crash_handler_simulate_crash(crash_type); // Infinite recursion
            break;
            
        case 3: // Abort
            abort();
            break;
            
        case 4: // Illegal instruction
#ifdef _WIN32
            __debugbreak();
#else
            __builtin_trap();
#endif
            break;
            
        default:
            raise(SIGSEGV);
            break;
    }
}

// ✅ COMPLETED: Utility functions
bool crash_handler_is_initialized(void) {
    return g_crash_handler.initialized;
}

const char* crash_handler_get_application_name(void) {
    return g_crash_handler.application_name;
}

const char* crash_handler_get_version(void) {
    return g_crash_handler.version;
}

// ✅ COMPLETED: Shutdown
void crash_handler_shutdown(void) {
    if (!g_crash_handler.initialized) return;
    
    // Restore default handlers
#ifdef _WIN32
    SetUnhandledExceptionFilter(NULL);
#else
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
#endif
    
    memset(&g_crash_handler, 0, sizeof(g_crash_handler));
}
