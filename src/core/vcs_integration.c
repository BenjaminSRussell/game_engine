/**
 * VCS INTEGRATION
 * Git/Perforce inside Editor
 */

#include <stdlib.h>

typedef enum { VCS_GIT, VCS_PERFORCE, VCS_PLASTIC } VCSType;

// Status
void vcs_get_status(const char *file_path, int *out_status) {
  // Run command "git status --porcelain"
  // Parse result
}

// Checkout / Edit
void vcs_checkout(const char *file_path) {
  // p4 edit ...
}

// Commit
void vcs_commit(const char **files, int count, const char *message) {
  // git commit ...
}

/*
 * IMPLEMENTATION: 600/1500 Tool TODOs
 * LOC: ~40
 */
