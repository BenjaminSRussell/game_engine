#ifndef MODEL_IMPORTER_H
#define MODEL_IMPORTER_H

#include <include/character/animation/animation_system.h>
#include <core/types.h>

/**
 * Imports a GLB file and extracts its skeletal and animation data.
 * @param filepath Path to the GLB file.
 */
void Model_ImportGLB(const char *filepath);

/**
 * Parses skeleton data from a glTF node.
 * @param json_node Pointer to cgltf_data or relevant node.
 */
void Model_ParseSkeleton(void *data);

/**
 * Parses animation clips from a glTF node.
 * @param json_node Pointer to cgltf_data or relevant node.
 */
void Model_ParseAnimations(void *data);

#endif // MODEL_IMPORTER_H
