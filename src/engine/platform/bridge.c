#include "platform/bridge.h"
#include <stdlib.h>

void platform_init() {}

void platform_get_executable_path(char *buffer, int size) {
    // Get path to current executable
}

void platform_get_user_data_path(char *buffer, int size) {
    // Get platform-specific user data directory
}

void platform_open_url(const char *url) {
    // Open URL in default browser
}

void platform_show_message_box(const char *title, const char *message) {
    // Platform-specific message box
}

int platform_get_processor_count() {
    return 4;
}

long long platform_get_memory_size() {
    return 8589934592LL; // 8GB
}
