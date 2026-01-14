#ifndef CORE_DEBUG_CRASH_HANDLER_H
#define CORE_DEBUG_CRASH_HANDLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool crash_handler_init(const char* application_name, const char* version,
                       const char* crash_directory, bool generate_minidumps,
                       bool capture_memory_state, bool upload_symbols);
void crash_handler_set_callback(void (*callback)(const char* crash_info));
void crash_handler_simulate_crash(int crash_type);
bool crash_handler_is_initialized(void);
const char* crash_handler_get_application_name(void);
const char* crash_handler_get_version(void);
void crash_handler_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // CORE_DEBUG_CRASH_HANDLER_H
