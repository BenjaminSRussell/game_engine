#ifndef ENGINE_AI_API_H
#define ENGINE_AI_API_H

#include <stddef.h>

/**
 * Initialize the AI API subsystem.
 * This registers standard command handlers and initializes the introspection registry.
 */
void ai_api_init(void);

/**
 * Execute a JSON-formatted command from an AI agent.
 * 
 * @param json_command  Null-terminated JSON string containing the command
 * @param json_response Buffer to write the null-terminated JSON response
 * @param response_size Size of the response buffer
 * @return 0 on success, non-zero on error
 *
 * Example Command:
 * {
 *   "command": "query_types",
 *   "params": {}
 * }
 *
 * Example Response:
 * {
 *   "status": "ok",
 *   "data": [ ... ]
 * }
 */
int ai_api_execute_command(const char* json_command, char* json_response, size_t response_size);

#endif // ENGINE_AI_API_H
