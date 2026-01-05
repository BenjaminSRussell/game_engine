#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VALIDATION_ERROR,
    VALIDATION_WARNING,
    VALIDATION_INFO
} ValidationSeverity;

typedef struct {
    ValidationSeverity severity;
    const char *message;
    uint64_t entity_id; // Entity with issue
    const char *suggestion; // How to fix
} ValidationIssue;

typedef struct {
    ValidationIssue *issues;
    int issue_count;
    int capacity;
} SceneValidator;

void scene_validator_init(SceneValidator *validator);
void scene_validator_destroy(SceneValidator *validator);

// Validation checks
void scene_validate_overlaps(SceneValidator *validator, void *scene);
void scene_validate_references(SceneValidator *validator, void *scene);
void scene_validate_lightmaps(SceneValidator *validator, void *scene);
void scene_validate_physics(SceneValidator *validator, void *scene);

// Issue management
void scene_validator_add_issue(SceneValidator *validator, ValidationSeverity severity,
                                 const char *message, uint64_t entity_id, const char *suggestion);
void scene_validator_clear(SceneValidator *validator);

// Display
void scene_validator_show_report(SceneValidator *validator);
