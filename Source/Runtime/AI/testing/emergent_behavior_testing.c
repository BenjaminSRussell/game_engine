#include "emergent_behavior_testing.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct emergent_test_context_t {
    bool initialized;
    behavior_test_t** tests;
    uint32_t test_count;
    uint32_t test_capacity;
    test_parameters_t default_parameters;
    emergent_test_report_t cumulative_report;
};

struct behavior_test_t {
    char* name;
    char* description;
    test_type_t type;
    test_parameters_t parameters;
    test_results_t results;
    bool (*test_function)(emergent_test_context_t* context, test_scenario_t* scenario, test_results_t* results);
};

emergent_error_t emergent_test_init(emergent_test_context_t** context) {
    if (!context) return EMERGENT_ERROR_INVALID_PARAM;
    
    emergent_test_context_t* ctx = (emergent_test_context_t*)calloc(1, sizeof(emergent_test_context_t));
    if (!ctx) return EMERGENT_ERROR_OUT_OF_MEMORY;
    
    ctx->test_capacity = 32;
    ctx->tests = (behavior_test_t**)calloc(ctx->test_capacity, sizeof(behavior_test_t*));
    if (!ctx->tests) {
        free(ctx);
        return EMERGENT_ERROR_OUT_OF_MEMORY;
    }
    
    // Set default parameters
    ctx->default_parameters.cohesion_distance = 5.0f;
    ctx->default_parameters.separation_distance = 2.0f;
    ctx->default_parameters.alignment_angle = 30.0f;
    ctx->default_parameters.formation_tolerance = 1.0f;
    ctx->default_parameters.obstacle_clearance = 3.0f;
    ctx->default_parameters.target_reach_distance = 2.0f;
    ctx->default_parameters.pattern_recognition_threshold = 0.7f;
    ctx->default_parameters.min_agent_count = 10;
    ctx->default_parameters.max_agent_count = 1000;
    ctx->default_parameters.max_test_duration = 60.0f;
    ctx->default_parameters.performance_threshold_ms = 16.67f;
    
    ctx->initialized = true;
    *context = ctx;
    return EMERGENT_SUCCESS;
}

void emergent_test_shutdown(emergent_test_context_t* context) {
    if (!context) return;
    
    for (uint32_t i = 0; i < context->test_count; i++) {
        free(context->tests[i]->name);
        free(context->tests[i]->description);
        free(context->tests[i]);
    }
    
    free(context->tests);
    free(context);
}

emergent_error_t emergent_test_register(emergent_test_context_t* context, behavior_test_t* test) {
    if (!context || !test) return EMERGENT_ERROR_INVALID_PARAM;
    if (!context->initialized) return EMERGENT_ERROR_NOT_INITIALIZED;
    
    if (context->test_count >= context->test_capacity) {
        return EMERGENT_ERROR_OUT_OF_MEMORY;
    }
    
    context->tests[context->test_count++] = test;
    return EMERGENT_SUCCESS;
}

emergent_error_t emergent_test_run_single(emergent_test_context_t* context, const char* test_name, test_scenario_t* scenario, test_results_t* results) {
    if (!context || !test_name || !scenario || !results) return EMERGENT_ERROR_INVALID_PARAM;
    
    // Find test
    behavior_test_t* test = NULL;
    for (uint32_t i = 0; i < context->test_count; i++) {
        if (strcmp(context->tests[i]->name, test_name) == 0) {
            test = context->tests[i];
            break;
        }
    }
    
    if (!test) return EMERGENT_ERROR_TEST_FAILED;
    
    // Run test
    bool success = test->test_function(context, scenario, results);
    return success ? EMERGENT_SUCCESS : EMERGENT_ERROR_TEST_FAILED;
}

emergent_error_t emergent_test_run_all(emergent_test_context_t* context, test_scenario_t* scenario, emergent_test_report_t* report) {
    if (!context || !scenario || !report) return EMERGENT_ERROR_INVALID_PARAM;
    
    memset(report, 0, sizeof(emergent_test_report_t));
    report->total_tests = context->test_count;
    
    for (uint32_t i = 0; i < context->test_count; i++) {
        test_results_t results;
        emergent_error_t error = emergent_test_run_single(context, context->tests[i]->name, scenario, &results);
        
        if (error == EMERGENT_SUCCESS) {
            if (results.result == TEST_RESULT_PASS) report->passed_tests++;
            else if (results.result == TEST_RESULT_FAIL) report->failed_tests++;
            else report->inconclusive_tests++;
            
            report->average_score += results.score;
            report->average_confidence += results.confidence;
            report->agents_tested_total += results.agents_tested;
        }
    }
    
    if (context->test_count > 0) {
        report->average_score /= context->test_count;
        report->average_confidence /= context->test_count;
    }
    
    return EMERGENT_SUCCESS;
}

emergent_error_t emergent_test_create_scenario(emergent_test_context_t* context, uint32_t agent_count, test_scenario_t** scenario) {
    if (!context || !scenario) return EMERGENT_ERROR_INVALID_PARAM;
    
    test_scenario_t* s = (test_scenario_t*)calloc(1, sizeof(test_scenario_t));
    if (!s) return EMERGENT_ERROR_OUT_OF_MEMORY;
    
    s->agent_count = agent_count;
    s->initial_positions = (vec3_t*)calloc(agent_count, sizeof(vec3_t));
    s->initial_velocities = (vec3_t*)calloc(agent_count, sizeof(vec3_t));
    
    if (!s->initial_positions || !s->initial_velocities) {
        free(s->initial_positions);
        free(s->initial_velocities);
        free(s);
        return EMERGENT_ERROR_OUT_OF_MEMORY;
    }
    
    // Generate random initial positions
    for (uint32_t i = 0; i < agent_count; i++) {
        s->initial_positions[i].x = (float)(rand() % 100 - 50);
        s->initial_positions[i].y = 0.0f;
        s->initial_positions[i].z = (float)(rand() % 100 - 50);
        
        s->initial_velocities[i].x = (float)(rand() % 10 - 5);
        s->initial_velocities[i].y = 0.0f;
        s->initial_velocities[i].z = (float)(rand() % 10 - 5);
    }
    
    s->environment_size = 100.0f;
    s->simulation_step = 0.016f; // 60 FPS
    s->max_steps = 3600; // 1 minute at 60 FPS
    
    *scenario = s;
    return EMERGENT_SUCCESS;
}

emergent_error_t emergent_test_destroy_scenario(test_scenario_t* scenario) {
    if (!scenario) return EMERGENT_ERROR_INVALID_PARAM;
    
    free(scenario->initial_positions);
    free(scenario->initial_velocities);
    free(scenario->obstacle_positions);
    free(scenario);
    
    return EMERGENT_SUCCESS;
}

emergent_error_t emergent_test_print_summary(emergent_test_context_t* context) {
    if (!context) return EMERGENT_ERROR_INVALID_PARAM;
    
    printf("=== Emergent Behavior Test Summary ===\n");
    printf("Total Tests: %u\n", context->cumulative_report.total_tests);
    printf("Passed: %u\n", context->cumulative_report.passed_tests);
    printf("Failed: %u\n", context->cumulative_report.failed_tests);
    printf("Inconclusive: %u\n", context->cumulative_report.inconclusive_tests);
    printf("Average Score: %.2f\n", context->cumulative_report.average_score);
    printf("Average Confidence: %.2f\n", context->cumulative_report.average_confidence);
    
    return EMERGENT_SUCCESS;
}
