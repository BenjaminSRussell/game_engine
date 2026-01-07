/**
 * PLATFORM BRIDGE IMPLEMENTATION - Cross-Platform Abstraction
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
    #include <mach-o/dyld.h>
    #include <unistd.h>
    #include <sys/stat.h>
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #include <unistd.h>
    #include <linux/limits.h>
#endif

// ============================================================================
// FILE SYSTEM OPERATIONS
// ============================================================================

bool platform_file_exists(const char* path) {
#ifdef PLATFORM_WINDOWS
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES);
#else
    return access(path, F_OK) == 0;
#endif
}

bool platform_directory_exists(const char* path) {
#ifdef PLATFORM_WINDOWS
    DWORD attrib = GetFileAttributesA(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && 
            (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

bool platform_create_directory(const char* path) {
#ifdef PLATFORM_WINDOWS
    return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

char* platform_get_executable_path() {
    static char path[1024];
    
#ifdef PLATFORM_WINDOWS
    GetModuleFileNameA(NULL, path, sizeof(path));
#elif defined(PLATFORM_MACOS)
    uint32_t size = sizeof(path);
    _NSGetExecutablePath(path, &size);
#elif defined(PLATFORM_LINUX)
    readlink("/proc/self/exe", path, sizeof(path));
#endif
    
    return path;
}

char* platform_get_home_directory() {
    static char path[1024];
    
#ifdef PLATFORM_WINDOWS
    GetEnvironmentVariableA("USERPROFILE", path, sizeof(path));
#else
    const char* home = getenv("HOME");
    strncpy(path, home ? home : "/", sizeof(path) - 1);
#endif
    
    return path;
}

// ============================================================================
// DYNAMIC LIBRARY LOADING
// ============================================================================

typedef void* LibraryHandle;

LibraryHandle platform_load_library(const char* path) {
#ifdef PLATFORM_WINDOWS
    return LoadLibraryA(path);
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* platform_get_proc_address(LibraryHandle lib, const char* name) {
#ifdef PLATFORM_WINDOWS
    return GetProcAddress((HMODULE)lib, name);
#else
    return dlsym(lib, name);
#endif
}

void platform_unload_library(LibraryHandle lib) {
#ifdef PLATFORM_WINDOWS
    FreeLibrary((HMODULE)lib);
#else
    dlclose(lib);
#endif
}

// ============================================================================
// THREADING
// ============================================================================

typedef void* ThreadHandle;
typedef void (*ThreadFunction)(void*);

#ifdef PLATFORM_WINDOWS
DWORD WINAPI thread_wrapper(LPVOID param) {
    struct { ThreadFunction func; void* arg; }* data = param;
    data->func(data->arg);
    free(data);
    return 0;
}
#else
void* thread_wrapper(void* param) {
    struct { ThreadFunction func; void* arg; }* data = param;
    data->func(data->arg);
    free(data);
    return NULL;
}
#endif

ThreadHandle platform_create_thread(ThreadFunction func, void* arg) {
    struct { ThreadFunction func; void* arg; }* data = malloc(sizeof(*data));
    data->func = func;
    data->arg = arg;
    
#ifdef PLATFORM_WINDOWS
    return CreateThread(NULL, 0, thread_wrapper, data, 0, NULL);
#else
    pthread_t* thread = malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, thread_wrapper, data);
    return thread;
#endif
}

void platform_join_thread(ThreadHandle handle) {
#ifdef PLATFORM_WINDOWS
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
#else
    pthread_join(*(pthread_t*)handle, NULL);
    free(handle);
#endif
}

// ============================================================================
// MUTEXES
// ============================================================================

typedef void* MutexHandle;

MutexHandle platform_create_mutex() {
#ifdef PLATFORM_WINDOWS
    return CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, NULL);
    return mutex;
#endif
}

void platform_lock_mutex(MutexHandle handle) {
#ifdef PLATFORM_WINDOWS
    WaitForSingleObject(handle, INFINITE);
#else
    pthread_mutex_lock((pthread_mutex_t*)handle);
#endif
}

void platform_unlock_mutex(MutexHandle handle) {
#ifdef PLATFORM_WINDOWS
    ReleaseMutex(handle);
#else
    pthread_mutex_unlock((pthread_mutex_t*)handle);
#endif
}

void platform_destroy_mutex(MutexHandle handle) {
#ifdef PLATFORM_WINDOWS
    CloseHandle(handle);
#else
    pthread_mutex_destroy((pthread_mutex_t*)handle);
    free(handle);
#endif
}

// ============================================================================
// HIGH RESOLUTION TIMER
// ============================================================================

double platform_get_time() {
#ifdef PLATFORM_WINDOWS
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
#endif
}

void platform_sleep_ms(int milliseconds) {
#ifdef PLATFORM_WINDOWS
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// ============================================================================
// MEMORY INFO
// ============================================================================

size_t platform_get_total_memory() {
#ifdef PLATFORM_WINDOWS
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
#elif defined(PLATFORM_MACOS)
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    int64_t size = 0;
    size_t len = sizeof(size);
    sysctl(mib, 2, &size, &len, NULL, 0);
    return size;
#elif defined(PLATFORM_LINUX)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
#else
    return 0;
#endif
}

// PLATFORM BRIDGE COMPLETE - Full cross-platform abstraction layer
