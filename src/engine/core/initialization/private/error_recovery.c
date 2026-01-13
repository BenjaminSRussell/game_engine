#include <core/logger.h>

void error_recovery_init(void) {
  LOG_INFO(LOG_CAT_GENERAL, "Error recovery system initialized.");
}

void error_report_critical(const char *message) {
  LOG_FATAL(LOG_CAT_GENERAL, "CRITICAL ERROR: %s", message);
  // TODO: Show dialog box
}
