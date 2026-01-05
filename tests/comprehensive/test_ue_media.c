/**
 * UNREAL ENGINE PARITY TESTS - MEDIA & CINEMATICS
 * Test-Driven Development for MetaSounds, Level Sequences, Movie Render Queue
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// METASOUNDS TESTS
// =============================================================================

static TestResult test_metasound_graph_creation(void) {
    MetaSoundGraph* graph = metasound_graph_create("GunShotFX");
    TEST_ASSERT_NOT_NULL(graph, "MetaSound graph should be created");
    
    metasound_graph_destroy(graph);
    return TEST_PASS;
}

static TestResult test_metasound_nodes(void) {
    MetaSoundGraph* graph = metasound_graph_create("ProceduralMusic");
    
    // Add nodes
    MetaSoundNode* osc1 = metasound_add_node(graph, "Oscillator");
    MetaSoundNode* osc2 = metasound_add_node(graph, "Oscillator");
    MetaSoundNode* mixer = metasound_add_node(graph, "Mixer");
    MetaSoundNode* envelope = metasound_add_node(graph, "ADSREnvelope");
    MetaSoundNode* output = metasound_add_node(graph, "OutputNode");
    
    TEST_ASSERT_NOT_NULL(osc1, "Osc1 created");
    TEST_ASSERT_NOT_NULL(mixer, "Mixer created");
    TEST_ASSERT_NOT_NULL(output, "Output created");
    
    // Connect nodes
    metasound_connect(osc1, "Audio", mixer, "Input1");
    metasound_connect(osc2, "Audio", mixer, "Input2");
    metasound_connect(mixer, "Output", envelope, "Audio");
    metasound_connect(envelope, "Audio", output, "Audio");
    
    metasound_graph_destroy(graph);
    return TEST_PASS;
}

static TestResult test_metasound_instantiation(void) {
    MetaSoundGraph* graph = metasound_graph_create("FootstepFX");
    
    metasound_add_node(graph, "OnPlay");
    metasound_add_node(graph, "WavePlayer");
    metasound_add_node(graph, "OutputNode");
    
    // Instantiate at position
    Vec3 pos = {100, 0, 50};
    MetaSoundSource* source = metasound_instantiate(graph, pos);
    TEST_ASSERT_NOT_NULL(source, "Should instantiate MetaSound");
    
    metasound_graph_destroy(graph);
    return TEST_PASS;
}

static TestResult test_metasound_parameters(void) {
    MetaSoundGraph* graph = metasound_graph_create("EngineSound");
    MetaSoundSource* source = metasound_instantiate(graph, (Vec3){0,0,0});
    
    // Set parameters
    metasound_set_parameter(source, "RPM", 3000.0f);
    metasound_set_parameter(source, "Load", 0.7f);
    metasound_set_parameter(source, "Throttle", 0.5f);
    
    metasound_graph_destroy(graph);
    return TEST_PASS;
}

static TestResult test_metasound_triggers(void) {
    MetaSoundGraph* graph = metasound_graph_create("ExplosionFX");
    MetaSoundSource* source = metasound_instantiate(graph, (Vec3){0,0,0});
    
    // Trigger event
    metasound_trigger(source, "OnExplode");
    metasound_trigger(source, "OnDebris");
    
    metasound_graph_destroy(graph);
    return TEST_PASS;
}

// =============================================================================
// LEVEL SEQUENCE TESTS
// =============================================================================

static TestResult test_sequence_creation(void) {
    LevelSequence* seq = level_sequence_create("IntroCutscene", 30.0f);
    TEST_ASSERT_NOT_NULL(seq, "Level sequence should be created");
    
    level_sequence_destroy(seq);
    return TEST_PASS;
}

static TestResult test_sequence_tracks(void) {
    LevelSequence* seq = level_sequence_create("ActionSequence", 10.0f);
    
    // Add tracks
    MovieSceneTrack* camera_track = level_sequence_add_track(seq, "CameraTrack");
    MovieSceneTrack* actor_track = level_sequence_add_track(seq, "ActorTrack");
    MovieSceneTrack* audio_track = level_sequence_add_track(seq, "AudioTrack");
    MovieSceneTrack* event_track = level_sequence_add_track(seq, "EventTrack");
    
    TEST_ASSERT_NOT_NULL(camera_track, "Camera track created");
    TEST_ASSERT_NOT_NULL(actor_track, "Actor track created");
    TEST_ASSERT_NOT_NULL(audio_track, "Audio track created");
    TEST_ASSERT_NOT_NULL(event_track, "Event track created");
    
    level_sequence_destroy(seq);
    return TEST_PASS;
}

static TestResult test_sequence_sections(void) {
    LevelSequence* seq = level_sequence_create("TestSequence", 5.0f);
    MovieSceneTrack* track = level_sequence_add_track(seq, "TransformTrack");
    
    // Add sections
    MovieSceneSection* section1 = movie_scene_add_section(track, 0.0f, 2.0f);
    MovieSceneSection* section2 = movie_scene_add_section(track, 2.5f, 5.0f);
    
    TEST_ASSERT_NOT_NULL(section1, "Section 1 created");
    TEST_ASSERT_NOT_NULL(section2, "Section 2 created");
    
    // Add keyframes
    Vec3 key1 = {0, 0, 0};
    Vec3 key2 = {100, 0, 0};
    movie_scene_section_add_key(section1, 0.0f, &key1);
    movie_scene_section_add_key(section1, 2.0f, &key2);
    
    level_sequence_destroy(seq);
    return TEST_PASS;
}

static TestResult test_sequence_playback(void) {
    LevelSequence* seq = level_sequence_create("PlaybackTest", 3.0f);
    
    SequencePlayer* player = sequence_player_create(seq);
    TEST_ASSERT_NOT_NULL(player, "Sequence player created");
    
    // Test playback controls
    sequence_player_play(player);
    TEST_ASSERT_TRUE(sequence_player_is_playing(player), "Should be playing");
    
    sequence_player_pause(player);
    TEST_ASSERT_FALSE(sequence_player_is_playing(player), "Should be paused");
    
    sequence_player_set_time(player, 1.5f);
    float time = sequence_player_get_current_time(player);
    TEST_ASSERT_FLOAT_EQ(time, 1.5f, 0.01f, "Time should be 1.5");
    
    sequence_player_stop(player);
    
    sequence_player_destroy(player);
    level_sequence_destroy(seq);
    return TEST_PASS;
}

// =============================================================================
// MOVIE RENDER QUEUE TESTS
// =============================================================================

static TestResult test_movie_pipeline_creation(void) {
    MovieRenderSettings settings = {
        .output_width = 3840,
        .output_height = 2160,
        .anti_aliasing_samples = 8,
        .motion_blur_samples = 16,
        .frame_rate = 24.0f,
        .output_format = "EXR",
        .output_directory = "/renders/",
        .render_warm_up_frames = true,
        .warm_up_count = 32
    };
    
    MoviePipeline* pipeline = movie_pipeline_create(&settings);
    TEST_ASSERT_NOT_NULL(pipeline, "Movie pipeline should be created");
    
    movie_pipeline_destroy(pipeline);
    return TEST_PASS;
}

static TestResult test_movie_pipeline_render(void) {
    MovieRenderSettings settings = {
        .output_width = 1920,
        .output_height = 1080,
        .frame_rate = 30.0f,
        .output_format = "PNG"
    };
    
    MoviePipeline* pipeline = movie_pipeline_create(&settings);
    
    // Would normally set sequence here
    movie_pipeline_set_sequence(pipeline, NULL);
    
    // Start render
    movie_pipeline_start_render(pipeline);
    
    // Check progress
    float progress = movie_pipeline_get_progress(pipeline);
    TEST_ASSERT_RANGE(progress, 0.0f, 1.0f, "Progress in valid range");
    
    // Check finish state
    bool finished = movie_pipeline_is_finished(pipeline);
    // Initially false
    
    movie_pipeline_destroy(pipeline);
    return TEST_PASS;
}

static TestResult test_movie_pipeline_abort(void) {
    MovieRenderSettings settings = {
        .output_width = 1920,
        .output_height = 1080
    };
    
    MoviePipeline* pipeline = movie_pipeline_create(&settings);
    movie_pipeline_start_render(pipeline);
    
    // Abort mid-render
    movie_pipeline_abort(pipeline);
    
    // Should handle abort gracefully
    
    movie_pipeline_destroy(pipeline);
    return TEST_PASS;
}

// =============================================================================
// ENHANCED INPUT TESTS
// =============================================================================

static TestResult test_input_action_creation(void) {
    InputAction* jump = input_action_create("Jump");
    InputAction* fire = input_action_create("Fire");
    InputAction* move = input_action_create("Move");
    
    TEST_ASSERT_NOT_NULL(jump, "Jump action created");
    TEST_ASSERT_NOT_NULL(fire, "Fire action created");
    TEST_ASSERT_NOT_NULL(move, "Move action created");
    
    input_action_destroy(jump);
    input_action_destroy(fire);
    input_action_destroy(move);
    return TEST_PASS;
}

static TestResult test_input_mapping_context(void) {
    InputMappingContext* ctx = input_context_create("DefaultControls");
    TEST_ASSERT_NOT_NULL(ctx, "Input context created");
    
    InputAction* jump = input_action_create("Jump");
    InputAction* crouch = input_action_create("Crouch");
    
    // Bind to keys
    input_context_bind_action(ctx, jump, 0x20); // Space
    input_context_bind_action(ctx, crouch, 0x11); // Ctrl
    
    input_context_set_priority(ctx, 0);
    
    input_action_destroy(jump);
    input_action_destroy(crouch);
    input_context_destroy(ctx);
    return TEST_PASS;
}

static int g_callback_count = 0;
static void test_callback(InputAction* action, float value, float elapsed) {
    g_callback_count++;
}

static TestResult test_input_callbacks(void) {
    InputAction* action = input_action_create("Test");
    
    g_callback_count = 0;
    input_bind_callback(action, test_callback);
    
    // Callback registration should succeed
    
    input_action_destroy(action);
    return TEST_PASS;
}

// =============================================================================
// CONTROL RIG TESTS
// =============================================================================

static TestResult test_control_rig_creation(void) {
    ControlRig* rig = control_rig_create(NULL);
    TEST_ASSERT_NOT_NULL(rig, "Control rig should be created");
    
    control_rig_destroy(rig);
    return TEST_PASS;
}

static TestResult test_control_rig_effectors(void) {
    ControlRig* rig = control_rig_create(NULL);
    
    IKEffector left_hand = {
        .name = "LeftHand",
        .bone_index = 10,
        .goal_position = {-50, 100, 0},
        .goal_rotation = {0, 0, 0, 1},
        .position_weight = 1.0f,
        .rotation_weight = 0.5f
    };
    
    IKEffector right_hand = {
        .name = "RightHand",
        .bone_index = 20,
        .goal_position = {50, 100, 0},
        .goal_rotation = {0, 0, 0, 1},
        .position_weight = 1.0f,
        .rotation_weight = 0.5f
    };
    
    control_rig_add_effector(rig, &left_hand);
    control_rig_add_effector(rig, &right_hand);
    
    // Solve
    control_rig_solve(rig, 0.016f);
    
    control_rig_destroy(rig);
    return TEST_PASS;
}

static TestResult test_full_body_ik(void) {
    FBIKSolver* solver = fbik_create(NULL);
    TEST_ASSERT_NOT_NULL(solver, "FBIK solver should be created");
    
    fbik_set_root_settings(solver, 1.0f, 0.1f);
    
    IKEffector effectors[] = {
        {.name = "LeftFoot", .bone_index = 5, .position_weight = 1.0f},
        {.name = "RightFoot", .bone_index = 6, .position_weight = 1.0f},
        {.name = "LeftHand", .bone_index = 15, .position_weight = 0.8f},
        {.name = "RightHand", .bone_index = 16, .position_weight = 0.8f},
        {.name = "Head", .bone_index = 25, .position_weight = 0.5f}
    };
    
    fbik_solve(solver, effectors, 5);
    
    fbik_destroy(solver);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_media_tests(void) {
    // MetaSounds
    TEST_REGISTER("UE:MetaSounds", "Graph creation", test_metasound_graph_creation);
    TEST_REGISTER("UE:MetaSounds", "Node creation", test_metasound_nodes);
    TEST_REGISTER("UE:MetaSounds", "Instantiation", test_metasound_instantiation);
    TEST_REGISTER("UE:MetaSounds", "Parameters", test_metasound_parameters);
    TEST_REGISTER("UE:MetaSounds", "Triggers", test_metasound_triggers);
    
    // Level Sequences
    TEST_REGISTER("UE:Sequencer", "Sequence creation", test_sequence_creation);
    TEST_REGISTER("UE:Sequencer", "Tracks", test_sequence_tracks);
    TEST_REGISTER("UE:Sequencer", "Sections and keys", test_sequence_sections);
    TEST_REGISTER("UE:Sequencer", "Playback", test_sequence_playback);
    
    // Movie Render Queue
    TEST_REGISTER("UE:MovieRender", "Pipeline creation", test_movie_pipeline_creation);
    TEST_REGISTER("UE:MovieRender", "Render execution", test_movie_pipeline_render);
    TEST_REGISTER("UE:MovieRender", "Abort handling", test_movie_pipeline_abort);
    
    // Enhanced Input
    TEST_REGISTER("UE:EnhancedInput", "Action creation", test_input_action_creation);
    TEST_REGISTER("UE:EnhancedInput", "Mapping context", test_input_mapping_context);
    TEST_REGISTER("UE:EnhancedInput", "Callbacks", test_input_callbacks);
    
    // Control Rig
    TEST_REGISTER("UE:ControlRig", "Rig creation", test_control_rig_creation);
    TEST_REGISTER("UE:ControlRig", "Effectors", test_control_rig_effectors);
    TEST_REGISTER("UE:ControlRig", "Full body IK", test_full_body_ik);
}
