#include <editor/importer/model_importer.h>
#include <core/logger.h>
#include <core/memory.h>
#include "include/vendor/cgltf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                                   MODEL IMPORTER (Runtime)
 * =================================================================================================
 *
 * GOAL: "Including skeletons in the meshs and assets that are characters".
 * STRATEGY:
 *   Parse GLB (glTF Binary) files using cgltf.
 *   Extract Skin/Joint data for the Animation System.
 */

void Model_ImportGLB(const char *filepath) {
  LOG_INFO("Model Importer: Importing GLB file: %s", filepath);

  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, filepath, &data);

  if (result != cgltf_result_success) {
    LOG_ERROR("Model Importer: Failed to parse GLB: %d", (int)result);
    return;
  }

  result = cgltf_load_buffers(&options, data, filepath);
  if (result != cgltf_result_success) {
    LOG_ERROR("Model Importer: Failed to load buffers: %d", (int)result);
    cgltf_free(data);
    return;
  }

  // Parse skeleton and animations
  Model_ParseSkeleton(data);
  Model_ParseAnimations(data);

  LOG_INFO("Model Importer: Successfully imported GLB: %s", filepath);
  cgltf_free(data);
}

void Model_ParseSkeleton(void *ptr) {
  cgltf_data *data = (cgltf_data *)ptr;
  if (!data || data->skins_count == 0) {
    LOG_INFO("Model Importer: No skins/skeletons found in GLB");
    return;
  }

  for (size_t s = 0; s < data->skins_count; s++) {
    cgltf_skin *skin = &data->skins[s];
    LOG_INFO("Model Importer: Found skin: %s", skin->name ? skin->name : "unnamed");

    u32 bone_count = (u32)skin->joints_count;
    if (bone_count > MAX_BONES) {
      LOG_WARN("Model Importer: Skin has too many bones (%u), capping at %d", bone_count, MAX_BONES);
      bone_count = MAX_BONES;
    }

    Skeleton *skeleton = animation_create_skeleton(bone_count);
    if (!skeleton) continue;

    for (u32 i = 0; i < bone_count; i++) {
      cgltf_node *joint = skin->joints[i];
      Bone *bone = &skeleton->bones[i];

      strncpy(bone->name, joint->name ? joint->name : "joint", sizeof(bone->name) - 1);
      
      // Find parent index
      bone->parent_index = -1;
      if (joint->parent) {
        for (u32 j = 0; j < bone_count; j++) {
          if (skin->joints[j] == joint->parent) {
            bone->parent_index = (i32)j;
            break;
          }
        }
      }

      // Read inverse bind matrix if available
      if (skin->inverse_bind_matrices) {
        cgltf_accessor_read_float(skin->inverse_bind_matrices, i, (f32*)&bone->inverse_bind_pose, 16);
      } else {
        bone->inverse_bind_pose = mat4_identity();
      }
      
      // Local bind pose (initial transform of the joint node)
      cgltf_node_transform_local(joint, (f32*)bone->local_bind_pose.m);
    }

    LOG_INFO("Model Importer: Extracted skeleton with %u bones", skeleton->bone_count);
    // In a real implementation, we would register this skeleton as an asset.
    // For now, we'll just log success and clean up or store in some global registry.
    animation_destroy_skeleton(skeleton); 
  }
}

void Model_ParseAnimations(void *ptr) {
  cgltf_data *data = (cgltf_data *)ptr;
  if (!data || data->animations_count == 0) {
    LOG_INFO("Model Importer: No animations found in GLB");
    return;
  }

  for (size_t a = 0; a < data->animations_count; a++) {
    cgltf_animation *anim = &data->animations[a];
    LOG_INFO("Model Importer: Found animation: %s", anim->name ? anim->name : "unnamed");

    // Sample extraction logic: in a real implementation, we would convert 
    // cgltf channels to AnimationChannel and AnimationClip.
    // For now, we log the presence and count.
    LOG_INFO("Model Importer: Animation '%s' has %zu channels", 
             anim->name ? anim->name : "unnamed", anim->channels_count);
  }
}
