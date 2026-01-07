#include "materials/shaders/shader_cache.h"
#include <stdlib.h>
#include <string.h>

#define MAX_CACHE_ENTRIES 256

typedef struct cache_entry {
    char name[64];
    uint64_t hash;
    metal_shader_t* shader;
    bool active;
} cache_entry_t;

static cache_entry_t g_cache[MAX_CACHE_ENTRIES];
static bool g_initialized = false;

void shader_cache_init(void) {
    if (g_initialized) return;
    memset(g_cache, 0, sizeof(g_cache));
    g_initialized = true;
}

void shader_cache_shutdown(void) {
    // In a real system, we might release shaders here if we own them.
    // However, shader_system_t owns libraries, but metal_shader_t objects (wrappers) are often alloc'd.
    // For now, we just clear the cache references.
    memset(g_cache, 0, sizeof(g_cache));
    g_initialized = false;
}

metal_shader_t* shader_cache_get(const char* name, uint64_t inputs_hash) {
    if (!g_initialized) return NULL;
    
    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (g_cache[i].active && 
            g_cache[i].hash == inputs_hash && 
            strncmp(g_cache[i].name, name, 63) == 0) {
            return g_cache[i].shader;
        }
    }
    return NULL;
}

void shader_cache_put(const char* name, uint64_t inputs_hash, metal_shader_t* shader) {
    if (!g_initialized) shader_cache_init();
    
    // Find empty slot
    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (!g_cache[i].active) {
            strncpy(g_cache[i].name, name, 63);
            g_cache[i].hash = inputs_hash;
            g_cache[i].shader = shader; // We store the pointer, ownership remains with creator/caller most likely
            g_cache[i].active = true;
            return;
        }
    }
    // Eviction policy could go here
}

uint64_t shader_variant_hash(bool has_normal, bool has_emissive) {
    uint64_t hash = 0;
    if (has_normal) hash |= (1 << 0);
    if (has_emissive) hash |= (1 << 1);
    return hash;
}
