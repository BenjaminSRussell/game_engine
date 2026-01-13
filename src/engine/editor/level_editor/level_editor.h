#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include <stdbool.h>

typedef struct LevelEditor_Context LevelEditor_Context;

void LevelEditor_Init(void);
void LevelEditor_Update(float delta_time);
void LevelEditor_Shutdown(void);

void LevelEditor_LoadLevel(const char* level_name);
void LevelEditor_SaveLevel(const char* level_name);

bool LevelEditor_IsActive(void);
void LevelEditor_ToggleActive(void);

#endif // LEVEL_EDITOR_H
