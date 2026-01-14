#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include "../core/core.h"

// Encryption algorithms
typedef enum {
    CRYPTO_ALGORITHM_AES_256_GCM = 0,
    CRYPTO_ALGORITHM_CHACHA20_POLY1305,
    CRYPTO_ALGORITHM_AES_256_CBC,
    CRYPTO_ALGORITHM_COUNT
} CryptoAlgorithm;

// Key exchange methods
typedef enum {
    KEY_EXCHANGE_DIFFIE_HELLMAN = 0,
    KEY_EXCHANGE_ELLIPTIC_CURVE,
    KEY_EXCHANGE_RSA,
    KEY_EXCHANGE_COUNT
} KeyExchangeMethod;

// Hash algorithms
typedef enum {
    HASH_ALGORITHM_SHA256 = 0,
    HASH_ALGORITHM_SHA384,
    HASH_ALGORITHM_SHA512,
    HASH_ALGORITHM_BLAKE2,
    HASH_ALGORITHM_COUNT
} HashAlgorithm;

// Certificate validation result
typedef enum {
    CERT_VALID = 0,
    CERT_INVALID_SIGNATURE,
    CERT_EXPIRED,
    CERT_NOT_YET_VALID,
    CERT_REVOKED,
    CERT_UNKNOWN_CA,
    CERT_INVALID_FORMAT
} CertificateValidationResult;

// Crypto statistics
typedef struct {
    uint64_t messages_encrypted;
    uint64_t messages_decrypted;
    uint64_t bytes_processed;
    float average_encryption_time_ms;
    float average_decryption_time_ms;
    uint64_t replay_attacks_blocked;
    uint64_t authentication_failures;
} CryptoStatistics;

// Key pair for asymmetric encryption
typedef struct {
    uint8_t private_key[256];
    size_t private_key_len;
    uint8_t public_key[256];
    size_t public_key_len;
    KeyExchangeMethod method;
} CryptoKeyPair;

// Digital signature
typedef struct {
    uint8_t signature[512];
    size_t signature_len;
    HashAlgorithm hash_algorithm;
} DigitalSignature;

// Crypto system interface
typedef struct CryptoSystem {
    // Lifecycle
    bool (*init)(struct CryptoSystem* crypto);
    void (*shutdown)(struct CryptoSystem* crypto);
    
    // Key management
    bool (*generate_key)(struct CryptoSystem* crypto, uint8_t* key, size_t key_size);
    bool (*set_key)(struct CryptoSystem* crypto, const uint8_t* key, size_t key_size);
    bool (*generate_keypair)(struct CryptoSystem* crypto, CryptoKeyPair* keypair, KeyExchangeMethod method);
    bool (*derive_shared_secret)(struct CryptoSystem* crypto, const CryptoKeyPair* local_keypair,
                                 const uint8_t* remote_public_key, size_t remote_key_len,
                                 uint8_t* shared_secret, size_t* secret_len);
    
    // Encryption/Decryption
    bool (*encrypt)(struct CryptoSystem* crypto, const uint8_t* plaintext, size_t plaintext_len,
                   const uint8_t* aad, size_t aad_len, uint8_t* ciphertext, size_t* ciphertext_len,
                   uint8_t* tag, size_t tag_len, CryptoAlgorithm algorithm);
    bool (*decrypt)(struct CryptoSystem* crypto, const uint8_t* ciphertext, size_t ciphertext_len,
                   const uint8_t* aad, size_t aad_len, const uint8_t* tag, size_t tag_len,
                   uint8_t* plaintext, size_t* plaintext_len, CryptoAlgorithm algorithm);
    
    // Authentication
    bool (*generate_hmac)(struct CryptoSystem* crypto, const uint8_t* data, size_t data_len,
                         const uint8_t* key, size_t key_len, uint8_t* hmac, size_t hmac_len,
                         HashAlgorithm algorithm);
    bool (*verify_hmac)(struct CryptoSystem* crypto, const uint8_t* data, size_t data_len,
                       const uint8_t* key, size_t key_len, const uint8_t* hmac, size_t hmac_len,
                       HashAlgorithm algorithm);
    
    // Digital signatures
    bool (*sign_data)(struct CryptoSystem* crypto, const uint8_t* data, size_t data_len,
                     const CryptoKeyPair* keypair, DigitalSignature* signature);
    bool (*verify_signature)(struct CryptoSystem* crypto, const uint8_t* data, size_t data_len,
                            const DigitalSignature* signature, const uint8_t* public_key, size_t key_len);
    
    // Security
    bool (*check_replay_attack)(struct CryptoSystem* crypto, uint64_t timestamp, uint64_t sequence);
    CertificateValidationResult (*validate_certificate)(struct CryptoSystem* crypto,
                                                       const uint8_t* certificate, size_t cert_len);
    
    // Performance
    bool (*benchmark_throughput)(struct CryptoSystem* crypto);
    CryptoStatistics (*get_statistics)(struct CryptoSystem* crypto);
    
    // Internal state
    bool initialized;
    CryptoAlgorithm default_algorithm;
    HashAlgorithm default_hash_algorithm;
    KeyExchangeMethod default_key_exchange;
    CryptoStatistics stats;
} CryptoSystem;

// Global crypto functions (simplified interface)
bool crypto_init(void);
bool crypto_generate_key(uint8_t* key, size_t key_size);
bool crypto_set_key(const uint8_t* key, size_t key_size);
bool crypto_encrypt_aes_gcm(const uint8_t* plaintext, size_t plaintext_len,
                           const uint8_t* aad, size_t aad_len,
                           uint8_t* ciphertext, size_t* ciphertext_len,
                           uint8_t* tag, size_t tag_len);
bool crypto_decrypt_aes_gcm(const uint8_t* ciphertext, size_t ciphertext_len,
                           const uint8_t* aad, size_t aad_len,
                           const uint8_t* tag, size_t tag_len,
                           uint8_t* plaintext, size_t* plaintext_len);
bool crypto_generate_hmac(const uint8_t* data, size_t data_len,
                         const uint8_t* key, size_t key_len,
                         uint8_t* hmac, size_t hmac_len);
bool crypto_verify_hmac(const uint8_t* data, size_t data_len,
                        const uint8_t* key, size_t key_len,
                        const uint8_t* hmac, size_t hmac_len);
bool crypto_diffie_hellman_exchange(const uint8_t* private_key, size_t private_key_len,
                                   const uint8_t* public_key, size_t public_key_len,
                                   uint8_t* shared_secret, size_t* shared_secret_len);
bool crypto_generate_dh_keypair(uint8_t* private_key, size_t* private_key_len,
                               uint8_t* public_key, size_t* public_key_len);
bool crypto_check_replay_attack(uint64_t timestamp, uint64_t sequence);
bool crypto_validate_certificate(const uint8_t* certificate, size_t cert_len);
bool crypto_benchmark_throughput(void);
void crypto_get_statistics(uint64_t* messages_encrypted, uint64_t* messages_decrypted,
                         uint64_t* bytes_processed, float* average_time_ms);

// Factory functions
CryptoSystem* crypto_create_system(void);
void crypto_destroy_system(CryptoSystem* crypto);

// Utility functions
const char* crypto_algorithm_to_string(CryptoAlgorithm algorithm);
const char* key_exchange_to_string(KeyExchangeMethod method);
const char* hash_algorithm_to_string(HashAlgorithm algorithm);
const char* cert_validation_to_string(CertificateValidationResult result);

size_t crypto_get_key_size(CryptoAlgorithm algorithm);
size_t crypto_get_iv_size(CryptoAlgorithm algorithm);
size_t crypto_get_tag_size(CryptoAlgorithm algorithm);
size_t crypto_get_hash_size(HashAlgorithm algorithm);

// =================================================================================================
//                              SECURE DATA STORAGE
// =================================================================================================

// Secure storage initialization and management
bool encryption_init(uint64_t key_rotation_interval_ms);
void encryption_shutdown(void);

// Player data encryption/decryption
bool encryption_encrypt_player_data(uint64_t player_id, const char* data, char* encrypted_output, size_t output_size);
bool encryption_decrypt_player_data(uint64_t player_id, const char* encrypted_data, char* decrypted_output, size_t output_size);

// Password security
bool encryption_hash_password(const char* password, const char* salt, char* hash_output, size_t output_size);
bool encryption_verify_password(const char* password, const char* stored_hash, const char* salt);
bool encryption_generate_salt(char* salt_output, size_t output_size);

// Key management
bool encryption_rotate_player_key(uint64_t player_id);
bool encryption_rotate_master_key(void);
bool encryption_should_rotate_keys(void);
void encryption_update_key_rotation_interval(uint64_t interval_ms);

// Statistics
void encryption_get_statistics(uint32_t* encryptions, uint32_t* decryptions, uint32_t* key_rotations);

#endif // ENCRYPTION_H
