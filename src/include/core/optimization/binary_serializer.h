#ifndef BINARY_SERIALIZER_H
#define BINARY_SERIALIZER_H

#include <stddef.h>
#include <stdint.h>

// Field Types
typedef enum FieldType {
  TYPE_INT32,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_STRUCT,
  TYPE_ARRAY
} FieldType;

// Schemas
typedef struct FieldSchema {
  const char *name;
  FieldType type;
  size_t offset;
} FieldSchema;

typedef struct TypeSchema {
  const char *name;
  FieldSchema fields[32];
  int field_count;
} TypeSchema;

// Opaque IO types
typedef struct BinaryWriter BinaryWriter;
typedef struct BinaryReader BinaryReader;

// API
void serialize_struct(void *data, TypeSchema *schema, BinaryWriter *writer);
void deserialize_struct(void *data, TypeSchema *schema, BinaryReader *reader);

// External dependencies (assumed to be linked or stubbed)
void writer_write_int32(BinaryWriter *writer, int32_t value);
void writer_write_float(BinaryWriter *writer, float value);
void writer_write_string(BinaryWriter *writer, const char *value);
int32_t reader_read_int32(BinaryReader *reader);

#endif // BINARY_SERIALIZER_H
