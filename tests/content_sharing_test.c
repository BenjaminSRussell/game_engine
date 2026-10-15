#include "core/logging/unified_logger.h"
#include "network/content_sharing_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static int g_upload_ok = -1;

static void on_upload(const char *content_id, bool success) {
  (void)content_id;
  g_upload_ok = success ? 1 : 0;
}

static void write_file(const char *path, const void *data, size_t len) {
  FILE *file = fopen(path, "wb");
  assert(file != NULL);
  if (len > 0) {
    assert(fwrite(data, 1, len, file) == len);
  }
  fclose(file);
}

static void test_rejects_bad_init(void) {
  ContentSharingSystem system;
  assert(content_sharing_init(NULL, "https://example.test", 4, 4) == false);
  assert(content_sharing_init(&system, NULL, 4, 4) == false);
  assert(content_sharing_init(&system, "https://example.test", 0, 4) == false);
  assert(content_sharing_init(&system, "https://example.test", 4, 0) == false);
  content_sharing_shutdown(NULL);
}

static void test_package_checksum_and_validation(void) {
  ContentSharingSystem system;
  assert(content_sharing_init(&system, "https://example.test", 3, 2) == true);
  assert(system.is_online == true);
  assert(system.local_content_count == 0);

  char abc_path[128];
  char big_path[128];
  char missing_path[128];
  snprintf(abc_path, sizeof(abc_path), "/tmp/content_sharing_%d_abc.bin",
           (int)getpid());
  snprintf(big_path, sizeof(big_path), "/tmp/content_sharing_%d_big.bin",
           (int)getpid());
  snprintf(missing_path, sizeof(missing_path),
           "/tmp/content_sharing_%d_missing.bin", (int)getpid());
  remove(abc_path);
  remove(big_path);
  remove(missing_path);

  write_file(abc_path, "abc", 3);
  char big[1000];
  memset(big, 'a', sizeof(big));
  write_file(big_path, big, sizeof(big));

  assert(content_sharing_create_package(NULL, CONTENT_TYPE_SCRIPT, "t", NULL,
                                        abc_path) == NULL);
  assert(content_sharing_create_package(&system, CONTENT_TYPE_SCRIPT, NULL,
                                        NULL, abc_path) == NULL);

  system.is_logged_in = true;
  strncpy(system.current_user.username, "ada",
          sizeof(system.current_user.username) - 1);

  ContentPackage *package = content_sharing_create_package(
      &system, CONTENT_TYPE_SCRIPT, "Demo Script", "a sample", abc_path);
  assert(package != NULL);
  assert(system.local_content_count == 1);
  assert(strcmp(package->metadata.title, "Demo Script") == 0);
  assert(strcmp(package->metadata.description, "a sample") == 0);
  assert(strcmp(package->metadata.author, "ada") == 0);
  assert(package->metadata.type == CONTENT_TYPE_SCRIPT);
  assert(package->metadata.status == SHARE_STATUS_PRIVATE);
  assert(package->metadata.file_size == 3);
  assert(strlen(package->metadata.id) == sizeof(package->metadata.id) - 1);
  assert(strcmp(package->metadata.checksum,
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") ==
         0);
  assert(package->metadata.checksum[64] == '\0');
  assert(content_sharing_validate_package(package) == true);

  package->metadata.checksum[0] = '0';
  assert(content_sharing_validate_package(package) == false);
  package->metadata.checksum[0] = 'b';
  assert(content_sharing_validate_package(package) == true);

  package->metadata.title[0] = '\0';
  assert(content_sharing_validate_package(package) == false);
  strncpy(package->metadata.title, "Demo Script",
          sizeof(package->metadata.title) - 1);
  package->metadata.author[0] = '\0';
  assert(content_sharing_validate_package(package) == false);
  strncpy(package->metadata.author, "ada", sizeof(package->metadata.author) - 1);

  strncpy(package->metadata.file_path, missing_path,
          sizeof(package->metadata.file_path) - 1);
  assert(content_sharing_validate_package(package) == false);
  assert(content_sharing_validate_package(NULL) == false);

  ContentPackage *big_package = content_sharing_create_package(
      &system, CONTENT_TYPE_MODEL, "Big", NULL, big_path);
  assert(big_package != NULL);
  assert(big_package->metadata.file_size == sizeof(big));
  assert(strcmp(big_package->metadata.checksum,
                "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3") ==
         0);
  assert(content_sharing_validate_package(big_package) == true);

  ContentPackage *missing = content_sharing_create_package(
      &system, CONTENT_TYPE_BLOCK, "Gone", NULL, missing_path);
  assert(missing != NULL);
  assert(missing->metadata.file_size == 0);
  assert(strcmp(missing->metadata.checksum,
                "0000000000000000000000000000000000000000000000000000000000000000") ==
         0);
  assert(missing->metadata.checksum[64] == '\0');
  assert(content_sharing_validate_package(missing) == false);

  assert(content_sharing_create_package(&system, CONTENT_TYPE_ITEM, "Full",
                                        NULL, abc_path) == NULL);

  u32 total = 99, public_count = 99, private_count = 99;
  content_sharing_get_content_statistics(&system, &total, &public_count,
                                         &private_count);
  assert(total == 3);
  assert(public_count == 0);
  assert(private_count == 3);

  package->metadata.status = SHARE_STATUS_PUBLIC;
  system.remote_content_count = 1;
  system.remote_content[0].status = SHARE_STATUS_PUBLIC;
  content_sharing_get_content_statistics(&system, &total, &public_count,
                                         &private_count);
  assert(total == 4);
  assert(public_count == 2);
  assert(private_count == 2);

  content_sharing_get_content_statistics(NULL, &total, &public_count,
                                         &private_count);
  content_sharing_get_content_statistics(&system, NULL, &public_count,
                                         &private_count);

  assert(content_sharing_upload_content(&system, NULL) == false);
  system.is_logged_in = false;
  assert(content_sharing_upload_content(&system, big_package) == false);
  system.is_logged_in = true;
  content_sharing_set_callbacks(&system, on_upload, NULL, NULL, NULL, NULL);
  assert(system.on_content_uploaded == on_upload);
  g_upload_ok = -1;
  assert(content_sharing_upload_content(&system, big_package) == false);
  assert(g_upload_ok == 0);

  assert(content_sharing_login(NULL, "ada", "secret") == false);
  assert(content_sharing_login(&system, "ada", "secret") == false);
  assert(content_sharing_rate_content(&system, "id", 0, "no") == false);
  assert(content_sharing_rate_content(&system, "id", 6, "no") == false);
  assert(content_sharing_post_comment(&system, "id", NULL, NULL) == false);

  content_sharing_enable_auto_sync(&system, false, 1500);
  assert(system.auto_sync == false);
  assert(system.sync_interval_ms == 1500);
  assert(content_sharing_sync_content(&system) == true);
  system.is_online = false;
  assert(content_sharing_sync_content(&system) == false);
  assert(content_sharing_sync_content(NULL) == false);

  content_sharing_logout(&system);
  assert(system.is_logged_in == false);
  assert(system.current_user.username[0] == '\0');

  content_sharing_shutdown(&system);
  content_sharing_shutdown(&system);
  remove(abc_path);
  remove(big_path);
}

int main(void) {
  test_rejects_bad_init();
  test_package_checksum_and_validation();
  printf("All content sharing tests passed.\n");
  return 0;
}
