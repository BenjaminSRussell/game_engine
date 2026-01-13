/*
 * string_utils.c
 * Utility string helpers implementation
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 * 
 * Provides string manipulation, formatting, and utility functions
 */

#include <core/string_utils.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *string_duplicate(const char *str) {
  if (!str)
    return NULL;
  char *dup = (char *)malloc(strlen(str) + 1);
  if (dup)
    strcpy(dup, str);
  return dup;
}

char *string_concat(const char *str1, const char *str2) {
  if (!str1 || !str2)
    return NULL;
  u32 len = strlen(str1) + strlen(str2) + 1;
  char *result = (char *)malloc(len);
  if (result) {
    strcpy(result, str1);
    strcat(result, str2);
  }
  return result;
}

char *string_concat_n(const char **strings, u32 count) {
  if (!strings || count == 0)
    return NULL;

  u32 total_len = 0;
  for (u32 i = 0; i < count; i++) {
    if (strings[i])
      total_len += strlen(strings[i]);
  }

  char *result = (char *)malloc(total_len + 1);
  if (!result)
    return NULL;

  result[0] = '\0';
  for (u32 i = 0; i < count; i++) {
    if (strings[i])
      strcat(result, strings[i]);
  }

  return result;
}

char *string_format(const char *fmt, ...) {
  if (!fmt)
    return NULL;

  va_list args;
  va_start(args, fmt);

  u32 size = 256;
  char *buffer = (char *)malloc(size);

  while (buffer) {
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(buffer, size, fmt, args_copy);
    va_end(args_copy);

    if (needed < (int)size)
      break;

    size = needed + 1;
    char *new_buffer = (char *)realloc(buffer, size);
    if (!new_buffer) {
      free(buffer);
      va_end(args);
      return NULL;
    }
    buffer = new_buffer;
  }

  va_end(args);
  return buffer;
}

bool string_equals(const char *str1, const char *str2) {
  if (!str1 || !str2)
    return str1 == str2;
  return strcmp(str1, str2) == 0;
}

bool string_equals_case_insensitive(const char *str1, const char *str2) {
  if (!str1 || !str2)
    return str1 == str2;
  while (*str1 && *str2) {
    if (tolower(*str1) != tolower(*str2))
      return false;
    str1++;
    str2++;
  }
  return *str1 == *str2;
}

char *string_substring(const char *str, u32 start, u32 length) {
  if (!str || start >= strlen(str))
    return NULL;

  char *result = (char *)malloc(length + 1);
  if (!result)
    return NULL;

  strncpy(result, str + start, length);
  result[length] = '\0';

  return result;
}

i32 string_find_char(const char *str, char c) {
  if (!str)
    return -1;
  const char *pos = strchr(str, c);
  return pos ? (i32)(pos - str) : -1;
}

i32 string_find_substring(const char *str, const char *substr) {
  if (!str || !substr)
    return -1;
  const char *pos = strstr(str, substr);
  return pos ? (i32)(pos - str) : -1;
}

char **string_split(const char *str, const char *delim, u32 *out_count) {
  if (!str || !out_count || !delim)
    return NULL;

  // First pass: count tokens
  u32 count = 0;
  char *temp_str = string_duplicate(str);
  if (!temp_str)
    return NULL;

  char *token = strtok(temp_str, delim);
  while (token) {
    count++;
    token = strtok(NULL, delim);
  }
  free(temp_str);

  // Allocate result array
  char **result = (char **)malloc(sizeof(char *) * count);
  if (!result) {
    *out_count = 0;
    return NULL;
  }

  // Second pass: store tokens
  u32 index = 0;
  char *copy = string_duplicate(str);
  if (!copy) {
    free(result);
    *out_count = 0;
    return NULL;
  }

  token = strtok(copy, delim);
  while (token && index < count) {
    result[index++] = string_duplicate(token);
    token = strtok(NULL, delim);
  }

  free(copy);
  *out_count = index;
  return result;
}

char *string_trim(const char *str) {
  if (!str)
    return NULL;

  while (*str && isspace(*str))
    str++;

  u32 len = strlen(str);
  while (len > 0 && isspace(str[len - 1]))
    len--;

  char *result = (char *)malloc(len + 1);
  if (result) {
    strncpy(result, str, len);
    result[len] = '\0';
  }

  return result;
}

void string_to_lower(char *str) {
  if (!str)
    return;
  for (char *p = str; *p; p++) {
    *p = tolower(*p);
  }
}

void string_to_upper(char *str) {
  if (!str)
    return;
  for (char *p = str; *p; p++) {
    *p = toupper(*p);
  }
}

u32 string_length(const char *str) { return str ? strlen(str) : 0; }

u32 string_hash(const char *str) {
  if (!str)
    return 0;

  u32 hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

void string_free_array(char **arr, u32 count) {
  if (!arr)
    return;
  for (u32 i = 0; i < count; i++) {
    if (arr[i])
      free(arr[i]);
  }
  free(arr);
}
