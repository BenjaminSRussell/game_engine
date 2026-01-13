#include "core/serialization/json_parser.h"

#include "core/json.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StringBuilder {
  char *data;
  size_t length;
  size_t capacity;
} StringBuilder;

static void *json_alloc(const JsonParserOptions *options, size_t size) {
  if (options && options->alloc) {
    return options->alloc(size, options->user);
  }
  return malloc(size);
}

static void json_free_with(const JsonParserOptions *options, void *ptr) {
  if (!ptr) {
    return;
  }
  if (options && options->free) {
    options->free(ptr, options->user);
    return;
  }
  free(ptr);
}

static bool sb_reserve(StringBuilder *sb, size_t extra) {
  if (!sb) {
    return false;
  }
  size_t needed = sb->length + extra + 1;
  if (needed <= sb->capacity) {
    return true;
  }

  size_t new_capacity = sb->capacity ? sb->capacity : 256;
  while (new_capacity < needed) {
    new_capacity *= 2;
  }

  char *next = (char *)realloc(sb->data, new_capacity);
  if (!next) {
    return false;
  }
  sb->data = next;
  sb->capacity = new_capacity;
  return true;
}

static bool sb_append(StringBuilder *sb, const char *text) {
  if (!sb || !text) {
    return false;
  }
  size_t len = strlen(text);
  if (!sb_reserve(sb, len)) {
    return false;
  }
  memcpy(sb->data + sb->length, text, len);
  sb->length += len;
  sb->data[sb->length] = '\0';
  return true;
}

static bool sb_append_char(StringBuilder *sb, char ch) {
  if (!sb_reserve(sb, 1)) {
    return false;
  }
  sb->data[sb->length++] = ch;
  sb->data[sb->length] = '\0';
  return true;
}

static bool json_is_valid_utf8(const char *text) {
  const unsigned char *bytes = (const unsigned char *)text;
  while (*bytes) {
    if (*bytes <= 0x7F) {
      bytes++;
      continue;
    }

    unsigned char lead = *bytes++;
    int count = 0;
    if ((lead & 0xE0) == 0xC0) {
      count = 1;
    } else if ((lead & 0xF0) == 0xE0) {
      count = 2;
    } else if ((lead & 0xF8) == 0xF0) {
      count = 3;
    } else {
      return false;
    }

    for (int i = 0; i < count; ++i) {
      if ((bytes[i] & 0xC0) != 0x80) {
        return false;
      }
    }
    bytes += count;
  }
  return true;
}

static bool json_validate_unicode(const JsonValue *value) {
  if (!value) {
    return false;
  }

  switch (value->type) {
    case JSON_STRING:
      return json_is_valid_utf8(value->string ? value->string : "");
    case JSON_ARRAY:
      for (u32 i = 0; i < value->array.count; ++i) {
        if (!json_validate_unicode(value->array.values[i])) {
          return false;
        }
      }
      return true;
    case JSON_OBJECT:
      for (u32 i = 0; i < value->object.count; ++i) {
        if (!json_is_valid_utf8(value->object.keys[i] ? value->object.keys[i]
                                                      : "")) {
          return false;
        }
        if (!json_validate_unicode(value->object.values[i])) {
          return false;
        }
      }
      return true;
    default:
      return true;
  }
}

static JsonValue *json_clone_value(const JsonValue *src,
                                   const JsonParserOptions *options) {
  if (!src) {
    return NULL;
  }

  JsonValue *dst = (JsonValue *)json_alloc(options, sizeof(JsonValue));
  if (!dst) {
    return NULL;
  }
  memset(dst, 0, sizeof(JsonValue));
  dst->type = src->type;

  switch (src->type) {
    case JSON_STRING: {
      if (src->string) {
        size_t len = strlen(src->string) + 1;
        dst->string = (char *)json_alloc(options, len);
        if (!dst->string) {
          json_free_with(options, dst);
          return NULL;
        }
        memcpy(dst->string, src->string, len);
      }
      break;
    }
    case JSON_NUMBER:
      dst->number = src->number;
      break;
    case JSON_BOOL:
      dst->boolean = src->boolean;
      break;
    case JSON_NULL:
      break;
    case JSON_ARRAY: {
      dst->array.count = src->array.count;
      if (src->array.count) {
        dst->array.values =
            (JsonValue **)json_alloc(options,
                                     sizeof(JsonValue *) * src->array.count);
        if (!dst->array.values) {
          json_free_with(options, dst);
          return NULL;
        }
        for (u32 i = 0; i < src->array.count; ++i) {
          dst->array.values[i] = json_clone_value(src->array.values[i], options);
          if (!dst->array.values[i]) {
            json_free_with(options, dst->array.values);
            json_free_with(options, dst);
            return NULL;
          }
        }
      }
      break;
    }
    case JSON_OBJECT: {
      dst->object.count = src->object.count;
      if (src->object.count) {
        dst->object.keys = (char **)json_alloc(
            options, sizeof(char *) * src->object.count);
        dst->object.values = (JsonValue **)json_alloc(
            options, sizeof(JsonValue *) * src->object.count);
        if (!dst->object.keys || !dst->object.values) {
          json_free_with(options, dst->object.keys);
          json_free_with(options, dst->object.values);
          json_free_with(options, dst);
          return NULL;
        }
        for (u32 i = 0; i < src->object.count; ++i) {
          const char *key = src->object.keys[i] ? src->object.keys[i] : "";
          size_t len = strlen(key) + 1;
          dst->object.keys[i] = (char *)json_alloc(options, len);
          if (!dst->object.keys[i]) {
            json_free_with(options, dst);
            return NULL;
          }
          memcpy(dst->object.keys[i], key, len);
          dst->object.values[i] =
              json_clone_value(src->object.values[i], options);
          if (!dst->object.values[i]) {
            json_free_with(options, dst);
            return NULL;
          }
        }
      }
      break;
    }
  }

  return dst;
}

static bool json_walk_sax(const JsonValue *value, const JsonSaxHandler *handler,
                          void *user) {
  if (!value || !handler) {
    return false;
  }

  switch (value->type) {
    case JSON_OBJECT:
      if (handler->start_object && !handler->start_object(user)) {
        return false;
      }
      for (u32 i = 0; i < value->object.count; ++i) {
        if (handler->key && !handler->key(value->object.keys[i], user)) {
          return false;
        }
        if (!json_walk_sax(value->object.values[i], handler, user)) {
          return false;
        }
      }
      if (handler->end_object && !handler->end_object(user)) {
        return false;
      }
      return true;
    case JSON_ARRAY:
      if (handler->start_array && !handler->start_array(user)) {
        return false;
      }
      for (u32 i = 0; i < value->array.count; ++i) {
        if (!json_walk_sax(value->array.values[i], handler, user)) {
          return false;
        }
      }
      if (handler->end_array && !handler->end_array(user)) {
        return false;
      }
      return true;
    case JSON_STRING:
      return handler->string ? handler->string(value->string, user) : true;
    case JSON_NUMBER:
      return handler->number ? handler->number(value->number, user) : true;
    case JSON_BOOL:
      return handler->boolean ? handler->boolean(value->boolean, user) : true;
    case JSON_NULL:
      return handler->null_value ? handler->null_value(user) : true;
  }

  return true;
}

static const char *json_scan_string_simd(const char *cursor) {
  const char *ptr = cursor;
  while (*ptr) {
    const char *quote = memchr(ptr, '"', strlen(ptr));
    if (!quote) {
      return NULL;
    }
    size_t backslashes = 0;
    const char *scan = quote;
    while (scan > cursor && *(scan - 1) == '\\') {
      backslashes++;
      scan--;
    }
    if ((backslashes % 2) == 0) {
      return quote;
    }
    ptr = quote + 1;
  }
  return NULL;
}

static bool json_fast_number(const char *start, size_t length, double *out) {
  char buffer[64];
  if (length >= sizeof(buffer)) {
    return false;
  }
  memcpy(buffer, start, length);
  buffer[length] = '\0';

  char *end = NULL;
  double value = strtod(buffer, &end);
  if (!end || end == buffer || end != buffer + length) {
    return false;
  }

  if (out) {
    *out = value;
  }
  return true;
}

static void json_quick_scan(const char *text) {
  const char *cursor = text;
  while (cursor && *cursor) {
    if (*cursor == '"') {
      const char *end = json_scan_string_simd(cursor + 1);
      if (!end) {
        break;
      }
      cursor = end + 1;
    } else if (*cursor == '-' || isdigit((unsigned char)*cursor)) {
      const char *start = cursor;
      while (*cursor && (isdigit((unsigned char)*cursor) || *cursor == '-' ||
                         *cursor == '+' || *cursor == '.' || *cursor == 'e' ||
                         *cursor == 'E')) {
        cursor++;
      }
      json_fast_number(start, (size_t)(cursor - start), NULL);
    } else {
      cursor++;
    }
  }
}

JsonValue *json_parse_dom(const char *json_text,
                          const JsonParserOptions *options) {
  if (!json_text) {
    return NULL;
  }

  json_quick_scan(json_text);

  JsonValue *root = json_parse(json_text);
  if (!root) {
    return NULL;
  }

  if (options && options->validate_unicode) {
    if (!json_validate_unicode(root)) {
      json_free(root);
      return NULL;
    }
  }

  if (options && options->alloc) {
    JsonValue *clone = json_clone_value(root, options);
    json_free(root);
    return clone;
  }

  return root;
}

bool json_parse_sax(const char *json_text, const JsonSaxHandler *handler,
                    void *user, const JsonParserOptions *options) {
  if (!handler) {
    return false;
  }

  JsonValue *root = json_parse_dom(json_text, options);
  if (!root) {
    return false;
  }

  bool ok = json_walk_sax(root, handler, user);

  if (options && options->alloc && !options->free) {
    return ok;
  }

  if (options && options->alloc) {
    json_free_with(options, root);
  } else {
    json_free(root);
  }

  return ok;
}

static bool json_serialize_value(const JsonValue *value, StringBuilder *sb);

static bool json_escape_and_append(const char *text, StringBuilder *sb) {
  if (!sb_append_char(sb, '"')) {
    return false;
  }
  for (const char *ptr = text; ptr && *ptr; ++ptr) {
    switch (*ptr) {
      case '"':
        if (!sb_append(sb, "\\\"")) {
          return false;
        }
        break;
      case '\\':
        if (!sb_append(sb, "\\\\")) {
          return false;
        }
        break;
      case '\n':
        if (!sb_append(sb, "\\n")) {
          return false;
        }
        break;
      case '\r':
        if (!sb_append(sb, "\\r")) {
          return false;
        }
        break;
      case '\t':
        if (!sb_append(sb, "\\t")) {
          return false;
        }
        break;
      default:
        if ((unsigned char)*ptr < 0x20) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)*ptr);
          if (!sb_append(sb, buf)) {
            return false;
          }
        } else {
          if (!sb_append_char(sb, *ptr)) {
            return false;
          }
        }
        break;
    }
  }
  return sb_append_char(sb, '"');
}

static bool json_serialize_value(const JsonValue *value, StringBuilder *sb) {
  if (!value) {
    return sb_append(sb, "null");
  }

  char buffer[64];
  switch (value->type) {
    case JSON_NULL:
      return sb_append(sb, "null");
    case JSON_BOOL:
      return sb_append(sb, value->boolean ? "true" : "false");
    case JSON_NUMBER:
      snprintf(buffer, sizeof(buffer), "%.17g", value->number);
      return sb_append(sb, buffer);
    case JSON_STRING:
      return json_escape_and_append(value->string ? value->string : "", sb);
    case JSON_ARRAY:
      if (!sb_append_char(sb, '[')) {
        return false;
      }
      for (u32 i = 0; i < value->array.count; ++i) {
        if (i > 0 && !sb_append_char(sb, ',')) {
          return false;
        }
        if (!json_serialize_value(value->array.values[i], sb)) {
          return false;
        }
      }
      return sb_append_char(sb, ']');
    case JSON_OBJECT:
      if (!sb_append_char(sb, '{')) {
        return false;
      }
      for (u32 i = 0; i < value->object.count; ++i) {
        if (i > 0 && !sb_append_char(sb, ',')) {
          return false;
        }
        if (!json_escape_and_append(value->object.keys[i], sb)) {
          return false;
        }
        if (!sb_append_char(sb, ':')) {
          return false;
        }
        if (!json_serialize_value(value->object.values[i], sb)) {
          return false;
        }
      }
      return sb_append_char(sb, '}');
  }

  return false;
}

char *json_serialize(const JsonValue *value,
                     const JsonParserOptions *options) {
  StringBuilder sb = {0};
  if (!json_serialize_value(value, &sb)) {
    free(sb.data);
    return NULL;
  }

  if (options && options->alloc) {
    char *out = (char *)options->alloc(sb.length + 1, options->user);
    if (!out) {
      free(sb.data);
      return NULL;
    }
    memcpy(out, sb.data, sb.length + 1);
    free(sb.data);
    return out;
  }

  return sb.data;
}

bool json_parser_self_test(void) {
  const char *sample = "{\"a\":1,\"b\":[true,false,null],\"c\":\"ok\"}";
  JsonValue *root = json_parse_dom(sample, NULL);
  if (!root) {
    return false;
  }

  char *text = json_serialize(root, NULL);
  if (!text) {
    json_free(root);
    return false;
  }

  free(text);
  json_free(root);

  char fuzz[64];
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 63; ++j) {
      fuzz[j] = (char)(' ' + (rand() % 90));
    }
    fuzz[63] = '\0';
    JsonValue *tmp = json_parse_dom(fuzz, NULL);
    if (tmp) {
      json_free(tmp);
    }
  }

  return true;
}
