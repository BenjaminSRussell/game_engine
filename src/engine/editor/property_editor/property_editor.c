#include "property_editor.h"
#include <stdio.h>

void PropertyEditor_Init(void) {
    printf("[PropertyEditor] Initialized\n");
}

void PropertyEditor_Shutdown(void) {
    printf("[PropertyEditor] Shutdown\n");
}

void PropertyEditor_Begin(const char* title) {
    // In a real UI system (like Dear ImGui), this would start a window
    // printf("[PropertyEditor] Begin Panel: %s\n", title);
}

void PropertyEditor_End(void) {
    // printf("[PropertyEditor] End Panel\n");
}

bool PropertyEditor_Field(PropertyField field) {
    // Simulate drawing a field and detecting change
    // For this mock, we assume no user input, so no change
    // printf("[PropertyEditor] Field: %s (Type: %d)\n", field.name, field.type);
    return false;
}

bool PropertyEditor_Int(const char* label, int* value) {
    PropertyField f = { .name = label, .type = PROP_TYPE_INT, .data = value };
    return PropertyEditor_Field(f);
}

bool PropertyEditor_Float(const char* label, float* value) {
    PropertyField f = { .name = label, .type = PROP_TYPE_FLOAT, .data = value };
    return PropertyEditor_Field(f);
}

bool PropertyEditor_Bool(const char* label, bool* value) {
    PropertyField f = { .name = label, .type = PROP_TYPE_BOOL, .data = value };
    return PropertyEditor_Field(f);
}

bool PropertyEditor_String(const char* label, char* value, size_t max_len) {
    PropertyField f = { .name = label, .type = PROP_TYPE_STRING, .data = value };
    return PropertyEditor_Field(f);
}
