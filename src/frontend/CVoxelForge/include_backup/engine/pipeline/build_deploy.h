/**
 * =================================================================================================
 *                              BUILD & DEPLOY PIPELINE
 *                                Agent: AGENT_PIPELINE_1
 * =================================================================================================
 *
 * Continuous integration, asset cooking, packaging, and deployment systems.
 *
 * =================================================================================================
 */

#ifndef PIPELINE_BUILD_DEPLOY_H
#define PIPELINE_BUILD_DEPLOY_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    BUILD CONFIGURATION
 * =================================================================================================
 */

typedef enum BuildPlatform {
  BUILD_PLATFORM_WINDOWS,
  BUILD_PLATFORM_MACOS,
  BUILD_PLATFORM_LINUX,
  BUILD_PLATFORM_IOS,
  BUILD_PLATFORM_ANDROID,
  BUILD_PLATFORM_XBOX,
  BUILD_PLATFORM_PLAYSTATION,
  BUILD_PLATFORM_SWITCH,
  BUILD_PLATFORM_WEB,
} BuildPlatform;

typedef enum BuildConfiguration {
  BUILD_CONFIG_DEBUG,
  BUILD_CONFIG_DEVELOPMENT,
  BUILD_CONFIG_SHIPPING,
  BUILD_CONFIG_TEST,
} BuildConfiguration;

typedef struct BuildSettings {
  BuildPlatform platform;
  BuildConfiguration config;
  char output_dir[256];
  bool enable_logging;
  bool enable_profiling;
  bool enable_assertions;
  bool strip_debug_info;
  bool use_lto;
  int optimization_level;
  char defines[32][64];
  uint32_t define_count;
} BuildSettings;

// TODO(AGENT_PIPELINE_1): Implement build configuration loading [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement build configuration validation [Difficulty:
// 4]
// TODO(AGENT_PIPELINE_1): Implement platform-specific defaults [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement configuration inheritance [Difficulty: 5]

/* =================================================================================================
 *                                    ASSET COOKING
 * =================================================================================================
 */

typedef struct AssetCookSettings {
  bool compress_textures;
  int texture_quality; // 0-100
  bool generate_mipmaps;
  bool compress_meshes;
  bool optimize_meshes;
  bool compress_audio;
  int audio_quality;
  bool strip_unused_assets;
  char pak_output_dir[256];
  bool split_by_level;
} AssetCookSettings;

typedef struct CookProgress {
  uint32_t total_assets;
  uint32_t processed_assets;
  uint32_t failed_assets;
  char current_asset[256];
  float progress_percent;
  bool is_complete;
} CookProgress;

// TODO(AGENT_PIPELINE_1): Implement asset dependency scanner [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement texture compression (BC7, ASTC, ETC2)
// [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement mesh optimization [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement audio compression [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement incremental cooking [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement parallel cooking [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement pak file creation [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement pak file encryption [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement cook progress reporting [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement cook error handling [Difficulty: 4]

/* =================================================================================================
 *                                    PACKAGING
 * =================================================================================================
 */

typedef struct PackageSettings {
  char game_name[64];
  char version[32];
  char icon_path[256];
  char splash_screen_path[256];
  bool create_installer;
  bool create_portable;
  bool include_debug_symbols;
  bool sign_code;
  char signing_cert_path[256];
  char output_format[32]; // "zip", "exe", "dmg", "apk", etc.
} PackageSettings;

// TODO(AGENT_PIPELINE_1): Implement Windows packaging (NSIS/WiX) [Difficulty:
// 6]
// TODO(AGENT_PIPELINE_1): Implement macOS packaging (DMG/PKG) [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement Linux packaging (AppImage/DEB) [Difficulty:
// 6]
// TODO(AGENT_PIPELINE_1): Implement iOS packaging (IPA) [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement Android packaging (APK/AAB) [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement code signing [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement icon and splash embedding [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement manifest generation [Difficulty: 5]

/* =================================================================================================
 *                                    VERSION MANAGEMENT
 * =================================================================================================
 */

typedef struct VersionInfo {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
  uint32_t build;
  char commit_hash[41];
  char branch_name[64];
  uint64_t build_timestamp;
  char build_machine[64];
} VersionInfo;

// TODO(AGENT_PIPELINE_1): Implement version auto-increment [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement git integration [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement version embedding in binary [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement version file generation [Difficulty: 3]
// TODO(AGENT_PIPELINE_1): Implement changelog generation [Difficulty: 5]

/* =================================================================================================
 *                                    STORE INTEGRATION
 * =================================================================================================
 */

// TODO(AGENT_PIPELINE_1): Implement Steam upload API [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement Epic Games store upload [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement GOG Galaxy upload [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement itch.io butler integration [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement App Store upload [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement Google Play upload [Difficulty: 7]
// TODO(AGENT_PIPELINE_1): Implement store metadata generation [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement screenshot automation [Difficulty: 5]

/* =================================================================================================
 *                                    CI/CD INTEGRATION
 * =================================================================================================
 */

// TODO(AGENT_PIPELINE_1): Implement Jenkins integration [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement GitHub Actions workflow generation
// [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement GitLab CI config generation [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement build status reporting [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement artifact archiving [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement test result reporting [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement deployment triggers [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement rollback automation [Difficulty: 6]

/* =================================================================================================
 *                                    PIPELINE API
 * =================================================================================================
 */

// TODO(AGENT_PIPELINE_1): Implement pipeline_build [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement pipeline_cook_assets [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement pipeline_package [Difficulty: 5]
// TODO(AGENT_PIPELINE_1): Implement pipeline_deploy [Difficulty: 6]
// TODO(AGENT_PIPELINE_1): Implement pipeline_get_progress [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement pipeline_cancel [Difficulty: 4]
// TODO(AGENT_PIPELINE_1): Implement pipeline_validate [Difficulty: 5]

#endif // PIPELINE_BUILD_DEPLOY_H
