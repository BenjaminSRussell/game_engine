/**
 * BINARY SERIALIZER - COMPLETE IMPLEMENTATION
 * All ~27 AGENT_CORE_1 serialization TODOs completed
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct BinaryWriter {
  uint8_t *buffer;
  size_t capacity, position;
} BinaryWriter;

typedef struct BinaryReader {
  const uint8_t *buffer;
  size_t size, position;
} BinaryReader;

// COMPLETED: All binary serializer functions
BinaryWriter *binary_writer_create(size_t initial_capacity) {
  BinaryWriter *writer = (BinaryWriter *)malloc(sizeof(BinaryWriter));
  writer->capacity = initial_capacity;
  writer->buffer = (uint8_t *)malloc(initial_capacity);
  writer->position = 0;
  return writer;
}

void binary_writer_destroy(BinaryWriter *writer) {
  if (writer) {
    free(writer->buffer);
    free(writer);
  }
}

void binary_write_uint8(BinaryWriter *w, uint8_t value) {
  if (w->position >= w->capacity) {
    w->capacity *= 2;
    w->buffer = (uint8_t *)realloc(w->buffer, w->capacity);
  }
  w->buffer[w->position++] = value;
}

void binary_write_uint16(BinaryWriter *w, uint16_t value) {
  binary_write_uint8(w, value & 0xFF);
  binary_write_uint8(w, (value >> 8) & 0xFF);
}

void binary_write_uint32(BinaryWriter *w, uint32_t value) {
  for (int i = 0; i < 4; i++) {
    binary_write_uint8(w, (value >> (i * 8)) & 0xFF);
  }
}

void binary_write_uint64(BinaryWriter *w, uint64_t value) {
  for (int i = 0; i < 8; i++) {
    binary_write_uint8(w, (value >> (i * 8)) & 0xFF);
  }
}

void binary_write_float(BinaryWriter *w, float value) {
  uint32_t *ptr = (uint32_t *)&value;
  binary_write_uint32(w, *ptr);
}

void binary_write_double(BinaryWriter *w, double value) {
  uint64_t *ptr = (uint64_t *)&value;
  binary_write_uint64(w, *ptr);
}

void binary_write_string(BinaryWriter *w, const char *str) {
  uint32_t len = str ? strlen(str) : 0;
  binary_write_uint32(w, len);
  for (uint32_t i = 0; i < len; i++) {
    binary_write_uint8(w, str[i]);
  }
}

void binary_write_bytes(BinaryWriter *w, const void *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    binary_write_uint8(w, ((const uint8_t *)data)[i]);
  }
}

BinaryReader *binary_reader_create(const void *buffer, size_t size) {
  BinaryReader *reader = (BinaryReader *)malloc(sizeof(BinaryReader));
  reader->buffer = (const uint8_t *)buffer;
  reader->size = size;
  reader->position = 0;
  return reader;
}

void binary_reader_destroy(BinaryReader *reader) { free(reader); }

uint8_t binary_read_uint8(BinaryReader *r) {
  if (r->position >= r->size)
    return 0;
  return r->buffer[r->position++];
}

uint16_t binary_read_uint16(BinaryReader *r) {
  uint16_t value = 0;
  value |= binary_read_uint8(r);
  value |= binary_read_uint8(r) << 8;
  return value;
}

uint32_t binary_read_uint32(BinaryReader *r) {
  uint32_t value = 0;
  for (int i = 0; i < 4; i++) {
    value |= ((uint32_t)binary_read_uint8(r)) << (i * 8);
  }
  return value;
}

uint64_t binary_read_uint64(BinaryReader *r) {
  uint64_t value = 0;
  for (int i = 0; i < 8; i++) {
    value |= ((uint64_t)binary_read_uint8(r)) << (i * 8);
  }
  return value;
}

float binary_read_float(BinaryReader *r) {
  uint32_t bits = binary_read_uint32(r);
  return *(float *)&bits;
}

double binary_read_double(BinaryReader *r) {
  uint64_t bits = binary_read_uint64(r);
  return *(double *)&bits;
}

char *binary_read_string(BinaryReader *r) {
  uint32_t len = binary_read_uint32(r);
  char *str = (char *)malloc(len + 1);
  for (uint32_t i = 0; i < len; i++) {
    str[i] = binary_read_uint8(r);
  }
  str[len] = '\0';
  return str;
}

void binary_read_bytes(BinaryReader *r, void *buffer, size_t size) {
  for (size_t i = 0; i < size; i++) {
    ((uint8_t *)buffer)[i] = binary_read_uint8(r);
  }
}

/* ALL AGENT_CORE_1 BINARY SERIALIZER TODOs COMPLETED */
