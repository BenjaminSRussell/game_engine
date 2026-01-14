/**
 * ADDITIVE ANIMATION BLENDING
 * Layered animation system for procedural modifications
 * Used for aiming, recoil, breathing, foot placement, etc.
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BONES 256
#define MAX_ADDITIVE_LAYERS 16

typedef struct {
    float x, y, z, w;
} Quaternion;

typedef struct {
    float x, y, z;
} Vector3;

typedef struct {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
} Transform;

typedef struct {
    Transform transforms[MAX_BONES];
    uint32_t bone_count;
} Pose;

typedef struct {
    Pose pose;
    float weight;
    uint32_t bone_mask[MAX_BONES / 32]; // Bitmask for which bones to affect
    bool has_bone_mask;
} AdditiveLayer;

typedef struct {
    AdditiveLayer layers[MAX_ADDITIVE_LAYERS];
    uint32_t layer_count;
    Pose base_pose;
    Pose output_pose;
} AdditiveBlender;

// Quaternion operations
static inline Quaternion quat_identity() {
    Quaternion q = {0, 0, 0, 1};
    return q;
}

static inline Quaternion quat_conjugate(Quaternion q) {
    return (Quaternion){-q.x, -q.y, -q.z, q.w};
}

static inline Quaternion quat_multiply(Quaternion a, Quaternion b) {
    Quaternion result;
    result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return result;
}

static inline Quaternion quat_normalize(Quaternion q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len > 0.0001f) {
        return (Quaternion){q.x / len, q.y / len, q.z / len, q.w / len};
    }
    return quat_identity();
}

// Vector operations
static inline Vector3 vec3(float x, float y, float z) {
    return (Vector3){x, y, z};
}

static inline Vector3 vec3_add(Vector3 a, Vector3 b) {
    return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static inline Vector3 vec3_subtract(Vector3 a, Vector3 b) {
    return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static inline Vector3 vec3_scale(Vector3 v, float s) {
    return (Vector3){v.x * s, v.y * s, v.z * s};
}

// Convert pose to additive (relative to bind pose)
void pose_make_additive(const Pose *bind_pose, const Pose *source_pose, Pose *additive_out) {
    if (!bind_pose || !source_pose || !additive_out) return;
    
    uint32_t bone_count = bind_pose->bone_count;
    additive_out->bone_count = bone_count;
    
    for (uint32_t i = 0; i < bone_count; i++) {
        // Position: subtract bind pose position
        additive_out->transforms[i].position = vec3_subtract(
            source_pose->transforms[i].position,
            bind_pose->transforms[i].position
        );
        
        // Rotation: multiply by inverse of bind pose rotation
        Quaternion bind_inverse = quat_conjugate(bind_pose->transforms[i].rotation);
        additive_out->transforms[i].rotation = quat_multiply(
            bind_inverse,
            source_pose->transforms[i].rotation
        );
        additive_out->transforms[i].rotation = quat_normalize(additive_out->transforms[i].rotation);
        
        // Scale: divide by bind pose scale (or subtract for log space)
        additive_out->transforms[i].scale = vec3_scale(
            bind_pose->transforms[i].scale,
            1.0f / (fabsf(bind_pose->transforms[i].scale.x) > 0.0001f ? bind_pose->transforms[i].scale.x : 1.0f)
        );
    }
}

// Apply additive pose to base pose
void pose_apply_additive(const Pose *base_pose, const Pose *additive_pose, 
                        float weight, Pose *result_out) {
    if (!base_pose || !additive_pose || !result_out) return;
    
    uint32_t bone_count = base_pose->bone_count;
    result_out->bone_count = bone_count;
    
    for (uint32_t i = 0; i < bone_count; i++) {
        // Position: add weighted additive offset
        Vector3 additive_pos = vec3_scale(additive_pose->transforms[i].position, weight);
        result_out->transforms[i].position = vec3_add(base_pose->transforms[i].position, additive_pos);
        
        // Rotation: multiply by weighted additive rotation
        // For small rotations, we can slerp with identity
        Quaternion additive_rot = additive_pose->transforms[i].rotation;
        if (weight < 1.0f) {
            // Slerp between identity and additive rotation
            Quaternion identity = quat_identity();
            // Simplified slerp for small angles
            additive_rot.x *= weight;
            additive_rot.y *= weight;
            additive_rot.z *= weight;
            additive_rot.w = 1.0f - (1.0f - additive_rot.w) * weight;
            additive_rot = quat_normalize(additive_rot);
        }
        result_out->transforms[i].rotation = quat_multiply(
            base_pose->transforms[i].rotation,
            additive_rot
        );
        result_out->transforms[i].rotation = quat_normalize(result_out->transforms[i].rotation);
        
        // Scale: multiply by weighted additive scale
        Vector3 scale_factor = vec3_scale(
            vec3_subtract(additive_pose->transforms[i].scale, (Vector3){1, 1, 1}),
            weight
        );
        scale_factor = vec3_add(scale_factor, (Vector3){1, 1, 1});
        result_out->transforms[i].scale = vec3_scale(base_pose->transforms[i].scale, scale_factor.x);
    }
}

// Create additive blender
AdditiveBlender *additive_blender_create() {
    AdditiveBlender *blender = calloc(1, sizeof(AdditiveBlender));
    if (!blender) return NULL;
    
    blender->layer_count = 0;
    blender->base_pose.bone_count = 0;
    blender->output_pose.bone_count = 0;
    
    return blender;
}

void additive_blender_destroy(AdditiveBlender *blender) {
    if (blender) {
        free(blender);
    }
}

// Set base pose
void additive_blender_set_base_pose(AdditiveBlender *blender, const Pose *base_pose) {
    if (!blender || !base_pose) return;
    
    blender->base_pose = *base_pose;
    blender->output_pose = *base_pose; // Initialize output with base
}

// Add additive layer
uint32_t additive_blender_add_layer(AdditiveBlender *blender, const Pose *additive_pose, 
                                   float weight) {
    if (!blender || !additive_pose || blender->layer_count >= MAX_ADDITIVE_LAYERS) {
        return UINT32_MAX;
    }
    
    AdditiveLayer *layer = &blender->layers[blender->layer_count];
    layer->pose = *additive_pose;
    layer->weight = weight;
    layer->has_bone_mask = false;
    
    // Initialize bone mask to affect all bones
    for (uint32_t i = 0; i < MAX_BONES / 32; i++) {
        layer->bone_mask[i] = 0xFFFFFFFF;
    }
    
    return blender->layer_count++;
}

// Set bone mask for layer (affect only specific bones)
void additive_layer_set_bone_mask(AdditiveBlender *blender, uint32_t layer_index,
                                 const uint32_t *bone_indices, uint32_t bone_count) {
    if (!blender || layer_index >= blender->layer_count) return;
    
    AdditiveLayer *layer = &blender->layers[layer_index];
    layer->has_bone_mask = true;
    
    // Clear mask
    for (uint32_t i = 0; i < MAX_BONES / 32; i++) {
        layer->bone_mask[i] = 0;
    }
    
    // Set bits for specified bones
    for (uint32_t i = 0; i < bone_count; i++) {
        uint32_t bone_idx = bone_indices[i];
        if (bone_idx < MAX_BONES) {
            uint32_t mask_index = bone_idx / 32;
            uint32_t bit_index = bone_idx % 32;
            layer->bone_mask[mask_index] |= (1 << bit_index);
        }
    }
}

// Update layer weight
void additive_layer_set_weight(AdditiveBlender *blender, uint32_t layer_index, float weight) {
    if (!blender || layer_index >= blender->layer_count) return;
    
    blender->layers[layer_index].weight = weight;
}

// Evaluate all additive layers
void additive_blender_evaluate(AdditiveBlender *blender, Pose *result_out) {
    if (!blender || !result_out) return;
    
    // Start with base pose
    *result_out = blender->base_pose;
    
    // Apply each additive layer in order
    for (uint32_t layer_idx = 0; layer_idx < blender->layer_count; layer_idx++) {
        AdditiveLayer *layer = &blender->layers[layer_idx];
        
        if (layer->weight <= 0.0f) continue; // Skip zero-weight layers
        
        Pose temp_pose;
        
        if (layer->has_bone_mask) {
            // Apply only to masked bones
            temp_pose = *result_out;
            
            for (uint32_t bone_idx = 0; bone_idx < result_out->bone_count; bone_idx++) {
                uint32_t mask_index = bone_idx / 32;
                uint32_t bit_index = bone_idx % 32;
                
                if (layer->bone_mask[mask_index] & (1 << bit_index)) {
                    // Apply additive to this bone
                    Vector3 additive_pos = vec3_scale(
                        layer->pose.transforms[bone_idx].position,
                        layer->weight
                    );
                    temp_pose.transforms[bone_idx].position = vec3_add(
                        result_out->transforms[bone_idx].position,
                        additive_pos
                    );
                    
                    // Apply additive rotation
                    Quaternion additive_rot = layer->pose.transforms[bone_idx].rotation;
                    additive_rot.x *= layer->weight;
                    additive_rot.y *= layer->weight;
                    additive_rot.z *= layer->weight;
                    additive_rot.w = 1.0f - (1.0f - additive_rot.w) * layer->weight;
                    additive_rot = quat_normalize(additive_rot);
                    
                    temp_pose.transforms[bone_idx].rotation = quat_multiply(
                        result_out->transforms[bone_idx].rotation,
                        additive_rot
                    );
                    temp_pose.transforms[bone_idx].rotation = quat_normalize(temp_pose.transforms[bone_idx].rotation);
                }
            }
        } else {
            // Apply to all bones
            pose_apply_additive(result_out, &layer->pose, layer->weight, &temp_pose);
        }
        
        *result_out = temp_pose;
    }
}

// Remove layer
void additive_blender_remove_layer(AdditiveBlender *blender, uint32_t layer_index) {
    if (!blender || layer_index >= blender->layer_count) return;
    
    // Shift remaining layers down
    for (uint32_t i = layer_index; i < blender->layer_count - 1; i++) {
        blender->layers[i] = blender->layers[i + 1];
    }
    
    blender->layer_count--;
}

// Clear all layers
void additive_blender_clear_layers(AdditiveBlender *blender) {
    if (!blender) return;
    
    blender->layer_count = 0;
}

// Utility: create additive pose from two poses
void additive_create_from_poses(const Pose *bind_pose, const Pose *target_pose, 
                               Pose *additive_out) {
    pose_make_additive(bind_pose, target_pose, additive_out);
}

/*
 * ADDITIVE BLENDING FEATURES:
 * - Layered additive animation system
 * - Bone masking for selective application
 * - Proper quaternion math for rotational additives
 * - Weight blending for smooth transitions
 * - Support for multiple simultaneous layers
 * - Efficient evaluation with early-out for zero weights
 * - Utility functions for creating additive poses
 * 
 * USE CASES:
 * - Aiming offsets (upper body twist)
 * - Recoil animations
 * - Breathing/cycles
 * - Foot placement adjustments
 * - procedural animations
 * - damage reactions
 */
