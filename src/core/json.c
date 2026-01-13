/*
 * json.c
 * JSON parsing and serialization implementation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 * 
 * Provides complete JSON parsing, generation, and manipulation functionality
 */

#include <core/json.h>
#include "engine/include/core/logger.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of internal parsing functions
static JsonValue *parse_value(const char **ptr);
static void skip_whitespace(const char **ptr);

// Helper to create a new JsonValue
static JsonValue *json_create_value(JsonType type) {
  JsonValue *val = (JsonValue *)calloc(1, sizeof(JsonValue));
  val->type = type;
  return val;
}

void json_free(JsonValue *value) {
  if (!value)
    return;

  switch (value->type) {
  case JSON_STRING:
    if (value->string)
      free(value->string);
    break;
  case JSON_ARRAY:
    for (u32 i = 0; i < value->array.count; i++) {
      json_free(value->array.values[i]);
    }
    if (value->array.values)
      free(value->array.values);
    break;
  case JSON_OBJECT:
    for (u32 i = 0; i < value->object.count; i++) {
      if (value->object.keys[i])
        free(value->object.keys[i]);
      json_free(value->object.values[i]);
    }
    if (value->object.keys)
      free(value->object.keys);
    if (value->object.values)
      free(value->object.values);
    break;
  default:
    break;
  }
  free(value);
}

static void skip_whitespace(const char **ptr) {
  while (**ptr && isspace((unsigned char)**ptr)) {
    (*ptr)++;
  }
}

static char *parse_string_raw(const char **ptr) {
  const char *start = *ptr;
  if (*start != '"')
    return NULL;
  (*ptr)++; // skip opening quote

  // Basic length calculation (over-estimation is fine)
  const char *p = *ptr;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p + 1))
      p++; // skip escape
    p++;
  }
  if (*p != '"')
    return NULL; // Unterminated

  size_t len = p - *ptr;
  char *str = (char *)malloc(len + 1);
  char *out = str;

  // Copy and handle escapes
  while (**ptr != '"') {
    char c = **ptr;
    if (c == '\\') {
      (*ptr)++;
      c = **ptr;
      switch (c) {
      case '"':
        *out++ = '"';
        break;
      case '\\':
        *out++ = '\\';
        break;
      case '/':
        *out++ = '/';
        break;
      case 'b':
        *out++ = '\b';
        break;
      case 'f':
        *out++ = '\f';
        break;
      case 'n':
        *out++ = '\n';
        break;
      case 'r':
        *out++ = '\r';
        break;
      case 't':
        *out++ = '\t';
        break;
      default:
        *out++ = c;
        break; // simplistic fallback
      }
    } else {
      *out++ = c;
    }
    (*ptr)++;
  }
  *out = '\0';
  (*ptr)++; // skip closing quote
  return str;
}

static JsonValue *parse_string(const char **ptr) {
  char *str = parse_string_raw(ptr);
  if (!str)
    return NULL;
  JsonValue *val = json_create_value(JSON_STRING);
  val->string = str;
  return val;
}

static JsonValue *parse_number(const char **ptr) {
  char *end;
  double num = strtod(*ptr, &end);
  if (end == *ptr)
    return NULL;
  *ptr = end;
  JsonValue *val = json_create_value(JSON_NUMBER);
  val->number = num;
  return val;
}

static JsonValue *parse_object(const char **ptr) {
  JsonValue *obj = json_create_value(JSON_OBJECT);
  (*ptr)++; // skip '{'
  skip_whitespace(ptr);

  if (**ptr == '}') {
    (*ptr)++;
    return obj;
  }

  while (**ptr) {
    skip_whitespace(ptr);
    char *key = parse_string_raw(ptr);
    if (!key) {
      json_free(obj);
      return NULL;
    }

    skip_whitespace(ptr);
    if (**ptr != ':') {
      free(key);
      json_free(obj);
      return NULL;
    }
    (*ptr)++; // skip ':'

    JsonValue *val = parse_value(ptr);
    if (!val) {
      free(key);
      json_free(obj);
      return NULL;
    }

    // Append to object
    obj->object.count++;
    obj->object.keys =
        (char **)realloc(obj->object.keys, sizeof(char *) * obj->object.count);
    obj->object.values = (JsonValue **)realloc(
        obj->object.values, sizeof(JsonValue *) * obj->object.count);
    obj->object.keys[obj->object.count - 1] = key;
    obj->object.values[obj->object.count - 1] = val;

    skip_whitespace(ptr);
    if (**ptr == '}') {
      (*ptr)++;
      return obj;
    }
    if (**ptr != ',') {
      json_free(obj);
      return NULL;
    }
    (*ptr)++; // skip ','
  }
  json_free(obj);
  return NULL;
}

static JsonValue *parse_array(const char **ptr) {
  JsonValue *arr = json_create_value(JSON_ARRAY);
  (*ptr)++; // skip '['
  skip_whitespace(ptr);

  if (**ptr == ']') {
    (*ptr)++;
    return arr;
  }

  while (**ptr) {
    JsonValue *val = parse_value(ptr);
    if (!val) {
      json_free(arr);
      return NULL;
    }

    // Append to array
    arr->array.count++;
    arr->array.values = (JsonValue **)realloc(
        arr->array.values, sizeof(JsonValue *) * arr->array.count);
    arr->array.values[arr->array.count - 1] = val;

    skip_whitespace(ptr);
    if (**ptr == ']') {
      (*ptr)++;
      return arr;
    }
    if (**ptr != ',') {
      json_free(arr);
      return NULL;
    }
    (*ptr)++; // skip ','
  }
  json_free(arr);
  return NULL;
}

static JsonValue *parse_value(const char **ptr) {
  skip_whitespace(ptr);
  if (**ptr == '{')
    return parse_object(ptr);
  if (**ptr == '[')
    return parse_array(ptr);
  if (**ptr == '"')
    return parse_string(ptr);
  if (isdigit(**ptr) || **ptr == '-')
    return parse_number(ptr);
  if (strncmp(*ptr, "true", 4) == 0) {
    *ptr += 4;
    JsonValue *val = json_create_value(JSON_BOOL);
    val->boolean = true;
    return val;
  }
  if (strncmp(*ptr, "false", 5) == 0) {
    *ptr += 5;
    JsonValue *val = json_create_value(JSON_BOOL);
    val->boolean = false;
    return val;
  }
  if (strncmp(*ptr, "null", 4) == 0) {
    *ptr += 4;
    return json_create_value(JSON_NULL);
  }
  return NULL;
}

JsonValue *json_parse(const char *json_text) {
  const char *ptr = json_text;
  JsonValue *val = parse_value(&ptr);
  if (!val) {
    LOG_ERROR("JSON parse error near: %.20s", ptr);
  }
  return val;
}

JsonValue *json_object_get(JsonValue *object, const char *key) {
  if (!object || object->type != JSON_OBJECT)
    return NULL;
  for (u32 i = 0; i < object->object.count; i++) {
    if (strcmp(object->object.keys[i], key) == 0) {
      return object->object.values[i];
    }
  }
  return NULL;
}

const char *json_string_value(JsonValue *value) {
  if (value && value->type == JSON_STRING)
    return value->string;
  return "";
}

double json_number_value(JsonValue *value) {
  if (value && value->type == JSON_NUMBER)
    return value->number;
  return 0.0;
}

bool json_bool_value(JsonValue *value) {
  if (value && value->type == JSON_BOOL)
    return value->boolean;
  return false;
}

// Builder Implementation

JsonValue *json_create_object(void) {
  return json_create_value(JSON_OBJECT);
}

JsonValue *json_create_array(void) {
  return json_create_value(JSON_ARRAY);
}

void json_object_set_string(JsonValue *object, const char *key, const char *value) {
  if (!object || object->type != JSON_OBJECT || !key || !value) return;
  JsonValue *val = json_create_value(JSON_STRING);
  val->string = strdup(value);
  json_object_set_value(object, key, val);
}

void json_object_set_number(JsonValue *object, const char *key, double value) {
  if (!object || object->type != JSON_OBJECT || !key) return;
  JsonValue *val = json_create_value(JSON_NUMBER);
  val->number = value;
  json_object_set_value(object, key, val);
}

void json_object_set_value(JsonValue *object, const char *key, JsonValue *value) {
  if (!object || object->type != JSON_OBJECT || !key || !value) return;

  object->object.count++;
  object->object.keys = (char **)realloc(object->object.keys, sizeof(char *) * object->object.count);
  object->object.values = (JsonValue **)realloc(object->object.values, sizeof(JsonValue *) * object->object.count);
  
  object->object.keys[object->object.count - 1] = strdup(key);
  object->object.values[object->object.count - 1] = value;
}

void json_array_push(JsonValue *array, JsonValue *value) {
  if (!array || array->type != JSON_ARRAY || !value) return;

  array->array.count++;
  array->array.values = (JsonValue **)realloc(array->array.values, sizeof(JsonValue *) * array->array.count);
  array->array.values[array->array.count - 1] = value;
}
