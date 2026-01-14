#ifndef EMERGENT_BEHAVIOR_TESTING_H
#define EMERGENT_BEHAVIOR_TESTING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct emergent_test_context_t emergent_test_context_t;
typedef struct behavior_test_t behavior_test_t;
typedef struct test_scenario_t test_scenario_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef enum {
    TEST_RESULT_PASS,
    TEST_RESULT_FAIL,
    TEST_RESULT_INCONCLUSIVE,
    TEST_RESULT_RUNNING,
    TEST_RESULT_NOT_STARTED
} test_result_t;

typedef enum {
    TEST_TYPE_FLOCKING_COHESION,
    TEST_TYPE_FLOCKING_SEPARATION,
    TEST_TYPE_FLOCKING_ALIGNMENT,
    TEST_TYPE_FORMATION_MAINTENANCE,
    TEST_TYPE_OBSTACLE_AVOIDANCE,
    TEST_TYPE_TARGET_SEEKING,
    TEST_TYPE_EMERGENT_PATTERN,
    TEST_TYPE_SCALABILITY,
    TEST_TYPE_PERFORMANCE,
    TEST_TYPE_STRESS
} test_type_t;

typedef struct {
    float cohesion_distance;
    float separation_distance;
    float alignment_angle;
    float formation_tolerance;
    float obstacle_clearance;
    float target_reach_distance;
    float pattern_recognition_threshold;
    uint32_t min_agent_count;
    uint32_t max_agent_count;
    float max_test_duration;
    float performance_threshold_ms;
} test_parameters_t;

typedef struct {
    test_result_t result;
    float score;
    float confidence;
    uint32_t agents_tested;
    float test_duration;
    char* failure_reason;
    float* metrics;
    uint32_t metric_count;
} test_results_t;

typedef struct {
    char* name;
    char* description;
    test_type_t type;
    test_parameters_t parameters;
    test_results_t results;
    bool (*test_function)(emergent_test_context_t* context, test_scenario_t* scenario, test_results_t* results);
} behavior_test_t;

typedef struct {
    uint32_t agent_count;
    vec3_t* initial_positions;
    vec3_t* initial_velocities;
    vec3_t target_position;
    vec3_t* obstacle_positions;
    uint32_t obstacle_count;
    float environment_size;
    float simulation_step;
    uint32_t max_steps;
} test_scenario_t;

typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t inconclusive_tests;
    float average_score;
    float average_confidence;
    float total_test_time;
    uint32_t agents_tested_total;
} emergent_test_report_t;

typedef enum {
    EMERGENT_SUCCESS = 0,
    EMERGENT_ERROR_INVALID_PARAM = -1,
    EMERGENT_ERROR_OUT_OF_MEMORY = -2,
    EMERGENT_ERROR_NOT_INITIALIZED = -3,
    EMERGENT_ERROR_TEST_FAILED = -4,
    EMERGENT_ERROR_SCENARIO_INVALID = -5
} emergent_error_t;

emergent_error_t emergent_test_init(emergent_test_context_t** context);
void emergent_test_shutdown(emergent_test_context_t* context);

emergent_error_t emergent_test_register(emergent_test_context_t* context, behavior_test_t* test);
emergent_error_t emergent_test_unregister(emergent_test_context_t* context, const char* test_name);

emergent_error_t emergent_test_run_single(emergent_test_context_t* context, const char* test_name, test_scenario_t* scenario, test_results_t* results);
emergent_error_t emergent_test_run_all(emergent_test_context_t* context, test_scenario_t* scenario, emergent_test_report_t* report);
emergent_error_t emergent_test_run_suite(emergent_test_context_t* context, const char** test_names, uint32_t test_count, test_scenario_t* scenario, emergent_test_report_t* report);

emergent_error_t emergent_test_create_scenario(emergent_test_context_t* context, uint32_t agent_count, test_scenario_t** scenario);
emergent_error_t emergent_test_destroy_scenario(test_scenario_t* scenario);

emergent_error_t emergent_test_generate_report(emergent_test_context_t* context, emergent_test_report_t* report);
emergent_error_t emergent_test_save_report(emergent_test_context_t* context, const char* filename);
emergent_error_t emergent_test_print_summary(emergent_test_context_t* context);

emergent_error_t emergent_test_set_parameters(emergent_test_context_t* context, const test_parameters_t* params);
emergent_error_t emergent_test_get_parameters(emergent_test_context_t* context, test_parameters_t* params);

#ifdef __cplusplus
}
#endif

#endif
