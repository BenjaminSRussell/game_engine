/**
 * =================================================================================================
 *                          MACOS DYLIB HOT RELOAD
 * =================================================================================================
 *
 * Implements versioned dylib loading for live code updates on macOS.
 * Works around macOS file locking by loading versioned copies (game_v1.dylib,
 * game_v2.dylib).
 *
 * =================================================================================================
 */

#include <dlfcn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

// Hot reload state
typedef struct {
    void *handle;
    int version;
    char path[512];
    time_t lastModified;
    bool isValid;
} DylibInfo;

static DylibInfo g_currentDylib = {0};
static DylibInfo g_previousDylib = {0};
static int g_versionCounter = 0;
static bool g_reloadInProgress = false;
static void (*g_gameUpdateFunc)(float) = NULL;
static void (*g_gameInitFunc)(void) = NULL;
static void (*g_gameShutdownFunc)(void) = NULL;

// State serialization buffer
static void *g_serializedState = NULL;
static size_t g_stateSize = 0;

// Signal handling for crash recovery
static struct sigaction g_oldSigAction;
static bool g_crashDuringReload = false;

// Crash signal handler
void hot_reload_signal_handler(int sig, siginfo_t *info, void *context) {
    if (g_reloadInProgress) {
        g_crashDuringReload = true;
        printf("Crash detected during hot reload! Rolling back...\n");
        
        // Rollback to previous version
        if (g_previousDylib.handle && g_previousDylib.isValid) {
            // Switch back to previous dylib
            if (g_currentDylib.handle) {
                dlclose(g_currentDylib.handle);
            }
            g_currentDylib = g_previousDylib;
            
            // Restore function pointers
            g_gameUpdateFunc = dlsym(g_currentDylib.handle, "GameUpdate");
            g_gameInitFunc = dlsym(g_currentDylib.handle, "GameInit");
            g_gameShutdownFunc = dlsym(g_currentDylib.handle, "GameShutdown");
            
            printf("Rolled back to version %d\n", g_currentDylib.version);
        }
    }
    
    // Call original handler
    if (g_oldSigAction.sa_sigaction) {
        g_oldSigAction.sa_sigaction(sig, info, context);
    } else {
        exit(sig);
    }
}

// Initialize crash recovery
void hot_reload_init_crash_recovery() {
    struct sigaction action;
    action.sa_sigaction = hot_reload_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    
    sigaction(SIGSEGV, &action, &g_oldSigAction);
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGILL, &action, NULL);
}

// Copy and version dylib
bool hot_reload_copy_and_version_dylib(const char *sourcePath) {
    struct stat st;
    if (stat(sourcePath, &st) != 0) {
        printf("Failed to stat source dylib: %s\n", sourcePath);
        return false;
    }
    
    // Check if file is newer
    if (g_currentDylib.lastModified >= st.st_mtime) {
        return true; // No update needed
    }
    
    // Increment version
    g_versionCounter++;
    
    // Create versioned filename
    char versionedPath[512];
    snprintf(versionedPath, sizeof(versionedPath), "game_v%d.dylib", g_versionCounter);
    
    // Copy file
    FILE *src = fopen(sourcePath, "rb");
    if (!src) {
        printf("Failed to open source dylib: %s\n", sourcePath);
        return false;
    }
    
    FILE *dst = fopen(versionedPath, "wb");
    if (!dst) {
        fclose(src);
        printf("Failed to create versioned dylib: %s\n", versionedPath);
        return false;
    }
    
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytesRead, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    printf("Copied %s to %s (version %d)\n", sourcePath, versionedPath, g_versionCounter);
    return true;
}

// Safe dylib loading with validation
bool hot_reload_load_dylib_safe(const char *dylibPath) {
    // Load dylib
    void *handle = dlopen(dylibPath, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        printf("Failed to load dylib %s: %s\n", dylibPath, dlerror());
        return false;
    }
    
    // Validate required symbols
    void *updateFunc = dlsym(handle, "GameUpdate");
    void *initFunc = dlsym(handle, "GameInit");
    void *shutdownFunc = dlsym(handle, "GameShutdown");
    
    if (!updateFunc || !initFunc || !shutdownFunc) {
        printf("Missing required symbols in dylib %s\n", dylibPath);
        dlclose(handle);
        return false;
    }
    
    // Store previous dylib info
    if (g_currentDylib.handle) {
        g_previousDylib = g_currentDylib;
    }
    
    // Update current dylib info
    g_currentDylib.handle = handle;
    g_currentDylib.version = g_versionCounter;
    strncpy(g_currentDylib.path, dylibPath, sizeof(g_currentDylib.path) - 1);
    g_currentDylib.path[sizeof(g_currentDylib.path) - 1] = '\0';
    
    struct stat st;
    if (stat(dylibPath, &st) == 0) {
        g_currentDylib.lastModified = st.st_mtime;
    }
    g_currentDylib.isValid = true;
    
    // Update function pointers
    g_gameUpdateFunc = updateFunc;
    g_gameInitFunc = initFunc;
    g_gameShutdownFunc = shutdownFunc;
    
    printf("Successfully loaded dylib %s (version %d)\n", dylibPath, g_versionCounter);
    return true;
}

// Serialize game state before reload
bool hot_reload_serialize_state() {
    // This would call into the game to serialize its state
    // For now, allocate a dummy buffer
    if (g_serializedState) {
        free(g_serializedState);
    }
    
    g_stateSize = 1024 * 1024; // 1MB dummy state
    g_serializedState = malloc(g_stateSize);
    
    if (!g_serializedState) {
        printf("Failed to allocate state serialization buffer\n");
        return false;
    }
    
    // Zero out for now (would be filled by game)
    memset(g_serializedState, 0, g_stateSize);
    
    printf("Serialized %zu bytes of game state\n", g_stateSize);
    return true;
}

// Deserialize game state after reload
bool hot_reload_deserialize_state() {
    if (!g_serializedState || g_stateSize == 0) {
        printf("No serialized state available\n");
        return false;
    }
    
    // This would call into the game to deserialize its state
    // For now, just verify we have the data
    printf("Deserialized %zu bytes of game state\n", g_stateSize);
    
    // Free the serialized state
    free(g_serializedState);
    g_serializedState = NULL;
    g_stateSize = 0;
    
    return true;
}

// Cleanup previous version
void hot_reload_cleanup_previous() {
    if (g_previousDylib.handle) {
        printf("Cleaning up previous dylib version %d\n", g_previousDylib.version);
        
        // Call shutdown if available
        if (g_gameShutdownFunc) {
            g_gameShutdownFunc();
        }
        
        dlclose(g_previousDylib.handle);
        
        // Delete the old dylib file
        if (unlink(g_previousDylib.path) == 0) {
            printf("Deleted old dylib: %s\n", g_previousDylib.path);
        } else {
            printf("Failed to delete old dylib: %s (may be locked)\n", g_previousDylib.path);
        }
        
        memset(&g_previousDylib, 0, sizeof(g_previousDylib));
    }
}

// Main hot reload function
bool hot_reload_check_and_reload(const char *sourcePath) {
    if (g_reloadInProgress) {
        return false; // Already reloading
    }
    
    struct stat st;
    if (stat(sourcePath, &st) != 0) {
        return false;
    }
    
    // Check if file is newer
    if (g_currentDylib.lastModified >= st.st_mtime) {
        return true; // No update needed
    }
    
    printf("Detected new dylib version, initiating hot reload...\n");
    
    g_reloadInProgress = true;
    g_crashDuringReload = false;
    
    // Step 1: Copy and version the new dylib
    if (!hot_reload_copy_and_version_dylib(sourcePath)) {
        g_reloadInProgress = false;
        return false;
    }
    
    // Step 2: Serialize current state
    if (!hot_reload_serialize_state()) {
        g_reloadInProgress = false;
        return false;
    }
    
    // Step 3: Load the new dylib
    char versionedPath[512];
    snprintf(versionedPath, sizeof(versionedPath), "game_v%d.dylib", g_versionCounter);
    
    if (!hot_reload_load_dylib_safe(versionedPath)) {
        // Rollback on failure
        if (g_previousDylib.handle && g_previousDylib.isValid) {
            printf("Load failed, rolling back to previous version\n");
            g_currentDylib = g_previousDylib;
            
            // Restore function pointers
            g_gameUpdateFunc = dlsym(g_currentDylib.handle, "GameUpdate");
            g_gameInitFunc = dlsym(g_currentDylib.handle, "GameInit");
            g_gameShutdownFunc = dlsym(g_currentDylib.handle, "GameShutdown");
        }
        
        g_reloadInProgress = false;
        return false;
    }
    
    // Step 4: Initialize new version
    if (g_gameInitFunc) {
        g_gameInitFunc();
    }
    
    // Step 5: Deserialize state
    if (!hot_reload_deserialize_state()) {
        printf("Warning: Failed to deserialize state\n");
    }
    
    // Step 6: Cleanup previous version
    hot_reload_cleanup_previous();
    
    printf("Hot reload completed successfully (version %d)\n", g_versionCounter);
    g_reloadInProgress = false;
    
    return !g_crashDuringReload;
}

// Initialize hot reload system
void hot_reload_init(const char *initialDylib) {
    hot_reload_init_crash_recovery();
    
    // Load initial dylib
    if (!hot_reload_load_dylib_safe(initialDylib)) {
        printf("Failed to load initial dylib: %s\n", initialDylib);
        return;
    }
    
    // Initialize game
    if (g_gameInitFunc) {
        g_gameInitFunc();
    }
    
    printf("Hot reload system initialized with %s\n", initialDylib);
}

// Update function that can be called from main loop
void hot_reload_update(float deltaTime) {
    if (g_gameUpdateFunc && !g_reloadInProgress) {
        g_gameUpdateFunc(deltaTime);
    }
}

// Check for hot reload (call this periodically)
void hot_reload_check(const char *sourcePath) {
    hot_reload_check_and_reload(sourcePath);
}

// Cleanup
void hot_reload_shutdown() {
    if (g_gameShutdownFunc) {
        g_gameShutdownFunc();
    }
    
    if (g_currentDylib.handle) {
        dlclose(g_currentDylib.handle);
    }
    
    if (g_previousDylib.handle) {
        dlclose(g_previousDylib.handle);
    }
    
    if (g_serializedState) {
        free(g_serializedState);
    }
    
    // Cleanup old dylib files
    for (int i = 1; i <= g_versionCounter; i++) {
        char path[512];
        snprintf(path, sizeof(path), "game_v%d.dylib", i);
        unlink(path);
    }
    
    printf("Hot reload system shutdown\n");
}

// Symbol table patching implementation using mach_override
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <libkern/OSAtomic.h>

// Structure to track patched symbols
typedef struct SymbolPatch {
    char symbol_name[256];
    void *original_function;
    void *replacement_function;
    void *trampoline_function;
    bool is_active;
    struct SymbolPatch *next;
} SymbolPatch;

static SymbolPatch *g_patches = NULL;
static OSSpinLock g_patch_lock = OS_SPINLOCK_INIT;

// Error codes for symbol patching
typedef enum {
    PATCH_SUCCESS = 0,
    PATCH_SYMBOL_NOT_FOUND = -1,
    PATCH_ALREADY_PATCHED = -2,
    PATCH_PERMISSION_DENIED = -3,
    PATCH_INVALID_ADDRESS = -4,
    PATCH_ALLOCATION_FAILED = -5
} PatchError;

// Helper function to find symbol in loaded dylibs
static void* find_symbol_address(const char *symbol_name) {
    uint32_t image_count = _dyld_image_count();
    
    for (uint32_t i = 0; i < image_count; i++) {
        const struct mach_header *header = _dyld_get_image_header(i);
        if (!header) continue;
        
        const char *image_name = _dyld_get_image_name(i);
        if (!image_name) continue;
        
        // Skip system libraries for performance
        if (strstr(image_name, "/usr/lib/") || strstr(image_name, "/System/")) {
            continue;
        }
        
        // Try to find symbol in this image
        void *symbol_addr = dlsym(RTLD_DEFAULT, symbol_name);
        if (symbol_addr) {
            // Verify symbol is within this image's address space
            uintptr_t slide = _dyld_get_image_vmaddr_slide(i);
            uintptr_t image_base = (uintptr_t)header + slide;
            uintptr_t image_end = image_base + header->sizeofcmds;
            
            if ((uintptr_t)symbol_addr >= image_base && (uintptr_t)symbol_addr < image_end) {
                return symbol_addr;
            }
        }
    }
    
    return NULL;
}

// Helper function to make memory writable
static PatchError make_memory_writable(void *address, size_t size) {
    uintptr_t page_start = (uintptr_t)address & ~(uintptr_t)(sysconf(_SC_PAGESIZE) - 1);
    size_t page_size = sysconf(_SC_PAGESIZE);
    
    int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    if (mprotect((void*)page_start, page_size, prot) != 0) {
        return PATCH_PERMISSION_DENIED;
    }
    
    return PATCH_SUCCESS;
}

// Helper function to restore memory protection
static PatchError restore_memory_protection(void *address, size_t size) {
    uintptr_t page_start = (uintptr_t)address & ~(uintptr_t)(sysconf(_SC_PAGESIZE) - 1);
    size_t page_size = sysconf(_SC_PAGESIZE);
    
    int prot = PROT_READ | PROT_EXEC;
    if (mprotect((void*)page_start, page_size, prot) != 0) {
        return PATCH_PERMISSION_DENIED;
    }
    
    return PATCH_SUCCESS;
}

// Simple trampoline generator for x86_64
static PatchError generate_trampoline(void *original_func, void **trampoline_out) {
    const size_t trampoline_size = 32; // Enough for our jump instruction
    void *trampoline = mmap(NULL, trampoline_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (!trampoline) {
        return PATCH_ALLOCATION_FAILED;
    }
    
    uint8_t *code = (uint8_t*)trampoline;
    
    // Generate trampoline: jump to original function
    // mov rax, original_func (10 bytes)
    code[0] = 0x48; code[1] = 0xB8;
    memcpy(&code[2], &original_func, sizeof(void*));
    
    // jmp rax (2 bytes)
    code[10] = 0xFF; code[11] = 0xE0;
    
    *trampoline_out = trampoline;
    return PATCH_SUCCESS;
}

// Patch a function with jump to replacement
static PatchError patch_function_jump(void *target_func, void *replacement_func) {
    PatchError err = make_memory_writable(target_func, 12);
    if (err != PATCH_SUCCESS) {
        return err;
    }
    
    uint8_t *code = (uint8_t*)target_func;
    
    // Generate jump instruction: jmp replacement_func
    // mov rax, replacement_func (10 bytes)
    code[0] = 0x48; code[1] = 0xB8;
    memcpy(&code[2], &replacement_func, sizeof(void*));
    
    // jmp rax (2 bytes)
    code[10] = 0xFF; code[11] = 0xE0;
    
    err = restore_memory_protection(target_func, 12);
    return err;
}

// Find existing patch entry
static SymbolPatch* find_patch(const char *symbol_name) {
    SymbolPatch *current = g_patches;
    while (current) {
        if (strcmp(current->symbol_name, symbol_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Add new patch entry
static SymbolPatch* add_patch(const char *symbol_name) {
    SymbolPatch *patch = calloc(1, sizeof(SymbolPatch));
    if (!patch) {
        return NULL;
    }
    
    strncpy(patch->symbol_name, symbol_name, sizeof(patch->symbol_name) - 1);
    
    OSSpinLockLock(&g_patch_lock);
    patch->next = g_patches;
    g_patches = patch;
    OSSpinLockUnlock(&g_patch_lock);
    
    return patch;
}

// Public API functions

PatchError symbol_table_patch(const char *symbol_name, void *replacement_func) {
    if (!symbol_name || !replacement_func) {
        return PATCH_INVALID_ADDRESS;
    }
    
    OSSpinLockLock(&g_patch_lock);
    
    // Check if already patched
    SymbolPatch *existing = find_patch(symbol_name);
    if (existing && existing->is_active) {
        OSSpinLockUnlock(&g_patch_lock);
        return PATCH_ALREADY_PATCHED;
    }
    
    // Find symbol address
    void *symbol_addr = find_symbol_address(symbol_name);
    if (!symbol_addr) {
        OSSpinLockUnlock(&g_patch_lock);
        return PATCH_SYMBOL_NOT_FOUND;
    }
    
    // Create patch entry if needed
    SymbolPatch *patch = existing;
    if (!patch) {
        patch = add_patch(symbol_name);
        if (!patch) {
            OSSpinLockUnlock(&g_patch_lock);
            return PATCH_ALLOCATION_FAILED;
        }
    }
    
    // Generate trampoline to original function
    PatchError err = generate_trampoline(symbol_addr, &patch->trampoline_function);
    if (err != PATCH_SUCCESS) {
        OSSpinLockUnlock(&g_patch_lock);
        return err;
    }
    
    // Patch the function
    err = patch_function_jump(symbol_addr, replacement_func);
    if (err != PATCH_SUCCESS) {
        if (patch->trampoline_function) {
            munmap(patch->trampoline_function, 32);
            patch->trampoline_function = NULL;
        }
        OSSpinLockUnlock(&g_patch_lock);
        return err;
    }
    
    patch->original_function = symbol_addr;
    patch->replacement_function = replacement_func;
    patch->is_active = true;
    
    OSSpinLockUnlock(&g_patch_lock);
    
    // Flush instruction cache
    __builtin___clear_cache((char*)symbol_addr, (char*)symbol_addr + 12);
    
    return PATCH_SUCCESS;
}

PatchError symbol_table_unpatch(const char *symbol_name) {
    if (!symbol_name) {
        return PATCH_INVALID_ADDRESS;
    }
    
    OSSpinLockLock(&g_patch_lock);
    
    SymbolPatch *patch = find_patch(symbol_name);
    if (!patch || !patch->is_active) {
        OSSpinLockUnlock(&g_patch_lock);
        return PATCH_SYMBOL_NOT_FOUND;
    }
    
    // Restore original function using trampoline
    PatchError err = patch_function_jump(patch->original_function, patch->trampoline_function);
    if (err != PATCH_SUCCESS) {
        OSSpinLockUnlock(&g_patch_lock);
        return err;
    }
    
    patch->is_active = false;
    
    OSSpinLockUnlock(&g_patch_lock);
    
    // Flush instruction cache
    __builtin___clear_cache((char*)patch->original_function, 
                          (char*)patch->original_function + 12);
    
    return PATCH_SUCCESS;
}

void* symbol_table_get_original(const char *symbol_name) {
    if (!symbol_name) {
        return NULL;
    }
    
    OSSpinLockLock(&g_patch_lock);
    SymbolPatch *patch = find_patch(symbol_name);
    void *original = patch ? patch->trampoline_function : NULL;
    OSSpinLockUnlock(&g_patch_lock);
    
    return original;
}

bool symbol_table_is_patched(const char *symbol_name) {
    if (!symbol_name) {
        return false;
    }
    
    OSSpinLockLock(&g_patch_lock);
    SymbolPatch *patch = find_patch(symbol_name);
    bool is_patched = patch ? patch->is_active : false;
    OSSpinLockUnlock(&g_patch_lock);
    
    return is_patched;
}

void symbol_table_cleanup(void) {
    OSSpinLockLock(&g_patch_lock);
    
    SymbolPatch *current = g_patches;
    while (current) {
        SymbolPatch *next = current->next;
        
        if (current->trampoline_function) {
            munmap(current->trampoline_function, 32);
        }
        
        free(current);
        current = next;
    }
    
    g_patches = NULL;
    
    OSSpinLockUnlock(&g_patch_lock);
}

// Get error description
const char* symbol_table_get_error_string(PatchError error) {
    switch (error) {
    case PATCH_SUCCESS:
        return "Success";
    case PATCH_SYMBOL_NOT_FOUND:
        return "Symbol not found";
    case PATCH_ALREADY_PATCHED:
        return "Symbol already patched";
    case PATCH_PERMISSION_DENIED:
        return "Permission denied (memory protection)";
    case PATCH_INVALID_ADDRESS:
        return "Invalid address";
    case PATCH_ALLOCATION_FAILED:
        return "Memory allocation failed";
    default:
        return "Unknown error";
    }
}
