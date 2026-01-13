#ifndef COMMAND_HISTORY_H
#define COMMAND_HISTORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward decls
typedef struct Allocator Allocator;

// Basic types if common.h not available
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef double f64;

#define MAX_COMMAND_HISTORY 1024

typedef enum {
    COMMAND_SUCCESS = 0,
    COMMAND_FAILURE = 1
} CommandResult;

typedef struct Command Command;

struct Command {
    char command_name[64];
    char description[256];
    uint32_t command_id;
    bool can_undo;
    bool can_redo;
    bool is_executed;
    
    uint32_t merge_count;
    uint64_t execution_time;
    uint64_t created_time;
    uint64_t modified_time;
    
    void *command_data;
    size_t command_data_size;
    
    CommandResult (*execute)(Command*);
    CommandResult (*undo)(Command*);
    CommandResult (*redo)(Command*);
    bool (*can_merge_with)(Command*, Command*);
    CommandResult (*merge_with)(Command*, Command*);
    void (*cleanup)(Command*);
};

typedef struct CommandHistory {
    u32 max_history_size;
    i32 current_index;
    bool auto_merge;
    bool enable_statistics;
    
    Allocator* command_allocator;
    Command* history[MAX_COMMAND_HISTORY];
    u32 history_size;
    
    u32 total_commands;
    f64 total_execution_time;
    u32 merge_count;
    u32 undo_count;
    f64 total_undo_time;
    u32 redo_count;
    f64 total_redo_time;
    
    void (*command_executed_callback)(Command*, void*);
    void (*command_undone_callback)(Command*, void*);
    void (*command_redone_callback)(Command*, void*);
    void* callback_user_data;
    
    bool is_initialized;
} CommandHistory;

#include "math/vec3.h"

// Transform Command Data
typedef struct {
    u32 entity_id;
    Vec3 old_position;
    Vec3 new_position;
    Vec3 old_rotation;
    Vec3 new_rotation;
    Vec3 old_scale;
    Vec3 new_scale;
} TransformCommandData;

// Property Command Data
typedef struct {
    u32 entity_id;
    char property_name[64];
    char old_value[256];
    char new_value[256];
    u32 value_type;
} PropertyCommandData;

// API
CommandHistory* command_history_create(u32 max_history_size);
void command_history_destroy(CommandHistory* history);
bool command_history_initialize(CommandHistory* history);
void command_history_clear(CommandHistory* history);
void command_history_clear_future(CommandHistory* history);

CommandResult command_history_execute(CommandHistory* history, Command* command);
CommandResult command_history_undo(CommandHistory* history);
CommandResult command_history_redo(CommandHistory* history);

bool command_history_can_undo(CommandHistory* history);
bool command_history_can_redo(CommandHistory* history);

void command_history_get_statistics(CommandHistory* history,
                                   u32* total_commands,
                                   u32* undo_count,
                                   u32* redo_count,
                                   u32* merge_count,
                                   f64* avg_execution_time,
                                   f64* avg_undo_time,
                                   f64* avg_redo_time);

void command_history_print_statistics(CommandHistory* history);
void command_print_info(Command* command);

// Helper constructors
Command* command_create(const char* name, const char* description,
                       CommandResult (*execute_func)(Command*),
                       CommandResult (*undo_func)(Command*),
                       CommandResult (*redo_func)(Command*),
                       void (*cleanup_func)(Command*));
                       
#endif // COMMAND_HISTORY_H
