#ifndef VM_TYPES_H
#define VM_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Value types
typedef enum {
  VAL_NULL,
  VAL_BOOL,
  VAL_NUMBER, // float
  VAL_OBJ     // Pointer to heap object
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    float number;
    void *obj;
  } as;
} Value;

// Helpers for Value creation
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = (float)(value)}})
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = (value)}})
#define NIL_VAL ((Value){VAL_NULL, {.number = 0}})

#endif // VM_TYPES_H
