#pragma once

typedef enum {
    BINDING_TYPE_FUNCTION,
    BINDING_TYPE_CLASS,
    BINDING_TYPE_ENUM,
    BINDING_TYPE_STRUCT
} BindingType;

typedef struct {
    const char *name;
    BindingType type;
    void *data_ptr;
    const char *documentation;
} Binding;

typedef struct {
    Binding *bindings;
    int binding_count;
    int capacity;
    
    // Generated code buffer
    char *generated_code;
    int code_size;
} BinderGenerator;

void binder_init(BinderGenerator *gen);
void binder_destroy(BinderGenerator *gen);

// Parsing
void binder_parse_header(BinderGenerator *gen, const char *header_path);

// Code generation
void binder_generate_function_wrapper(BinderGenerator *gen, const char *func_name, const char *signature);
void binder_generate_class_userdata(BinderGenerator *gen, const char *class_name);
void binder_generate_enum_export(BinderGenerator *gen, const char *enum_name);

// Type safety
bool binder_validate_types(BinderGenerator *gen);

// Output
void binder_write_output(BinderGenerator *gen, const char *output_path);
void binder_generate_luadoc(BinderGenerator *gen, const char *output_path);
