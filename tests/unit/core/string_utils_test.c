/* Unit tests for src/engine/core/string_utils.c (issue #320).
 *
 * Built by tests/registry/test_string_utils.cmake, which compiles this file and
 * the unit directly, so the test links no Engine code.
 *
 * string_utils.h declares functions this unit does not define (for example
 * string_split_free and string_starts_with), and the unit defines functions the
 * header omits. Only the functions the unit defines are tested. The omitted ones
 * are prototyped below.
 *
 * CHECK does not use assert(), so the result does not depend on NDEBUG.
 */

#include <core/string_utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defined in string_utils.c but not declared in string_utils.h. */
char *string_concat(const char *str1, const char *str2);
char *string_concat_n(const char **strings, u32 count);
bool string_equals(const char *str1, const char *str2);
bool string_equals_case_insensitive(const char *str1, const char *str2);
i32 string_find_char(const char *str, char c);
i32 string_find_substring(const char *str, const char *substr);
u32 string_hash(const char *str);
void string_free_array(char **arr, u32 count);

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    g_checks++;                                                                \
    if (!(cond)) {                                                             \
      g_failures++;                                                            \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
    }                                                                          \
  } while (0)

/* True when `got` is non-NULL and equal to `want`. */
static bool same_string(const char *got, const char *want) {
  return got != NULL && want != NULL && strcmp(got, want) == 0;
}

static void test_duplicate(void) {
  char src[] = "hello";
  char *dup = string_duplicate(src);
  CHECK(same_string(dup, "hello"));
  CHECK(dup != src);
  free(dup);

  dup = string_duplicate("");
  CHECK(same_string(dup, ""));
  free(dup);

  CHECK(string_duplicate(NULL) == NULL);
}

static void test_length(void) {
  CHECK(string_length(NULL) == 0);
  CHECK(string_length("") == 0);
  CHECK(string_length("hello") == 5);
}

static void test_substring(void) {
  char *s = string_substring("hello", 1, 3);
  CHECK(same_string(s, "ell"));
  free(s);

  s = string_substring("hello", 0, 5);
  CHECK(same_string(s, "hello"));
  free(s);

  s = string_substring("hello", 4, 1);
  CHECK(same_string(s, "o"));
  free(s);

  s = string_substring("hello", 2, 0);
  CHECK(same_string(s, ""));
  free(s);

  /* A length past the end is clamped to the end of the source string. */
  s = string_substring("hello", 3, 10);
  CHECK(same_string(s, "lo"));
  free(s);

  /* A start at or past the end yields NULL. */
  CHECK(string_substring("hello", 5, 1) == NULL);
  CHECK(string_substring("hello", 9, 1) == NULL);
  CHECK(string_substring(NULL, 0, 1) == NULL);
}

static void test_trim(void) {
  char *t = string_trim("  hello  ");
  CHECK(same_string(t, "hello"));
  free(t);

  t = string_trim("\t\n hi \r\n");
  CHECK(same_string(t, "hi"));
  free(t);

  /* Interior whitespace is kept. */
  t = string_trim("  a b  ");
  CHECK(same_string(t, "a b"));
  free(t);

  t = string_trim("   ");
  CHECK(same_string(t, ""));
  free(t);

  t = string_trim("");
  CHECK(same_string(t, ""));
  free(t);

  CHECK(string_trim(NULL) == NULL);
}

static void test_concat(void) {
  char *c = string_concat("foo", "bar");
  CHECK(same_string(c, "foobar"));
  free(c);

  c = string_concat("", "x");
  CHECK(same_string(c, "x"));
  free(c);

  c = string_concat("x", "");
  CHECK(same_string(c, "x"));
  free(c);

  CHECK(string_concat(NULL, "x") == NULL);
  CHECK(string_concat("x", NULL) == NULL);
}

static void test_concat_n(void) {
  const char *parts[] = {"a", "b", "c"};
  char *c = string_concat_n(parts, 3);
  CHECK(same_string(c, "abc"));
  free(c);

  /* NULL entries are skipped. */
  const char *with_null[] = {"a", NULL, "b"};
  c = string_concat_n(with_null, 3);
  CHECK(same_string(c, "ab"));
  free(c);

  CHECK(string_concat_n(parts, 0) == NULL);
  CHECK(string_concat_n(NULL, 3) == NULL);
}

static void test_format(void) {
  char *f = string_format("%d-%s", 42, "x");
  CHECK(same_string(f, "42-x"));
  free(f);

  f = string_format("plain text");
  CHECK(same_string(f, "plain text"));
  free(f);

  CHECK(string_format(NULL) == NULL);

  /* Output at and beyond the 256-byte first buffer exercises the resize path. */
  static char big[400];
  const size_t lengths[] = {255, 256, 257, 300};
  for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
    memset(big, 'a', lengths[i]);
    big[lengths[i]] = '\0';
    f = string_format("%s", big);
    CHECK(f != NULL && strlen(f) == lengths[i] && strcmp(f, big) == 0);
    free(f);
  }
}

static void test_equals(void) {
  CHECK(string_equals("abc", "abc"));
  CHECK(!string_equals("abc", "ABC"));
  CHECK(!string_equals("abc", "abcd"));
  CHECK(!string_equals("abcd", "abc"));
  CHECK(string_equals("", ""));
  CHECK(string_equals(NULL, NULL));
  CHECK(!string_equals(NULL, "a"));
  CHECK(!string_equals("a", NULL));
}

static void test_equals_case_insensitive(void) {
  CHECK(string_equals_case_insensitive("Hello", "hELLO"));
  CHECK(!string_equals_case_insensitive("abc", "abd"));
  CHECK(!string_equals_case_insensitive("abc", "ab"));
  CHECK(!string_equals_case_insensitive("ab", "abc"));
  CHECK(string_equals_case_insensitive("", ""));
  CHECK(string_equals_case_insensitive(NULL, NULL));
  CHECK(!string_equals_case_insensitive(NULL, ""));
}

static void test_find(void) {
  CHECK(string_find_char("hello", 'l') == 2);
  CHECK(string_find_char("hello", 'h') == 0);
  CHECK(string_find_char("hello", 'z') == -1);
  CHECK(string_find_char("", 'a') == -1);
  CHECK(string_find_char(NULL, 'a') == -1);

  CHECK(string_find_substring("hello world", "world") == 6);
  /* The whole substring must match, not just its first character. */
  CHECK(string_find_substring("aab", "ab") == 1);
  CHECK(string_find_substring("hello", "hello") == 0);
  CHECK(string_find_substring("hello", "xyz") == -1);
  CHECK(string_find_substring(NULL, "a") == -1);
  CHECK(string_find_substring("a", NULL) == -1);
}

static void test_hash(void) {
  /* djb2 with seed 5381. */
  CHECK(string_hash(NULL) == 0);
  CHECK(string_hash("") == 5381u);
  CHECK(string_hash("a") == 177670u);
  CHECK(string_hash("abc") == 193485963u);
  /* Order matters. */
  CHECK(string_hash("abc") != string_hash("acb"));
}

static void test_case_conversion(void) {
  char s[] = "HeLLo 123!";
  string_to_lower(s);
  CHECK(same_string(s, "hello 123!"));
  string_to_upper(s);
  CHECK(same_string(s, "HELLO 123!"));

  /* NULL is accepted and ignored. */
  string_to_lower(NULL);
  string_to_upper(NULL);
}

static void test_split(void) {
  u32 count = 0;
  char **parts = string_split("a,b,c", ",", &count);
  CHECK(parts != NULL && count == 3);
  if (parts && count == 3) {
    CHECK(same_string(parts[0], "a"));
    CHECK(same_string(parts[1], "b"));
    CHECK(same_string(parts[2], "c"));
  }
  string_free_array(parts, count);

  parts = string_split("key=value", "=", &count);
  CHECK(parts != NULL && count == 2);
  if (parts && count == 2) {
    CHECK(same_string(parts[0], "key"));
    CHECK(same_string(parts[1], "value"));
  }
  string_free_array(parts, count);

  parts = string_split("hello", ",", &count);
  CHECK(parts != NULL && count == 1);
  if (parts && count == 1) {
    CHECK(same_string(parts[0], "hello"));
  }
  string_free_array(parts, count);

  /* The input string is not modified. */
  char input[] = "x,y";
  parts = string_split(input, ",", &count);
  CHECK(strcmp(input, "x,y") == 0);
  string_free_array(parts, count);

  /* Invalid arguments return NULL. */
  CHECK(string_split(NULL, ",", &count) == NULL);
  CHECK(string_split("a,b", NULL, &count) == NULL);
  CHECK(string_split("a,b", ",", NULL) == NULL);

  string_free_array(NULL, 0);
}

int main(void) {
  test_duplicate();
  test_length();
  test_substring();
  test_trim();
  test_concat();
  test_concat_n();
  test_format();
  test_equals();
  test_equals_case_insensitive();
  test_find();
  test_hash();
  test_case_conversion();
  test_split();

  if (g_failures != 0) {
    fprintf(stderr, "string_utils: %d of %d checks failed\n", g_failures,
            g_checks);
    return 1;
  }
  printf("string_utils: %d checks passed\n", g_checks);
  return 0;
}
