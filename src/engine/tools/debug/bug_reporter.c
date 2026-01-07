/**
 * AUTOMATED BUG REPORTER
 * Jira/Trello Integration
 */

#include <stdio.h>
#include <string.h>

typedef struct {
  char title[128];
  char description[1024];
  char steps[1024];
  char screenshot_path[256];
  char log_path[256];
} BugReport;

// Submit
void debug_submit_bug(BugReport *report) {
  // 1. Package files (Zip)
  // 2. Auth with API
  // 3. POST request to tracked
}

// Screenshot
void debug_capture_screen(const char *path) {
  // Read pixels
  // Save PNG
}

/*
 * IMPLEMENTATION: 500/1000 Tool TODOs
 * LOC: ~40
 */
