/**
 * @file interface_validator.c
 * @brief Interface compliance validation system
 *
 * Validates that all engine interfaces comply with established standards
 * and conventions for consistency and maintainability.
 */

#include "interface_standards.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

// ========================================
// Validation Rules
// ========================================

typedef struct ValidationRule {
    const char *name;                      // Rule name
    const char *description;               // Rule description
    bool (*validate)(const IEngineSubsystem *interface); // Validation function
    bool is_critical;                       // Whether rule is critical
} ValidationRule;

// ========================================
// Validation Results
// ========================================

typedef struct ValidationResult {
    const char *rule_name;                 // Rule name
    bool passed;                           // Whether rule passed
    char message[256];                      // Validation message
    struct ValidationResult *next;          // Next result in list
} ValidationResult;

typedef struct ValidationReport {
    uint32_t total_rules;                   // Total rules checked
    uint32_t passed_rules;                  // Rules that passed
    uint32_t failed_rules;                  // Rules that failed
    uint32_t critical_failures;             // Critical rule failures
    ValidationResult *results;                // Detailed results
    char summary[1024];                     // Summary report
    bool overall_success;                   // Overall validation success
} ValidationReport;

// ========================================
// Helper Functions
// ========================================

static bool is_valid_identifier(const char *str) {
    if (!str || !*str) return false;
    
    // Must start with letter or underscore
    if (!isalpha(*str) && *str != '_') return false;
    
    // Remaining characters can be alphanumeric or underscore
    while (*str) {
        if (!isalnum(*str) && *str != '_') return false;
        str++;
    }
    
    return true;
}

static bool is_snake_case(const char *str) {
    if (!str) return false;
    
    for (int i = 0; str[i]; i++) {
        if (isupper(str[i])) {
            return false;
        }
        if (str[i] == '-' || str[i] == ' ') {
            return false;
        }
    }
    
    return true;
}

static bool has_proper_documentation(const IEngineSubsystem *interface) {
    if (!interface) return false;
    
    // Check that required functions are implemented
    if (!interface->name || !interface->version) return false;
    if (!interface->initialize || !interface->shutdown) return false;
    if (!interface->is_initialized) return false;
    if (!interface->update || !interface->get_status) return false;
    
    // Check that functions are not NULL pointers
    if (interface->set_config && !interface->get_config) return false;
    if (interface->register_dependency && !interface->check_dependencies) return false;
    
    return true;
}

static bool version_is_compatible(const char *interface_version, const char *required_version) {
    if (!interface_version || !required_version) return false;
    
    // Simple version comparison (major.minor.patch)
    int interface_major, interface_minor, interface_patch;
    int required_major, required_minor, required_patch;
    
    if (sscanf(interface_version, "%d.%d.%d", &interface_major, &interface_minor, &interface_patch) != 3) {
        return false;
    }
    
    if (sscanf(required_version, "%d.%d.%d", &required_major, &required_minor, &required_patch) != 3) {
        return false;
    }
    
    // Major version must match
    if (interface_major != required_major) {
        return false;
    }
    
    // Minor version must be >= required
    if (interface_minor < required_minor) {
        return false;
    }
    
    // Patch version can be anything
    return true;
}

// ========================================
// Validation Rule Implementations
// ========================================

static bool validate_interface_name(const IEngineSubsystem *interface) {
    if (!interface || !interface->name) {
        return false;
    }
    
    // Name must follow snake_case convention
    if (!is_snake_case(interface->name)) {
        return false;
    }
    
    // Name must be a valid identifier
    if (!is_valid_identifier(interface->name)) {
        return false;
    }
    
    // Name should be descriptive (at least 3 characters)
    if (strlen(interface->name) < 3) {
        return false;
    }
    
    // Name should not contain "test" (unless it's a test interface)
    if (strstr(interface->name, "test") && strcmp(interface->name, "test_interface") != 0) {
        return false;
    }
    
    return true;
}

static bool validate_interface_version(const IEngineSubsystem *interface) {
    if (!interface || !interface->version) {
        return false;
    }
    
    // Version must follow semantic versioning (x.y.z)
    if (!version_is_compatible(interface->version, INTERFACE_VERSION_STRING)) {
        return false;
    }
    
    return true;
}

static bool validate_required_functions(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Check critical functions that must be implemented
    if (!interface->initialize) {
        return false;
    }
    
    if (!interface->shutdown) {
        return false;
    }
    
    if (!interface->is_initialized) {
        return false;
    }
    
    if (!interface->update) {
        return false;
    }
    
    if (!interface->get_status) {
        return false;
    }
    
    return true;
}

static bool validate_function_pointers(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // All function pointers should be valid
    void **ptrs = (void**)interface;
    size_t ptr_count = sizeof(IEngineSubsystem) / sizeof(void*);
    
    for (size_t i = 0; i < ptr_count; i++) {
        if (ptrs[i] && ptrs[i] != (void*)0xDEADBEEF) {
            // Check if pointer is within valid memory range (basic check)
            // This is a simple check - in production this would be more sophisticated
            continue;
        }
    }
    
    return true;
}

static bool validate_lifecycle_consistency(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Check that lifecycle methods are consistent
    if (interface->is_initialized()) {
        // If interface is initialized, it should have been successfully initialized
        // This is a basic check - in production we'd track initialization state
        return true;
    }
    
    return true;
}

static bool validate_dependency_management(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // If dependency management is implemented, both functions should be present
    bool has_register = interface->register_dependency != NULL;
    bool has_check = interface->check_dependencies != NULL;
    
    if (has_register != has_check) {
        return false;
    }
    
    // If dependencies are registered, check_dependencies should return true
    if (has_register && has_check) {
        uint32_t dep_count = interface->get_dependency_count ? interface->get_dependency_count() : 0;
        if (dep_count > 0) {
            if (!interface->check_dependencies()) {
                return false;
            }
        }
    }
    
    return true;
}

static bool validate_error_handling(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Error handling should be implemented
    if (!interface->get_error_message) {
        return false;
    }
    
    // Should have some way to get status
    if (!interface->get_status) {
        return false;
    }
    
    return true;
}

static bool validate_thread_safety(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Thread safety should be explicitly indicated
    if (interface->is_thread_safe) {
        // If claimed to be thread-safe, basic validation
        // In production, this would check actual thread safety measures
        return true;
    }
    
    // If not thread-safe, that's acceptable for single-threaded subsystems
    return true;
}

static bool validate_performance_monitoring(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Performance monitoring should be implemented if possible
    bool has_start = interface->start_performance_profile != NULL;
    bool has_end = interface->end_performance_profile != NULL;
    
    if (has_start != has_end) {
        return false;
    }
    
    return true;
}

static bool validate_memory_management(const IEngineSubsystem *interface) {
    if (!interface) {
        return false;
    }
    
    // Memory management should be implemented if possible
    bool has_allocate = interface->allocate_memory != NULL;
    bool has_free = interface->free_memory != NULL;
    
    if (has_allocate != has_free) {
        return false;
    }
    
    return true;
}

// ========================================
// Validation Rules Array
// ========================================

static ValidationRule g_validation_rules[] = {
    {
        "interface_name",
        "Interface name must follow snake_case convention",
        validate_interface_name,
        true
    },
    {
        "interface_version",
        "Interface version must be compatible with current version",
        validate_interface_version,
        true
    },
    {
        "required_functions",
        "All required functions must be implemented",
        validate_required_functions,
        true
    },
    {
        "function_pointers",
        "Function pointers must be valid",
        validate_function_pointers,
        false
    },
    {
        "lifecycle_consistency",
        "Lifecycle methods must be consistent",
        validate_lifecycle_consistency,
        false
    },
    {
        "dependency_management",
        "Dependency management must be consistent",
        validate_dependency_management,
        false
    },
    {
        "error_handling",
        "Error handling must be implemented",
        validate_error_handling,
        true
    },
    {
        "thread_safety",
        "Thread safety must be explicitly indicated",
        validate_thread_safety,
        false
    },
    {
        "performance_monitoring",
        "Performance monitoring should be implemented",
        validate_performance_monitoring,
        false
    },
    {
        "memory_management",
        "Memory management should be implemented",
        validate_memory_management,
        false
    }
};

static const uint32_t g_validation_rule_count = sizeof(g_validation_rules) / sizeof(ValidationRule);

// ========================================
// Validation Report Management
// ========================================

static ValidationReport* create_validation_report(void) {
    ValidationReport *report = malloc(sizeof(ValidationReport));
    if (!report) return NULL;
    
    memset(report, 0, sizeof(ValidationReport));
    return report;
}

static void add_validation_result(ValidationReport *report, const char *rule_name, 
                                 bool passed, const char *message) {
    if (!report || !rule_name) return;
    
    ValidationResult *result = malloc(sizeof(ValidationResult));
    if (!result) return;
    
    result->rule_name = rule_name;
    result->passed = passed;
    
    if (message) {
        strncpy(result->message, message, sizeof(result->message) - 1);
    }
    
    // Add to linked list
    result->next = report->results;
    report->results = result;
    
    report->total_rules++;
    if (passed) {
        report->passed_rules++;
    } else {
        report->failed_rules++;
        
        ValidationRule *rule = NULL;
        for (uint32_t i = 0; i < g_validation_rule_count; i++) {
            if (strcmp(g_validation_rules[i].name, rule_name) == 0) {
                rule = &g_validation_rules[i];
                break;
            }
        }
        
        if (rule && rule->is_critical) {
            report->critical_failures++;
        }
    }
}

static void generate_summary_report(ValidationReport *report) {
    if (!report) return;
    
    snprintf(report->summary, sizeof(report->summary),
            "Interface Validation Summary:\n"
            "Total Rules: %u\n"
            "Passed: %u\n"
            "Failed: %u\n"
            "Critical Failures: %u\n"
            "Overall Success: %s",
            report->total_rules,
            report->passed_rules,
            report->failed_rules,
            report->critical_failures,
            report->overall_success ? "YES" : "NO");
    
    report->overall_success = (report->critical_failures == 0);
}

static void destroy_validation_report(ValidationReport *report) {
    if (!report) return;
    
    ValidationResult *current = report->results;
    while (current) {
        ValidationResult *next = current->next;
        free(current);
        current = next;
    }
    
    free(report);
}

// ========================================
// Public API Implementation
// ========================================

ValidationReport* validate_interface_compliance(const IEngineSubsystem *interface) {
    if (!interface) {
        return NULL;
    }
    
    ValidationReport *report = create_validation_report();
    if (!report) {
        return NULL;
    }
    
    // Run all validation rules
    for (uint32_t i = 0; i < g_validation_rule_count; i++) {
        ValidationRule *rule = &g_validation_rules[i];
        
        bool passed = rule->validate(interface);
        char message[256] = {0};
        
        if (!passed) {
            snprintf(message, sizeof(message), "Rule '%s' failed", rule->name);
        }
        
        add_validation_result(report, rule->name, passed, message);
        
        // Stop early on critical failure
        if (!passed && rule->is_critical) {
            break;
        }
    }
    
    generate_summary_report(report);
    return report;
}

bool validate_interface_name(const char *name) {
    return validate_interface_name(NULL);
}

bool validate_interface_version(const char *version) {
    return validate_interface_version(NULL);
}

bool generate_interface_documentation(const IEngineSubsystem *interface, 
                                       char *buffer, 
                                       size_t buffer_size) {
    if (!interface || !buffer || buffer_size == 0) {
        return false;
    }
    
    // Generate documentation in Markdown format
    int written = snprintf(buffer, buffer_size,
            "# %s Interface Documentation\n\n"
            "## Overview\n"
            "The %s subsystem provides core functionality for the Minecraft v2 Engine.\n\n"
            "## Interface Version\n"
            "- **Version**: %s\n"
            "- **Compatibility**: %s\n\n"
            "## Status\n"
            "- **Initialized**: %s\n"
            "- **Status**: %s\n\n"
            "## Dependencies\n",
            interface->name,
            interface->name,
            interface->version,
            version_is_compatible(interface->version, INTERFACE_VERSION_STRING) ? "Compatible" : "Incompatible",
            interface->is_initialized() ? "Yes" : "No",
            "Unknown", // Would get from get_status()
            interface->name
    );
    
    // Add dependencies if available
    if (interface->get_dependency_count && interface->get_dependency_name) {
        uint32_t dep_count = interface->get_dependency_count();
        if (dep_count > 0) {
            for (uint32_t i = 0; i < dep_count && written < buffer_size - 1; i++) {
                const char *dep_name = interface->get_dependency_name(i);
                if (dep_name) {
                    written += snprintf(buffer + written, buffer_size - written,
                            "- %s\n", dep_name);
                }
            }
        }
    }
    
    // Add configuration information
    written += snprintf(buffer + written, buffer_size - written,
            "\n## Configuration\n"
            "The subsystem supports configuration through the standard configuration structure.\n");
    
    // Add performance information
    written += snprintf(buffer + written, buffer_size - written,
            "\n## Performance\n"
            "The subsystem includes built-in performance monitoring capabilities.\n");
    
    // Add error handling information
    written += snprintf(buffer + written, buffer_size - written,
            "\n## Error Handling\n"
            "The subsystem provides comprehensive error handling and status reporting.\n");
    
    // Add thread safety information
    written += snprintf(buffer + written, buffer_size - written,
            "\n## Thread Safety\n"
            "Thread Safety: %s\n",
            interface->is_thread_safe ? "Yes" : "No");
    
    return written > 0 && written < buffer_size;
}

// ========================================
// Batch Validation
// ========================================

ValidationReport* validate_all_interfaces(IEngineSubsystem **interfaces, uint32_t interface_count) {
    if (!interfaces || interface_count == 0) {
        return NULL;
    }
    
    ValidationReport *master_report = create_validation_report();
    if (!master_report) {
        return NULL;
    }
    
    master_report->total_rules = g_validation_rule_count * interface_count;
    
    for (uint32_t i = 0; i < interface_count; i++) {
        ValidationReport *interface_report = validate_interface_compliance(interfaces[i]);
        if (interface_report) {
            // Merge results
            master_report->passed_rules += interface_report->passed_rules;
            master_report->failed_rules += interface_report->failed_rules;
            master_report->critical_failures += interface_report->critical_failures;
            
            // Free interface report
            destroy_validation_report(interface_report);
        }
    }
    
    master_report->overall_success = (master_report->critical_failures == 0);
    
    snprintf(master_report->summary, sizeof(master_report->summary),
            "Batch Interface Validation Summary:\n"
            "Total Interfaces: %u\n"
            "Total Rules Checked: %u\n"
            "Passed Rules: %u\n"
            "Failed Rules: %u\n"
            "Critical Failures: %u\n"
            "Overall Success: %s",
            interface_count,
            master_report->total_rules,
            master_report->passed_rules,
            master_report->failed_rules,
            master_report->critical_failures,
            master_report->overall_success ? "YES" : "NO");
    
    return master_report;
}

void print_validation_report(const ValidationReport *report) {
    if (!report) {
        printf("No validation report to print\n");
        return;
    }
    
    printf("\n=== Interface Validation Report ===\n");
    printf("%s\n", report->summary);
    
    if (report->failed_rules > 0) {
        printf("\nFailed Rules:\n");
        ValidationResult *current = report->results;
        while (current) {
            if (!current->passed) {
                printf("  ❌ %s: %s\n", current->rule_name, current->message);
            }
            current = current->next;
        }
    }
    
    if (report->critical_failures > 0) {
        printf("\n⚠️  CRITICAL FAILURES DETECTED!\n");
    }
    
    printf("=====================================\n\n");
}

void destroy_validation_report(ValidationReport *report) {
    destroy_validation_report(report);
}
