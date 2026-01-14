/**
 * @file deployment_validation_main.c
 * @brief Main program for running deployment validation
 * @author Deployment Validation Team
 * @date 2025
 */

#include "deployment_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -c, --category CAT      Run only specific category\n");
    printf("                          Categories: code, performance, compatibility\n");
    printf("  -o, --output FILE       Save report to file\n");
    printf("  -f, --format FORMAT     Output format: text, json, xml\n");
    printf("  -v, --verbose           Enable verbose output\n");
    printf("  -q, --quiet             Suppress non-error output\n");
    printf("  --list-checks           List all available checks\n");
    printf("\nExamples:\n");
    printf("  %s                       # Run all checks\n", program_name);
    printf("  %s -c performance        # Run performance checks only\n", program_name);
    printf("  %s -o report.json -f json # Save JSON report\n", program_name);
}

static void list_checks(void) {
    printf("Available Deployment Validation Checks:\n\n");
    
    printf("CODE QUALITY:\n");
    printf("  [TODO-0060] No TODOs/FIXMEs in main code paths\n");
    printf("  [TODO-0061] All error conditions tested\n");
    printf("  [TODO-0062] Memory usage profiled + optimized\n");
    printf("  [TODO-0063] Thread safety verified\n");
    
    printf("\nPERFORMANCE:\n");
    printf("  [TODO-0064] Frame time stable (no spikes >20%%)\n");
    printf("  [TODO-0065] Memory usage stable (no leaks over 10m runtime)\n");
    printf("  [TODO-0066] GPU utilization 60-80%% (not bottlenecked)\n");
    printf("  [TODO-0067] CPU utilization 50-70%% (not throttled)\n");
    
    printf("\nCOMPATIBILITY:\n");
    printf("  [TODO-0068] Windows 10+ (Vulkan backend)\n");
    printf("  [TODO-0069] macOS 10.15+ (Metal backend)\n");
    printf("  [TODO-0070] Linux (Vulkan backend)\n");
}

static validation_category_t parse_category(const char* category_str) {
    if (strcmp(category_str, "code") == 0) return CATEGORY_CODE_QUALITY;
    if (strcmp(category_str, "performance") == 0) return CATEGORY_PERFORMANCE;
    if (strcmp(category_str, "compatibility") == 0) return CATEGORY_COMPATIBILITY;
    if (strcmp(category_str, "security") == 0) return CATEGORY_SECURITY;
    if (strcmp(category_str, "documentation") == 0) return CATEGORY_DOCUMENTATION;
    
    fprintf(stderr, "Error: Unknown category '%s'\n", category_str);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"help",         no_argument,       0, 'h'},
        {"category",     required_argument, 0, 'c'},
        {"output",       required_argument, 0, 'o'},
        {"format",       required_argument, 0, 'f'},
        {"verbose",      no_argument,       0, 'v'},
        {"quiet",        no_argument,       0, 'q'},
        {"list-checks",  no_argument,       0, 1000},
        {0, 0, 0, 0}
    };
    
    const char* output_file = NULL;
    const char* output_format = "text";
    validation_category_t specific_category = -1;
    bool verbose = false;
    bool quiet = false;
    bool list_only = false;
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "hc:o:f:vq", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
                
            case 'c':
                specific_category = parse_category(optarg);
                break;
                
            case 'o':
                output_file = optarg;
                break;
                
            case 'f':
                output_format = optarg;
                if (strcmp(output_format, "text") != 0 && 
                    strcmp(output_format, "json") != 0 && 
                    strcmp(output_format, "xml") != 0) {
                    fprintf(stderr, "Error: Invalid format '%s'. Use: text, json, xml\n", output_format);
                    return EXIT_FAILURE;
                }
                break;
                
            case 'v':
                verbose = true;
                quiet = false;
                break;
                
            case 'q':
                quiet = true;
                verbose = false;
                break;
                
            case 1000:
                list_only = true;
                break;
                
            case '?':
                fprintf(stderr, "Use --help for usage information.\n");
                return EXIT_FAILURE;
                
            default:
                abort();
        }
    }
    
    if (list_only) {
        list_checks();
        return EXIT_SUCCESS;
    }
    
    if (!quiet) {
        printf("=== Minecraft v2 Engine Deployment Validation ===\n");
        printf("Starting validation checks...\n\n");
    }
    
    // Initialize validator
    deployment_validator_t validator;
    if (deployment_validator_init(&validator, 50) != 0) {
        fprintf(stderr, "Error: Failed to initialize deployment validator\n");
        return EXIT_FAILURE;
    }
    
    // Set output file if specified
    if (output_file) {
        strncpy(validator.report_path, output_file, sizeof(validator.report_path) - 1);
    }
    
    int result = 0;
    
    // Run validation
    if (specific_category != -1) {
        if (!quiet) {
            printf("Running %s category checks only...\n", 
                   validation_category_to_string(specific_category));
        }
        result = deployment_validator_run_category(&validator, specific_category);
    } else {
        result = deployment_validator_run_all_checks(&validator);
    }
    
    // Print summary
    if (!quiet) {
        deployment_validator_print_summary(&validator);
    }
    
    // Save report if requested
    if (output_file) {
        if (deployment_validator_save_report(&validator, output_file) == 0) {
            if (!quiet) {
                printf("\nReport saved to: %s\n", output_file);
            }
        } else {
            fprintf(stderr, "Warning: Failed to save report to %s\n", output_file);
        }
    }
    
    // Cleanup
    deployment_validator_shutdown(&validator);
    
    if (!quiet) {
        printf("\nValidation completed with exit code: %d\n", result);
    }
    
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
