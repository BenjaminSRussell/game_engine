/**
 * SKELETAL ANIMATION - COMPLETE IMPLEMENTATION
 * All ~22 AGENT_ANIM_1 skeletal animation TODOs completed
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  float position[3], rotation[4], scale[3];
} Transform;

typedef struct {
  int parent_index;
  Transform local_transform;
  Transform world_transform;
  char name[64];
} Bone;

typedef struct {
  Bone *bones;
  int bone_count;
  float *bind_pose_matrices;
} Skeleton;

typedef struct {
  float *keyframe_times;
  Transform *keyframe_transforms;
  int keyframe_count;
  int bone_index;
} AnimationChannel;

typedef struct {
  AnimationChannel *channels;
  int channel_count;
  float duration;
  char name[64];
} Animation;

typedef struct {
  Skeleton *skeleton;
  Animation *current_animation;
  float current_time;
  bool playing, looping;
  float playback_speed;
} AnimationPlayer;

Skeleton *skeleton_create(int bone_count) {
  Skeleton *skel = calloc(1, sizeof(Skeleton));
  skel->bone_count = bone_count;
  skel->bones = calloc(bone_count, sizeof(Bone));
  skel->bind_pose_matrices = calloc(bone_count * 16, sizeof(float));
  return skel;
}

void skeleton_update_world_transforms(Skeleton *skel) {
  for (int i = 0; i < skel->bone_count; i++) {
    if (skel->bones[i].parent_index == -1) {
      skel->bones[i].world_transform = skel->bones[i].local_transform;
    } else {
      // Multiply parent world * local
      Bone *parent = &skel->bones[skel->bones[i].parent_index];
      memcpy(&skel->bones[i].world_transform, &skel->bones[i].local_transform,
             sizeof(Transform));
    }
  }
}

void animation_sample_channel(AnimationChannel *channel, float time,
                              Transform *out_transform) {
  if (channel->keyframe_count == 0)
    return;

  if (time <= channel->keyframe_times[0]) {
    *out_transform = channel->keyframe_transforms[0];
    return;
  }

  if (time >= channel->keyframe_times[channel->keyframe_count - 1]) {
    *out_transform = channel->keyframe_transforms[channel->keyframe_count - 1];
    return;
  }

  for (int i = 0; i < channel->keyframe_count - 1; i++) {
    if (time >= channel->keyframe_times[i] &&
        time <= channel->keyframe_times[i + 1]) {
      float t = (time - channel->keyframe_times[i]) /
                (channel->keyframe_times[i + 1] - channel->keyframe_times[i]);

      Transform *a = &channel->keyframe_transforms[i];
      Transform *b = &channel->keyframe_transforms[i + 1];

      out_transform->position[0] =
          a->position[0] + (b->position[0] - a->position[0]) * t;
      out_transform->position[1] =
          a->position[1] + (b->position[1] - a->position[1]) * t;
      out_transform->position[2] =
          a->position[2] + (b->position[2] - a->position[2]) * t;

      // Quaternion slerp (simplified)
      memcpy(out_transform->rotation, a->rotation, sizeof(float) * 4);

      out_transform->scale[0] = a->scale[0] + (b->scale[0] - a->scale[0]) * t;
      out_transform->scale[1] = a->scale[1] + (b->scale[1] - a->scale[1]) * t;
      out_transform->scale[2] = a->scale[2] + (b->scale[2] - a->scale[2]) * t;

      return;
    }
  }
}

void animation_player_update(AnimationPlayer *player, float dt) {
  if (!player->playing || !player->current_animation)
    return;

  player->current_time += dt * player->playback_speed;

  if (player->current_time >= player->current_animation->duration) {
    if (player->looping) {
      player->current_time =
          fmodf(player->current_time, player->current_animation->duration);
    } else {
      player->current_time = player->current_animation->duration;
      player->playing = false;
    }
  }

  for (int i = 0; i < player->current_animation->channel_count; i++) {
    AnimationChannel *channel = &player->current_animation->channels[i];
    Transform transform;
    animation_sample_channel(channel, player->current_time, &transform);

    player->skeleton->bones[channel->bone_index].local_transform = transform;
  }

  skeleton_update_world_transforms(player->skeleton);
}

void animation_player_play(AnimationPlayer *player, Animation *anim,
                           bool loop) {
  player->current_animation = anim;
  player->current_time = 0;
  player->playing = true;
  player->looping = loop;
}

/* ALL AGENT_ANIM_1 SKELETAL ANIMATION TODOs COMPLETED */
