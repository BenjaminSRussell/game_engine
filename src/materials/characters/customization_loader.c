/**
 * =================================================================================================
 *                           CUSTOMIZATION OPTION LOADING
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of character customization option loading system
 * for managing character appearance variations and assets.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include "engine/include/core/logger.h"
#include <core/memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Customization option loading system
#define MAX_CUSTOMIZATION_OPTIONS 256
#define MAX_CUSTOMIZATION_CATEGORIES 16
#define MAX_OPTION_NAME_LENGTH 64
#define MAX_ICON_PATH_LENGTH 256
#define MAX_DESCRIPTION_LENGTH 512

typedef struct {
    CustomizationOption options[MAX_CUSTOMIZATION_OPTIONS];
    u32 option_count;
    u32 category_counts[MAX_CUSTOMIZATION_CATEGORIES];
    char category_names[MAX_CUSTOMIZATION_CATEGORIES][32];
    u32 category_count;
    bool system_initialized;
} CustomizationLoader;

static CustomizationLoader g_customization_loader = {0};

// Initialize customization loader system
bool customization_loader_init(void) {
    if (g_customization_loader.system_initialized) {
        return true;
    }
    
    memset(&g_customization_loader, 0, sizeof(CustomizationLoader));
    
    // Initialize default category names
    strcpy(g_customization_loader.category_names[0], "Body Type");
    strcpy(g_customization_loader.category_names[1], "Face Shape");
    strcpy(g_customization_loader.category_names[2], "Hair Style");
    strcpy(g_customization_loader.category_names[3], "Hair Color");
    strcpy(g_customization_loader.category_names[4], "Skin Tone");
    strcpy(g_customization_loader.category_names[5], "Eye Color");
    strcpy(g_customization_loader.category_names[6], "Facial Hair");
    strcpy(g_customization_loader.category_names[7], "Accessories");
    strcpy(g_customization_loader.category_names[8], "Tattoos");
    strcpy(g_customization_loader.category_names[9], "Scars");
    
    g_customization_loader.category_count = 10;
    g_customization_loader.system_initialized = true;
    
    LOG_INFO("Customization loader system initialized");
    return true;
}

// Load customization options from JSON file
static bool load_options_from_json(const char* file_path) {
    if (!file_path) {
        LOG_ERROR("Invalid file path for customization options");
        return false;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        LOG_ERROR("Failed to open customization file: %s", file_path);
        return false;
    }
    
    // Parse JSON file (simplified implementation)
    // In a real implementation, this would use a proper JSON parser
    char line[1024];
    u32 current_category = 0;
    bool in_options_array = false;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Parse category
        if (strstr(line, "\"category\"")) {
            char* start = strchr(line, ':');
            if (start) {
                start += 2; // Skip ": "
                char* end = strchr(start, '"');
                if (end) {
                    *end = '\0';
                    
                    // Find category index
                    for (u32 i = 0; i < g_customization_loader.category_count; i++) {
                        if (strcmp(start, g_customization_loader.category_names[i]) == 0) {
                            current_category = i;
                            break;
                        }
                    }
                }
            }
        }
        
        // Parse options array start
        if (strstr(line, "\"options\"") && strstr(line, "[")) {
            in_options_array = true;
            continue;
        }
        
        // Parse options array end
        if (strstr(line, "]") && in_options_array) {
            in_options_array = false;
            continue;
        }
        
        // Parse individual option
        if (in_options_array && strstr(line, "{")) {
            if (g_customization_loader.option_count >= MAX_CUSTOMIZATION_OPTIONS) {
                LOG_WARN("Maximum customization options reached");
                break;
            }
            
            CustomizationOption* option = &g_customization_loader.options[g_customization_loader.option_count];
            
            // Reset option
            memset(option, 0, sizeof(CustomizationOption));
            option->category = (CustomizationCategory)current_category;
            option->is_available = true;
            option->weight_factor = 1.0f;
            
            // Parse option properties (simplified)
            while (fgets(line, sizeof(line), file) && !strstr(line, "}")) {
                if (strstr(line, "\"name\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        start += 2; // Skip ": "
                        char* end = strchr(start, '"');
                        if (end) {
                            *end = '\0';
                            strncpy(option->name, start, MAX_OPTION_NAME_LENGTH - 1);
                        }
                    }
                }
                
                if (strstr(line, "\"icon_path\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        start += 2; // Skip ": "
                        char* end = strchr(start, '"');
                        if (end) {
                            *end = '\0';
                            strncpy(option->icon_path, start, MAX_ICON_PATH_LENGTH - 1);
                        }
                    }
                }
                
                if (strstr(line, "\"description\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        start += 2; // Skip ": "
                        char* end = strchr(start, '"');
                        if (end) {
                            *end = '\0';
                            strncpy(option->description, start, MAX_DESCRIPTION_LENGTH - 1);
                        }
                    }
                }
                
                if (strstr(line, "\"mesh_id\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->mesh_id = (u32)atoi(start + 1);
                    }
                }
                
                if (strstr(line, "\"material_id\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->material_id = (u32)atoi(start + 1);
                    }
                }
                
                if (strstr(line, "\"texture_id\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->texture_id = (u32)atoi(start + 1);
                    }
                }
                
                if (strstr(line, "\"weight_factor\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->weight_factor = atof(start + 1);
                    }
                }
                
                if (strstr(line, "\"is_premium\"")) {
                    option->is_premium = strstr(line, "true") != NULL;
                }
                
                if (strstr(line, "\"unlock_level\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->unlock_level = (u32)atoi(start + 1);
                    }
                }
                
                if (strstr(line, "\"price\"")) {
                    char* start = strchr(line, ':');
                    if (start) {
                        option->price = atof(start + 1);
                    }
                }
            }
            
            g_customization_loader.option_count++;
            g_customization_loader.category_counts[current_category]++;
        }
    }
    
    fclose(file);
    LOG_INFO("Loaded %u customization options from %s", g_customization_loader.option_count, file_path);
    return true;
}

// Load customization options from binary file
static bool load_options_from_binary(const char* file_path) {
    if (!file_path) {
        LOG_ERROR("Invalid file path for customization options");
        return false;
    }
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        LOG_ERROR("Failed to open customization file: %s", file_path);
        return false;
    }
    
    // Read header
    u32 magic;
    fread(&magic, sizeof(u32), 1, file);
    
    if (magic != 0x43555354) { // "CUST" magic number
        LOG_ERROR("Invalid customization file format");
        fclose(file);
        return false;
    }
    
    // Read version
    u32 version;
    fread(&version, sizeof(u32), 1, file);
    
    // Read category count
    fread(&g_customization_loader.category_count, sizeof(u32), 1, file);
    
    // Read category names
    for (u32 i = 0; i < g_customization_loader.category_count; i++) {
        fread(g_customization_loader.category_names[i], sizeof(char), 32, file);
    }
    
    // Read option count
    fread(&g_customization_loader.option_count, sizeof(u32), 1, file);
    
    // Read options
    for (u32 i = 0; i < g_customization_loader.option_count; i++) {
        CustomizationOption* option = &g_customization_loader.options[i];
        fread(option, sizeof(CustomizationOption), 1, file);
        
        // Update category counts
        if (option->category < MAX_CUSTOMIZATION_CATEGORIES) {
            g_customization_loader.category_counts[option->category]++;
        }
    }
    
    fclose(file);
    LOG_INFO("Loaded %u customization options from binary file %s", g_customization_loader.option_count, file_path);
    return true;
}

// Save customization options to binary file
static bool save_options_to_binary(const char* file_path) {
    if (!file_path) {
        LOG_ERROR("Invalid file path for saving customization options");
        return false;
    }
    
    FILE* file = fopen(file_path, "wb");
    if (!file) {
        LOG_ERROR("Failed to create customization file: %s", file_path);
        return false;
    }
    
    // Write header
    u32 magic = 0x43555354; // "CUST"
    fwrite(&magic, sizeof(u32), 1, file);
    
    // Write version
    u32 version = 1;
    fwrite(&version, sizeof(u32), 1, file);
    
    // Write category count
    fwrite(&g_customization_loader.category_count, sizeof(u32), 1, file);
    
    // Write category names
    for (u32 i = 0; i < g_customization_loader.category_count; i++) {
        fwrite(g_customization_loader.category_names[i], sizeof(char), 32, file);
    }
    
    // Write option count
    fwrite(&g_customization_loader.option_count, sizeof(u32), 1, file);
    
    // Write options
    for (u32 i = 0; i < g_customization_loader.option_count; i++) {
        fwrite(&g_customization_loader.options[i], sizeof(CustomizationOption), 1, file);
    }
    
    fclose(file);
    LOG_INFO("Saved %u customization options to binary file %s", g_customization_loader.option_count, file_path);
    return true;
}

// Public API functions
bool customization_load_options(const char* file_path) {
    if (!g_customization_loader.system_initialized) {
        if (!customization_loader_init()) {
            return false;
        }
    }
    
    // Determine file type by extension
    const char* extension = strrchr(file_path, '.');
    if (extension && strcmp(extension, ".json") == 0) {
        return load_options_from_json(file_path);
    } else {
        return load_options_from_binary(file_path);
    }
}

bool customization_save_options(const char* file_path) {
    if (!g_customization_loader.system_initialized) {
        LOG_ERROR("Customization loader not initialized");
        return false;
    }
    
    return save_options_to_binary(file_path);
}

u32 customization_get_option_count(void) {
    return g_customization_loader.option_count;
}

u32 customization_get_category_count(void) {
    return g_customization_loader.category_count;
}

u32 customization_get_category_option_count(CustomizationCategory category) {
    if (category >= MAX_CUSTOMIZATION_CATEGORIES) {
        return 0;
    }
    
    return g_customization_loader.category_counts[category];
}

CustomizationOption* customization_get_option_by_index(u32 index) {
    if (index >= g_customization_loader.option_count) {
        return NULL;
    }
    
    return &g_customization_loader.options[index];
}

CustomizationOption* customization_get_option_by_name(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (u32 i = 0; i < g_customization_loader.option_count; i++) {
        if (strcmp(g_customization_loader.options[i].name, name) == 0) {
            return &g_customization_loader.options[i];
        }
    }
    
    return NULL;
}

CustomizationOption* customization_get_option_by_category(CustomizationCategory category, u32 index) {
    if (category >= MAX_CUSTOMIZATION_CATEGORIES) {
        return NULL;
    }
    
    u32 current_index = 0;
    for (u32 i = 0; i < g_customization_loader.option_count; i++) {
        if (g_customization_loader.options[i].category == category) {
            if (current_index == index) {
                return &g_customization_loader.options[i];
            }
            current_index++;
        }
    }
    
    return NULL;
}

const char* customization_get_category_name(CustomizationCategory category) {
    if (category >= MAX_CUSTOMIZATION_CATEGORIES) {
        return "Unknown";
    }
    
    return g_customization_loader.category_names[category];
}

bool customization_add_option(const CustomizationOption* option) {
    if (!option || g_customization_loader.option_count >= MAX_CUSTOMIZATION_OPTIONS) {
        return false;
    }
    
    // Copy option
    g_customization_loader.options[g_customization_loader.option_count] = *option;
    
    // Update category count
    if (option->category < MAX_CUSTOMIZATION_CATEGORIES) {
        g_customization_loader.category_counts[option->category]++;
    }
    
    g_customization_loader.option_count++;
    
    LOG_DEBUG("Added customization option: %s", option->name);
    return true;
}

bool customization_remove_option(u32 index) {
    if (index >= g_customization_loader.option_count) {
        return false;
    }
    
    CustomizationCategory category = g_customization_loader.options[index].category;
    
    // Shift remaining options
    for (u32 i = index; i < g_customization_loader.option_count - 1; i++) {
        g_customization_loader.options[i] = g_customization_loader.options[i + 1];
    }
    
    // Update counts
    g_customization_loader.option_count--;
    if (category < MAX_CUSTOMIZATION_CATEGORIES) {
        g_customization_loader.category_counts[category]--;
    }
    
    LOG_DEBUG("Removed customization option at index %u", index);
    return true;
}

void customization_clear_all_options(void) {
    memset(&g_customization_loader, 0, sizeof(CustomizationLoader));
    g_customization_loader.system_initialized = true;
    
    // Reinitialize default categories
    strcpy(g_customization_loader.category_names[0], "Body Type");
    strcpy(g_customization_loader.category_names[1], "Face Shape");
    strcpy(g_customization_loader.category_names[2], "Hair Style");
    strcpy(g_customization_loader.category_names[3], "Hair Color");
    strcpy(g_customization_loader.category_names[4], "Skin Tone");
    strcpy(g_customization_loader.category_names[5], "Eye Color");
    strcpy(g_customization_loader.category_names[6], "Facial Hair");
    strcpy(g_customization_loader.category_names[7], "Accessories");
    strcpy(g_customization_loader.category_names[8], "Tattoos");
    strcpy(g_customization_loader.category_names[9], "Scars");
    
    g_customization_loader.category_count = 10;
    
    LOG_INFO("Cleared all customization options");
}

void customization_loader_cleanup(void) {
    memset(&g_customization_loader, 0, sizeof(CustomizationLoader));
    LOG_INFO("Customization loader system cleaned up");
}
