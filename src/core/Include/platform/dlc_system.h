// Platform/DLC System
#ifndef DLC_SYSTEM_H
#define DLC_SYSTEM_H

#include <common.h>

typedef struct {
  char id[32];
  char name[64];
  char mount_point[64];
  bool is_mounted;
  bool is_downloaded;
} DLCContent;

#ifdef __cplusplus
extern "C" {
#endif

void dlc_system_init(void);
void dlc_mount(const char *dlc_id);
bool dlc_is_available(const char *dlc_id);

#ifdef __cplusplus
}
#endif

#endif
