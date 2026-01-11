#ifndef MODEL_IMPORTER_H
#define MODEL_IMPORTER_H

#include "character/animation/animation_system.h"
#include "core/asset_manager.h"
#include "core/types.h"

/**
 * Imports a GLB file and extracts its skeletal and animation data.
 * @param manager Asset Manager to register resources to.
 * @param filepath Path to the GLB file.
 */
void Model_ImportGLB(AssetManager *manager, const char *filepath);

/**
 * Parses skeleton data from a glTF node.
 * @param manager Asset Manager to register resources to.
 * @param json_node Pointer to cgltf_data or relevant node.
 */
void Model_ParseSkeleton(AssetManager *manager, void *data);

/**
 * Parses animation clips from a glTF node.
 * @param manager Asset Manager to register resources to.
 * @param json_node Pointer to cgltf_data or relevant node.
 */
void Model_ParseAnimations(AssetManager *manager, void *data);

#endif // MODEL_IMPORTER_H
