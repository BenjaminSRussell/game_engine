#ifndef CORE_SERIALIZATION_SCHEMA_VALIDATOR_H
#define CORE_SERIALIZATION_SCHEMA_VALIDATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsonValue JsonValue;

typedef struct SchemaValidator {
  char last_error[256];
  char last_path[256];
} SchemaValidator;

SchemaValidator *validator_create(void);
void validator_destroy(SchemaValidator *validator);

bool validator_validate(SchemaValidator *validator, const JsonValue *schema,
                        JsonValue *document);

const char *validator_last_error(const SchemaValidator *validator);
const char *validator_last_path(const SchemaValidator *validator);

bool schema_validator_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
