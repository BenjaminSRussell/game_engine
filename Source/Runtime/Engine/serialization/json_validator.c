#include "core/serialization/json_validator.h"

#include "core/json.h"

#include <include/math/math_all.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void json_set_error(JsonValidationError *error, const char *path,
                           const char *message) {
  if (!error) {
    return;
  }
  snprintf(error->path, sizeof(error->path), "%s", path ? path : "$");
  snprintf(error->message, sizeof(error->message), "%s", message);
}

static bool json_values_equal(const JsonValue *a, const JsonValue *b) {
  if (!a || !b || a->type != b->type) {
    return false;
  }

  switch (a->type) {
    case JSON_NULL:
      return true;
    case JSON_BOOL:
      return a->boolean == b->boolean;
    case JSON_NUMBER:
      return a->number == b->number;
    case JSON_STRING:
      return strcmp(a->string ? a->string : "", b->string ? b->string : "") == 0;
    case JSON_ARRAY:
      if (a->array.count != b->array.count) {
        return false;
      }
      for (u32 i = 0; i < a->array.count; ++i) {
        if (!json_values_equal(a->array.values[i], b->array.values[i])) {
          return false;
        }
      }
      return true;
    case JSON_OBJECT:
      if (a->object.count != b->object.count) {
        return false;
      }
      for (u32 i = 0; i < a->object.count; ++i) {
        JsonValue *b_val = json_object_get((JsonValue *)b, a->object.keys[i]);
        if (!b_val) {
          return false;
        }
        if (!json_values_equal(a->object.values[i], b_val)) {
          return false;
        }
      }
      return true;
  }

  return false;
}

static const char *json_type_name(JsonType type) {
  switch (type) {
    case JSON_NULL:
      return "null";
    case JSON_BOOL:
      return "boolean";
    case JSON_NUMBER:
      return "number";
    case JSON_STRING:
      return "string";
    case JSON_ARRAY:
      return "array";
    case JSON_OBJECT:
      return "object";
  }
  return "unknown";
}

static bool json_type_matches(JsonType actual, const char *expected) {
  if (!expected) {
    return true;
  }
  if (strcmp(expected, "integer") == 0) {
    if (actual != JSON_NUMBER) {
      return false;
    }
    return true;
  }
  if (strcmp(expected, "number") == 0) {
    return actual == JSON_NUMBER;
  }
  if (strcmp(expected, "string") == 0) {
    return actual == JSON_STRING;
  }
  if (strcmp(expected, "boolean") == 0) {
    return actual == JSON_BOOL;
  }
  if (strcmp(expected, "null") == 0) {
    return actual == JSON_NULL;
  }
  if (strcmp(expected, "object") == 0) {
    return actual == JSON_OBJECT;
  }
  if (strcmp(expected, "array") == 0) {
    return actual == JSON_ARRAY;
  }
  return true;
}

static JsonValue *json_clone_simple(const JsonValue *src) {
  if (!src) {
    return NULL;
  }

  JsonValue *dst = (JsonValue *)calloc(1, sizeof(JsonValue));
  if (!dst) {
    return NULL;
  }
  dst->type = src->type;

  switch (src->type) {
    case JSON_STRING:
      if (src->string) {
        dst->string = strdup(src->string);
      }
      break;
    case JSON_NUMBER:
      dst->number = src->number;
      break;
    case JSON_BOOL:
      dst->boolean = src->boolean;
      break;
    case JSON_ARRAY:
      dst->array.count = src->array.count;
      dst->array.values =
          (JsonValue **)calloc(src->array.count, sizeof(JsonValue *));
      if (!dst->array.values) {
        free(dst);
        return NULL;
      }
      for (u32 i = 0; i < src->array.count; ++i) {
        dst->array.values[i] = json_clone_simple(src->array.values[i]);
      }
      break;
    case JSON_OBJECT:
      dst->object.count = src->object.count;
      dst->object.keys = (char **)calloc(src->object.count, sizeof(char *));
      dst->object.values =
          (JsonValue **)calloc(src->object.count, sizeof(JsonValue *));
      if (!dst->object.keys || !dst->object.values) {
        free(dst->object.keys);
        free(dst->object.values);
        free(dst);
        return NULL;
      }
      for (u32 i = 0; i < src->object.count; ++i) {
        dst->object.keys[i] = strdup(src->object.keys[i]);
        dst->object.values[i] = json_clone_simple(src->object.values[i]);
      }
      break;
    default:
      break;
  }

  return dst;
}

static bool json_object_add(JsonValue *object, const char *key, JsonValue *value) {
  if (!object || object->type != JSON_OBJECT || !key || !value) {
    return false;
  }

  u32 new_count = object->object.count + 1;
  char **new_keys = (char **)malloc(sizeof(char *) * new_count);
  JsonValue **new_values =
      (JsonValue **)malloc(sizeof(JsonValue *) * new_count);
  if (!new_keys || !new_values) {
    free(new_keys);
    free(new_values);
    return false;
  }

  if (object->object.count > 0) {
    memcpy(new_keys, object->object.keys,
           sizeof(char *) * object->object.count);
    memcpy(new_values, object->object.values,
           sizeof(JsonValue *) * object->object.count);
  }

  new_keys[new_count - 1] = strdup(key);
  new_values[new_count - 1] = value;

  free(object->object.keys);
  free(object->object.values);

  object->object.keys = new_keys;
  object->object.values = new_values;
  object->object.count = new_count;
  return true;
}

static bool json_is_required(const JsonValue *required, const char *key) {
  if (!required || required->type != JSON_ARRAY || !key) {
    return false;
  }
  for (u32 i = 0; i < required->array.count; ++i) {
    JsonValue *entry = required->array.values[i];
    if (entry && entry->type == JSON_STRING && entry->string &&
        strcmp(entry->string, key) == 0) {
      return true;
    }
  }
  return false;
}

static bool json_validate_node(const JsonValue *schema, JsonValue *value,
                               JsonValidationError *error, const char *path) {
  if (!schema) {
    return true;
  }

  const char *expected_type = NULL;
  JsonValue *type_val = json_object_get((JsonValue *)schema, "type");
  if (type_val && type_val->type == JSON_STRING) {
    expected_type = type_val->string;
  }

  if (value && expected_type && !json_type_matches(value->type, expected_type)) {
    char msg[128];
    snprintf(msg, sizeof(msg), "Expected %s, got %s", expected_type,
             json_type_name(value->type));
    json_set_error(error, path, msg);
    return false;
  }

  if (value && expected_type && strcmp(expected_type, "integer") == 0 &&
      value->type == JSON_NUMBER) {
    double rounded = floor(value->number);
    if (rounded != value->number) {
      json_set_error(error, path, "Expected integer value");
      return false;
    }
  }

  JsonValue *enum_val = json_object_get((JsonValue *)schema, "enum");
  if (enum_val && enum_val->type == JSON_ARRAY) {
    bool matched = false;
    for (u32 i = 0; i < enum_val->array.count; ++i) {
      if (json_values_equal(enum_val->array.values[i], value)) {
        matched = true;
        break;
      }
    }
    if (!matched) {
      json_set_error(error, path, "Value not present in enum");
      return false;
    }
  }

  if (value && value->type == JSON_NUMBER) {
    JsonValue *min_val = json_object_get((JsonValue *)schema, "min");
    JsonValue *max_val = json_object_get((JsonValue *)schema, "max");
    if (min_val && min_val->type == JSON_NUMBER && value->number < min_val->number) {
      json_set_error(error, path, "Value below minimum");
      return false;
    }
    if (max_val && max_val->type == JSON_NUMBER && value->number > max_val->number) {
      json_set_error(error, path, "Value above maximum");
      return false;
    }
  }

  if (value && value->type == JSON_OBJECT) {
    JsonValue *props = json_object_get((JsonValue *)schema, "properties");
    JsonValue *required = json_object_get((JsonValue *)schema, "required");

    if (props && props->type == JSON_OBJECT) {
      for (u32 i = 0; i < props->object.count; ++i) {
        const char *key = props->object.keys[i];
        JsonValue *prop_schema = props->object.values[i];
        JsonValue *prop_value = json_object_get(value, key);

        char child_path[256];
        snprintf(child_path, sizeof(child_path), "%s.%s", path, key);

        if (!prop_value) {
          JsonValue *default_val =
              json_object_get((JsonValue *)prop_schema, "default");
          if (default_val) {
            JsonValue *clone = json_clone_simple(default_val);
            if (!json_object_add(value, key, clone)) {
              json_set_error(error, child_path, "Failed to apply default");
              return false;
            }
          } else if (json_is_required(required, key)) {
            json_set_error(error, child_path, "Missing required field");
            return false;
          }
          continue;
        }

        if (!json_validate_node(prop_schema, prop_value, error, child_path)) {
          return false;
        }
      }
    }
  }

  if (value && value->type == JSON_ARRAY) {
    JsonValue *items = json_object_get((JsonValue *)schema, "items");
    if (items) {
      for (u32 i = 0; i < value->array.count; ++i) {
        char child_path[256];
        if (!json_validate_node(items, value->array.values[i], error,
                                child_path)) {
          return false;
        }
      }
    }
  }

  return true;
}

bool validate_schema(const JsonValue *schema, JsonValue *document,
                     JsonValidationError *error) {
  if (!schema || !document) {
    json_set_error(error, "$", "Schema or document missing");
    return false;
  }

  return json_validate_node(schema, document, error, "$");
}

bool json_validator_self_test(void) {
  const char *schema_text =
      "{\"type\":\"object\",\"properties\":{\"x\":{\"type\":\"number\",\"min\":0,\"default\":1}},\"required\":[\"x\"]}";
  const char *doc_text = "{}";

  JsonValue *schema = json_parse(schema_text);
  JsonValue *doc = json_parse(doc_text);
  JsonValidationError error = {0};

  bool ok = validate_schema(schema, doc, &error);
  json_free(schema);
  json_free(doc);
  return ok;
}
