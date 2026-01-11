#ifndef EDITOR_UNDO_COMMAND_HISTORY_H
#define EDITOR_UNDO_COMMAND_HISTORY_H

#include "core/core.h"
#include "core/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                        COMMAND PATTERN UNDO SYSTEM - AGENT_EDITOR_2
// =================================================================================================

// Maximum command history size
#define MAX_COMMAND_HISTORY 100

// Command execution result
typedef enum {
    COMMAND_SUCCESS,
    COMMAND_FAILURE,
    COMMAND_PARTIAL_SUCCESS
} CommandResult;

// Command interface
typedef struct Command {
    u32 command_id;
    char command_name[128];
    char description[256];
    
    // Command state
    bool can_undo;
    bool can_redo;
    bool is_executed;
    u64 execution_time;
    
    // Command data (opaque pointer to command-specific data)
    void* command_data;
    size_t command_data_size;
    
    // Virtual functions
    CommandResult (*execute)(struct Command* command);
    CommandResult (*undo)(struct Command* command);
    CommandResult (*redo)(struct Command* command);
    void (*cleanup)(struct Command* command);
    bool (*can_merge_with)(struct Command* command, struct Command* other);
    CommandResult (*merge_with)(struct Command* command, struct Command* other);
    
    // Metadata
    u64 created_time;
    u64 modified_time;
    u32 merge_count;
} Command;

// Command history system
typedef struct {
    Command* history[MAX_COMMAND_HISTORY];
    u32 history_size;
    i32 current_index;        // -1 means no current command
    
    // Statistics
    u32 total_commands;
    u32 undo_count;
    u32 redo_count;
    u32 merge_count;
    f64 total_execution_time;
    f64 total_undo_time;
    f64 total_redo_time;
    
    // Configuration
    u32 max_history_size;
    bool auto_merge;
    bool enable_statistics;
    
    // Callbacks
    void (*command_executed_callback)(Command* command, void* user_data);
    void (*command_undone_callback)(Command* command, void* user_data);
    void (*command_redone_callback)(Command* command, void* user_data);
    void* callback_user_data;
    
    // Memory management
    Allocator* command_allocator;
    
    bool is_initialized;
} CommandHistory;

// =================================================================================================
//                                    CORE API FUNCTIONS
// =================================================================================================

// System management
CommandHistory* command_history_create(u32 max_history_size);
void command_history_destroy(CommandHistory* history);
bool command_history_initialize(CommandHistory* history);

// Command execution
CommandResult command_history_execute(CommandHistory* history, Command* command);
CommandResult command_history_undo(CommandHistory* history);
CommandResult command_history_redo(CommandHistory* history);
CommandResult command_history_undo_multiple(CommandHistory* history, u32 count);
CommandResult command_history_redo_multiple(CommandHistory* history, u32 count);

// History management
void command_history_clear(CommandHistory* history);
void command_history_clear_future(CommandHistory* history);
bool command_history_can_undo(CommandHistory* history);
bool command_history_can_redo(CommandHistory* history);
u32 command_history_get_undo_count(CommandHistory* history);
u32 command_history_get_redo_count(CommandHistory* history);

// Command access
Command* command_history_get_current_command(CommandHistory* history);
Command* command_history_get_command_at(CommandHistory* history, u32 index);
Command* command_history_get_last_command(CommandHistory* history);

// Configuration
void command_history_set_max_size(CommandHistory* history, u32 max_size);
void command_history_enable_auto_merge(CommandHistory* history, bool enable);
void command_history_enable_statistics(CommandHistory* history, bool enable);

// Callbacks
void command_history_set_executed_callback(CommandHistory* history,
                                           void (*callback)(Command*, void*),
                                           void* user_data);
void command_history_set_undone_callback(CommandHistory* history,
                                         void (*callback)(Command*, void*),
                                         void* user_data);
void command_history_set_redone_callback(CommandHistory* history,
                                         void (*callback)(Command*, void*),
                                         void* user_data);

// =================================================================================================
//                                COMMAND CREATION
// =================================================================================================

// Base command creation
Command* command_create(const char* name, const char* description,
                       CommandResult (*execute_func)(Command*),
                       CommandResult (*undo_func)(Command*),
                       CommandResult (*redo_func)(Command*),
                       void (*cleanup_func)(Command*));

// Command utilities
void command_set_data(Command* command, void* data, size_t size);
void* command_get_data(Command* command);
void command_set_mergeable(Command* command, bool can_merge);
bool command_is_mergeable(Command* command);

// =================================================================================================
//                            BUILT-IN COMMAND TYPES
// =================================================================================================

// Transform command
typedef struct {
    u32 entity_id;
    Vec3 old_position;
    Vec3 new_position;
    Vec3 old_rotation;
    Vec3 new_rotation;
    Vec3 old_scale;
    Vec3 new_scale;
} TransformCommandData;

Command* create_transform_command(u32 entity_id,
                                Vec3 old_pos, Vec3 new_pos,
                                Vec3 old_rot, Vec3 new_rot,
                                Vec3 old_scale, Vec3 new_scale);

// Property command
typedef struct {
    u32 entity_id;
    char property_name[64];
    char old_value[256];
    char new_value[256];
    u32 value_type;  // Enum for property types
} PropertyCommandData;

Command* create_property_command(u32 entity_id,
                                 const char* property_name,
                                 const char* old_value,
                                 const char* new_value,
                                 u32 value_type);

// Create/delete command
typedef struct {
    u32 entity_id;
    char entity_name[128];
    void* entity_data;
    size_t entity_data_size;
    bool is_create;  // true for create, false for delete
} CreateDeleteCommandData;

Command* create_create_command(u32 entity_id, const char* entity_name, 
                              void* entity_data, size_t data_size);
Command* create_delete_command(u32 entity_id, const char* entity_name,
                              void* entity_data, size_t data_size);

// Multi-command (composite)
typedef struct {
    Command** commands;
    u32 command_count;
    bool execute_all_succeeds;
} MultiCommandData;

Command* create_multi_command(Command** commands, u32 count, bool all_must_succeed);

// =================================================================================================
//                                COMMAND UTILITIES
// =================================================================================================

// Command validation
bool command_validate(Command* command);
bool command_is_valid(Command* command);

// Command comparison
bool command_equals(Command* cmd1, Command* cmd2);
bool command_is_similar(Command* cmd1, Command* cmd2);

// Command serialization
bool command_serialize(Command* command, char* buffer, size_t buffer_size);
Command* command_deserialize(const char* buffer, size_t buffer_size);

// Command debugging
void command_print_info(Command* command);
void command_print_data(Command* command);

// =================================================================================================
//                                STATISTICS
// =================================================================================================

void command_history_get_statistics(CommandHistory* history,
                                   u32* total_commands,
                                   u32* undo_count,
                                   u32* redo_count,
                                   u32* merge_count,
                                   f64* avg_execution_time,
                                   f64* avg_undo_time,
                                   f64* avg_redo_time);

void command_history_print_statistics(CommandHistory* history);
void command_history_reset_statistics(CommandHistory* history);

// =================================================================================================
//                                MEMORY MANAGEMENT
// =================================================================================================

// Command allocation
void* command_allocate(CommandHistory* history, size_t size);
void command_deallocate(CommandHistory* history, void* ptr);

// Command pool management
void command_history_set_allocator(CommandHistory* history, Allocator* allocator);
Allocator* command_history_get_allocator(CommandHistory* history);

// =================================================================================================
//                                DEBUG UTILITIES
// =================================================================================================

void command_history_print_history(CommandHistory* history);
void command_history_validate_integrity(CommandHistory* history);
bool command_history_is_healthy(CommandHistory* history);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_UNDO_COMMAND_HISTORY_H
