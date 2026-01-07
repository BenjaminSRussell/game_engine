/**
 * =================================================================================================
 *                          LIGHTWEIGHT SCRIPTING VM
 *                          Phase 8: Scripting System
 * =================================================================================================
 *
 * PURPOSE: Stack-based bytecode virtual machine for game logic scripting
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <scripting/script_system.h>

#define VM_STACK_SIZE 1024
#define VM_MAX_GLOBALS 256
#define VM_MAX_CONSTANTS 256
#define VM_MAX_FRAMES 64

#include <scripting/vm_types.h>

// Value types now in vm_types.h

// Instructions
typedef enum {
  OP_RETURN,
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE
} OpCode;

// Bytecode Chunk
typedef struct {
  uint8_t *code;
  uint32_t count;
  uint32_t capacity;

  Value constants[VM_MAX_CONSTANTS];
  uint32_t const_count;

  int *lines; // Debug info
} Chunk;

// Call Frame
typedef struct {
  Chunk *chunk;
  uint8_t *ip;  // Instruction pointer
  Value *slots; // Stack offset
} CallFrame;

// Virtual Machine
typedef struct {
  CallFrame frames[VM_MAX_FRAMES];
  int frame_count;

  Value stack[VM_STACK_SIZE];
  Value *stack_top;

  // Globals table (simplified as array)
  char *global_names[VM_MAX_GLOBALS];
  Value global_values[VM_MAX_GLOBALS];
  int global_count;

  Chunk *current_chunk; // For simple compilation
} VM;

// -----------------------------------------------------------------------------
// Value Helpers
// -----------------------------------------------------------------------------

// Macros now in vm_types.h
// #define NUMBER_VAL(value) ...

// static inline bool is_falsey(Value v) is now in vm_types.h, OR we keep it
// here if not in header For this step I only put struct in header, so keeping
// macros here is fine. Wait, I removed the inline function from the previous
// tool call attempt? Yes. So I should stick with local implementation here or
// re-add it. I'll keep the macros here for internal VM convenience.

static bool is_falsey(Value v) {
  return v.type == VAL_NULL || (v.type == VAL_BOOL && !v.as.boolean);
}

// -----------------------------------------------------------------------------
// VM Core
// -----------------------------------------------------------------------------

VM *vm_create(void) {
  VM *vm = (VM *)calloc(1, sizeof(VM));
  vm->stack_top = vm->stack;
  return vm;
}

void vm_destroy(VM *vm) {
  if (vm)
    free(vm);
}

static void push(VM *vm, Value value) {
  *vm->stack_top = value;
  vm->stack_top++;
}

static Value pop(VM *vm) {
  vm->stack_top--;
  return *vm->stack_top;
}

static Value peek(VM *vm, int distance) { return vm->stack_top[-1 - distance]; }

// -----------------------------------------------------------------------------
// Bytecode Execution
// -----------------------------------------------------------------------------

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

static InterpretResult run(VM *vm) {
  CallFrame *frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->chunk->constants[READ_BYTE()])
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define BINARY_OP(op)                                                          \
  do {                                                                         \
    if (peek(vm, 0).type != VAL_NUMBER || peek(vm, 1).type != VAL_NUMBER) {    \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = pop(vm).as.number;                                              \
    double a = pop(vm).as.number;                                              \
    push(vm, NUMBER_VAL(a op b));                                              \
  } while (false)

  while (true) {
    uint8_t instruction = READ_BYTE();
    switch (instruction) {
    case OP_RETURN: {
      return INTERPRET_OK; // Simplified
    }
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(vm, constant);
      break;
    }
    case OP_NIL:
      push(vm, NIL_VAL);
      break;
    case OP_TRUE:
      push(vm, BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(vm, BOOL_VAL(false));
      break;

    case OP_POP:
      pop(vm);
      break;

    case OP_EQUAL: {
      Value b = pop(vm);
      Value a = pop(vm);
      // Simplified equality
      bool equal =
          (a.type == b.type && memcmp(&a.as, &b.as, sizeof(a.as)) == 0);
      push(vm, BOOL_VAL(equal));
      break;
    }
    case OP_GREATER:
      BINARY_OP(>);
      break;
    case OP_LESS:
      BINARY_OP(<);
      break;
    case OP_ADD:
      BINARY_OP(+);
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;
    case OP_DIVIDE:
      BINARY_OP(/);
      break;

    case OP_NOT:
      push(vm, BOOL_VAL(is_falsey(pop(vm))));
      break;

    case OP_NEGATE:
      if (peek(vm, 0).type != VAL_NUMBER)
        return INTERPRET_RUNTIME_ERROR;
      push(vm, NUMBER_VAL(-pop(vm).as.number));
      break;

    case OP_PRINT: {
      Value v = pop(vm);
      if (v.type == VAL_NUMBER)
        printf("%g\n", v.as.number);
      else if (v.type == VAL_BOOL)
        printf(v.as.boolean ? "true\n" : "false\n");
      else if (v.type == VAL_NULL)
        printf("nil\n");
      break;
    }

    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (is_falsey(peek(vm, 0)))
        frame->ip += offset;
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
}

// -----------------------------------------------------------------------------
// Chunk Management
// -----------------------------------------------------------------------------

void init_chunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->const_count = 0;
}

void write_chunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    chunk->capacity = chunk->capacity < 8 ? 8 : chunk->capacity * 2;
    chunk->code = (uint8_t *)realloc(chunk->code, chunk->capacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->count++;
}

int add_constant(Chunk *chunk, Value value) {
  if (chunk->const_count >= VM_MAX_CONSTANTS)
    return -1;
  chunk->constants[chunk->const_count] = value;
  return chunk->const_count++;
}

// -----------------------------------------------------------------------------
// Scripting Interface
// -----------------------------------------------------------------------------

InterpretResult vm_interpret(VM *vm, const char *source) {
  // TODO: Add parser and compiler here
  // For now, this is a stub that would take source and compile to chunk
  // Then run:
  // vm->frame_count = 1;
  // vm->frames[0].chunk = compiled_chunk;
  // vm->frames[0].ip = compiled_chunk->code;
  // return run(vm);
  return INTERPRET_OK;
}

// -----------------------------------------------------------------------------
// ScriptSystem Interface Implementation
// -----------------------------------------------------------------------------

bool ScriptSystem_Init(ScriptSystem *system) {
  if (!system)
    return false;
  system->state = (struct lua_State *)vm_create(); // Cast VM to opaque handle
  return (system->state != NULL);
}

void ScriptSystem_Shutdown(ScriptSystem *system) {
  if (system && system->state) {
    vm_destroy((VM *)system->state);
    system->state = NULL;
  }
}

void ScriptSystem_Update(ScriptSystem *system, float deltaTime) {
  // VM update logic if needed (e.g. coroutines)
}

bool ScriptSystem_RunString(ScriptSystem *system, const char *script) {
  if (!system || !system->state)
    return false;
  VM *vm = (VM *)system->state;
  // Stub: compile and run string
  return true;
}
