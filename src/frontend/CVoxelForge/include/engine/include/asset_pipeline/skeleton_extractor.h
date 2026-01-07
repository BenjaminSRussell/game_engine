#ifndef SKELETON_EXTRACTOR_H
#define SKELETON_EXTRACTOR_H

#include "unreal_asset_importer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Unreal Bone Transform
 */
typedef struct {
    float position[3];
    float rotation[4];  // Quaternion (X, Y, Z, W)
    float scale[3];
} UnrealBoneTransform;

/**
 * Unreal Bone
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    int32_t parent_index;  // -1 for root bone
    float position[3];     // Relative to parent
    UnrealBoneTransform bind_pose;
} UnrealBone;

/**
 * Unreal Skeleton Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    uint32_t bone_count;
    UnrealBone* bones;
} UnrealSkeletonData;

/**
 * Skeleton Extractor Context
 * Main structure for managing skeleton extraction
 */
typedef struct {
    UnrealPackage* package;
    UnrealSkeletonData* skeleton_data;
    char error_message[512];
    bool extraction_successful;
} SkeletonExtractorContext;

// Core API Functions
SkeletonExtractorContext* Skeleton_CreateExtractor(UnrealPackage* package, const char* skeleton_name);
void Skeleton_DestroyExtractor(SkeletonExtractorContext* context);
UnrealSkeletonData* Skeleton_ExtractSkeleton(UnrealPackage* package, const char* skeleton_name);

// Utility Functions
bool Skeleton_SaveSkeletonData(UnrealSkeletonData* skeleton_data, const char* output_path);
bool Skeleton_ValidateSkeleton(UnrealSkeletonData* skeleton);
const char* Skeleton_GetErrorString(SkeletonExtractorContext* context);

// Memory Management
void Skeleton_FreeSkeletonData(UnrealSkeletonData* skeleton);

#endif // SKELETON_EXTRACTOR_H
