/*
 * encryption_test.c
 * Player-data encrypt/decrypt, password hashing, and key rotation.
 * Compiles encryption.c directly and does not link Engine.
 */

#include "network/security/encryption.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

u64 get_current_time_ms(void) { return 1000000; }

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static int is_hex(const char *text) {
    size_t i;
    if (!text || text[0] == '\0') {
        return 0;
    }
    for (i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        int digit = (c >= '0' && c <= '9');
        int lower = (c >= 'a' && c <= 'f');
        if (!digit && !lower) {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    char encrypted[256];
    char decrypted[256];
    char encrypted_again[256];
    char after_rotate[256];
    char hash_a[128];
    char hash_b[128];
    char hash_wrong[128];
    char salt[128];
    char *too_big;
    uint32_t encryptions = 0;
    uint32_t decryptions = 0;
    uint32_t rotations = 0;
    const char *plain = "player-save-v1";
    const uint64_t player_id = 42;

    expect_true(!encryption_encrypt_player_data(player_id, NULL, encrypted, sizeof(encrypted)),
                "null plaintext");
    expect_true(!encryption_encrypt_player_data(player_id, plain, NULL, sizeof(encrypted)),
                "null encrypt output");
    expect_true(!encryption_encrypt_player_data(player_id, plain, encrypted, 0),
                "zero encrypt output");
    expect_true(!encryption_decrypt_player_data(player_id, NULL, decrypted, sizeof(decrypted)),
                "null ciphertext");
    expect_true(!encryption_decrypt_player_data(player_id, "aa", NULL, sizeof(decrypted)),
                "null decrypt output");
    expect_true(!encryption_decrypt_player_data(player_id, "aa", decrypted, 0),
                "zero decrypt output");
    expect_true(!encryption_hash_password(NULL, "salt", hash_a, sizeof(hash_a)),
                "null password");
    expect_true(!encryption_hash_password("pw", NULL, hash_a, sizeof(hash_a)),
                "null salt");
    expect_true(!encryption_hash_password("pw", "salt", NULL, sizeof(hash_a)),
                "null hash output");
    expect_true(!encryption_hash_password("pw", "salt", hash_a, 0),
                "zero hash output");
    expect_true(!encryption_verify_password(NULL, "hash", "salt"), "null verify password");
    expect_true(!encryption_verify_password("pw", NULL, "salt"), "null stored hash");
    expect_true(!encryption_verify_password("pw", "hash", NULL), "null verify salt");
    expect_true(!encryption_generate_salt(NULL, sizeof(salt)), "null salt output");
    expect_true(!encryption_generate_salt(salt, 0), "zero salt output");

    expect_true(encryption_init(60ull * 60ull * 1000ull), "init");
    expect_true(!encryption_should_rotate_keys(), "interval not elapsed");

    too_big = (char *)malloc(4097);
    if (!too_big) {
        fail("oom");
    }
    memset(too_big, 'x', 4096);
    too_big[4096] = '\0';
    expect_true(!encryption_encrypt_player_data(player_id, too_big, encrypted, sizeof(encrypted)),
                "oversized plaintext");
    free(too_big);

    expect_true(encryption_encrypt_player_data(player_id, plain, encrypted, sizeof(encrypted)),
                "encrypt");
    expect_true(strcmp(encrypted, plain) != 0, "ciphertext is not plaintext");
    expect_true(is_hex(encrypted), "ciphertext hex");
    expect_true(strlen(encrypted) == strlen(plain) * 2, "ciphertext length");

    expect_true(encryption_encrypt_player_data(player_id, plain, encrypted_again, sizeof(encrypted_again)),
                "encrypt again");
    expect_true(strcmp(encrypted, encrypted_again) == 0, "same key is deterministic");

    expect_true(encryption_decrypt_player_data(player_id, encrypted, decrypted, sizeof(decrypted)),
                "decrypt");
    expect_true(strcmp(decrypted, plain) == 0, "round trip");

    encryption_get_statistics(&encryptions, &decryptions, &rotations);
    expect_true(encryptions == 2, "encryption count");
    expect_true(decryptions == 1, "decryption count");
    expect_true(rotations == 0, "no rotations yet");

    expect_true(encryption_generate_salt(salt, sizeof(salt)), "salt");
    expect_true(strlen(salt) == 64, "salt length");
    expect_true(is_hex(salt), "salt hex");

    expect_true(encryption_hash_password("s3cret", salt, hash_a, sizeof(hash_a)), "hash");
    expect_true(strlen(hash_a) == 64, "hash length");
    expect_true(is_hex(hash_a), "hash hex");
    expect_true(encryption_hash_password("s3cret", salt, hash_b, sizeof(hash_b)), "hash again");
    expect_true(strcmp(hash_a, hash_b) == 0, "hash is deterministic");
    expect_true(encryption_hash_password("other", salt, hash_wrong, sizeof(hash_wrong)),
                "other hash");
    expect_true(strcmp(hash_a, hash_wrong) != 0, "different password");
    expect_true(encryption_verify_password("s3cret", hash_a, salt), "verify");
    expect_true(!encryption_verify_password("other", hash_a, salt), "reject wrong password");
    expect_true(!encryption_verify_password("s3cret", hash_a, "different-salt"),
                "reject wrong salt");

    expect_true(encryption_rotate_player_key(player_id), "rotate player key");
    expect_true(encryption_decrypt_player_data(player_id, encrypted, after_rotate, sizeof(after_rotate)),
                "decrypt after rotate");
    expect_true(strcmp(after_rotate, plain) != 0, "old ciphertext uses the new key");
    expect_true(encryption_encrypt_player_data(player_id, plain, encrypted_again, sizeof(encrypted_again)),
                "encrypt after rotate");
    expect_true(strcmp(encrypted, encrypted_again) != 0, "rotated key changes ciphertext");
    expect_true(encryption_decrypt_player_data(player_id, encrypted_again, decrypted, sizeof(decrypted)),
                "decrypt new ciphertext");
    expect_true(strcmp(decrypted, plain) == 0, "round trip after rotate");

    expect_true(encryption_rotate_master_key(), "rotate master");
    expect_true(encryption_decrypt_player_data(player_id, encrypted_again, decrypted, sizeof(decrypted)),
                "player data survives master rotation");
    expect_true(strcmp(decrypted, plain) == 0, "master rotation does not change player key");
    expect_true(!encryption_should_rotate_keys(), "master rotation refreshes the timer");

    encryption_get_statistics(&encryptions, &decryptions, &rotations);
    expect_true(encryptions == 3, "encryption count after rotate");
    expect_true(decryptions == 4, "decryption count after rotate");
    expect_true(rotations == 2, "player and master rotations");

    encryption_update_key_rotation_interval(0);
    expect_true(encryption_should_rotate_keys(), "zero interval is due");

    encryption_shutdown();
    expect_true(encryption_init(60ull * 60ull * 1000ull), "re-init");
    expect_true(encryption_encrypt_player_data(player_id, plain, encrypted, sizeof(encrypted)),
                "encrypt after re-init");
    expect_true(encryption_decrypt_player_data(player_id, encrypted, decrypted, sizeof(decrypted)),
                "decrypt after re-init");
    expect_true(strcmp(decrypted, plain) == 0, "round trip after re-init");
    encryption_get_statistics(&encryptions, &decryptions, &rotations);
    expect_true(encryptions == 1 && decryptions == 1 && rotations == 0, "stats reset on init");
    encryption_shutdown();

    printf("ok\n");
    return 0;
}
