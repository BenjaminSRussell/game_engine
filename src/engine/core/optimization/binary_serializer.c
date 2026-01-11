/**
 * @file binary_serializer.c
 * @brief Reflection-based Binary Serializer.
 *
 * Serializes structs to compact binary format using schema definitions.
 * Supports versioning and endianness swapping.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <core/optimization/binary_serializer.h>
#include <string.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

// Structs and Enums defined in binary_serializer.h

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

void serialize_struct(void *data, TypeSchema *schema, BinaryWriter *writer) {
  for (int i = 0; i < schema->field_count; i++) {
    FieldSchema *f = &schema->fields[i];
    void *field_ptr = (uint8_t *)data + f->offset;

    switch (f->type) {
    case TYPE_INT32:
      writer_write_int32(writer, *(int32_t *)field_ptr);
      break;
    case TYPE_FLOAT:
      writer_write_float(writer, *(float *)field_ptr);
      break;
    case TYPE_STRING: {
      const char *str = *(const char **)field_ptr;
      writer_write_string(writer, str);
    } break;
    case TYPE_STRUCT:
    case TYPE_ARRAY:
      // TODO: Implement recursion for nested types
      break;
    }
  }
}

void deserialize_struct(void *data, TypeSchema *schema, BinaryReader *reader) {
  for (int i = 0; i < schema->field_count; i++) {
    FieldSchema *f = &schema->fields[i];
    void *field_ptr = (uint8_t *)data + f->offset;

    switch (f->type) {
    case TYPE_INT32:
      *(int32_t *)field_ptr = reader_read_int32(reader);
      break;
    case TYPE_FLOAT:
    case TYPE_STRING:
    case TYPE_STRUCT:
    case TYPE_ARRAY:
      // TODO: Implement deserialization for these types
      break;
    }
  }
}
