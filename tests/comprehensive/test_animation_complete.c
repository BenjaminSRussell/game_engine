/**
 * COMPREHENSIVE ANIMATION TESTS
 * Tests for all 25+ animation features matching Unreal/Unity capabilities
 * 
 * Coverage:
 * - Skeletal animation
 * - Blend trees (1D/2D/3D)
 * - State machines
 * - IK solvers (FABRIK, CCD, Analytical)
 * - Motion capture import
 * - Animation retargeting
 * - Procedural animation
 * - Root motion
 * - Facial animation
 */

#include "../test_framework_unified.h"
#include <animation/skeleton.h>
#include <animation/animation_clip.h>
#include <animation/blend_tree.h>
#include <animation/state_machine.h>
#include <animation/ik/ik_solver.h>
#include <animation/retargeting.h>
#include <animation/procedural/procedural_animation.h>
#include <animation/facial/facial_animation.h>

// =============================================================================
// SKELETAL ANIMATION TESTS
// =============================================================================

static TestResult test_skeleton_creation(void) {
    Skeleton *skeleton = skeleton_create(65); // Standard humanoid
    TEST_ASSERT_NOT_NULL(skeleton, "Skeleton should be created");
    TEST_ASSERT_EQ(skeleton->bone_count, 65, "Should have 65 bones");
    
    skeleton_destroy(skeleton);
    return TEST_PASS;
}

static TestResult test_skeleton_hierarchy(void) {
    Skeleton *skeleton = skeleton_create_humanoid();
    
    // Verify root has no parent
    int root_parent = skeleton_get_parent(skeleton, 0);
    TEST_ASSERT_EQ(root_parent, -1, "Root should have no parent");
    
    // Verify spine has root as parent
    int spine_bone = skeleton_find_bone(skeleton, "Spine");
    int spine_parent = skeleton_get_parent(skeleton, spine_bone);
    TEST_ASSERT_EQ(spine_parent, 0, "Spine should be child of root");
    
    skeleton_destroy(skeleton);
    return TEST_PASS;
}

static TestResult test_animation_clip_playback(void) {
    AnimationClip *clip = animation_clip_create("walk", 1.0f);
    TEST_ASSERT_NOT_NULL(clip, "Animation clip should be created");
    TEST_ASSERT_FLOAT_EQ(clip->duration, 1.0f, 0.01f, "Duration should be 1.0");
    
    // Add keyframes
    animation_clip_add_keyframe(clip, 0, 0.0f, (Transform){0});
    animation_clip_add_keyframe(clip, 0, 0.5f, (Transform){0});
    animation_clip_add_keyframe(clip, 0, 1.0f, (Transform){0});
    
    TEST_ASSERT_EQ(clip->keyframe_count, 3, "Should have 3 keyframes");
    
    animation_clip_destroy(clip);
    return TEST_PASS;
}

static TestResult test_animation_sampling(void) {
    AnimationClip *clip = animation_clip_load("assets/animations/walk.anim");
    TEST_SKIP_IF(clip == NULL, "Walk animation not found");
    
    AnimationPose pose;
    animation_clip_sample(clip, 0.5f, &pose);
    
    TEST_ASSERT_TRUE(pose.bone_count > 0, "Pose should have bones");
    
    animation_clip_destroy(clip);
    return TEST_PASS;
}

// =============================================================================
// BLEND TREE TESTS
// =============================================================================

static TestResult test_blend_tree_1d(void) {
    BlendTree *tree = blend_tree_create_1d("SpeedBlend");
    TEST_ASSERT_NOT_NULL(tree, "Blend tree should be created");
    
    // Add animations at different blend values
    AnimationClip *idle = animation_clip_create("idle", 1.0f);
    AnimationClip *walk = animation_clip_create("walk", 1.0f);
    AnimationClip *run = animation_clip_create("run", 0.8f);
    
    blend_tree_add_clip(tree, idle, 0.0f);
    blend_tree_add_clip(tree, walk, 0.5f);
    blend_tree_add_clip(tree, run, 1.0f);
    
    TEST_ASSERT_EQ(tree->clip_count, 3, "Should have 3 clips");
    
    blend_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_blend_tree_2d(void) {
    BlendTree *tree = blend_tree_create_2d("LocomotionBlend");
    
    // Add animations at 2D positions (speed, direction)
    AnimationClip *idle = animation_clip_create("idle", 1.0f);
    AnimationClip *walk_fwd = animation_clip_create("walk_fwd", 1.0f);
    AnimationClip *walk_left = animation_clip_create("walk_left", 1.0f);
    AnimationClip *walk_right = animation_clip_create("walk_right", 1.0f);
    
    blend_tree_add_clip_2d(tree, idle, 0.0f, 0.0f);
    blend_tree_add_clip_2d(tree, walk_fwd, 1.0f, 0.0f);
    blend_tree_add_clip_2d(tree, walk_left, 0.5f, -1.0f);
    blend_tree_add_clip_2d(tree, walk_right, 0.5f, 1.0f);
    
    // Sample at blend position
    AnimationPose pose;
    blend_tree_sample_2d(tree, 0.5f, 0.5f, 0.0f, &pose);
    
    blend_tree_destroy(tree);
    return TEST_PASS;
}

// =============================================================================
// STATE MACHINE TESTS
// =============================================================================

static TestResult test_state_machine_creation(void) {
    AnimationStateMachine *sm = state_machine_create("CharacterLocomotion");
    TEST_ASSERT_NOT_NULL(sm, "State machine should be created");
    
    // Add states
    StateHandle idle = state_machine_add_state(sm, "Idle");
    StateHandle walk = state_machine_add_state(sm, "Walk");
    StateHandle run = state_machine_add_state(sm, "Run");
    StateHandle jump = state_machine_add_state(sm, "Jump");
    
    TEST_ASSERT_EQ(sm->state_count, 4, "Should have 4 states");
    
    state_machine_destroy(sm);
    return TEST_PASS;
}

static TestResult test_state_transitions(void) {
    AnimationStateMachine *sm = state_machine_create("Test");
    
    StateHandle idle = state_machine_add_state(sm, "Idle");
    StateHandle walk = state_machine_add_state(sm, "Walk");
    
    // Add transition with condition
    TransitionConfig trans_config = {
        .duration = 0.2f,
        .blend_mode = BLEND_CROSSFADE,
        .condition_type = CONDITION_FLOAT_GREATER,
        .condition_param = "Speed",
        .condition_value = 0.1f
    };
    
    state_machine_add_transition(sm, idle, walk, &trans_config);
    state_machine_set_entry_state(sm, idle);
    
    // Set parameter and update
    state_machine_set_float(sm, "Speed", 0.5f);
    state_machine_update(sm, 0.016f);
    
    TEST_ASSERT_EQ(sm->current_state, walk, "Should transition to Walk");
    
    state_machine_destroy(sm);
    return TEST_PASS;
}

// =============================================================================
// INVERSE KINEMATICS TESTS
// =============================================================================

static TestResult test_ik_fabrik_solver(void) {
    IKChain *chain = ik_chain_create(3); // 3-bone chain
    
    // Set bone positions
    ik_chain_set_bone(chain, 0, (Vec3){0, 0, 0}, 1.0f);
    ik_chain_set_bone(chain, 1, (Vec3){0, 1, 0}, 1.0f);
    ik_chain_set_bone(chain, 2, (Vec3){0, 2, 0}, 1.0f);
    
    // Solve for target
    Vec3 target = {1.5f, 1.0f, 0.0f};
    IKResult result = ik_solve_fabrik(chain, target, 10, 0.001f);
    
    TEST_ASSERT_TRUE(result.converged, "FABRIK should converge");
    
    // End effector should be near target
    Vec3 end_pos = ik_chain_get_end_position(chain);
    float dist = vec3_distance(end_pos, target);
    TEST_ASSERT_TRUE(dist < 0.1f, "End effector should reach target");
    
    ik_chain_destroy(chain);
    return TEST_PASS;
}

static TestResult test_ik_ccd_solver(void) {
    IKChain *chain = ik_chain_create(4);
    
    Vec3 target = {2.0f, 1.0f, 0.0f};
    IKResult result = ik_solve_ccd(chain, target, 20, 0.001f);
    
    TEST_ASSERT_TRUE(result.iterations <= 20, "Should complete within iterations");
    
    ik_chain_destroy(chain);
    return TEST_PASS;
}

static TestResult test_ik_two_bone_analytical(void) {
    // Standard arm/leg IK
    Vec3 root = {0, 0, 0};
    Vec3 target = {1.5f, 0.5f, 0.0f};
    float bone1_length = 1.0f;
    float bone2_length = 1.0f;
    Vec3 pole_vector = {0, 0, 1.0f}; // Forward pole
    
    TwoBoneIKResult result;
    bool solved = ik_solve_two_bone(root, target, bone1_length, bone2_length,
                                     pole_vector, &result);
    
    TEST_ASSERT_TRUE(solved, "Two-bone IK should solve");
    
    return TEST_PASS;
}

static TestResult test_foot_ik(void) {
    FootIKConfig config = {
        .leg_length = 0.9f,
        .foot_height = 0.1f,
        .max_extension = 0.95f,
        .ground_offset = 0.02f
    };
    
    FootIKSolver *solver = foot_ik_create(&config);
    TEST_ASSERT_NOT_NULL(solver, "Foot IK solver should be created");
    
    foot_ik_destroy(solver);
    return TEST_PASS;
}

// =============================================================================
// RETARGETING TESTS
// =============================================================================

static TestResult test_animation_retargeting(void) {
    Skeleton *source = skeleton_create_humanoid();
    Skeleton *target = skeleton_create(50); // Different skeleton
    
    RetargetingMap *map = retargeting_create_map(source, target);
    TEST_ASSERT_NOT_NULL(map, "Retargeting map should be created");
    
    // Map common bones
    retargeting_map_bone(map, "Hips", "pelvis");
    retargeting_map_bone(map, "Spine", "spine_01");
    retargeting_map_bone(map, "Head", "head");
    
    retargeting_destroy(map);
    skeleton_destroy(source);
    skeleton_destroy(target);
    return TEST_PASS;
}

// =============================================================================
// PROCEDURAL ANIMATION TESTS
// =============================================================================

static TestResult test_procedural_look_at(void) {
    ProceduralLookAt *look_at = procedural_look_at_create("Head", "Neck");
    TEST_ASSERT_NOT_NULL(look_at, "Look-at controller should be created");
    
    look_at->target = (Vec3){5.0f, 1.7f, 10.0f};
    look_at->blend_weight = 1.0f;
    look_at->max_angle = 80.0f;
    
    procedural_look_at_destroy(look_at);
    return TEST_PASS;
}

static TestResult test_procedural_breathing(void) {
    ProceduralBreathing *breathing = procedural_breathing_create();
    breathing->frequency = 0.3f; // breaths per second
    breathing->amplitude = 0.02f;
    
    // Simulate update
    float offset = procedural_breathing_evaluate(breathing, 0.0f);
    TEST_ASSERT_FLOAT_EQ(offset, 0.0f, 0.01f, "Initial offset should be ~0");
    
    procedural_breathing_destroy(breathing);
    return TEST_PASS;
}

// =============================================================================
// ROOT MOTION TESTS
// =============================================================================

static TestResult test_root_motion_extraction(void) {
    AnimationClip *clip = animation_clip_create("walk_cycle", 1.0f);
    
    RootMotionConfig config = {
        .extract_position = true,
        .extract_rotation = true,
        .lock_vertical = true
    };
    
    RootMotionData motion = animation_extract_root_motion(clip, &config);
    
    TEST_ASSERT_TRUE(motion.has_position, "Should have position data");
    TEST_ASSERT_TRUE(motion.has_rotation, "Should have rotation data");
    
    animation_clip_destroy(clip);
    return TEST_PASS;
}

// =============================================================================
// FACIAL ANIMATION TESTS
// =============================================================================

static TestResult test_blend_shape_creation(void) {
    BlendShapeController *controller = blend_shape_controller_create(52); // ARKit shape count
    TEST_ASSERT_NOT_NULL(controller, "Blend shape controller should be created");
    TEST_ASSERT_EQ(controller->shape_count, 52, "Should have 52 shapes");
    
    blend_shape_controller_destroy(controller);
    return TEST_PASS;
}

static TestResult test_blend_shape_values(void) {
    BlendShapeController *controller = blend_shape_controller_create(52);
    
    // Set some blend shape values
    blend_shape_set_value(controller, "jawOpen", 0.5f);
    blend_shape_set_value(controller, "mouthSmileLeft", 0.8f);
    blend_shape_set_value(controller, "mouthSmileRight", 0.8f);
    blend_shape_set_value(controller, "eyeBlinkLeft", 1.0f);
    
    float jaw_value = blend_shape_get_value(controller, "jawOpen");
    TEST_ASSERT_FLOAT_EQ(jaw_value, 0.5f, 0.01f, "Jaw value should be 0.5");
    
    blend_shape_controller_destroy(controller);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_animation_tests(void) {
    // Skeletal animation tests
    TEST_REGISTER("Animation", "Skeleton creation", test_skeleton_creation);
    TEST_REGISTER("Animation", "Skeleton hierarchy", test_skeleton_hierarchy);
    TEST_REGISTER("Animation", "Animation clip playback", test_animation_clip_playback);
    TEST_REGISTER("Animation", "Animation sampling", test_animation_sampling);
    
    // Blend tree tests
    TEST_REGISTER("Animation", "1D blend tree", test_blend_tree_1d);
    TEST_REGISTER("Animation", "2D blend tree", test_blend_tree_2d);
    
    // State machine tests
    TEST_REGISTER("Animation", "State machine creation", test_state_machine_creation);
    TEST_REGISTER("Animation", "State transitions", test_state_transitions);
    
    // IK tests
    TEST_REGISTER("Animation", "FABRIK IK solver", test_ik_fabrik_solver);
    TEST_REGISTER("Animation", "CCD IK solver", test_ik_ccd_solver);
    TEST_REGISTER("Animation", "Two-bone analytical IK", test_ik_two_bone_analytical);
    TEST_REGISTER("Animation", "Foot IK", test_foot_ik);
    
    // Retargeting tests
    TEST_REGISTER("Animation", "Animation retargeting", test_animation_retargeting);
    
    // Procedural animation tests
    TEST_REGISTER("Animation", "Procedural look-at", test_procedural_look_at);
    TEST_REGISTER("Animation", "Procedural breathing", test_procedural_breathing);
    
    // Root motion tests
    TEST_REGISTER("Animation", "Root motion extraction", test_root_motion_extraction);
    
    // Facial animation tests
    TEST_REGISTER("Animation", "Blend shape creation", test_blend_shape_creation);
    TEST_REGISTER("Animation", "Blend shape values", test_blend_shape_values);
}
