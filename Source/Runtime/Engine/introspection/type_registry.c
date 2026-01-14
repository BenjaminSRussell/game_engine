#include "core/introspection/type_registry.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static TypeDefinition registry[MAX_REGISTERED_TYPES];
static int registry_count = 0;
static bool initialized = false;

// Buffer for simple JSON generation
static char json_buffer[65536]; 

void type_registry_init(void) {
    if (initialized) return;
    memset(registry, 0, sizeof(registry));
    registry_count = 0;
    initialized = true;
    
    // Register basic primitives
    type_registry_register("int", sizeof(int), TYPE_KIND_PRIMITIVE, "Standard integer");
    type_registry_register("float", sizeof(float), TYPE_KIND_PRIMITIVE, "32-bit floating point");
    type_registry_register("bool", sizeof(bool), TYPE_KIND_PRIMITIVE, "Boolean flag");
    type_registry_register("vec3", sizeof(float)*3, TYPE_KIND_STRUCT, "3D Vector");
}

int type_registry_register(const char* name, size_t size, TypeKind kind, const char* description) {
    if (registry_count >= MAX_REGISTERED_TYPES) return -1;
    
    // Check duplicate
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].name, name) == 0) return i;
    }
    
    int id = registry_count++;
    TypeDefinition* def = &registry[id];
    def->name = name; // Assumed static or managed by caller
    def->size = size;
    def->kind = kind;
    def->description = description;
    def->field_count = 0;
    
    return id;
}

void type_registry_add_field(int type_id, const char* field_name, const char* field_type_name, size_t offset, size_t size) {
    if (type_id < 0 || type_id >= registry_count) return;
    TypeDefinition* def = &registry[type_id];
    
    if (def->field_count >= MAX_TYPE_FIELDS) return;
    
    FieldMetadata* field = &def->fields[def->field_count++];
    field->name = field_name;
    field->type_name = field_type_name;
    field->offset = offset;
    field->size = size;
    field->is_array = false;
    field->array_count = 0;
}

const TypeDefinition* type_registry_get(const char* name) {
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].name, name) == 0) {
            return &registry[i];
        }
    }
    return NULL;
}

const TypeDefinition** type_registry_get_all(size_t* out_count) {
    static const TypeDefinition* ptrs[MAX_REGISTERED_TYPES];
    for (int i = 0; i < registry_count; i++) {
        ptrs[i] = &registry[i];
    }
    *out_count = registry_count;
    return ptrs;
}

const char* type_registry_generate_json_schema(const char* type_name) {
    const TypeDefinition* def = type_registry_get(type_name);
    if (!def) return "{}";
    
    // Simple schema generation
    // In a real implementation this would be recursive and more robust
    int offset = 0;
    offset += snprintf(json_buffer + offset, 65536 - offset, "{\n  \"type\": \"%s\",\n  \"description\": \"%s\",\n  \"properties\": {\n", def->name, def->description ? def->description : "");
    
    for (size_t i = 0; i < def->field_count; i++) {
        const FieldMetadata* field = &def->fields[i];
        const char* json_type = "object";
        
        // Map basic types to JSON types
        if (strcmp(field->type_name, "int") == 0) json_type = "integer";
        else if (strcmp(field->type_name, "float") == 0) json_type = "number";
        else if (strcmp(field->type_name, "bool") == 0) json_type = "boolean";
        
        offset += snprintf(json_buffer + offset, 65536 - offset, "    \"%s\": { \"type\": \"%s\" }%s\n", 
            field->name, json_type, (i < def->field_count - 1) ? "," : "");
    }
    
    offset += snprintf(json_buffer + offset, 65536 - offset, "  }\n}");
    return json_buffer;
}
