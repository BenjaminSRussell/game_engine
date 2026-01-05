#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/engine/ai/api/ai_api.h"

// Simple assertion macro
#define ASSERT_EQ(val1, val2) \
    if ((val1) != (val2)) { \
        printf("FAILED: %s line %d: %d != %d\n", __FILE__, __LINE__, (int)(val1), (int)(val2)); \
        return 1; \
    }

#define ASSERT_STR_CONTAINS(haystack, needle) \
    if (strstr(haystack, needle) == NULL) { \
        printf("FAILED: %s line %d: String '%s' not found in '%s'\n", __FILE__, __LINE__, needle, haystack); \
        return 1; \
    }

int main(void) {
    printf("Initializing AI API...\n");
    ai_api_init();
    
    char response[4096];
    int result;
    
    // Test 1: Query All Types
    printf("Test 1: Query Types\n");
    const char* cmd1 = "{ \"command\": \"query_types\" }";
    result = ai_api_execute_command(cmd1, response, sizeof(response));
    ASSERT_EQ(result, 0);
    printf("Response: %s\n", response);
    ASSERT_STR_CONTAINS(response, "\"status\": \"ok\"");
    ASSERT_STR_CONTAINS(response, "\"vec3\"");
    ASSERT_STR_CONTAINS(response, "\"float\"");
    
    // Test 2: Get Schema for vec3
    printf("\nTest 2: Get Schema (vec3)\n");
    const char* cmd2 = "{ \"command\": \"get_schema\", \"type_name\": \"vec3\" }";
    result = ai_api_execute_command(cmd2, response, sizeof(response));
    ASSERT_EQ(result, 0);
    printf("Response: %s\n", response);
    ASSERT_STR_CONTAINS(response, "\"type\": \"vec3\"");
    // Depending on how basic type registration handles fields, 
    // vec3 usually doesn't have sub-fields registered in the init() function yet
    // unless we look at type_registry.c. 
    // In type_registry.c/init, we only did type_registry_register("vec3", ...). 
    // We didn't add fields like x,y,z in init().
    // So let's check it returns success.
    
    // Test 3: Invalid Command
    printf("\nTest 3: Invalid Command\n");
    const char* cmd3 = "{ \"command\": \"make_coffee\" }";
    result = ai_api_execute_command(cmd3, response, sizeof(response));
    ASSERT_EQ(result, -1);
    printf("Response: %s\n", response);
    ASSERT_STR_CONTAINS(response, "\"error\"");
    
    printf("\nALL TESTS PASSED\n");
    return 0;
}
