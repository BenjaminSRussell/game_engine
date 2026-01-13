#ifndef UNREAL_BRIDGE_H
#define UNREAL_BRIDGE_H

#include <stdbool.h>

// Unreal Bridge API Functions
void Unreal_ImportAssets(void);
void Unreal_ImportMeshes(void);
void Unreal_ImportMaterials(void);
void Unreal_ImportAnimations(void);
bool Unreal_ConvertCoordinates(float* position, float* normal, float* tangent);
void Unreal_BatchImport(const char* folder_path);

#endif // UNREAL_BRIDGE_H
