#include <editor/reflection.h>
#include <stdio.h>
#include <string.h>

/**
 * =================================================================================================
 *                                   INSPECTOR & PROPERTIES - IMPLEMENTATION
 * =================================================================================================
 */

// Global registry of reflected structs
#define MAX_REFLECTED_STRUCTS 256
static StructMetadata g_struct_registry[MAX_REFLECTED_STRUCTS];
static uint32_t g_struct_count = 0;

// Internal: Find metadata for a struct by name
static StructMetadata *find_struct_metadata(const char *name) {
  for (uint32_t i = 0; i < g_struct_count; i++) {
    if (strcmp(g_struct_registry[i].name, name) == 0) {
      return &g_struct_registry[i];
    }
  }
  return NULL;
}

// TASK_1110: Implement Property Reflection system (metadata for C structs)
void editor_reflection_register_struct(StructMetadata meta) {
  if (g_struct_count >= MAX_REFLECTED_STRUCTS) {
    fprintf(stderr, "[EDITOR] Error: Struct registry full!\n");
    return;
  }

  g_struct_registry[g_struct_count++] = meta;
  printf("[EDITOR] Registered struct: %s with %u properties\n", meta.name,
         meta.property_count);
}

// TASK_1111: Map Data Types -> UI Widgets
// Mock implementation of UI drawing based on reflection
void editor_draw_property(void *data_ptr, PropertyMetadata *prop) {
  void *value_ptr = (char *)data_ptr + prop->offset;

  switch (prop->type) {
  case PROPERTY_TYPE_FLOAT:
    // UI_SliderFloat(prop->name, (float*)value_ptr, prop->min_val,
    // prop->max_val);
    printf("  [Float] %s: %f\n", prop->name, *(float *)value_ptr);
    break;
  case PROPERTY_TYPE_INT:
    // UI_InputInt(prop->name, (int*)value_ptr);
    printf("  [Int] %s: %d\n", prop->name, *(int *)value_ptr);
    break;
  case PROPERTY_TYPE_BOOL:
    // UI_Checkbox(prop->name, (bool*)value_ptr);
    printf("  [Bool] %s: %s\n", prop->name,
           *(bool *)value_ptr ? "true" : "false");
    break;
  case PROPERTY_TYPE_VEC3:
    // UI_DragFloat3(prop->name, (float*)value_ptr);
    {
      float *v = (float *)value_ptr;
      printf("  [Vec3] %s: { %f, %f, %f }\n", prop->name, v[0], v[1], v[2]);
    }
    break;
  case PROPERTY_TYPE_STRUCT:
    // TASK_1112: Support "Nested Structs"
    if (prop->struct_meta) {
      printf("  [Struct] %s {\n", prop->name);
      for (uint32_t i = 0; i < prop->struct_meta->property_count; i++) {
        editor_draw_property(value_ptr, &prop->struct_meta->properties[i]);
      }
      printf("  }\n");
    }
    break;
  default:
    printf("  [Unknown] %s\n", prop->name);
    break;
  }
}

void editor_draw_inspector(void *target_obj, const char *struct_name) {
  StructMetadata *meta = find_struct_metadata(struct_name);
  if (!meta) {
    printf("[EDITOR] No metadata found for struct: %s\n", struct_name);
    return;
  }

  printf("--- Inspector: %s ---\n", meta->name);
  for (uint32_t i = 0; i < meta->property_count; i++) {
    editor_draw_property(target_obj, &meta->properties[i]);
  }
}
