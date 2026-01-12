/**
 * =================================================================================================
 *                           ASSET TEMPLATES - CHARACTER SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Pre-built character templates with full rig, animations, and customization.
 * Ready for drag-and-drop use in any project.
 *
 * =================================================================================================
 */

#ifndef ASSET_TEMPLATES_CHARACTER_H
#define ASSET_TEMPLATES_CHARACTER_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    CHARACTER TEMPLATE TYPES
 * =================================================================================================
 */

typedef enum CharacterTemplateType {
  CHARACTER_HUMANOID_MALE,
  CHARACTER_HUMANOID_FEMALE,
  CHARACTER_HUMANOID_CHILD,
  CHARACTER_QUADRUPED_DOG,
  CHARACTER_QUADRUPED_CAT,
  CHARACTER_QUADRUPED_HORSE,
  CHARACTER_QUADRUPED_WOLF,
  CHARACTER_BIPEDAL_BIRD,
  CHARACTER_FLYING_DRAGON,
  CHARACTER_FLYING_BIRD,
  CHARACTER_AQUATIC_FISH,
  CHARACTER_AQUATIC_SHARK,
  CHARACTER_INSECT_SPIDER,
  CHARACTER_CUSTOM,
} CharacterTemplateType;

/* =================================================================================================
 *                                    SKELETON STRUCTURE
 * =================================================================================================
 */

typedef struct BoneDefinition {
  char name[32];
  int32_t parent_index;
  float local_position[3];
  float local_rotation[4];
  float local_scale[3];
  bool is_essential; // Required for retargeting
} BoneDefinition;

typedef struct SkeletonTemplate {
  char name[64];
  BoneDefinition *bones;
  uint32_t bone_count;
  // IK chains
  struct {
    int32_t start, end;
  } ik_chains[8];
  uint32_t ik_chain_count;
  // Twist bones
  int32_t twist_bone_indices[16];
  uint32_t twist_bone_count;
} SkeletonTemplate;

// TODO(AGENT_TEMPLATE_1): Implement skeleton template creation [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement skeleton validation [Difficulty: 4]
// TODO(AGENT_TEMPLATE_1): Implement skeleton retargeting system [Difficulty: 8]
// TODO(AGENT_TEMPLATE_1): Implement skeleton mirroring [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement skeleton LOD (bone reduction) [Difficulty:
// 6]
// TODO(AGENT_TEMPLATE_1): Implement skeleton preview visualization [Difficulty:
// 5]

/* =================================================================================================
 *                                    CHARACTER CUSTOMIZATION
 * =================================================================================================
 */

typedef enum CustomizationCategory {
  CUSTOMIZE_BODY_TYPE,
  CUSTOMIZE_FACE_SHAPE,
  CUSTOMIZE_HAIR_STYLE,
  CUSTOMIZE_HAIR_COLOR,
  CUSTOMIZE_SKIN_TONE,
  CUSTOMIZE_EYE_COLOR,
  CUSTOMIZE_FACIAL_HAIR,
  CUSTOMIZE_ACCESSORIES,
  CUSTOMIZE_TATTOOS,
  CUSTOMIZE_SCARS,
} CustomizationCategory;

typedef struct CustomizationOption {
  char name[32];
  char icon_path[128];
  uint32_t mesh_id;
  uint32_t material_id;
  float morph_weights[16];
  bool is_default;
} CustomizationOption;

typedef struct CharacterCustomization {
  CustomizationOption *options_per_category[10];
  uint32_t option_counts[10];
  int32_t selected_indices[10];
  float body_sliders[32]; // Height, weight, muscle, etc.
} CharacterCustomization;

// TODO(AGENT_TEMPLATE_1): Implement customization option loading [Difficulty:
// 5]
// TODO(AGENT_TEMPLATE_1): Implement body morph blending [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement mesh attachment system [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement color customization [Difficulty: 4]
// TODO(AGENT_TEMPLATE_1): Implement randomize character [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement save/load character preset [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement character preview turntable [Difficulty: 5]

/* =================================================================================================
 *                                    ANIMATION SET
 * =================================================================================================
 */

typedef struct AnimationEntry {
  char name[64];
  char path[256];
  float duration;
  bool is_looping;
  bool is_additive;
  float blend_in_time;
  float blend_out_time;
} AnimationEntry;

typedef struct AnimationSet {
  char name[64];
  CharacterTemplateType compatible_type;
  AnimationEntry *animations;
  uint32_t animation_count;
  // Categories
  struct {
    char name[32];
    uint32_t start, count;
  } categories[16];
  uint32_t category_count;
} AnimationSet;

// TODO(AGENT_TEMPLATE_1): Implement animation set loading [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement animation retargeting [Difficulty: 8]
// TODO(AGENT_TEMPLATE_1): Implement animation blending presets [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement animation events (footsteps, etc.)
// [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement animation root motion extraction
// [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement animation mirroring [Difficulty: 6]

/* =================================================================================================
 *                                    HUMANOID TEMPLATE
 * =================================================================================================
 */

typedef struct HumanoidTemplate {
  CharacterTemplateType type;
  char name[64];

  // Mesh data
  uint32_t base_mesh_id;
  uint32_t lod_mesh_ids[4];

  // Skeleton
  SkeletonTemplate skeleton;

  // Customization
  CharacterCustomization customization;

  // Animations
  AnimationSet locomotion_set; // Walk, run, jump, etc.
  AnimationSet combat_set;     // Attack, block, dodge
  AnimationSet social_set;     // Wave, sit, dance

  // Physics
  float capsule_radius;
  float capsule_height;
  struct {
    int32_t bone;
    float radius;
  } ragdoll_shapes[24];
  uint32_t ragdoll_shape_count;

  // Clothing/Armor slots
  int32_t attachment_bones[16];
  uint32_t attachment_bone_count;
} HumanoidTemplate;

// TODO(AGENT_TEMPLATE_1): Implement humanoid template creation [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement humanoid template instantiation
// [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement humanoid IK setup (feet, hands)
// [Difficulty: 7]
// TODO(AGENT_TEMPLATE_1): Implement humanoid ragdoll setup [Difficulty: 7]
// TODO(AGENT_TEMPLATE_1): Implement humanoid clothing system [Difficulty: 7]
// TODO(AGENT_TEMPLATE_1): Implement humanoid facial rig [Difficulty: 8]
// TODO(AGENT_TEMPLATE_1): Implement humanoid eye tracking [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement humanoid hand poses [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement humanoid procedural breathing [Difficulty:
// 5]
// TODO(AGENT_TEMPLATE_1): Implement humanoid look-at system [Difficulty: 6]

/* =================================================================================================
 *                                    QUADRUPED TEMPLATE
 * =================================================================================================
 */

typedef struct QuadrupedTemplate {
  CharacterTemplateType type;
  char name[64];
  uint32_t base_mesh_id;
  SkeletonTemplate skeleton;
  AnimationSet locomotion_set;
  AnimationSet behavior_set; // Sit, eat, sleep, etc.
  float body_length;
  float body_height;
  float leg_length;
  float tail_segments;
} QuadrupedTemplate;

// TODO(AGENT_TEMPLATE_1): Implement quadruped 4-leg IK [Difficulty: 8]
// TODO(AGENT_TEMPLATE_1): Implement quadruped tail physics [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement quadruped gait patterns [Difficulty: 7]
// TODO(AGENT_TEMPLATE_1): Implement quadruped terrain adaptation [Difficulty:
// 7]
// TODO(AGENT_TEMPLATE_1): Implement quadruped ear/face animations [Difficulty:
// 5]

/* =================================================================================================
 *                                    FLYING CREATURE TEMPLATE
 * =================================================================================================
 */

typedef struct FlyingCreatureTemplate {
  CharacterTemplateType type;
  char name[64];
  uint32_t base_mesh_id;
  SkeletonTemplate skeleton;
  AnimationSet flight_set;
  AnimationSet ground_set;
  float wingspan;
  float wing_frequency;
  float glide_ratio;
  float max_bank_angle;
} FlyingCreatureTemplate;

// TODO(AGENT_TEMPLATE_1): Implement wing procedural animation [Difficulty: 7]
// TODO(AGENT_TEMPLATE_1): Implement flight physics integration [Difficulty: 8]
// TODO(AGENT_TEMPLATE_1): Implement landing/takeoff transitions [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement wind response [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement feather/membrane rendering [Difficulty: 7]

/* =================================================================================================
 *                                    CHARACTER TEMPLATE API
 * =================================================================================================
 */

// TODO(AGENT_TEMPLATE_1): Implement character_template_load [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement character_template_instantiate [Difficulty:
// 5]
// TODO(AGENT_TEMPLATE_1): Implement character_template_customize [Difficulty:
// 6]
// TODO(AGENT_TEMPLATE_1): Implement character_template_apply_animation
// [Difficulty: 5]
// TODO(AGENT_TEMPLATE_1): Implement character_template_setup_physics
// [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement character_template_attach_equipment
// [Difficulty: 6]
// TODO(AGENT_TEMPLATE_1): Implement character_template_serialize [Difficulty:
// 5]
// TODO(AGENT_TEMPLATE_1): Implement character_template_import_fbx [Difficulty:
// 7]
// TODO(AGENT_TEMPLATE_1): Implement character_template_import_gltf [Difficulty:
// 7]
// TODO(AGENT_TEMPLATE_1): Implement character_template_validate [Difficulty: 5]

#endif // ASSET_TEMPLATES_CHARACTER_H
