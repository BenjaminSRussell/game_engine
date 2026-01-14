#ifndef CORE_SERIALIZATION_JSON_PARSER_H
#define CORE_SERIALIZATION_JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsonValue JsonValue;

typedef struct JsonParserOptions {
  void *(*alloc)(size_t size, void *user);
  void (*free)(void *ptr, void *user);
  void *user;
  bool validate_unicode;
} JsonParserOptions;

typedef struct JsonSaxHandler {
  bool (*start_object)(void *user);
  bool (*end_object)(void *user);
  bool (*start_array)(void *user);
  bool (*end_array)(void *user);
  bool (*key)(const char *key, void *user);
  bool (*string)(const char *value, void *user);
  bool (*number)(double value, void *user);
  bool (*boolean)(bool value, void *user);
  bool (*null_value)(void *user);
} JsonSaxHandler;

JsonValue *json_parse_dom(const char *json_text,
                          const JsonParserOptions *options);

bool json_parse_sax(const char *json_text, const JsonSaxHandler *handler,
                    void *user, const JsonParserOptions *options);

char *json_serialize(const JsonValue *value,
                     const JsonParserOptions *options);

bool json_parser_self_test(void);

#ifdef __cplusplus
}
#endif

#endif
