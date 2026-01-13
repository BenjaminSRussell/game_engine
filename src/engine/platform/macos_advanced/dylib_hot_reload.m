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

// TODO(AGENT_MACOS_2): Implement dylib copy and versioning
//   - Detect new build (game.dylib.new)
//   - Copy to game_v{N}.dylib
//   - Increment version counter
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Implement safe dylib loading
//   - dlopen with RTLD_NOW | RTLD_LOCAL
//   - Validate required symbols exist (GameUpdate, GameInit)
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create state serialization for reload
//   - Serialize game memory/state before unload
//   - Deserialize into new instance after load
//   - Maintain pointer fixups if necessary
//   - Difficulty: 7

// TODO(AGENT_MACOS_2): Implement previous version cleanup
//   - Delete old dylib versions (game_v{N-1}.dylib)
//   - Handle failure to delete (if still locked)
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create crash recovery safety net
//   - Catch signals during reload
//   - Rollback to previous known good dylib if init fails
//   - Difficulty: 6

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
