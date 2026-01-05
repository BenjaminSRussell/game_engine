#include "networking/security/encryption.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/test.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_004 - Unit Tests for AES-GCM Encryption
// Features: Comprehensive test coverage for encryption, decryption, and security features

// Test framework macros
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            log_error("ASSERTION FAILED: %s:%d - %s", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    ASSERT((expected) == (actual))

#define ASSERT_NE(expected, actual) \
    ASSERT((expected) != (actual))

#define ASSERT_STR_EQ(expected, actual) \
    ASSERT(string_equals((expected), (actual)))

#define ASSERT_NOT_NULL(ptr) \
    ASSERT((ptr) != NULL)

#define ASSERT_MEM_EQ(expected, actual, size) \
    ASSERT(memcmp((expected), (actual), (size)) == 0)

// Test data
static const uint8_t test_plaintext[] = "Hello, World! This is a test message for AES-GCM encryption.";
static const uint8_t test_key[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const uint8_t test_aad[] = "Additional authenticated data";

// Test: Crypto System Initialization
static bool test_crypto_init(void) {
    log_info("Testing crypto system initialization...");
    
    bool result = crypto_init();
    ASSERT(result);
    
    log_info("✅ Crypto system initialization test passed");
    return true;
}

// Test: Key Generation
static bool test_key_generation(void) {
    log_info("Testing key generation...");
    
    uint8_t key[32];
    bool result = crypto_generate_key(key, sizeof(key));
    ASSERT(result);
    
    // Verify key is not all zeros
    bool all_zero = true;
    for (size_t i = 0; i < sizeof(key); i++) {
        if (key[i] != 0) {
            all_zero = false;
            break;
        }
    }
    ASSERT(!all_zero);
    
    // Test invalid key size
    uint8_t invalid_key[16];
    result = crypto_generate_key(invalid_key, sizeof(invalid_key));
    ASSERT(!result);
    
    log_info("✅ Key generation test passed");
    return true;
}

// Test: Key Setting
static bool test_key_setting(void) {
    log_info("Testing key setting...");
    
    bool result = crypto_set_key(test_key, sizeof(test_key));
    ASSERT(result);
    
    // Test invalid key size
    uint8_t invalid_key[16];
    result = crypto_set_key(invalid_key, sizeof(invalid_key));
    ASSERT(!result);
    
    // Test NULL key
    result = crypto_set_key(NULL, 32);
    ASSERT(!result);
    
    log_info("✅ Key setting test passed");
    return true;
}

// Test: AES-GCM Encryption
static bool test_aes_gcm_encryption(void) {
    log_info("Testing AES-GCM encryption...");
    
    // Initialize crypto system
    crypto_init();
    crypto_set_key(test_key, sizeof(test_key));
    
    uint8_t ciphertext[256];
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    bool result = crypto_encrypt_aes_gcm(test_plaintext, strlen((char*)test_plaintext),
                                        test_aad, strlen((char*)test_aad),
                                        ciphertext, &ciphertext_len,
                                        tag, sizeof(tag));
    ASSERT(result);
    
    // Verify ciphertext is different from plaintext
    ASSERT_NE(0, memcmp(test_plaintext, ciphertext, strlen((char*)test_plaintext)));
    
    // Verify tag is not all zeros
    bool tag_all_zero = true;
    for (size_t i = 0; i < sizeof(tag); i++) {
        if (tag[i] != 0) {
            tag_all_zero = false;
            break;
        }
    }
    ASSERT(!tag_all_zero);
    
    log_info("✅ AES-GCM encryption test passed");
    return true;
}

// Test: AES-GCM Decryption
static bool test_aes_gcm_decryption(void) {
    log_info("Testing AES-GCM decryption...");
    
    // Initialize crypto system
    crypto_init();
    crypto_set_key(test_key, sizeof(test_key));
    
    // Encrypt data
    uint8_t ciphertext[256];
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    bool result = crypto_encrypt_aes_gcm(test_plaintext, strlen((char*)test_plaintext),
                                        test_aad, strlen((char*)test_aad),
                                        ciphertext, &ciphertext_len,
                                        tag, sizeof(tag));
    ASSERT(result);
    
    // Decrypt data
    uint8_t decrypted[256];
    size_t decrypted_len = sizeof(decrypted);
    
    result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len,
                                   test_aad, strlen((char*)test_aad),
                                   tag, sizeof(tag),
                                   decrypted, &decrypted_len);
    ASSERT(result);
    
    // Verify decrypted data matches original
    ASSERT_EQ(strlen((char*)test_plaintext), decrypted_len);
    ASSERT_MEM_EQ(test_plaintext, decrypted, strlen((char*)test_plaintext));
    
    log_info("✅ AES-GCM decryption test passed");
    return true;
}

// Test: Authentication Tag Validation
static bool test_authentication_tag_validation(void) {
    log_info("Testing authentication tag validation...");
    
    // Initialize crypto system
    crypto_init();
    crypto_set_key(test_key, sizeof(test_key));
    
    // Encrypt data
    uint8_t ciphertext[256];
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    bool result = crypto_encrypt_aes_gcm(test_plaintext, strlen((char*)test_plaintext),
                                        test_aad, strlen((char*)test_aad),
                                        ciphertext, &ciphertext_len,
                                        tag, sizeof(tag));
    ASSERT(result);
    
    // Modify tag and try to decrypt (should fail)
    uint8_t modified_tag[16];
    memcpy(modified_tag, tag, sizeof(tag));
    modified_tag[0] ^= 0xFF; // Flip a bit
    
    uint8_t decrypted[256];
    size_t decrypted_len = sizeof(decrypted);
    
    result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len,
                                   test_aad, strlen((char*)test_aad),
                                   modified_tag, sizeof(modified_tag),
                                   decrypted, &decrypted_len);
    ASSERT(!result); // Should fail
    
    log_info("✅ Authentication tag validation test passed");
    return true;
}

// Test: HMAC Generation
static bool test_hmac_generation(void) {
    log_info("Testing HMAC generation...");
    
    uint8_t hmac[32];
    bool result = crypto_generate_hmac(test_plaintext, strlen((char*)test_plaintext),
                                      test_key, sizeof(test_key),
                                      hmac, sizeof(hmac));
    ASSERT(result);
    
    // Verify HMAC is not all zeros
    bool hmac_all_zero = true;
    for (size_t i = 0; i < sizeof(hmac); i++) {
        if (hmac[i] != 0) {
            hmac_all_zero = false;
            break;
        }
    }
    ASSERT(!hmac_all_zero);
    
    // Test invalid parameters
    result = crypto_generate_hmac(NULL, 10, test_key, sizeof(test_key), hmac, sizeof(hmac));
    ASSERT(!result);
    
    result = crypto_generate_hmac(test_plaintext, 10, NULL, sizeof(test_key), hmac, sizeof(hmac));
    ASSERT(!result);
    
    result = crypto_generate_hmac(test_plaintext, 10, test_key, sizeof(test_key), NULL, sizeof(hmac));
    ASSERT(!result);
    
    log_info("✅ HMAC generation test passed");
    return true;
}

// Test: HMAC Verification
static bool test_hmac_verification(void) {
    log_info("Testing HMAC verification...");
    
    // Generate HMAC
    uint8_t hmac[32];
    bool result = crypto_generate_hmac(test_plaintext, strlen((char*)test_plaintext),
                                      test_key, sizeof(test_key),
                                      hmac, sizeof(hmac));
    ASSERT(result);
    
    // Verify correct HMAC
    result = crypto_verify_hmac(test_plaintext, strlen((char*)test_plaintext),
                               test_key, sizeof(test_key),
                               hmac, sizeof(hmac));
    ASSERT(result);
    
    // Verify incorrect HMAC (should fail)
    uint8_t wrong_hmac[32];
    memcpy(wrong_hmac, hmac, sizeof(hmac));
    wrong_hmac[0] ^= 0xFF; // Flip a bit
    
    result = crypto_verify_hmac(test_plaintext, strlen((char*)test_plaintext),
                               test_key, sizeof(test_key),
                               wrong_hmac, sizeof(wrong_hmac));
    ASSERT(!result);
    
    log_info("✅ HMAC verification test passed");
    return true;
}

// Test: Diffie-Hellman Key Exchange
static bool test_diffie_hellman_exchange(void) {
    log_info("Testing Diffie-Hellman key exchange...");
    
    // Generate key pairs
    uint8_t private_key1[32], public_key1[32];
    size_t priv1_len = sizeof(private_key1), pub1_len = sizeof(public_key1);
    
    bool result = crypto_generate_dh_keypair(private_key1, &priv1_len, public_key1, &pub1_len);
    ASSERT(result);
    
    uint8_t private_key2[32], public_key2[32];
    size_t priv2_len = sizeof(private_key2), pub2_len = sizeof(public_key2);
    
    result = crypto_generate_dh_keypair(private_key2, &priv2_len, public_key2, &pub2_len);
    ASSERT(result);
    
    // Exchange keys and compute shared secrets
    uint8_t shared_secret1[32], shared_secret2[32];
    size_t secret1_len = sizeof(shared_secret1), secret2_len = sizeof(shared_secret2);
    
    result = crypto_diffie_hellman_exchange(private_key1, priv1_len, public_key2, pub2_len,
                                          shared_secret1, &secret1_len);
    ASSERT(result);
    
    result = crypto_diffie_hellman_exchange(private_key2, priv2_len, public_key1, pub1_len,
                                          shared_secret2, &secret2_len);
    ASSERT(result);
    
    // Verify shared secrets match
    ASSERT_EQ(secret1_len, secret2_len);
    ASSERT_MEM_EQ(shared_secret1, shared_secret2, secret1_len);
    
    log_info("✅ Diffie-Hellman key exchange test passed");
    return true;
}

// Test: Replay Attack Protection
static bool test_replay_attack_protection(void) {
    log_info("Testing replay attack protection...");
    
    uint64_t current_time = time_get_current_ms();
    
    // Test valid timestamp
    bool result = crypto_check_replay_attack(current_time, 12345);
    ASSERT(result);
    
    // Test old timestamp (should fail)
    result = crypto_check_replay_attack(current_time - 400000, 12345); // 400 seconds ago
    ASSERT(!result);
    
    // Test future timestamp (should fail)
    result = crypto_check_replay_attack(current_time + 120000, 12345); // 120 seconds in future
    ASSERT(!result);
    
    // Test low sequence number (should fail)
    result = crypto_check_replay_attack(current_time, 100); // Lower than previous
    ASSERT(!result);
    
    log_info("✅ Replay attack protection test passed");
    return true;
}

// Test: Certificate Validation
static bool test_certificate_validation(void) {
    log_info("Testing certificate validation...");
    
    // Test with dummy certificate data
    uint8_t dummy_cert[] = {0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01};
    
    bool result = crypto_validate_certificate(dummy_cert, sizeof(dummy_cert));
    ASSERT(result);
    
    // Test with empty certificate (should fail)
    result = crypto_validate_certificate(NULL, 0);
    ASSERT(!result);
    
    log_info("✅ Certificate validation test passed");
    return true;
}

// Test: Large Data Encryption
static bool test_large_data_encryption(void) {
    log_info("Testing large data encryption...");
    
    // Initialize crypto system
    crypto_init();
    crypto_set_key(test_key, sizeof(test_key));
    
    // Create large test data (1KB)
    uint8_t large_data[1024];
    for (size_t i = 0; i < sizeof(large_data); i++) {
        large_data[i] = (uint8_t)(i & 0xFF);
    }
    
    // Encrypt
    uint8_t ciphertext[1024 + 32]; // Extra space for padding
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    bool result = crypto_encrypt_aes_gcm(large_data, sizeof(large_data),
                                        NULL, 0,
                                        ciphertext, &ciphertext_len,
                                        tag, sizeof(tag));
    ASSERT(result);
    
    // Decrypt
    uint8_t decrypted[1024];
    size_t decrypted_len = sizeof(decrypted);
    
    result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len,
                                   NULL, 0,
                                   tag, sizeof(tag),
                                   decrypted, &decrypted_len);
    ASSERT(result);
    
    // Verify data integrity
    ASSERT_EQ(sizeof(large_data), decrypted_len);
    ASSERT_MEM_EQ(large_data, decrypted, sizeof(large_data));
    
    log_info("✅ Large data encryption test passed");
    return true;
}

// Test: Multiple Sequential Operations
static bool test_multiple_operations(void) {
    log_info("Testing multiple sequential operations...");
    
    // Initialize crypto system
    crypto_init();
    crypto_set_key(test_key, sizeof(test_key));
    
    // Perform multiple encrypt/decrypt operations
    for (int i = 0; i < 10; i++) {
        char test_msg[64];
        snprintf(test_msg, sizeof(test_msg), "Test message %d", i);
        
        uint8_t ciphertext[128];
        uint8_t tag[16];
        size_t ciphertext_len = sizeof(ciphertext);
        
        bool result = crypto_encrypt_aes_gcm((uint8_t*)test_msg, strlen(test_msg),
                                            NULL, 0,
                                            ciphertext, &ciphertext_len,
                                            tag, sizeof(tag));
        ASSERT(result);
        
        uint8_t decrypted[64];
        size_t decrypted_len = sizeof(decrypted);
        
        result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len,
                                       NULL, 0,
                                       tag, sizeof(tag),
                                       decrypted, &decrypted_len);
        ASSERT(result);
        
        ASSERT_EQ(strlen(test_msg), decrypted_len);
        ASSERT_MEM_EQ(test_msg, decrypted, strlen(test_msg));
    }
    
    log_info("✅ Multiple sequential operations test passed");
    return true;
}

// Test: Crypto Statistics
static bool test_crypto_statistics(void) {
    log_info("Testing crypto statistics...");
    
    // Initialize crypto system
    crypto_init();
    
    // Perform some operations
    uint8_t key[32];
    crypto_generate_key(key, sizeof(key));
    
    uint8_t ciphertext[256];
    uint8_t tag[16];
    size_t ciphertext_len = sizeof(ciphertext);
    
    crypto_encrypt_aes_gcm(test_plaintext, strlen((char*)test_plaintext),
                          NULL, 0,
                          ciphertext, &ciphertext_len,
                          tag, sizeof(tag));
    
    // Get statistics
    uint64_t messages_encrypted, messages_decrypted;
    uint64_t bytes_processed;
    float average_time_ms;
    
    crypto_get_statistics(&messages_encrypted, &messages_decrypted,
                         &bytes_processed, &average_time_ms);
    
    // Verify statistics are reasonable
    ASSERT(messages_encrypted > 0);
    ASSERT(bytes_processed > 0);
    ASSERT(average_time_ms >= 0.0f);
    
    log_info("✅ Crypto statistics test passed");
    return true;
}

// Main test runner
bool run_encryption_tests(void) {
    log_info("=== Starting AES-GCM Encryption Unit Tests ===");
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_crypto_init();
    all_passed &= test_key_generation();
    all_passed &= test_key_setting();
    all_passed &= test_aes_gcm_encryption();
    all_passed &= test_aes_gcm_decryption();
    all_passed &= test_authentication_tag_validation();
    all_passed &= test_hmac_generation();
    all_passed &= test_hmac_verification();
    all_passed &= test_diffie_hellman_exchange();
    all_passed &= test_replay_attack_protection();
    all_passed &= test_certificate_validation();
    all_passed &= test_large_data_encryption();
    all_passed &= test_multiple_operations();
    all_passed &= test_crypto_statistics();
    
    if (all_passed) {
        log_info("🎉 All encryption tests passed!");
    } else {
        log_error("❌ Some encryption tests failed!");
    }
    
    log_info("=== AES-GCM Encryption Unit Tests Complete ===");
    return all_passed;
}
