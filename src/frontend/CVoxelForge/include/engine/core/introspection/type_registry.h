#ifndef ENGINE_INTROSPECTION_TYPE_REGISTRY_H
#define ENGINE_INTROSPECTION_TYPE_REGISTRY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Support for up to 1024 fields per type and 4096 types total
#define MAX_TYPE_FIELDS 1024
#define MAX_REGISTERED_TYPES 4096

typedef enum {
    TYPE_KIND_PRIMITIVE,
    TYPE_KIND_STRUCT,
    TYPE_KIND_ENUM,
    TYPE_KIND_POINTER,
    TYPE_KIND_ARRAY
} TypeKind;

typedef struct {
    const char* name;
    const char* type_name;  // Reference to another registered type
    size_t offset;
    size_t size;
    bool is_array;
    size_t array_count;     // 0 if dynamic array pointer
} FieldMetadata;

typedef struct {
    const char* name;
    TypeKind kind;
    size_t size;
    size_t alignment;
    
    FieldMetadata fields[MAX_TYPE_FIELDS];
    size_t field_count;
    
    const char* description; // Human/AI readable description
    const char* schema_json; // Cached JSON schema
} TypeDefinition;

// -- Registry API --

// Initialize the registry
void type_registry_init(void);

// Register a new type
// Returns type ID (index) or -1 on failure
int type_registry_register(const char* name, size_t size, TypeKind kind, const char* description);

// Add a field to a struct type
void type_registry_add_field(int type_id, const char* field_name, const char* field_type_name, size_t offset, size_t size);

// Get type by name
const TypeDefinition* type_registry_get(const char* name);

// Get all types (for AI discovery)
// returns array of pointers
const TypeDefinition** type_registry_get_all(size_t* out_count);

// Generate JSON schema for a type
// Returns pointer to internal buffer (valid until next call)
const char* type_registry_generate_json_schema(const char* type_name);

#endif // ENGINE_INTROSPECTION_TYPE_REGISTRY_H
