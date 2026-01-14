#include <core/logger.h>

void error_recovery_init(void) {
  LOG_INFO("Error recovery system initialized.");
}

void error_report_critical(const char *message) {
  LOG_FATAL("CRITICAL ERROR: %s", message);
  // TODO: Show dialog box
}
