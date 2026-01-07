#include "shader_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

shader_system_t* shader_system_create(metal_device_t* device) {
    shader_system_t* sys = calloc(1, sizeof(shader_system_t));
    sys->device = device;
    sys->library_capacity = 16;
    sys->libraries = calloc(sys->library_capacity, sizeof(metal_shader_library_t));
    return sys;
}

void shader_system_destroy(shader_system_t* sys) {
    if (!sys) return;
    for (uint32_t i = 0; i < sys->library_count; i++) {
        // In Obj-C ARC, these would be released automatically or we'd need a release pool.
        // Assuming Manual Retain Release (MRR) or bridged cast context, but for strictly C/Metal interaction:
        // We technically can't "free" the id<MTLLibrary> from C directly without Obj-C runtime calls or bridging.
        // Since we are compiling as C/Obj-C, we assume ARC is handled or we rely on the OS.
        // Ideally, this file should be compiled as .m or have mixed source.
        sys->libraries[i].library = nil; 
    }
    free(sys->libraries);
    free(sys);
}

metal_shader_library_t* shader_load_library(shader_system_t* sys, const char* path) {
    if (sys->library_count >= sys->library_capacity) {
        // Simple expansion
        sys->library_capacity *= 2;
        sys->libraries = realloc(sys->libraries, sys->library_capacity * sizeof(metal_shader_library_t));
    }

    metal_shader_library_t* lib = &sys->libraries[sys->library_count];
    memset(lib, 0, sizeof(metal_shader_library_t));
    strncpy(lib->path, path, 255);

    NSError* error = nil;
    NSString* ns_path = [NSString stringWithUTF8String:path];
    NSURL* url = [NSURL fileURLWithPath:ns_path];
    
    // Check file stats for modification time
    struct stat st;
    if (stat(path, &st) == 0) {
        lib->last_modified = st.st_mtime;
    }

    lib->library = [sys->device->device newLibraryWithURL:url error:&error];

    if (error) {
        NSLog(@"Failed to load shader library at %s: %@", path, error);
        // Don't increment count or return invalid pointer
        return NULL;
    }
    
    if (!lib->library) {
        NSLog(@"Failed to load shader library at %s: Unknown error", path);
        return NULL;
    }

    sys->library_count++;
    return lib;
}

metal_shader_t* shader_get_function(metal_shader_library_t* lib, const char* name) {
    return shader_get_function_with_constants(lib, name, nil);
}

metal_shader_t* shader_get_function_with_constants(metal_shader_library_t* lib, const char* name, MTLFunctionConstantValues* constants) {
    if (!lib || !lib->library) return NULL;

    metal_shader_t* shader = calloc(1, sizeof(metal_shader_t));
    strncpy(shader->name, name, 63);

    NSError* error = nil;
    NSString* funcName = [NSString stringWithUTF8String:name];
    
    if (constants) {
        shader->function = [lib->library newFunctionWithName:funcName
                                              constantValues:constants
                                                       error:&error];
    } else {
        shader->function = [lib->library newFunctionWithName:funcName];
    }

    if (!shader->function) {
        NSLog(@"Failed to get function %s: %@", name, error ? error : @"Function not found");
        free(shader);
        return NULL;
    }

    shader->type = [shader->function functionType];
    return shader;
}

void shader_system_check_reload(shader_system_t* sys) {
    for (uint32_t i = 0; i < sys->library_count; i++) {
        metal_shader_library_t* lib = &sys->libraries[i];

        struct stat st;
        if (stat(lib->path, &st) == 0 && st.st_mtime > lib->last_modified) {
            // Reload library
            NSError* error = nil;
            NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:lib->path]];
            id<MTLLibrary> new_lib = [sys->device->device newLibraryWithURL:url error:&error];

            if (new_lib && !error) {
                lib->library = new_lib; // Replace the library
                lib->last_modified = st.st_mtime;
                NSLog(@"Reloaded shader library: %s", lib->path);
                
                // Note: Existing metal_shader_t instances pointing to functions from the old library 
                // will remain valid but won't be updated. The renderer needs to re-fetch functions 
                // or we need a way to notify/update them. 
                // For this task, we just reload the library reference.
            } else {
                 NSLog(@"Failed to hot-reload shader library %s: %@", lib->path, error);
            }
        }
    }
}
