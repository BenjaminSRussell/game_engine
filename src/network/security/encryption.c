// Secure Data Storage and Encryption Implementation
#include "network/security/encryption.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =================================================================================================
//                              ENCRYPTION SYSTEM
// =================================================================================================

#define AES_KEY_SIZE 256
#define RSA_KEY_SIZE 2048
#define SALT_SIZE 32
#define IV_SIZE 16
#define HASH_SIZE 32
#define MAX_ENCRYPTED_DATA_SIZE 4096

// Simple XOR-based encryption for demonstration (in production, use proper crypto libraries)
typedef struct EncryptionKey {
    uint8_t data[AES_KEY_SIZE / 8];
    uint8_t iv[IV_SIZE];
    uint64_t created_time;
    uint64_t expires_time;
    bool is_valid;
} EncryptionKey;

typedef struct SecureStorage {
    EncryptionKey master_key;
    EncryptionKey* player_keys;
    uint32_t key_count;
    uint32_t key_capacity;
    
    // Key rotation settings
    uint64_t key_rotation_interval_ms;
    uint64_t last_rotation_time;
    
    // Statistics
    uint32_t encryptions_performed;
    uint32_t decryptions_performed;
    uint32_t key_rotations;
} SecureStorage;

static SecureStorage g_secure_storage = {0};

// Helper functions
static void generate_random_bytes(uint8_t* buffer, size_t size) {
    // Simple pseudo-random generator (in production, use cryptographically secure RNG)
    static uint32_t seed = 0;
    if (seed == 0) {
        seed = (uint32_t)time(NULL);
    }
    
    for (size_t i = 0; i < size; i++) {
        seed = seed * 1103515245 + 12345;
        buffer[i] = (uint8_t)(seed >> 24);
    }
}

static void xor_encrypt(const uint8_t* input, uint8_t* output, size_t size, const uint8_t* key) {
    for (size_t i = 0; i < size; i++) {
        output[i] = input[i] ^ key[i % (AES_KEY_SIZE / 8)];
    }
}

static void xor_decrypt(const uint8_t* input, uint8_t* output, size_t size, const uint8_t* key) {
    // XOR is symmetric, so decryption is the same as encryption
    xor_encrypt(input, output, size, key);
}

static bool derive_key_from_password(const char* password, const uint8_t* salt, 
                                   uint8_t* key, size_t key_size) {
    if (!password || !salt || !key) {
        return false;
    }
    
    // Simple key derivation (in production, use PBKDF2 or Argon2)
    size_t password_len = strlen(password);
    
    for (size_t i = 0; i < key_size; i++) {
        uint8_t hash = 0;
        for (size_t j = 0; j < password_len; j++) {
            hash += password[j] + salt[i % SALT_SIZE];
        }
        key[i] = hash ^ salt[i % SALT_SIZE];
    }
    
    return true;
}

static EncryptionKey* find_or_create_player_key(uint64_t player_id) {
    // Find existing key
    for (uint32_t i = 0; i < g_secure_storage.key_count; i++) {
        // In a real implementation, we'd store player_id with the key
        // For now, just return the first available key
        return &g_secure_storage.player_keys[i];
    }
    
    // Create new key
    if (g_secure_storage.key_count >= g_secure_storage.key_capacity) {
        uint32_t new_capacity = g_secure_storage.key_capacity == 0 ? 64 : g_secure_storage.key_capacity * 2;
        EncryptionKey* new_keys = realloc(g_secure_storage.player_keys, new_capacity * sizeof(EncryptionKey));
        if (!new_keys) {
            return NULL;
        }
        g_secure_storage.player_keys = new_keys;
        g_secure_storage.key_capacity = new_capacity;
    }
    
    EncryptionKey* key = &g_secure_storage.player_keys[g_secure_storage.key_count++];
    generate_random_bytes(key->data, sizeof(key->data));
    generate_random_bytes(key->iv, sizeof(key->iv));
    key->created_time = get_current_time_ms();
    key->expires_time = key->created_time + g_secure_storage.key_rotation_interval_ms;
    key->is_valid = true;
    
    return key;
}

// Main encryption functions
bool encryption_init(uint64_t key_rotation_interval_ms) {
    memset(&g_secure_storage, 0, sizeof(SecureStorage));
    
    // Generate master key
    generate_random_bytes(g_secure_storage.master_key.data, sizeof(g_secure_storage.master_key.data));
    generate_random_bytes(g_secure_storage.master_key.iv, sizeof(g_secure_storage.master_key.iv));
    g_secure_storage.master_key.created_time = get_current_time_ms();
    g_secure_storage.master_key.expires_time = g_secure_storage.master_key.created_time + key_rotation_interval_ms;
    g_secure_storage.master_key.is_valid = true;
    
    g_secure_storage.key_rotation_interval_ms = key_rotation_interval_ms;
    g_secure_storage.last_rotation_time = get_current_time_ms();
    
    LOG_INFO("Encryption system initialized with key rotation interval %llu ms", key_rotation_interval_ms);
    return true;
}

void encryption_shutdown(void) {
    // Clear sensitive data
    if (g_secure_storage.player_keys) {
        memset(g_secure_storage.player_keys, 0, g_secure_storage.key_capacity * sizeof(EncryptionKey));
        free(g_secure_storage.player_keys);
    }
    memset(&g_secure_storage, 0, sizeof(SecureStorage));
    
    LOG_INFO("Encryption system shutdown and sensitive data cleared");
}

bool encryption_encrypt_player_data(uint64_t player_id, const char* data, char* encrypted_output, size_t output_size) {
    if (!data || !encrypted_output || output_size == 0) {
        return false;
    }
    
    size_t data_len = strlen(data);
    if (data_len >= MAX_ENCRYPTED_DATA_SIZE) {
        LOG_ERROR("Data too large for encryption: %zu bytes", data_len);
        return false;
    }
    
    EncryptionKey* key = find_or_create_player_key(player_id);
    if (!key || !key->is_valid) {
        LOG_ERROR("Failed to get encryption key for player %llu", player_id);
        return false;
    }
    
    // Check if key needs rotation
    uint64_t current_time = get_current_time_ms();
    if (current_time >= key->expires_time) {
        encryption_rotate_player_key(player_id);
        key = find_or_create_player_key(player_id);
        if (!key) {
            return false;
        }
    }
    
    // Simple encryption: XOR with key + base64 encode for storage
    uint8_t encrypted_raw[MAX_ENCRYPTED_DATA_SIZE];
    xor_encrypt((const uint8_t*)data, encrypted_raw, data_len, key->data);
    
    // Simple base64-like encoding for storage (in production, use proper base64)
    size_t encoded_len = 0;
    for (size_t i = 0; i < data_len && encoded_len < output_size - 1; i++) {
        // Convert to hex representation
        uint8_t byte = encrypted_raw[i];
        if (encoded_len + 3 < output_size) {
            snprintf(encrypted_output + encoded_len, 4, "%02x", byte);
            encoded_len += 2;
        }
    }
    encrypted_output[encoded_len] = '\0';
    
    g_secure_storage.encryptions_performed++;
    LOG_DEBUG("Encrypted %zu bytes for player %llu", data_len, player_id);
    
    return true;
}

bool encryption_decrypt_player_data(uint64_t player_id, const char* encrypted_data, char* decrypted_output, size_t output_size) {
    if (!encrypted_data || !decrypted_output || output_size == 0) {
        return false;
    }
    
    size_t encrypted_len = strlen(encrypted_data);
    if (encrypted_len >= MAX_ENCRYPTED_DATA_SIZE * 2) { // hex encoding doubles size
        LOG_ERROR("Encrypted data too large: %zu bytes", encrypted_len);
        return false;
    }
    
    EncryptionKey* key = find_or_create_player_key(player_id);
    if (!key || !key->is_valid) {
        LOG_ERROR("Failed to get decryption key for player %llu", player_id);
        return false;
    }
    
    // Decode from hex-like representation
    uint8_t encrypted_raw[MAX_ENCRYPTED_DATA_SIZE];
    size_t decoded_len = 0;
    
    for (size_t i = 0; i < encrypted_len - 1 && decoded_len < MAX_ENCRYPTED_DATA_SIZE; i += 2) {
        char hex_byte[3] = {encrypted_data[i], encrypted_data[i + 1], '\0'};
        encrypted_raw[decoded_len++] = (uint8_t)strtol(hex_byte, NULL, 16);
    }
    
    // Decrypt
    xor_decrypt(encrypted_raw, (uint8_t*)decrypted_output, decoded_len, key->data);
    decrypted_output[decoded_len] = '\0';
    
    g_secure_storage.decryptions_performed++;
    LOG_DEBUG("Decrypted %zu bytes for player %llu", decoded_len, player_id);
    
    return true;
}

bool encryption_hash_password(const char* password, const char* salt, char* hash_output, size_t output_size) {
    if (!password || !salt || !hash_output || output_size == 0) {
        return false;
    }
    
    uint8_t salt_bytes[SALT_SIZE];
    uint8_t key_bytes[AES_KEY_SIZE / 8];
    
    // Convert salt string to bytes (simple approach)
    size_t salt_len = strlen(salt);
    for (size_t i = 0; i < SALT_SIZE; i++) {
        salt_bytes[i] = (uint8_t)salt[i % salt_len];
    }
    
    // Derive key from password
    if (!derive_key_from_password(password, salt_bytes, key_bytes, sizeof(key_bytes))) {
        return false;
    }
    
    // Convert key to hex string for storage
    for (size_t i = 0; i < sizeof(key_bytes) && (i * 2) < output_size - 1; i++) {
        snprintf(hash_output + (i * 2), 3, "%02x", key_bytes[i]);
    }
    hash_output[sizeof(key_bytes) * 2] = '\0';
    
    return true;
}

bool encryption_verify_password(const char* password, const char* stored_hash, const char* salt) {
    if (!password || !stored_hash || !salt) {
        return false;
    }
    
    char computed_hash[HASH_SIZE * 2 + 1];
    if (!encryption_hash_password(password, salt, computed_hash, sizeof(computed_hash))) {
        return false;
    }
    
    return strcmp(computed_hash, stored_hash) == 0;
}

bool encryption_generate_salt(char* salt_output, size_t output_size) {
    if (!salt_output || output_size == 0) {
        return false;
    }
    
    uint8_t salt_bytes[SALT_SIZE];
    generate_random_bytes(salt_bytes, sizeof(salt_bytes));
    
    // Convert to hex string
    for (size_t i = 0; i < SALT_SIZE && (i * 2) < output_size - 1; i++) {
        snprintf(salt_output + (i * 2), 3, "%02x", salt_bytes[i]);
    }
    salt_output[SALT_SIZE * 2] = '\0';
    
    return true;
}

bool encryption_rotate_player_key(uint64_t player_id) {
    EncryptionKey* key = find_or_create_player_key(player_id);
    if (!key) {
        return false;
    }
    
    // Generate new key
    generate_random_bytes(key->data, sizeof(key->data));
    generate_random_bytes(key->iv, sizeof(key->iv));
    key->created_time = get_current_time_ms();
    key->expires_time = key->created_time + g_secure_storage.key_rotation_interval_ms;
    key->is_valid = true;
    
    g_secure_storage.key_rotations++;
    LOG_INFO("Rotated encryption key for player %llu", player_id);
    
    return true;
}

bool encryption_rotate_master_key(void) {
    // Generate new master key
    generate_random_bytes(g_secure_storage.master_key.data, sizeof(g_secure_storage.master_key.data));
    generate_random_bytes(g_secure_storage.master_key.iv, sizeof(g_secure_storage.master_key.iv));
    g_secure_storage.master_key.created_time = get_current_time_ms();
    g_secure_storage.master_key.expires_time = g_secure_storage.master_key.created_time + g_secure_storage.key_rotation_interval_ms;
    g_secure_storage.master_key.is_valid = true;
    
    g_secure_storage.last_rotation_time = get_current_time_ms();
    g_secure_storage.key_rotations++;
    
    LOG_INFO("Rotated master encryption key");
    
    // In a real implementation, we'd need to re-encrypt all player data with the new master key
    return true;
}

void encryption_get_statistics(uint32_t* encryptions, uint32_t* decryptions, uint32_t* key_rotations) {
    if (encryptions) *encryptions = g_secure_storage.encryptions_performed;
    if (decryptions) *decryptions = g_secure_storage.decryptions_performed;
    if (key_rotations) *key_rotations = g_secure_storage.key_rotations;
}

bool encryption_should_rotate_keys(void) {
    uint64_t current_time = get_current_time_ms();
    return (current_time - g_secure_storage.last_rotation_time) >= g_secure_storage.key_rotation_interval_ms;
}

void encryption_update_key_rotation_interval(uint64_t interval_ms) {
    g_secure_storage.key_rotation_interval_ms = interval_ms;
    LOG_INFO("Updated key rotation interval to %llu ms", interval_ms);
}
