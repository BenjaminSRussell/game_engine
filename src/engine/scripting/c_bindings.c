/**
 * =================================================================================================
 *                          SCRIPT C BINDINGS
 *                          Phase 8: Scripting System
 * =================================================================================================
 *
 * PURPOSE: Bind native C functions to the Scripting VM
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <scripting/vm_types.h>

// Assumes VMState or similar context is passed, but for now simple bindings
typedef Value ScriptValue; // Alias for compatibility with existing stubs if
                           // needed, or just replace usage
typedef struct {
  Value *stack;
  int stack_top;
} VMState;

typedef Value (*NativeFn)(int arg_count, Value *args);

#define MAX_NATIVES 128

typedef struct {
  char name[32];
  NativeFn function;
} NativeEntry;

static NativeEntry native_table[MAX_NATIVES];
static int native_count = 0;

void script_register_native(const char *name, NativeFn function) {
  if (native_count >= MAX_NATIVES)
    return;
  strncpy(native_table[native_count].name, name, 31);
  native_table[native_count].function = function;
  native_count++;
}

NativeFn script_find_native(const char *name) {
  for (int i = 0; i < native_count; i++) {
    if (strcmp(native_table[i].name, name) == 0) {
      return native_table[i].function;
    }
  }
  return NULL;
}

// Example Native: Print
Value native_print(int arg_count, Value *args) {
  for (int i = 0; i < arg_count; i++) {
    Value v = args[i];
    switch (v.type) {
    case VAL_NUMBER:
      printf("%g ", v.as.number);
      break;
    case VAL_BOOL:
      printf(v.as.boolean ? "true " : "false ");
      break;
    case VAL_NULL:
      printf("nil ");
      break;
    case VAL_OBJ:
      printf("<obj> ");
      break;
    }
  }
  printf("\n");
  return NIL_VAL;
}

void script_init_bindings(void) {
  script_register_native("print", native_print);
}
