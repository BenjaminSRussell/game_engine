#ifndef PROPERTY_EDITOR_H
#define PROPERTY_EDITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    PROP_TYPE_INT,
    PROP_TYPE_FLOAT,
    PROP_TYPE_BOOL,
    PROP_TYPE_STRING,
    PROP_TYPE_VEC3,
    PROP_TYPE_COLOR
} PropertyType;

typedef struct {
    const char* name;
    PropertyType type;
    void* data; // Pointer to the actual data
    bool read_only;
} PropertyField;

void PropertyEditor_Init(void);
void PropertyEditor_Shutdown(void);

// Begin a property inspector window/panel
void PropertyEditor_Begin(const char* title);
void PropertyEditor_End(void);

// Register/Draw a property field
// Returns true if the value was changed
bool PropertyEditor_Field(PropertyField field);

// Helpers for common types
bool PropertyEditor_Int(const char* label, int* value);
bool PropertyEditor_Float(const char* label, float* value);
bool PropertyEditor_Bool(const char* label, bool* value);
bool PropertyEditor_String(const char* label, char* value, size_t max_len);

#endif // PROPERTY_EDITOR_H
