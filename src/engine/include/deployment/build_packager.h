// Deployment/Build Packaging
#ifndef BUILD_PACKAGER_H
#define BUILD_PACKAGER_H

#include <common.h>

typedef enum {
  BUILD_DEVELOPMENT,
  BUILD_SHIPPING,
  BUILD_TEST
} BuildConfiguration;

typedef struct {
  char output_dir[256];
  BuildConfiguration config;
  bool compress_assets;
  bool strip_symbols;
  bool package_dlc;
} BuildSettings;

#ifdef __cplusplus
extern "C" {
#endif

void build_package_game(BuildSettings *settings);
void build_cook_assets(const char *platform);

#ifdef __cplusplus
}
#endif

#endif
