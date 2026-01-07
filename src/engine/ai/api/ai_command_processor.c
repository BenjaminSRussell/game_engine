#include "ai_api.h"
#include "../../core/introspection/type_registry.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Simple JSON parser helpers (placeholder for full parser)
// In a real implementation, use a robust JSON library like cJSON or the engine's internal one
static bool get_json_string(const char* json, const char* key, char* out_buffer, size_t max_len) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char* pos = strstr(json, search_key);
    if (!pos) return false;
    
    const char* val_start = strchr(pos, ':');
    if (!val_start) return false;
    
    // Skip whitespace and quote
    val_start++;
    while (*val_start == ' ' || *val_start == '\"') val_start++;
    
    // Copy until next quote
    size_t i = 0;
    while (val_start[i] != '\"' && val_start[i] != '\0' && i < max_len - 1) {
        out_buffer[i] = val_start[i];
        i++;
    }
    out_buffer[i] = '\0';
    return true;
}

void ai_api_init(void) {
    type_registry_init();
}

static void handle_query_types(char* response, size_t size) {
    size_t count = 0;
    const TypeDefinition** types = type_registry_get_all(&count);
    
    // Use a simpler approach to avoid buffer overflow logic complexity in this snippet
    int offset = snprintf(response, size, "{\"status\": \"ok\", \"data\": [");
    if (offset < 0) return; 
    
    for (size_t i = 0; i < count; i++) {
        if ((size_t)offset >= size - 10) break; // Safety margin
        
        int written = snprintf(response + offset, size - offset, "\"%s\"%s", 
            types[i]->name, (i < count - 1) ? ", " : "");
            
        if (written < 0) break;
        offset += written;
    }
    
    if ((size_t)offset < size - 2) {
        strcat(response, "]}");
    }
}

static void handle_get_schema(const char* json_cmd, char* response, size_t size) {
    char type_name[128];
    if (get_json_string(json_cmd, "type_name", type_name, sizeof(type_name))) {
        const char* schema = type_registry_generate_json_schema(type_name);
        // Note: In real production code, properly escape the nested JSON string
        snprintf(response, size, "{\"status\": \"ok\", \"data\": %s}", schema);
    } else {
        snprintf(response, size, "{\"status\": \"error\", \"message\": \"Missing parameter: type_name\"}");
    }
}

int ai_api_execute_command(const char* json_command, char* json_response, size_t response_size) {
    char command[128];
    if (!get_json_string(json_command, "command", command, sizeof(command))) {
        snprintf(json_response, response_size, "{\"status\": \"error\", \"message\": \"Invalid JSON: missing 'command' field\"}");
        return -1;
    }
    
    if (strcmp(command, "query_types") == 0) {
        handle_query_types(json_response, response_size);
    } else if (strcmp(command, "get_schema") == 0) {
        handle_get_schema(json_command, json_response, response_size);
    } else {
        snprintf(json_response, response_size, "{\"status\": \"error\", \"message\": \"Unknown command: %s\"}", command);
        return -1;
    }
    
    return 0;
}
