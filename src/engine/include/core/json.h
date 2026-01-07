#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "include/common.h"

typedef enum {
  JSON_NULL,
  JSON_BOOL,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;

struct JsonValue {
  JsonType type;
  union {
    bool boolean;
    double number;
    char *string;
    struct {
      JsonValue **values;
      u32 count;
    } array;
    struct {
      char **keys;
      JsonValue **values;
      u32 count;
    } object;
  };
};

// Parse a JSON string. Returns NULL on failure.
JsonValue *json_parse(const char *json_text);

// Free a JSON value and all its children.
void json_free(JsonValue *value);

// Helper functions for accessing data safely
JsonValue *json_object_get(JsonValue *object, const char *key);
const char *json_string_value(JsonValue *value);
double json_number_value(JsonValue *value);
bool json_bool_value(JsonValue *value);

#endif // JSON_PARSER_H
