// Editor/Foliage Editor Tool
#ifndef FOLIAGE_EDITOR_H
#define FOLIAGE_EDITOR_H

#include "include/common.h"

typedef struct {
  char mesh_path[128];
  f32 density;
  bool selected;
} FoliageTypeInfo;

typedef struct {
  FoliageTypeInfo types[32];
  u32 type_count;

  f32 brush_size;
  f32 paint_density;
  f32 erase_density;

  bool single_instance_mode;

} FoliageEditor;

#ifdef __cplusplus
extern "C" {
#endif

FoliageEditor *foliage_editor_create(void);
void foliage_editor_paint(FoliageEditor *editor, Vec3 pos);

#ifdef __cplusplus
}
#endif

#endif
