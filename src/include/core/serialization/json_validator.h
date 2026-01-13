#ifndef CORE_SERIALIZATION_JSON_VALIDATOR_H
#define CORE_SERIALIZATION_JSON_VALIDATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsonValue JsonValue;

typedef struct JsonValidationError {
  char path[256];
  char message[256];
} JsonValidationError;

bool validate_schema(const JsonValue *schema, JsonValue *document,
                     JsonValidationError *error);

bool json_validator_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
