#include "core/serialization/schema_validator.h"

#include "core/json.h"
#include "core/serialization/json_validator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void schema_set_error(SchemaValidator *validator, const char *path,
                             const char *message) {
  if (!validator) {
    return;
  }
  snprintf(validator->last_path, sizeof(validator->last_path), "%s",
           path ? path : "$");
  snprintf(validator->last_error, sizeof(validator->last_error), "%s",
           message ? message : "");
}

static bool schema_type_is_valid(const char *type) {
  if (!type) {
    return false;
  }
  return strcmp(type, "object") == 0 || strcmp(type, "array") == 0 ||
         strcmp(type, "string") == 0 || strcmp(type, "number") == 0 ||
         strcmp(type, "integer") == 0 || strcmp(type, "boolean") == 0 ||
         strcmp(type, "null") == 0;
}

static bool schema_validate_node(SchemaValidator *validator, const JsonValue *schema,
                                 const char *path) {
  if (!schema || schema->type != JSON_OBJECT) {
    schema_set_error(validator, path, "Schema node must be an object");
    return false;
  }

  JsonValue *type_val = json_object_get((JsonValue *)schema, "type");
  if (type_val && (type_val->type != JSON_STRING ||
                   !schema_type_is_valid(type_val->string))) {
    schema_set_error(validator, path, "Invalid or unsupported type");
    return false;
  }

  JsonValue *props = json_object_get((JsonValue *)schema, "properties");
  if (props && props->type != JSON_OBJECT) {
    schema_set_error(validator, path, "Properties must be an object");
    return false;
  }

  if (props && props->type == JSON_OBJECT) {
    for (u32 i = 0; i < props->object.count; ++i) {
      char child_path[256];
      snprintf(child_path, sizeof(child_path), "%s.%s", path,
               props->object.keys[i]);
      if (!schema_validate_node(validator, props->object.values[i], child_path)) {
        return false;
      }
    }
  }

  JsonValue *required = json_object_get((JsonValue *)schema, "required");
  if (required && required->type != JSON_ARRAY) {
    schema_set_error(validator, path, "Required must be an array");
    return false;
  }

  if (required && required->type == JSON_ARRAY) {
    for (u32 i = 0; i < required->array.count; ++i) {
      JsonValue *entry = required->array.values[i];
      if (!entry || entry->type != JSON_STRING) {
        schema_set_error(validator, path, "Required entries must be strings");
        return false;
      }
    }
  }

  JsonValue *items = json_object_get((JsonValue *)schema, "items");
  if (items) {
    if (!schema_validate_node(validator, items, path)) {
      return false;
    }
  }

  JsonValue *enum_val = json_object_get((JsonValue *)schema, "enum");
  if (enum_val && enum_val->type != JSON_ARRAY) {
    schema_set_error(validator, path, "Enum must be an array");
    return false;
  }

  JsonValue *min_val = json_object_get((JsonValue *)schema, "min");
  if (min_val && min_val->type != JSON_NUMBER) {
    schema_set_error(validator, path, "Min must be a number");
    return false;
  }

  JsonValue *max_val = json_object_get((JsonValue *)schema, "max");
  if (max_val && max_val->type != JSON_NUMBER) {
    schema_set_error(validator, path, "Max must be a number");
    return false;
  }

  return true;
}

SchemaValidator *validator_create(void) {
  SchemaValidator *validator = (SchemaValidator *)calloc(1, sizeof(SchemaValidator));
  return validator;
}

void validator_destroy(SchemaValidator *validator) {
  free(validator);
}

bool validator_validate(SchemaValidator *validator, const JsonValue *schema,
                        JsonValue *document) {
  if (!validator || !schema || !document) {
    schema_set_error(validator, "$", "Schema or document missing");
    return false;
  }

  if (!schema_validate_node(validator, schema, "$")) {
    return false;
  }

  JsonValidationError error = {0};
  if (!validate_schema(schema, document, &error)) {
    schema_set_error(validator, error.path, error.message);
    return false;
  }

  return true;
}

const char *validator_last_error(const SchemaValidator *validator) {
  return validator ? validator->last_error : "";
}

const char *validator_last_path(const SchemaValidator *validator) {
  return validator ? validator->last_path : "";
}

bool schema_validator_self_test(void) {
  const char *schema_text =
      "{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\"}}}";
  const char *doc_text = "{\"name\":\"ok\"}";

  JsonValue *schema = json_parse(schema_text);
  JsonValue *doc = json_parse(doc_text);

  SchemaValidator *validator = validator_create();
  bool ok = validator_validate(validator, schema, doc);

  validator_destroy(validator);
  json_free(schema);
  json_free(doc);

  return ok;
}
