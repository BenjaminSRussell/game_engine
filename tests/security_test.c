/*
 * security_test.c
 * Test suite for the security system
 */

#include "../src/engine/security/security_system.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_basic_validation() {
    printf("Testing basic input validation...\n");
    
    // Initialize security system
    assert(security_system_init() == 0);
    
    // Test safe string validation
    assert(security_validate_string_safe("hello world", 20) == true);
    assert(security_validate_string_safe("hello", 5) == true);
    assert(security_validate_string_safe("hello world", 5) == false);
    
    // Test filename validation
    assert(security_validate_filename("safe_file.txt") == true);
    assert(security_validate_filename("../../../etc/passwd") == false);
    assert(security_validate_filename("file<bad>.txt") == false);
    
    printf("✓ Basic validation tests passed\n");
}

void test_injection_detection() {
    printf("Testing injection detection...\n");
    
    security_validation_config_t config = {
        .rules = SECURITY_RULE_NO_SQL_INJECTION | 
                 SECURITY_RULE_NO_COMMAND_INJECTION | 
                 SECURITY_RULE_NO_XSS,
        .max_length = 1024,
        .severity = SECURITY_SEVERITY_HIGH
    };
    
    security_validation_result_t result;
    
    // Test SQL injection detection
    assert(security_validate_input("SELECT * FROM users", &config, &result) == false);
    assert(result.detected_threats & SECURITY_THREAT_SQL_INJECTION);
    
    // Test command injection detection
    assert(security_validate_input("rm -rf /", &config, &result) == false);
    assert(result.detected_threats & SECURITY_THREAT_COMMAND_INJECTION);
    
    // Test XSS detection
    assert(security_validate_input("<script>alert('xss')</script>", &config, &result) == false);
    assert(result.detected_threats & SECURITY_THREAT_XSS);
    
    printf("✓ Injection detection tests passed\n");
}

void test_safe_string_operations() {
    printf("Testing safe string operations...\n");
    
    char buffer[64];
    
    // Test safe copy
    assert(security_safe_string_copy(buffer, "hello world", sizeof(buffer)) == true);
    assert(strcmp(buffer, "hello world") == 0);
    
    // Test safe copy with overflow
    assert(security_safe_string_copy(buffer, "this is a very long string that will overflow the buffer", sizeof(buffer)) == false);
    
    // Test safe concat
    strcpy(buffer, "hello");
    assert(security_safe_string_concat(buffer, " world", sizeof(buffer)) == true);
    assert(strcmp(buffer, "hello world") == 0);
    
    // Test safe concat with overflow
    assert(security_safe_string_concat(buffer, " this is too long", sizeof(buffer)) == false);
    
    // Test safe sprintf
    assert(security_sprintf_safe(buffer, sizeof(buffer), "Number: %d, String: %s", 42, "test") > 0);
    assert(strcmp(buffer, "Number: 42, String: test") == 0);
    
    printf("✓ Safe string operation tests passed\n");
}

void test_player_security() {
    printf("Testing player security management...\n");
    
    // Initialize player profile
    player_security_profile_t* profile = security_player_profile_init(1001, "testplayer");
    assert(profile != NULL);
    assert(profile->player_id == 1001);
    assert(strcmp(profile->username, "testplayer") == 0);
    assert(profile->violation_count == 0);
    assert(profile->is_banned == false);
    
    // Retrieve profile
    player_security_profile_t* retrieved = security_player_profile_get(1001);
    assert(retrieved == profile);
    
    // Log violations
    uint32_t violation_id = security_log_violation(1001, SECURITY_THREAT_SQL_INJECTION, "SQL injection attempt", SECURITY_SEVERITY_HIGH);
    assert(violation_id > 0);
    assert(profile->violation_count == 1);
    assert(profile->current_threat_level >= THREAT_LEVEL_MEDIUM);
    
    // Test banning
    assert(security_ban_player(1001, 3600) == true);
    assert(security_is_player_banned(1001) == true);
    
    // Test unbanning
    assert(security_unban_player(1001) == true);
    assert(security_is_player_banned(1001) == false);
    
    printf("✓ Player security tests passed\n");
}

void test_statistics() {
    printf("Testing security statistics...\n");
    
    security_stats_t stats;
    security_get_statistics(&stats);
    
    // Should have some validations from previous tests
    assert(stats.total_validations > 0);
    assert(stats.blocked_attempts > 0);
    
    printf("Total validations: %llu\n", stats.total_validations);
    printf("Blocked attempts: %llu\n", stats.blocked_attempts);
    printf("Log entries: %u\n", stats.log_entries);
    
    printf("✓ Statistics tests passed\n");
}

int main() {
    printf("=== Security System Test Suite ===\n\n");
    
    test_basic_validation();
    test_injection_detection();
    test_safe_string_operations();
    test_player_security();
    test_statistics();
    
    // Cleanup
    security_system_shutdown();
    
    printf("\n✓ All security tests passed successfully!\n");
    return 0;
}
