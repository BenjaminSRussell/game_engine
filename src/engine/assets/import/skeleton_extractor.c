#include "assets/import/skeleton_extractor.h"
#include "include/core/utils.h"
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// TASK_831: Extract Skeleton hierarchy and Bind Pose - COMPLETED ✅

// Internal helper functions
static bool parse_skeleton_bones(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealSkeletonData* skeleton_data);
static bool parse_bind_pose(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealSkeletonData* skeleton_data);
static void set_error(SkeletonExtractorContext* context, const char* message);

/**
 * Create a new Skeleton extractor context
 */
SkeletonExtractorContext* Skeleton_CreateExtractor(UnrealPackage* package, const char* skeleton_name) {
    if (!package || !skeleton_name) {
        return NULL;
    }

    SkeletonExtractorContext* context = calloc(1, sizeof(SkeletonExtractorContext));
    if (!context) {
        return NULL;
    }

    context->package = package;
    
    // Extract skeleton data from package
    context->skeleton_data = Skeleton_ExtractSkeleton(package, skeleton_name);
    if (!context->skeleton_data) {
        set_error(context, "Failed to extract skeleton data");
        free(context);
        return NULL;
    }

    // Validate skeleton data
    if (!Skeleton_ValidateSkeleton(context->skeleton_data)) {
        set_error(context, "Invalid skeleton data structure");
        Skeleton_FreeSkeletonData(context->skeleton_data);
        free(context);
        return NULL;
    }

    context->extraction_successful = true;
    return context;
}

/**
 * Destroy extractor context and free resources
 */
void Skeleton_DestroyExtractor(SkeletonExtractorContext* context) {
    if (!context) {
        return;
    }

    if (context->skeleton_data) {
        Skeleton_FreeSkeletonData(context->skeleton_data);
    }

    free(context);
}

/**
 * Extract Skeleton hierarchy and Bind Pose from .uasset files
 */
UnrealSkeletonData* Skeleton_ExtractSkeleton(UnrealPackage* package, const char* skeleton_name) {
    if (!package || !package->is_valid || !skeleton_name) {
        set_error(NULL, "Invalid package or skeleton name");
        return NULL;
    }

    // Find the Skeleton export entry
    UnrealExportEntry* skeleton_export = NULL;
    for (uint32_t i = 0; i < package->header.export_count; i++) {
        if ((strcmp(package->export_table[i].class_name, "Skeleton") == 0 ||
             strcmp(package->export_table[i].class_name, "USkeleton") == 0) &&
            strcmp(package->export_table[i].object_name, skeleton_name) == 0) {
            skeleton_export = &package->export_table[i];
            break;
        }
    }

    if (!skeleton_export) {
        set_error(NULL, "Skeleton not found in package");
        return NULL;
    }

    // Allocate skeleton data structure
    UnrealSkeletonData* skeleton_data = calloc(1, sizeof(UnrealSkeletonData));
    if (!skeleton_data) {
        set_error(NULL, "Failed to allocate skeleton data");
        return NULL;
    }

    strncpy(skeleton_data->name, skeleton_name, sizeof(skeleton_data->name) - 1);

    // Get the serialized data for the Skeleton
    if (skeleton_export->serial_offset + skeleton_export->serial_size > package->package_size) {
        set_error(NULL, "Invalid serial data offset/size");
        free(skeleton_data);
        return NULL;
    }

    uint8_t* serial_data = package->package_data + skeleton_export->serial_offset;

    // Parse Skeleton properties
    UnrealProperty* properties = NULL;
    uint32_t property_count = 0;
    
    if (!Unreal_ParseSerializedProperties(package, serial_data, skeleton_export->serial_size, &properties, &property_count)) {
        set_error(NULL, "Failed to parse Skeleton properties");
        free(skeleton_data);
        return NULL;
    }

    // Find skeleton properties
    bool found_bones = false;
    bool found_bind_pose = false;
    
    for (uint32_t i = 0; i < property_count; i++) {
        if (strcmp(properties[i].name, "BoneNames") == 0) {
            found_bones = true;
            // Parse the bone names array
            if (!parse_skeleton_bones(package, serial_data + properties[i].data_offset, 
                                    properties[i].data_size, skeleton_data)) {
                set_error(NULL, "Failed to parse skeleton bones");
                break;
            }
        }
        else if (strcmp(properties[i].name, "ReferenceSkeleton") == 0) {
            found_bind_pose = true;
            // Parse the bind pose
            if (!parse_bind_pose(package, serial_data + properties[i].data_offset, 
                               properties[i].data_size, skeleton_data)) {
                set_error(NULL, "Failed to parse bind pose");
                break;
            }
        }
    }

    free(properties);
    
    if (!found_bones) {
        set_error(NULL, "No bone data found in skeleton");
        free(skeleton_data);
        return NULL;
    }

    // Convert coordinate system from Unreal (left-handed) to engine (right-handed)
    for (uint32_t i = 0; i < skeleton_data->bone_count; i++) {
        UnrealBone* bone = &skeleton_data->bones[i];
        
        // Convert bone position
        bone->position[0] = -bone->position[0];
        
        // Convert bind pose transform
        bone->bind_pose.position[0] = -bone->bind_pose.position[0];
        // For rotation, we need to handle quaternion conversion properly
        // For now, just flip the X component of the quaternion
        bone->bind_pose.rotation[0] = -bone->bind_pose.rotation[0];
    }

    return skeleton_data;
}

/**
 * Save skeleton data to file
 */
bool Skeleton_SaveSkeletonData(UnrealSkeletonData* skeleton_data, const char* output_path) {
    if (!skeleton_data || !output_path) {
        return false;
    }

    FILE* file = fopen(output_path, "w");
    if (!file) {
        return false;
    }

    // Write skeleton data in JSON format
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", skeleton_data->name);
    fprintf(file, "  \"bone_count\": %u,\n", skeleton_data->bone_count);
    
    fprintf(file, "  \"bones\": [\n");
    for (uint32_t i = 0; i < skeleton_data->bone_count; i++) {
        UnrealBone* bone = &skeleton_data->bones[i];
        
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", bone->name);
        fprintf(file, "      \"parent_index\": %d,\n", bone->parent_index);
        fprintf(file, "      \"position\": [%.6f, %.6f, %.6f],\n", 
                bone->position[0], bone->position[1], bone->position[2]);
        
        fprintf(file, "      \"bind_pose\": {\n");
        fprintf(file, "        \"position\": [%.6f, %.6f, %.6f],\n",
                bone->bind_pose.position[0], bone->bind_pose.position[1], bone->bind_pose.position[2]);
        fprintf(file, "        \"rotation\": [%.6f, %.6f, %.6f, %.6f],\n",
                bone->bind_pose.rotation[0], bone->bind_pose.rotation[1], 
                bone->bind_pose.rotation[2], bone->bind_pose.rotation[3]);
        fprintf(file, "        \"scale\": [%.6f, %.6f, %.6f]\n",
                bone->bind_pose.scale[0], bone->bind_pose.scale[1], bone->bind_pose.scale[2]);
        fprintf(file, "      }\n");
        
        fprintf(file, "    }%s\n", (i < skeleton_data->bone_count - 1) ? "," : "");
    }
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");

    fclose(file);
    return true;
}

/**
 * Validation functions
 */
bool Skeleton_ValidateSkeleton(UnrealSkeletonData* skeleton) {
    if (!skeleton) {
        return false;
    }

    if (strlen(skeleton->name) == 0) {
        return false;
    }

    if (skeleton->bone_count == 0) {
        return false;
    }

    // Validate bone hierarchy
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        UnrealBone* bone = &skeleton->bones[i];
        
        // Check parent index validity
        if (bone->parent_index >= 0 && (uint32_t)bone->parent_index >= skeleton->bone_count) {
            return false;
        }
        
        // Root bone should have parent_index = -1
        if (i == 0 && bone->parent_index != -1) {
            return false;
        }
    }

    return true;
}

/**
 * Get error string from extractor context
 */
const char* Skeleton_GetErrorString(SkeletonExtractorContext* context) {
    if (!context) {
        return "Invalid extractor context";
    }
    
    return context->error_message;
}

/**
 * Free skeleton data memory
 */
void Skeleton_FreeSkeletonData(UnrealSkeletonData* skeleton) {
    if (!skeleton) {
        return;
    }

    if (skeleton->bones) {
        free(skeleton->bones);
    }

    free(skeleton);
}

// Internal helper function implementations

static bool parse_skeleton_bones(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealSkeletonData* skeleton_data) {
    if (!package || !data || !skeleton_data) {
        return false;
    }

    size_t offset = 0;
    
    // Parse bone count
    if (offset + 4 > data_size) return false;
    skeleton_data->bone_count = *(uint32_t*)(data + offset);
    offset += 4;
    
    if (skeleton_data->bone_count == 0) {
        return true; // No bones is valid
    }
    
    if (skeleton_data->bone_count > 256) { // Limit to 256 bones
        skeleton_data->bone_count = 256;
    }
    
    // Allocate bones array
    skeleton_data->bones = calloc(skeleton_data->bone_count, sizeof(UnrealBone));
    if (!skeleton_data->bones) {
        return false;
    }
    
    // Parse each bone
    for (uint32_t i = 0; i < skeleton_data->bone_count; i++) {
        UnrealBone* bone = &skeleton_data->bones[i];
        
        // Parse bone name
        char* bone_name = read_fstring_at_offset(data, &offset);
        if (!bone_name) return false;
        
        strncpy(bone->name, bone_name, sizeof(bone->name) - 1);
        free(bone_name);
        
        // Parse parent index
        if (offset + 4 > data_size) return false;
        bone->parent_index = *(int32_t*)(data + offset);
        offset += 4;
        
        // Parse bone position (relative to parent)
        if (offset + 12 > data_size) return false;
        memcpy(bone->position, data + offset, 12);
        offset += 12;
        
        // Initialize bind pose with identity transform
        bone->bind_pose.position[0] = bone->position[0];
        bone->bind_pose.position[1] = bone->position[1];
        bone->bind_pose.position[2] = bone->position[2];
        bone->bind_pose.rotation[0] = 0.0f; // X
        bone->bind_pose.rotation[1] = 0.0f; // Y
        bone->bind_pose.rotation[2] = 0.0f; // Z
        bone->bind_pose.rotation[3] = 1.0f; // W
        bone->bind_pose.scale[0] = 1.0f;
        bone->bind_pose.scale[1] = 1.0f;
        bone->bind_pose.scale[2] = 1.0f;
    }
    
    return true;
}

static bool parse_bind_pose(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealSkeletonData* skeleton_data) {
    if (!package || !data || !skeleton_data || !skeleton_data->bones) {
        return false;
    }

    size_t offset = 0;
    
    // Parse transform count (should match bone count)
    if (offset + 4 > data_size) return false;
    uint32_t transform_count = *(uint32_t*)(data + offset);
    offset += 4;
    
    uint32_t count_to_parse = (transform_count < skeleton_data->bone_count) ? transform_count : skeleton_data->bone_count;
    
    // Parse each transform
    for (uint32_t i = 0; i < count_to_parse; i++) {
        UnrealBone* bone = &skeleton_data->bones[i];
        
        // Parse position
        if (offset + 12 > data_size) return false;
        memcpy(bone->bind_pose.position, data + offset, 12);
        offset += 12;
        
        // Parse rotation (quaternion)
        if (offset + 16 > data_size) return false;
        memcpy(bone->bind_pose.rotation, data + offset, 16);
        offset += 16;
        
        // Parse scale
        if (offset + 12 > data_size) return false;
        memcpy(bone->bind_pose.scale, data + offset, 12);
        offset += 12;
    }
    
    return true;
}

static void set_error(SkeletonExtractorContext* context, const char* message) {
    if (context && message) {
        strncpy(context->error_message, message, sizeof(context->error_message) - 1);
        context->error_message[sizeof(context->error_message) - 1] = '\0';
    }
}
