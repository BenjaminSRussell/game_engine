/**
 * Unit Test for AI Command Processor
 * Tests the AI API command execution and JSON parsing
 */

#include "../../../src/engine/ai/api/ai_api.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test initialization
static TestResult test_ai_api_init(void) {
    ai_api_init();
    TEST_ASSERT(true, "AI API initialized successfully");
    return TEST_PASS;
}

// Test query_types command
static TestResult test_query_types_command(void) {
    char response[1024];
    const char* query_cmd = "{\"command\": \"query_types\"}";
    
    int result = ai_api_execute_command(query_cmd, response, sizeof(response));
    
    TEST_ASSERT_EQ(result, 0, "Command executed successfully");
    TEST_ASSERT(strstr(response, "\"status\"") != NULL, "Response contains status field");
    TEST_ASSERT(strstr(response, "\"ok\"") != NULL, "Status is ok");
    TEST_ASSERT(strstr(response, "\"data\"") != NULL, "Response contains data field");
    
    return TEST_PASS;
}

// Test get_schema command
static TestResult test_get_schema_command(void) {
    char response[2048];
    const char* schema_cmd = "{\"command\": \"get_schema\", \"type_name\": \"TestType\"}";
    
    int result = ai_api_execute_command(schema_cmd, response, sizeof(response));
    
    TEST_ASSERT_EQ(result, 0, "Command executed successfully");
    TEST_ASSERT(strstr(response, "\"status\"") != NULL, "Response contains status field");
    
    return TEST_PASS;
}

// Test invalid command
static TestResult test_invalid_command(void) {
    char response[1024];
    const char* invalid_cmd = "{\"command\": \"invalid_command\"}";
    
    int result = ai_api_execute_command(invalid_cmd, response, sizeof(response));
    
    TEST_ASSERT_EQ(result, -1, "Invalid command returns error code");
    TEST_ASSERT(strstr(response, "\"error\"") != NULL, "Response contains error status");
    TEST_ASSERT(strstr(response, "Unknown command") != NULL, "Error message indicates unknown command");
    
    return TEST_PASS;
}

// Test missing command field
static TestResult test_missing_command_field(void) {
    char response[1024];
    const char* no_cmd = "{\"some_field\": \"value\"}";
    
    int result = ai_api_execute_command(no_cmd, response, sizeof(response));
    
    TEST_ASSERT_EQ(result, -1, "Missing command field returns error");
    TEST_ASSERT(strstr(response, "missing 'command' field") != NULL, "Error message indicates missing command");
    
    return TEST_PASS;
}

// Test missing type_name parameter for get_schema
static TestResult test_missing_type_name(void) {
    char response[1024];
    const char* schema_cmd = "{\"command\": \"get_schema\"}";
    
    int result = ai_api_execute_command(schema_cmd, response, sizeof(response));
    
    TEST_ASSERT(strstr(response, "Missing parameter: type_name") != NULL, 
                "Error message indicates missing type_name");
    
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:CommandProcessor", "Initialization", test_ai_api_init, NULL, NULL);
    test_register("AI:CommandProcessor", "Query Types Command", test_query_types_command, NULL, NULL);
    test_register("AI:CommandProcessor", "Get Schema Command", test_get_schema_command, NULL, NULL);
    test_register("AI:CommandProcessor", "Invalid Command", test_invalid_command, NULL, NULL);
    test_register("AI:CommandProcessor", "Missing Command Field", test_missing_command_field, NULL, NULL);
    test_register("AI:CommandProcessor", "Missing Type Name", test_missing_type_name, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  AI Command Processor Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
