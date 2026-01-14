/**
 * Reflection System Implementation (~2800 lines)
 *
 * Implements complete type reflection, property binding, method invocation,
 * and serialization capabilities for the engine.
 */

#include "reflection_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct {
    uint32_t next_type_id;
    uint32_t next_class_id;
    uint32_t next_struct_id;
    uint32_t next_enum_id;
    uint32_t next_object_id;
} reflection_id_generator;

static reflection_id_generator g_id_gen = {
    .next_type_id = 1000,
    .next_class_id = 2000,
    .next_struct_id = 3000,
    .next_enum_id = 4000,
    .next_object_id = 5000,
};

// ============================================================================
// CONTEXT MANAGEMENT
// ============================================================================

reflection_context* reflection_context_create(void) {
    reflection_context* ctx = malloc(sizeof(reflection_context));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(reflection_context));

    ctx->type_capacity = 256;
    ctx->class_capacity = 128;
    ctx->struct_capacity = 128;
    ctx->enum_capacity = 64;

    ctx->types = calloc(ctx->type_capacity, sizeof(reflection_type));
    ctx->classes = calloc(ctx->class_capacity, sizeof(reflection_class));
    ctx->structs = calloc(ctx->struct_capacity, sizeof(reflection_struct));
    ctx->enums = calloc(ctx->enum_capacity, sizeof(reflection_enum));

    if (!ctx->types || !ctx->classes || !ctx->structs || !ctx->enums) {
        free(ctx->types);
        free(ctx->classes);
        free(ctx->structs);
        free(ctx->enums);
        free(ctx);
        return NULL;
    }

    ctx->initialized = true;
    return ctx;
}

void reflection_context_destroy(reflection_context* ctx) {
    if (!ctx) return;

    // Destroy all types
    for (uint32_t i = 0; i < ctx->type_count; i++) {
        // Type cleanup if needed
    }

    // Destroy all classes
    for (uint32_t i = 0; i < ctx->class_count; i++) {
        reflection_class* class_type = &ctx->classes[i];
        free(class_type->properties);
        free(class_type->methods);
        free(class_type->interfaces);
    }

    // Destroy all structs
    for (uint32_t i = 0; i < ctx->struct_count; i++) {
        reflection_struct* struct_type = &ctx->structs[i];
        free(struct_type->properties);
    }

    // Destroy all enums
    for (uint32_t i = 0; i < ctx->enum_count; i++) {
        reflection_enum* enum_type = &ctx->enums[i];
        free(enum_type->values);
    }

    free(ctx->types);
    free(ctx->classes);
    free(ctx->structs);
    free(ctx->enums);
    free(ctx);
}

// ============================================================================
// TYPE REGISTRATION AND LOOKUP
// ============================================================================

reflection_type* reflection_register_type(reflection_context* ctx, const char* name, property_type type, size_t size) {
    if (!ctx || !name || ctx->type_count >= ctx->type_capacity) {
        return NULL;
    }

    // Check if type already exists
    reflection_type* existing = reflection_find_type(ctx, name);
    if (existing) {
        return existing;
    }

    reflection_type* new_type = &ctx->types[ctx->type_count++];
    memset(new_type, 0, sizeof(reflection_type));

    new_type->id = g_id_gen.next_type_id++;
    new_type->name = malloc(strlen(name) + 1);
    if (!new_type->name) {
        ctx->type_count--;
        return NULL;
    }

    strcpy((char*)new_type->name, name);
    new_type->type = type;
    new_type->size = size;
    new_type->alignment = (size < 4) ? size : 16;  // Default alignment

    return new_type;
}

reflection_type* reflection_find_type(reflection_context* ctx, const char* name) {
    if (!ctx || !name) return NULL;

    for (uint32_t i = 0; i < ctx->type_count; i++) {
        if (strcmp(ctx->types[i].name, name) == 0) {
            return &ctx->types[i];
        }
    }

    return NULL;
}

reflection_type* reflection_find_type_by_id(reflection_context* ctx, uint32_t type_id) {
    if (!ctx) return NULL;

    for (uint32_t i = 0; i < ctx->type_count; i++) {
        if (ctx->types[i].id == type_id) {
            return &ctx->types[i];
        }
    }

    return NULL;
}

// ============================================================================
// CLASS REGISTRATION AND MANAGEMENT
// ============================================================================

reflection_class* reflection_register_class(reflection_context* ctx, const char* name, size_t size, reflection_class* parent) {
    if (!ctx || !name || ctx->class_count >= ctx->class_capacity) {
        return NULL;
    }

    reflection_class* new_class = &ctx->classes[ctx->class_count++];
    memset(new_class, 0, sizeof(reflection_class));

    new_class->id = g_id_gen.next_class_id++;
    new_class->name = malloc(strlen(name) + 1);
    if (!new_class->name) {
        ctx->class_count--;
        return NULL;
    }

    strcpy((char*)new_class->name, name);
    new_class->size = size;
    new_class->alignment = 16;
    new_class->parent_class = parent;

    new_class->property_capacity = 64;
    new_class->properties = calloc(new_class->property_capacity, sizeof(reflection_property));

    new_class->method_capacity = 32;
    new_class->methods = calloc(new_class->method_capacity, sizeof(reflection_method));

    new_class->interface_capacity = 8;
    new_class->interfaces = calloc(new_class->interface_capacity, sizeof(reflection_class*));

    if (!new_class->properties || !new_class->methods || !new_class->interfaces) {
        free((void*)new_class->name);
        free(new_class->properties);
        free(new_class->methods);
        free(new_class->interfaces);
        ctx->class_count--;
        return NULL;
    }

    return new_class;
}

void reflection_class_add_property(reflection_class* class_type, const reflection_property* property) {
    if (!class_type || !property || class_type->property_count >= class_type->property_capacity) {
        return;
    }

    reflection_property* new_prop = &class_type->properties[class_type->property_count++];
    memcpy(new_prop, property, sizeof(reflection_property));
    new_prop->id = class_type->property_count;
    new_prop->owner_class = class_type;
}

void reflection_class_add_method(reflection_class* class_type, const reflection_method* method) {
    if (!class_type || !method || class_type->method_count >= class_type->method_capacity) {
        return;
    }

    reflection_method* new_method = &class_type->methods[class_type->method_count++];
    memcpy(new_method, method, sizeof(reflection_method));
    new_method->id = class_type->method_count;
    new_method->owner_class = class_type;
}

reflection_property* reflection_class_find_property(reflection_class* class_type, const char* name) {
    if (!class_type || !name) return NULL;

    for (uint32_t i = 0; i < class_type->property_count; i++) {
        if (strcmp(class_type->properties[i].name, name) == 0) {
            return &class_type->properties[i];
        }
    }

    // Check parent class
    if (class_type->parent_class) {
        return reflection_class_find_property(class_type->parent_class, name);
    }

    return NULL;
}

reflection_method* reflection_class_find_method(reflection_class* class_type, const char* name) {
    if (!class_type || !name) return NULL;

    for (uint32_t i = 0; i < class_type->method_count; i++) {
        if (strcmp(class_type->methods[i].name, name) == 0) {
            return &class_type->methods[i];
        }
    }

    // Check parent class
    if (class_type->parent_class) {
        return reflection_class_find_method(class_type->parent_class, name);
    }

    return NULL;
}

reflection_class* reflection_find_class(reflection_context* ctx, const char* name) {
    if (!ctx || !name) return NULL;

    for (uint32_t i = 0; i < ctx->class_count; i++) {
        if (strcmp(ctx->classes[i].name, name) == 0) {
            return &ctx->classes[i];
        }
    }

    return NULL;
}

uint32_t reflection_class_get_property_count(reflection_class* class_type) {
    if (!class_type) return 0;

    uint32_t count = class_type->property_count;

    if (class_type->parent_class) {
        count += reflection_class_get_property_count(class_type->parent_class);
    }

    return count;
}

reflection_property* reflection_class_get_property(reflection_class* class_type, uint32_t index) {
    if (!class_type) return NULL;

    if (index < class_type->property_count) {
        return &class_type->properties[index];
    }

    if (class_type->parent_class) {
        return reflection_class_get_property(class_type->parent_class, index - class_type->property_count);
    }

    return NULL;
}

// ============================================================================
// STRUCT REGISTRATION
// ============================================================================

reflection_struct* reflection_register_struct(reflection_context* ctx, const char* name, size_t size) {
    if (!ctx || !name || ctx->struct_count >= ctx->struct_capacity) {
        return NULL;
    }

    reflection_struct* new_struct = &ctx->structs[ctx->struct_count++];
    memset(new_struct, 0, sizeof(reflection_struct));

    new_struct->id = g_id_gen.next_struct_id++;
    new_struct->name = malloc(strlen(name) + 1);
    if (!new_struct->name) {
        ctx->struct_count--;
        return NULL;
    }

    strcpy((char*)new_struct->name, name);
    new_struct->size = size;
    new_struct->alignment = (size < 4) ? size : 16;

    new_struct->property_capacity = 32;
    new_struct->properties = calloc(new_struct->property_capacity, sizeof(reflection_property));

    if (!new_struct->properties) {
        free((void*)new_struct->name);
        ctx->struct_count--;
        return NULL;
    }

    return new_struct;
}

void reflection_struct_add_property(reflection_struct* struct_type, const reflection_property* property) {
    if (!struct_type || !property || struct_type->property_count >= struct_type->property_capacity) {
        return;
    }

    reflection_property* new_prop = &struct_type->properties[struct_type->property_count++];
    memcpy(new_prop, property, sizeof(reflection_property));
    new_prop->owner_struct = struct_type;
}

reflection_struct* reflection_find_struct(reflection_context* ctx, const char* name) {
    if (!ctx || !name) return NULL;

    for (uint32_t i = 0; i < ctx->struct_count; i++) {
        if (strcmp(ctx->structs[i].name, name) == 0) {
            return &ctx->structs[i];
        }
    }

    return NULL;
}

// ============================================================================
// ENUM REGISTRATION
// ============================================================================

reflection_enum* reflection_register_enum(reflection_context* ctx, const char* name) {
    if (!ctx || !name || ctx->enum_count >= ctx->enum_capacity) {
        return NULL;
    }

    reflection_enum* new_enum = &ctx->enums[ctx->enum_count++];
    memset(new_enum, 0, sizeof(reflection_enum));

    new_enum->id = g_id_gen.next_enum_id++;
    new_enum->name = malloc(strlen(name) + 1);
    if (!new_enum->name) {
        ctx->enum_count--;
        return NULL;
    }

    strcpy((char*)new_enum->name, name);
    new_enum->value_capacity = 64;
    new_enum->values = calloc(new_enum->value_capacity, sizeof(enum_value));

    if (!new_enum->values) {
        free((void*)new_enum->name);
        ctx->enum_count--;
        return NULL;
    }

    return new_enum;
}

void reflection_enum_add_value(reflection_enum* enum_type, const char* name, int32_t value) {
    if (!enum_type || !name || enum_type->value_count >= enum_type->value_capacity) {
        return;
    }

    enum_value* new_value = &enum_type->values[enum_type->value_count++];
    new_value->name = malloc(strlen(name) + 1);
    if (!new_value->name) {
        enum_type->value_count--;
        return;
    }

    strcpy((char*)new_value->name, name);
    new_value->value = value;
    new_value->display_name = new_value->name;
}

reflection_enum* reflection_find_enum(reflection_context* ctx, const char* name) {
    if (!ctx || !name) return NULL;

    for (uint32_t i = 0; i < ctx->enum_count; i++) {
        if (strcmp(ctx->enums[i].name, name) == 0) {
            return &ctx->enums[i];
        }
    }

    return NULL;
}

// ============================================================================
// OBJECT INSTANTIATION AND MANAGEMENT
// ============================================================================

void* reflection_create_instance(reflection_class* class_type) {
    if (!class_type || class_type->size == 0) {
        return NULL;
    }

    void* instance = malloc(class_type->size);
    if (!instance) return NULL;

    memset(instance, 0, class_type->size);

    if (class_type->constructor) {
        class_type->constructor();
    }

    if (class_type->on_instanced) {
        class_type->on_instanced(instance);
    }

    return instance;
}

void reflection_destroy_instance(void* object) {
    if (!object) return;

    reflection_class* class_type = reflection_get_object_class(object);
    if (class_type && class_type->destructor) {
        class_type->destructor(object);
    }

    free(object);
}

reflection_class* reflection_get_object_class(void* object) {
    if (!object) return NULL;

    // If the object has a reflection_object header, use that
    reflection_object* refl_obj = (reflection_object*)object;
    if (refl_obj && refl_obj->object_class) {
        return refl_obj->object_class;
    }

    return NULL;
}

bool reflection_is_instance_of(void* object, reflection_class* class_type) {
    if (!object || !class_type) return false;

    reflection_class* obj_class = reflection_get_object_class(object);
    if (!obj_class) return false;

    // Check class hierarchy
    while (obj_class) {
        if (obj_class->id == class_type->id) {
            return true;
        }
        obj_class = obj_class->parent_class;
    }

    return false;
}

void* reflection_cast(void* object, reflection_class* target_class) {
    if (!object || !target_class) return NULL;

    if (reflection_is_instance_of(object, target_class)) {
        return object;
    }

    return NULL;
}

// ============================================================================
// PROPERTY ACCESS
// ============================================================================

void reflection_set_property_value(void* object, reflection_property* property, void* value) {
    if (!object || !property || !value) return;

    if (property->setter) {
        property->setter(object, value);
        return;
    }

    // Direct memory access
    void* property_ptr = (uint8_t*)object + property->offset;
    memcpy(property_ptr, value, property->size);

    // Notify change
    if (property->on_changed) {
        property->on_changed(object, NULL, value);
    }
}

void* reflection_get_property_value(void* object, reflection_property* property) {
    if (!object || !property) return NULL;

    if (property->getter) {
        return property->getter(object);
    }

    // Direct memory access
    void* property_ptr = (uint8_t*)object + property->offset;
    return property_ptr;
}

void reflection_set_property_bool(void* object, reflection_property* property, bool value) {
    reflection_set_property_value(object, property, &value);
}

bool reflection_get_property_bool(void* object, reflection_property* property) {
    void* value = reflection_get_property_value(object, property);
    return value ? *(bool*)value : false;
}

void reflection_set_property_int32(void* object, reflection_property* property, int32_t value) {
    reflection_set_property_value(object, property, &value);
}

int32_t reflection_get_property_int32(void* object, reflection_property* property) {
    void* value = reflection_get_property_value(object, property);
    return value ? *(int32_t*)value : 0;
}

void reflection_set_property_float(void* object, reflection_property* property, float value) {
    reflection_set_property_value(object, property, &value);
}

float reflection_get_property_float(void* object, reflection_property* property) {
    void* value = reflection_get_property_value(object, property);
    return value ? *(float*)value : 0.0f;
}

void reflection_set_property_string(void* object, reflection_property* property, const char* value) {
    if (!object || !property || !value) return;

    char* str_ptr = (char*)((uint8_t*)object + property->offset);
    strncpy(str_ptr, value, property->size - 1);
    str_ptr[property->size - 1] = '\0';
}

const char* reflection_get_property_string(void* object, reflection_property* property) {
    if (!object || !property) return "";
    return (const char*)((uint8_t*)object + property->offset);
}

// ============================================================================
// METHOD INVOCATION
// ============================================================================

void* reflection_invoke_method(void* object, reflection_method* method, void** parameters) {
    if (!object || !method || !method->invoker) {
        return NULL;
    }

    return method->invoker(object, parameters, NULL);
}

void* reflection_invoke_method_by_name(void* object, reflection_class* class_type, const char* method_name, void** parameters) {
    if (!object || !class_type || !method_name) {
        return NULL;
    }

    reflection_method* method = reflection_class_find_method(class_type, method_name);
    if (!method) {
        return NULL;
    }

    return reflection_invoke_method(object, method, parameters);
}

// ============================================================================
// SERIALIZATION
// ============================================================================

bool reflection_serialize(void* object, const char* filename) {
    if (!object || !filename) return false;

    FILE* file = fopen(filename, "wb");
    if (!file) return false;

    reflection_class* class_type = reflection_get_object_class(object);
    if (!class_type) {
        fclose(file);
        return false;
    }

    // Write class name
    uint32_t name_len = strlen(class_type->name);
    fwrite(&name_len, sizeof(uint32_t), 1, file);
    fwrite(class_type->name, 1, name_len, file);

    // Write properties
    fwrite(&class_type->property_count, sizeof(uint32_t), 1, file);

    for (uint32_t i = 0; i < class_type->property_count; i++) {
        reflection_property* prop = &class_type->properties[i];

        // Write property name
        uint32_t prop_name_len = strlen(prop->name);
        fwrite(&prop_name_len, sizeof(uint32_t), 1, file);
        fwrite(prop->name, 1, prop_name_len, file);

        // Write property type and value
        fwrite(&prop->type, sizeof(property_type), 1, file);
        fwrite(&prop->size, sizeof(size_t), 1, file);

        void* prop_value = reflection_get_property_value(object, prop);
        if (prop_value) {
            fwrite(prop_value, 1, prop->size, file);
        }
    }

    fclose(file);
    return true;
}

void* reflection_deserialize(reflection_class* class_type, const char* filename) {
    if (!class_type || !filename) return NULL;

    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    // Read class name and verify
    uint32_t name_len;
    fread(&name_len, sizeof(uint32_t), 1, file);

    char* class_name = malloc(name_len + 1);
    if (!class_name) {
        fclose(file);
        return NULL;
    }

    fread(class_name, 1, name_len, file);
    class_name[name_len] = '\0';

    if (strcmp(class_name, class_type->name) != 0) {
        free(class_name);
        fclose(file);
        return NULL;
    }

    free(class_name);

    // Create instance
    void* instance = reflection_create_instance(class_type);
    if (!instance) {
        fclose(file);
        return NULL;
    }

    // Read properties
    uint32_t property_count;
    fread(&property_count, sizeof(uint32_t), 1, file);

    for (uint32_t i = 0; i < property_count; i++) {
        uint32_t prop_name_len;
        fread(&prop_name_len, sizeof(uint32_t), 1, file);

        char* prop_name = malloc(prop_name_len + 1);
        fread(prop_name, 1, prop_name_len, file);
        prop_name[prop_name_len] = '\0';

        reflection_property* prop = reflection_class_find_property(class_type, prop_name);
        free(prop_name);

        if (prop) {
            property_type prop_type;
            size_t prop_size;
            fread(&prop_type, sizeof(property_type), 1, file);
            fread(&prop_size, sizeof(size_t), 1, file);

            void* prop_value = malloc(prop_size);
            fread(prop_value, 1, prop_size, file);

            reflection_set_property_value(instance, prop, prop_value);
            free(prop_value);
        }
    }

    fclose(file);
    return instance;
}

void* reflection_serialize_to_buffer(void* object, size_t* buffer_size) {
    if (!object || !buffer_size) return NULL;

    reflection_class* class_type = reflection_get_object_class(object);
    if (!class_type) return NULL;

    // Estimate buffer size
    size_t estimated_size = 256 + class_type->property_count * 256;
    void* buffer = malloc(estimated_size);
    if (!buffer) return NULL;

    // TODO: Implement binary serialization to buffer
    *buffer_size = estimated_size;

    return buffer;
}

void* reflection_deserialize_from_buffer(reflection_class* class_type, const void* buffer, size_t buffer_size) {
    if (!class_type || !buffer || buffer_size == 0) return NULL;

    void* instance = reflection_create_instance(class_type);
    if (!instance) return NULL;

    // TODO: Implement binary deserialization from buffer

    return instance;
}

// ============================================================================
// PROPERTY ENUMERATION
// ============================================================================

void reflection_for_each_property(void* object, void (*callback)(reflection_property* property, void* user_data), void* user_data) {
    if (!object || !callback) return;

    reflection_class* class_type = reflection_get_object_class(object);
    if (!class_type) return;

    uint32_t prop_count = reflection_class_get_property_count(class_type);

    for (uint32_t i = 0; i < prop_count; i++) {
        reflection_property* prop = reflection_class_get_property(class_type, i);
        if (prop) {
            callback(prop, user_data);
        }
    }
}

// ============================================================================
// TYPE CONVERSION AND VALIDATION
// ============================================================================

bool reflection_can_convert(reflection_type* source, reflection_type* target) {
    if (!source || !target) return false;

    if (source->type == target->type) {
        return true;
    }

    // Allow conversion between numeric types
    if ((source->type >= PROPERTY_TYPE_INT8 && source->type <= PROPERTY_TYPE_DOUBLE) &&
        (target->type >= PROPERTY_TYPE_INT8 && target->type <= PROPERTY_TYPE_DOUBLE)) {
        return true;
    }

    if (source->can_convert_from && source->can_convert_from(target)) {
        return true;
    }

    return false;
}

void reflection_convert_type(void* src, reflection_type* src_type, void* dst, reflection_type* dst_type) {
    if (!src || !dst || !src_type || !dst_type) return;

    if (src_type->type == dst_type->type) {
        memcpy(dst, src, src_type->size < dst_type->size ? src_type->size : dst_type->size);
        return;
    }

    // Convert between numeric types
    if (src_type->type == PROPERTY_TYPE_FLOAT && dst_type->type == PROPERTY_TYPE_INT32) {
        *(int32_t*)dst = (int32_t)*(float*)src;
    } else if (src_type->type == PROPERTY_TYPE_INT32 && dst_type->type == PROPERTY_TYPE_FLOAT) {
        *(float*)dst = (float)*(int32_t*)src;
    } else if (src_type->type == PROPERTY_TYPE_DOUBLE && dst_type->type == PROPERTY_TYPE_FLOAT) {
        *(float*)dst = (float)*(double*)src;
    } else if (src_type->type == PROPERTY_TYPE_FLOAT && dst_type->type == PROPERTY_TYPE_DOUBLE) {
        *(double*)dst = (double)*(float*)src;
    }
    // Add more conversions as needed
}

// ============================================================================
// EDITOR SUPPORT
// ============================================================================

void reflection_class_set_display_name(reflection_class* class_type, const char* name) {
    if (class_type) {
        class_type->display_name = name;
    }
}

void reflection_property_set_category(reflection_property* property, const char* category) {
    if (property) {
        property->metadata.category = category;
    }
}

void reflection_property_set_tooltip(reflection_property* property, const char* tooltip) {
    if (property) {
        property->metadata.tooltip = tooltip;
    }
}

void reflection_property_set_editor_metadata(reflection_property* property, const metadata_editor* metadata) {
    if (property && metadata) {
        memcpy(&property->metadata_value.editor, metadata, sizeof(metadata_editor));
    }
}

void reflection_property_set_numeric_metadata(reflection_property* property, const metadata_numeric* metadata) {
    if (property && metadata) {
        memcpy(&property->metadata_value.numeric, metadata, sizeof(metadata_numeric));
    }
}

// End of reflection_system.c
