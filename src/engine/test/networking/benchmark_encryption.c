#include "networking/security/encryption.h"
#include "core/memory.h"
#include "core/log.h"
#include "core/time.h"
#include "core/random.h"
#include <stdlib.h>
#include <string.h>

// ✅ COMPLETED: NET_TEST_007 - Performance Benchmarks for Encryption Throughput
// Features: Comprehensive performance testing for AES-GCM encryption and related operations

// Benchmark configuration
typedef struct {
    size_t data_size;
    uint32_t iterations;
    uint32_t warmup_iterations;
    bool detailed_logging;
} BenchmarkConfig;

// Benchmark results
typedef struct {
    double encryption_throughput_mbps;
    double decryption_throughput_mbps;
    double hmac_throughput_mbps;
    double key_exchange_ops_per_sec;
    double average_encryption_time_ms;
    double average_decryption_time_ms;
    double average_hmac_time_ms;
    double average_key_exchange_time_ms;
    uint64_t total_encryption_time_ns;
    uint64_t total_decryption_time_ns;
    uint64_t total_hmac_time_ns;
    uint64_t total_key_exchange_time_ns;
    uint32_t successful_operations;
    uint32_t failed_operations;
} BenchmarkResults;

// Test data generation
static uint8_t* generate_test_data(size_t size) {
    uint8_t* data = memory_alloc(size);
    if (!data) return NULL;
    
    for (size_t i = 0; i < size; i++) {
        data[i] = (uint8_t)random_get_uint32();
    }
    
    return data;
}

// Benchmark: AES-GCM Encryption Throughput
static bool benchmark_encryption_throughput(const BenchmarkConfig* config, BenchmarkResults* results) {
    log_info("Benchmarking AES-GCM encryption throughput...");
    
    // Initialize crypto system
    crypto_init();
    
    // Generate test data
    uint8_t* test_data = generate_test_data(config->data_size);
    if (!test_data) {
        log_error("Failed to generate test data");
        return false;
    }
    
    // Generate encryption key
    uint8_t key[32];
    if (!crypto_generate_key(key, sizeof(key))) {
        memory_free(test_data);
        log_error("Failed to generate encryption key");
        return false;
    }
    
    crypto_set_key(key, sizeof(key));
    
    // Allocate buffers
    uint8_t* ciphertext = memory_alloc(config->data_size + 32); // Extra space for padding
    uint8_t* tag = memory_alloc(16);
    if (!ciphertext || !tag) {
        memory_free(test_data);
        memory_free(ciphertext);
        memory_free(tag);
        log_error("Failed to allocate buffers");
        return false;
    }
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        size_t ciphertext_len = config->data_size + 32;
        crypto_encrypt_aes_gcm(test_data, config->data_size, NULL, 0,
                             ciphertext, &ciphertext_len, tag, 16);
    }
    
    // Benchmark encryption
    uint64_t start_time = time_get_current_ns();
    uint32_t successful_ops = 0;
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        size_t ciphertext_len = config->data_size + 32;
        
        bool result = crypto_encrypt_aes_gcm(test_data, config->data_size, NULL, 0,
                                            ciphertext, &ciphertext_len, tag, 16);
        if (result) {
            successful_ops++;
        }
    }
    
    uint64_t end_time = time_get_current_ns();
    uint64_t total_time_ns = end_time - start_time;
    
    // Calculate results
    double total_time_ms = (double)total_time_ns / 1000000.0;
    double total_data_mb = (double)(config->data_size * successful_ops) / (1024.0 * 1024.0);
    
    results->encryption_throughput_mbps = total_data_mb / (total_time_ms / 1000.0);
    results->average_encryption_time_ms = total_time_ms / config->iterations;
    results->total_encryption_time_ns = total_time_ns;
    results->successful_operations = successful_ops;
    results->failed_operations = config->iterations - successful_ops;
    
    // Cleanup
    memory_free(test_data);
    memory_free(ciphertext);
    memory_free(tag);
    
    log_info("Encryption throughput: %.2f MB/s (%.3f ms per operation)", 
           results->encryption_throughput_mbps, results->average_encryption_time_ms);
    
    return true;
}

// Benchmark: AES-GCM Decryption Throughput
static bool benchmark_decryption_throughput(const BenchmarkConfig* config, BenchmarkResults* results) {
    log_info("Benchmarking AES-GCM decryption throughput...");
    
    // Initialize crypto system
    crypto_init();
    
    // Generate test data and encrypt it
    uint8_t* test_data = generate_test_data(config->data_size);
    uint8_t* ciphertext = memory_alloc(config->data_size + 32);
    uint8_t* tag = memory_alloc(16);
    uint8_t* decrypted = memory_alloc(config->data_size);
    
    if (!test_data || !ciphertext || !tag || !decrypted) {
        memory_free(test_data);
        memory_free(ciphertext);
        memory_free(tag);
        memory_free(decrypted);
        log_error("Failed to allocate buffers");
        return false;
    }
    
    // Generate key and encrypt test data
    uint8_t key[32];
    crypto_generate_key(key, sizeof(key));
    crypto_set_key(key, sizeof(key));
    
    size_t ciphertext_len = config->data_size + 32;
    crypto_encrypt_aes_gcm(test_data, config->data_size, NULL, 0,
                          ciphertext, &ciphertext_len, tag, 16);
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        size_t decrypted_len = config->data_size;
        crypto_decrypt_aes_gcm(ciphertext, ciphertext_len, NULL, 0,
                              tag, 16, decrypted, &decrypted_len);
    }
    
    // Benchmark decryption
    uint64_t start_time = time_get_current_ns();
    uint32_t successful_ops = 0;
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        size_t decrypted_len = config->data_size;
        
        bool result = crypto_decrypt_aes_gcm(ciphertext, ciphertext_len, NULL, 0,
                                            tag, 16, decrypted, &decrypted_len);
        if (result) {
            successful_ops++;
        }
    }
    
    uint64_t end_time = time_get_current_ns();
    uint64_t total_time_ns = end_time - start_time;
    
    // Calculate results
    double total_time_ms = (double)total_time_ns / 1000000.0;
    double total_data_mb = (double)(config->data_size * successful_ops) / (1024.0 * 1024.0);
    
    results->decryption_throughput_mbps = total_data_mb / (total_time_ms / 1000.0);
    results->average_decryption_time_ms = total_time_ms / config->iterations;
    results->total_decryption_time_ns = total_time_ns;
    
    // Cleanup
    memory_free(test_data);
    memory_free(ciphertext);
    memory_free(tag);
    memory_free(decrypted);
    
    log_info("Decryption throughput: %.2f MB/s (%.3f ms per operation)", 
           results->decryption_throughput_mbps, results->average_decryption_time_ms);
    
    return true;
}

// Benchmark: HMAC Generation Throughput
static bool benchmark_hmac_throughput(const BenchmarkConfig* config, BenchmarkResults* results) {
    log_info("Benchmarking HMAC generation throughput...");
    
    // Generate test data
    uint8_t* test_data = generate_test_data(config->data_size);
    uint8_t* key = generate_test_data(32);
    uint8_t* hmac = memory_alloc(32);
    
    if (!test_data || !key || !hmac) {
        memory_free(test_data);
        memory_free(key);
        memory_free(hmac);
        log_error("Failed to allocate buffers");
        return false;
    }
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        crypto_generate_hmac(test_data, config->data_size, key, 32, hmac, 32);
    }
    
    // Benchmark HMAC generation
    uint64_t start_time = time_get_current_ns();
    uint32_t successful_ops = 0;
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        bool result = crypto_generate_hmac(test_data, config->data_size, key, 32, hmac, 32);
        if (result) {
            successful_ops++;
        }
    }
    
    uint64_t end_time = time_get_current_ns();
    uint64_t total_time_ns = end_time - start_time;
    
    // Calculate results
    double total_time_ms = (double)total_time_ns / 1000000.0;
    double total_data_mb = (double)(config->data_size * successful_ops) / (1024.0 * 1024.0);
    
    results->hmac_throughput_mbps = total_data_mb / (total_time_ms / 1000.0);
    results->average_hmac_time_ms = total_time_ms / config->iterations;
    results->total_hmac_time_ns = total_time_ns;
    
    // Cleanup
    memory_free(test_data);
    memory_free(key);
    memory_free(hmac);
    
    log_info("HMAC throughput: %.2f MB/s (%.3f ms per operation)", 
           results->hmac_throughput_mbps, results->average_hmac_time_ms);
    
    return true;
}

// Benchmark: Diffie-Hellman Key Exchange
static bool benchmark_key_exchange(const BenchmarkConfig* config, BenchmarkResults* results) {
    log_info("Benchmarking Diffie-Hellman key exchange...");
    
    // Warmup iterations
    for (uint32_t i = 0; i < config->warmup_iterations; i++) {
        uint8_t priv1[32], pub1[32], priv2[32], pub2[32], shared1[32], shared2[32];
        size_t priv1_len = 32, pub1_len = 32, priv2_len = 32, pub2_len = 32;
        size_t shared1_len = 32, shared2_len = 32;
        
        crypto_generate_dh_keypair(priv1, &priv1_len, pub1, &pub1_len);
        crypto_generate_dh_keypair(priv2, &priv2_len, pub2, &pub2_len);
        crypto_diffie_hellman_exchange(priv1, priv1_len, pub2, pub2_len, shared1, &shared1_len);
        crypto_diffie_hellman_exchange(priv2, priv2_len, pub1, pub1_len, shared2, &shared2_len);
    }
    
    // Benchmark key exchange
    uint64_t start_time = time_get_current_ns();
    uint32_t successful_ops = 0;
    
    for (uint32_t i = 0; i < config->iterations; i++) {
        uint8_t priv1[32], pub1[32], priv2[32], pub2[32], shared1[32], shared2[32];
        size_t priv1_len = 32, pub1_len = 32, priv2_len = 32, pub2_len = 32;
        size_t shared1_len = 32, shared2_len = 32;
        
        bool result1 = crypto_generate_dh_keypair(priv1, &priv1_len, pub1, &pub1_len);
        bool result2 = crypto_generate_dh_keypair(priv2, &priv2_len, pub2, &pub2_len);
        bool result3 = crypto_diffie_hellman_exchange(priv1, priv1_len, pub2, pub2_len, shared1, &shared1_len);
        bool result4 = crypto_diffie_hellman_exchange(priv2, priv2_len, pub1, pub1_len, shared2, &shared2_len);
        
        if (result1 && result2 && result3 && result4) {
            successful_ops++;
        }
    }
    
    uint64_t end_time = time_get_current_ns();
    uint64_t total_time_ns = end_time - start_time;
    
    // Calculate results
    double total_time_ms = (double)total_time_ns / 1000000.0;
    
    results->key_exchange_ops_per_sec = successful_ops / (total_time_ms / 1000.0);
    results->average_key_exchange_time_ms = total_time_ms / config->iterations;
    results->total_key_exchange_time_ns = total_time_ns;
    
    log_info("Key exchange: %.0f operations/sec (%.3f ms per operation)", 
           results->key_exchange_ops_per_sec, results->average_key_exchange_time_ms);
    
    return true;
}

// Comprehensive benchmark suite
static bool run_comprehensive_benchmark(const BenchmarkConfig* config) {
    log_info("=== Starting Comprehensive Encryption Performance Benchmark ===");
    log_info("Data size: %zu bytes, Iterations: %u", config->data_size, config->iterations);
    
    BenchmarkResults results = {0};
    bool success = true;
    
    // Run all benchmarks
    success &= benchmark_encryption_throughput(config, &results);
    success &= benchmark_decryption_throughput(config, &results);
    success &= benchmark_hmac_throughput(config, &results);
    success &= benchmark_key_exchange(config, &results);
    
    // Print comprehensive results
    log_info("\n=== BENCHMARK RESULTS ===");
    log_info("Encryption Throughput: %.2f MB/s", results.encryption_throughput_mbps);
    log_info("Decryption Throughput: %.2f MB/s", results.decryption_throughput_mbps);
    log_info("HMAC Throughput: %.2f MB/s", results.hmac_throughput_mbps);
    log_info("Key Exchange Ops/sec: %.0f", results.key_exchange_ops_per_sec);
    
    log_info("\n=== TIMING BREAKDOWN ===");
    log_info("Avg Encryption Time: %.3f ms", results.average_encryption_time_ms);
    log_info("Avg Decryption Time: %.3f ms", results.average_decryption_time_ms);
    log_info("Avg HMAC Time: %.3f ms", results.average_hmac_time_ms);
    log_info("Avg Key Exchange Time: %.3f ms", results.average_key_exchange_time_ms);
    
    log_info("\n=== OPERATIONS SUMMARY ===");
    log_info("Successful Operations: %u", results.successful_operations);
    log_info("Failed Operations: %u", results.failed_operations);
    
    // Performance assessment
    log_info("\n=== PERFORMANCE ASSESSMENT ===");
    
    if (results.encryption_throughput_mbps > 100) {
        log_info("✅ Encryption throughput: EXCELLENT (>100 MB/s)");
    } else if (results.encryption_throughput_mbps > 50) {
        log_info("✅ Encryption throughput: GOOD (>50 MB/s)");
    } else if (results.encryption_throughput_mbps > 20) {
        log_info("⚠️  Encryption throughput: ACCEPTABLE (>20 MB/s)");
    } else {
        log_info("❌ Encryption throughput: POOR (<20 MB/s)");
    }
    
    if (results.key_exchange_ops_per_sec > 1000) {
        log_info("✅ Key exchange performance: EXCELLENT (>1000 ops/sec)");
    } else if (results.key_exchange_ops_per_sec > 500) {
        log_info("✅ Key exchange performance: GOOD (>500 ops/sec)");
    } else if (results.key_exchange_ops_per_sec > 200) {
        log_info("⚠️  Key exchange performance: ACCEPTABLE (>200 ops/sec)");
    } else {
        log_info("❌ Key exchange performance: POOR (<200 ops/sec)");
    }
    
    log_info("=== Benchmark Complete ===\n");
    
    return success;
}

// Multi-size benchmark
static bool run_multi_size_benchmark(void) {
    log_info("=== Running Multi-Size Benchmark Suite ===");
    
    // Test different data sizes
    size_t test_sizes[] = {64, 256, 1024, 4096, 16384, 65536};
    size_t num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t i = 0; i < num_sizes; i++) {
        BenchmarkConfig config = {0};
        config.data_size = test_sizes[i];
        config.iterations = 1000;
        config.warmup_iterations = 100;
        config.detailed_logging = (i == 0); // Only detailed for first test
        
        log_info("\n--- Testing %zu byte packets ---", config.data_size);
        run_comprehensive_benchmark(&config);
    }
    
    return true;
}

// Stress test benchmark
static bool run_stress_benchmark(void) {
    log_info("=== Running Stress Benchmark ===");
    
    BenchmarkConfig config = {0};
    config.data_size = 4096; // 4KB packets
    config.iterations = 10000; // High iteration count
    config.warmup_iterations = 1000;
    config.detailed_logging = false;
    
    return run_comprehensive_benchmark(&config);
}

// Main benchmark runner
bool run_encryption_benchmarks(void) {
    log_info("=== Starting Encryption Performance Benchmarks ===");
    
    bool all_passed = true;
    
    // Run standard benchmark suite
    BenchmarkConfig standard_config = {0};
    standard_config.data_size = 1024; // 1KB
    standard_config.iterations = 5000;
    standard_config.warmup_iterations = 500;
    standard_config.detailed_logging = true;
    
    all_passed &= run_comprehensive_benchmark(&standard_config);
    
    // Run multi-size benchmark
    all_passed &= run_multi_size_benchmark();
    
    // Run stress benchmark
    all_passed &= run_stress_benchmark();
    
    if (all_passed) {
        log_info("🎉 All encryption benchmarks completed successfully!");
    } else {
        log_error("❌ Some encryption benchmarks failed!");
    }
    
    log_info("=== Encryption Performance Benchmarks Complete ===");
    return all_passed;
}
