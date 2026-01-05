/**
 * UNREAL ENGINE PARITY - API STUBS
 * 
 * This header defines ALL expected APIs for Unreal Engine feature parity.
 * These are TDD stubs - tests are written against this API before implementation.
 * 
 * Each function returns sensible defaults or can be stubbed for testing.
 */

#ifndef UE_PARITY_STUBS_H
#define UE_PARITY_STUBS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// =============================================================================
// BASIC TYPES (Always available)
// =============================================================================

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float x, y, z, w; } Quaternion;
typedef struct { float m[16]; } Mat4;
typedef struct { Vec3 min, max; } AABB;
typedef struct { Vec3 position; Quaternion rotation; Vec3 scale; } Transform;
typedef uint64_t Entity;
typedef uint32_t Handle;

// Color types
typedef struct { float r, g, b, a; } Color;
typedef struct { uint8_t r, g, b, a; } Color32;

// Gameplay types
typedef struct GameplayTag { uint64_t hash; } GameplayTag;
typedef struct GameplayAbility { uint32_t id; } GameplayAbility;
typedef struct GameplayEffect { uint32_t id; float duration; } GameplayEffect;
typedef struct GameplayAttribute { const char* name; float value; } GameplayAttribute;

// =============================================================================
// STUB MODE - All functions return defaults when not implemented
// =============================================================================

#ifndef UE_IMPLEMENTED
#define UE_STUB_RETURN(type, val) return (type)(val)
#define UE_STUB_VOID() return
#define UE_STUB_PTR() return NULL
#else
#define UE_STUB_RETURN(type, val) 
#define UE_STUB_VOID()
#define UE_STUB_PTR()
#endif

// =============================================================================
// 1. RENDERING - NANITE (Virtualized Geometry)
// =============================================================================

typedef struct NaniteCluster NaniteCluster;
typedef struct NaniteMesh NaniteMesh;
typedef struct NaniteRenderData NaniteRenderData;

typedef struct {
    uint32_t max_triangles_per_cluster;
    float error_threshold;
    bool enable_streaming;
    size_t memory_budget_mb;
} NaniteConfig;

NaniteMesh* nanite_mesh_create(void* vertices, uint32_t vertex_count, 
                                uint32_t* indices, uint32_t index_count);
void nanite_mesh_destroy(NaniteMesh* mesh);
uint32_t nanite_mesh_get_cluster_count(NaniteMesh* mesh);
void nanite_render(NaniteRenderData* data, Mat4* view_proj);
float nanite_get_visible_triangles(NaniteMesh* mesh);

// =============================================================================
// 2. RENDERING - LUMEN (Global Illumination)
// =============================================================================

typedef struct LumenScene LumenScene;
typedef struct LumenProbe LumenProbe;

typedef struct {
    float ray_count_per_pixel;
    float indirect_lighting_intensity;
    bool software_ray_tracing;
    bool hardware_ray_tracing;
    float final_gather_quality;
    uint32_t max_trace_distance;
} LumenConfig;

typedef struct {
    Vec3 position;
    Vec3 normal;
    Color indirect_light;
    float ambient_occlusion;
} LumenSample;

LumenScene* lumen_scene_create(LumenConfig* config);
void lumen_scene_destroy(LumenScene* scene);
void lumen_scene_update(LumenScene* scene, float dt);
LumenSample lumen_trace_indirect(LumenScene* scene, Vec3 pos, Vec3 normal);
void lumen_invalidate_region(LumenScene* scene, AABB region);
void lumen_set_sky_light(LumenScene* scene, Color color, float intensity);

// =============================================================================
// 3. RENDERING - VIRTUAL SHADOW MAPS
// =============================================================================

typedef struct VirtualShadowMap VirtualShadowMap;

typedef struct {
    uint32_t page_size;        // Typically 128 or 256
    uint32_t physical_pages;   // Total physical memory pages
    uint32_t max_lights;
    bool enable_caching;
} VSMConfig;

VirtualShadowMap* vsm_create(VSMConfig* config);
void vsm_destroy(VirtualShadowMap* vsm);
void vsm_update_light(VirtualShadowMap* vsm, uint32_t light_id, Mat4* light_matrix);
void vsm_render_shadow_casters(VirtualShadowMap* vsm);
float vsm_sample(VirtualShadowMap* vsm, uint32_t light_id, Vec3 world_pos);

// =============================================================================
// 4. RENDERING - TEMPORAL SUPER RESOLUTION (TSR)
// =============================================================================

typedef struct TSRState TSRState;

typedef struct {
    float sharpness;
    float history_weight;
    uint32_t input_width;
    uint32_t input_height;
    uint32_t output_width;
    uint32_t output_height;
} TSRConfig;

TSRState* tsr_create(TSRConfig* config);
void tsr_destroy(TSRState* state);
void tsr_upscale(TSRState* state, Handle color_input, Handle motion_vectors,
                 Handle depth, Handle output);
void tsr_reset_history(TSRState* state);

// =============================================================================
// 5. WORLD PARTITION & STREAMING
// =============================================================================

typedef struct WorldPartition WorldPartition;
typedef struct StreamingLevel StreamingLevel;
typedef struct DataLayer DataLayer;

typedef struct {
    float cell_size;
    uint32_t loading_range;
    uint32_t max_loaded_cells;
    bool enable_hlod;
} WorldPartitionConfig;

WorldPartition* world_partition_create(WorldPartitionConfig* config);
void world_partition_destroy(WorldPartition* wp);
void world_partition_update(WorldPartition* wp, Vec3 viewer_position);
bool world_partition_is_cell_loaded(WorldPartition* wp, int x, int y, int z);
StreamingLevel* world_partition_get_cell_level(WorldPartition* wp, int x, int y, int z);
void world_partition_add_data_layer(WorldPartition* wp, DataLayer* layer);

// HLOD (Hierarchical Level of Detail)
typedef struct HLOD HLOD;
HLOD* hlod_create(float* lod_distances, uint32_t level_count);
void hlod_build_from_actors(HLOD* hlod, Entity* actors, uint32_t count);
void hlod_destroy(HLOD* hlod);

// =============================================================================
// 6. CONTROL RIG & FULL BODY IK
// =============================================================================

typedef struct ControlRig ControlRig;
typedef struct RigUnit RigUnit;
typedef struct FBIKSolver FBIKSolver;

typedef struct {
    const char* name;
    uint32_t bone_index;
    Vec3 goal_position;
    Quaternion goal_rotation;
    float position_weight;
    float rotation_weight;
} IKEffector;

ControlRig* control_rig_create(void* skeleton);
void control_rig_destroy(ControlRig* rig);
void control_rig_add_effector(ControlRig* rig, IKEffector* effector);
void control_rig_solve(ControlRig* rig, float dt);
void control_rig_get_pose(ControlRig* rig, Transform* out_pose, uint32_t bone_count);

// Full Body IK
FBIKSolver* fbik_create(void* skeleton);
void fbik_destroy(FBIKSolver* solver);
void fbik_set_root_settings(FBIKSolver* solver, float stiffness, float damping);
void fbik_solve(FBIKSolver* solver, IKEffector* effectors, uint32_t count);

// =============================================================================
// 7. NIAGARA PARTICLE SYSTEM
// =============================================================================

typedef struct NiagaraSystem NiagaraSystem;
typedef struct NiagaraEmitter NiagaraEmitter;
typedef struct NiagaraModule NiagaraModule;

typedef enum {
    NIAGARA_SPAWN_RATE,
    NIAGARA_SPAWN_BURST,
    NIAGARA_SPAWN_PER_UNIT
} NiagaraSpawnMode;

typedef struct {
    NiagaraSpawnMode spawn_mode;
    float spawn_rate;
    uint32_t max_particles;
    bool gpu_simulation;
    bool deterministic;
} NiagaraEmitterConfig;

NiagaraSystem* niagara_system_create(const char* name);
void niagara_system_destroy(NiagaraSystem* sys);
NiagaraEmitter* niagara_add_emitter(NiagaraSystem* sys, NiagaraEmitterConfig* config);
void niagara_emitter_add_module(NiagaraEmitter* emitter, NiagaraModule* module);
void niagara_system_activate(NiagaraSystem* sys, Vec3 position);
void niagara_system_update(NiagaraSystem* sys, float dt);
uint32_t niagara_get_particle_count(NiagaraSystem* sys);

// Niagara Data Interface
typedef struct NiagaraDataInterface NiagaraDataInterface;
NiagaraDataInterface* niagara_di_create_mesh_sample(void* mesh);
NiagaraDataInterface* niagara_di_create_collision_query(void* world);
NiagaraDataInterface* niagara_di_create_audio_spectrum(void);

// =============================================================================
// 8. CHAOS PHYSICS
// =============================================================================

typedef struct ChaosScene ChaosScene;
typedef struct ChaosRigidBody ChaosRigidBody;
typedef struct ChaosField ChaosField;
typedef struct ChaosClustering ChaosClustering;

// Destruction
typedef struct {
    uint32_t cluster_connection_method;
    float damage_threshold;
    uint32_t max_cluster_count;
    bool enable_clustering;
    float* cluster_sizes;
    uint32_t cluster_level_count;
} ChaosDestructionConfig;

typedef struct DestructibleMesh DestructibleMesh;
DestructibleMesh* chaos_destructible_create(void* mesh, ChaosDestructionConfig* config);
void chaos_destructible_destroy(DestructibleMesh* dm);
void chaos_destructible_apply_damage(DestructibleMesh* dm, Vec3 location, 
                                      float radius, float damage);
uint32_t chaos_destructible_get_chunk_count(DestructibleMesh* dm);

// Chaos Cloth
typedef struct ChaosCloth ChaosCloth;
typedef struct {
    float mass_per_area;
    float stiffness;
    float damping;
    float friction;
    float collision_thickness;
    bool self_collision;
} ChaosClothConfig;

ChaosCloth* chaos_cloth_create(void* mesh, ChaosClothConfig* config);
void chaos_cloth_destroy(ChaosCloth* cloth);
void chaos_cloth_simulate(ChaosCloth* cloth, float dt);
void chaos_cloth_apply_wind(ChaosCloth* cloth, Vec3 direction, float strength);

// Chaos Vehicles
typedef struct ChaosVehicle ChaosVehicle;
typedef struct {
    float chassis_mass;
    float max_engine_rpm;
    float* torque_curve;
    uint32_t torque_curve_points;
    uint32_t wheel_count;
    float suspension_max_raise;
    float suspension_max_drop;
} ChaosVehicleConfig;

ChaosVehicle* chaos_vehicle_create(ChaosVehicleConfig* config);
void chaos_vehicle_destroy(ChaosVehicle* vehicle);
void chaos_vehicle_set_throttle(ChaosVehicle* vehicle, float throttle);
void chaos_vehicle_set_steering(ChaosVehicle* vehicle, float steering);
void chaos_vehicle_set_brake(ChaosVehicle* vehicle, float brake);
float chaos_vehicle_get_speed_kmh(ChaosVehicle* vehicle);

// =============================================================================
// 9. METASOUNDS AUDIO
// =============================================================================

typedef struct MetaSoundGraph MetaSoundGraph;
typedef struct MetaSoundNode MetaSoundNode;
typedef struct MetaSoundSource MetaSoundSource;

typedef enum {
    METASOUND_INPUT_TRIGGER,
    METASOUND_INPUT_FLOAT,
    METASOUND_INPUT_INT,
    METASOUND_INPUT_BOOL,
    METASOUND_INPUT_AUDIO
} MetaSoundInputType;

MetaSoundGraph* metasound_graph_create(const char* name);
void metasound_graph_destroy(MetaSoundGraph* graph);
MetaSoundNode* metasound_add_node(MetaSoundGraph* graph, const char* node_type);
void metasound_connect(MetaSoundNode* from, const char* output_pin,
                       MetaSoundNode* to, const char* input_pin);
MetaSoundSource* metasound_instantiate(MetaSoundGraph* graph, Vec3 position);
void metasound_set_parameter(MetaSoundSource* source, const char* param, float value);
void metasound_trigger(MetaSoundSource* source, const char* trigger_name);

// =============================================================================
// 10. GAMEPLAY ABILITY SYSTEM (GAS)
// =============================================================================

typedef struct AbilitySystemComponent AbilitySystemComponent;
typedef struct GameplayAbility GameplayAbility;
typedef struct GameplayEffect GameplayEffect;
typedef struct GameplayTag GameplayTag;
typedef struct GameplayAttribute GameplayAttribute;

typedef struct {
    const char* name;
    float base_value;
    float current_value;
    float min_value;
    float max_value;
} AttributeDef;

AbilitySystemComponent* gas_create(Entity owner);
void gas_destroy(AbilitySystemComponent* asc);

// Abilities
Handle gas_give_ability(AbilitySystemComponent* asc, GameplayAbility* ability);
bool gas_try_activate_ability(AbilitySystemComponent* asc, Handle ability_handle);
void gas_cancel_ability(AbilitySystemComponent* asc, Handle ability_handle);
bool gas_can_activate_ability(AbilitySystemComponent* asc, Handle ability_handle);

// Effects
Handle gas_apply_effect(AbilitySystemComponent* target, GameplayEffect* effect,
                        AbilitySystemComponent* source, float level);
void gas_remove_effect(AbilitySystemComponent* asc, Handle effect_handle);
bool gas_has_effect(AbilitySystemComponent* asc, Handle effect_handle);

// Attributes
void gas_init_attribute(AbilitySystemComponent* asc, AttributeDef* attr);
float gas_get_attribute(AbilitySystemComponent* asc, const char* attr_name);
void gas_set_attribute(AbilitySystemComponent* asc, const char* attr_name, float value);

// Tags
void gas_add_tag(AbilitySystemComponent* asc, GameplayTag tag);
void gas_remove_tag(AbilitySystemComponent* asc, GameplayTag tag);
bool gas_has_tag(AbilitySystemComponent* asc, GameplayTag tag);
bool gas_has_any_tag(AbilitySystemComponent* asc, GameplayTag* tags, uint32_t count);
bool gas_has_all_tags(AbilitySystemComponent* asc, GameplayTag* tags, uint32_t count);

// =============================================================================
// 11. ENHANCED INPUT SYSTEM
// =============================================================================

typedef struct InputMappingContext InputMappingContext;
typedef struct InputAction InputAction;
typedef struct InputTrigger InputTrigger;
typedef struct InputModifier InputModifier;

typedef enum {
    INPUT_TRIGGER_DOWN,
    INPUT_TRIGGER_PRESSED,
    INPUT_TRIGGER_RELEASED,
    INPUT_TRIGGER_HOLD,
    INPUT_TRIGGER_TAP,
    INPUT_TRIGGER_PULSE,
    INPUT_TRIGGER_CHORD
} TriggerType;

InputAction* input_action_create(const char* name);
void input_action_destroy(InputAction* action);
void input_action_add_trigger(InputAction* action, InputTrigger* trigger);
void input_action_add_modifier(InputAction* action, InputModifier* modifier);

InputMappingContext* input_context_create(const char* name);
void input_context_destroy(InputMappingContext* ctx);
void input_context_bind_action(InputMappingContext* ctx, InputAction* action, 
                                uint32_t key_or_button);
void input_context_set_priority(InputMappingContext* ctx, int priority);

typedef void (*InputActionCallback)(InputAction* action, float value, float elapsed);
void input_bind_callback(InputAction* action, InputActionCallback callback);

// =============================================================================
// 12. MASS ENTITY (MASS AI)
// =============================================================================

typedef struct MassEntityManager MassEntityManager;
typedef struct MassFragment MassFragment;
typedef struct MassProcessor MassProcessor;
typedef struct MassSpawner MassSpawner;

typedef struct {
    uint32_t max_entities;
    bool use_zone_graph;
    float observation_radius;
} MassConfig;

MassEntityManager* mass_create(MassConfig* config);
void mass_destroy(MassEntityManager* mass);
void mass_register_fragment(MassEntityManager* mass, MassFragment* fragment, size_t size);
void mass_register_processor(MassEntityManager* mass, MassProcessor* processor);
Handle mass_spawn_entity(MassEntityManager* mass, void* template_data);
void mass_destroy_entity(MassEntityManager* mass, Handle entity);
void mass_tick(MassEntityManager* mass, float dt);
uint32_t mass_get_entity_count(MassEntityManager* mass);

// =============================================================================
// 13. STATE TREE (AI)
// =============================================================================

typedef struct StateTree StateTree;
typedef struct StateTreeState StateTreeState;
typedef struct StateTreeTask StateTreeTask;

StateTree* state_tree_create(const char* name);
void state_tree_destroy(StateTree* tree);
StateTreeState* state_tree_add_state(StateTree* tree, const char* name, 
                                      StateTreeState* parent);
void state_tree_add_task(StateTreeState* state, StateTreeTask* task);
void state_tree_add_transition(StateTree* tree, StateTreeState* from,
                                StateTreeState* to, const char* condition);
void state_tree_start(StateTree* tree, void* context);
void state_tree_tick(StateTree* tree, float dt);
const char* state_tree_get_current_state(StateTree* tree);

// =============================================================================
// 14. SMART OBJECTS
// =============================================================================

typedef struct SmartObjectSubsystem SmartObjectSubsystem;
typedef struct SmartObjectDefinition SmartObjectDefinition;
typedef struct SmartObjectSlot SmartObjectSlot;

typedef struct {
    Vec3 offset;
    Quaternion rotation;
    float interaction_distance;
    const char* activity_tag;
    void* behavior_definition;
} SmartObjectSlotDef;

SmartObjectSubsystem* smart_object_subsystem_create(void);
void smart_object_subsystem_destroy(SmartObjectSubsystem* ss);
Handle smart_object_register(SmartObjectSubsystem* ss, SmartObjectDefinition* def, 
                              Transform transform);
void smart_object_unregister(SmartObjectSubsystem* ss, Handle handle);
SmartObjectSlot* smart_object_find_slot(SmartObjectSubsystem* ss, Vec3 location, 
                                         float radius, const char* activity_tag);
bool smart_object_claim_slot(SmartObjectSlot* slot, Entity claimer);
void smart_object_release_slot(SmartObjectSlot* slot, Entity claimer);

// =============================================================================
// 15. PCG (PROCEDURAL CONTENT GENERATION)
// =============================================================================

typedef struct PCGGraph PCGGraph;
typedef struct PCGNode PCGNode;
typedef struct PCGData PCGData;

typedef enum {
    PCG_DATA_POINT,
    PCG_DATA_SPLINE,
    PCG_DATA_MESH,
    PCG_DATA_LANDSCAPE,
    PCG_DATA_VOLUME
} PCGDataType;

PCGGraph* pcg_graph_create(const char* name);
void pcg_graph_destroy(PCGGraph* graph);
PCGNode* pcg_add_node(PCGGraph* graph, const char* node_type);
void pcg_connect(PCGNode* from, const char* output, PCGNode* to, const char* input);
PCGData* pcg_execute(PCGGraph* graph, AABB bounds, uint32_t seed);
void pcg_data_destroy(PCGData* data);
uint32_t pcg_data_get_point_count(PCGData* data);
Vec3 pcg_data_get_point(PCGData* data, uint32_t index);

// =============================================================================
// 16. WATER SYSTEM
// =============================================================================

typedef struct WaterBody WaterBody;
typedef struct WaterZone WaterZone;
typedef struct WaterWaves WaterWaves;

typedef enum {
    WATER_TYPE_OCEAN,
    WATER_TYPE_LAKE,
    WATER_TYPE_RIVER,
    WATER_TYPE_CUSTOM
} WaterBodyType;

typedef struct {
    WaterBodyType type;
    float wave_amplitude;
    float wave_length;
    float wave_speed;
    Color surface_color;
    Color underwater_color;
    float visibility_depth;
    bool enable_caustics;
} WaterBodyConfig;

WaterBody* water_body_create(WaterBodyConfig* config);
void water_body_destroy(WaterBody* body);
void water_body_set_spline(WaterBody* body, Vec3* points, uint32_t count);
float water_body_get_height_at(WaterBody* body, Vec3 position);
Vec3 water_body_get_velocity_at(WaterBody* body, Vec3 position);
bool water_body_is_underwater(WaterBody* body, Vec3 position);
void water_body_apply_force(WaterBody* body, Vec3 position, Vec3 force);

// =============================================================================
// 17. LANDSCAPE / TERRAIN
// =============================================================================

typedef struct Landscape Landscape;
typedef struct LandscapeComponent LandscapeComponent;
typedef struct LandscapeLayer LandscapeLayer;

typedef struct {
    uint32_t component_size;
    uint32_t section_count;
    uint32_t quads_per_section;
    float scale_x, scale_y, scale_z;
    uint32_t max_lod;
} LandscapeConfig;

Landscape* landscape_create(LandscapeConfig* config);
void landscape_destroy(Landscape* landscape);
void landscape_import_heightmap(Landscape* landscape, float* data, 
                                 uint32_t width, uint32_t height);
float landscape_get_height_at(Landscape* landscape, float x, float y);
Vec3 landscape_get_normal_at(Landscape* landscape, float x, float y);

// Paint layers
LandscapeLayer* landscape_add_layer(Landscape* landscape, const char* name);
void landscape_paint_layer(Landscape* landscape, LandscapeLayer* layer,
                           float x, float y, float radius, float strength);
uint8_t landscape_get_layer_weight(Landscape* landscape, LandscapeLayer* layer, 
                                    float x, float y);

// =============================================================================
// 18. FOLIAGE SYSTEM
// =============================================================================

typedef struct FoliageType FoliageType;
typedef struct FoliageActor FoliageActor;
typedef struct FoliageInstance FoliageInstance;

typedef struct {
    void* mesh;
    float min_scale;
    float max_scale;
    float density;
    bool collision;
    float cull_distance_min;
    float cull_distance_max;
    bool cast_shadow;
} FoliageTypeConfig;

FoliageType* foliage_type_create(FoliageTypeConfig* config);
void foliage_type_destroy(FoliageType* type);
FoliageActor* foliage_actor_create(void);
void foliage_actor_destroy(FoliageActor* actor);
void foliage_actor_add_instances(FoliageActor* actor, FoliageType* type,
                                  Transform* transforms, uint32_t count);
uint32_t foliage_actor_get_instance_count(FoliageActor* actor);
void foliage_actor_remove_instances_in_box(FoliageActor* actor, AABB box);

// =============================================================================
// 19. MOVIE RENDER QUEUE
// =============================================================================

typedef struct MoviePipeline MoviePipeline;
typedef struct MoviePipelineConfig MoviePipelineConfig;
typedef struct MovieRenderQueue MovieRenderQueue;

typedef struct {
    uint32_t output_width;
    uint32_t output_height;
    uint32_t anti_aliasing_samples;
    uint32_t motion_blur_samples;
    float frame_rate;
    const char* output_format;  // "EXR", "PNG", "BMP", "JPG"
    const char* output_directory;
    bool render_warm_up_frames;
    uint32_t warm_up_count;
} MovieRenderSettings;

MoviePipeline* movie_pipeline_create(MovieRenderSettings* settings);
void movie_pipeline_destroy(MoviePipeline* pipeline);
void movie_pipeline_set_sequence(MoviePipeline* pipeline, void* level_sequence);
void movie_pipeline_start_render(MoviePipeline* pipeline);
float movie_pipeline_get_progress(MoviePipeline* pipeline);
bool movie_pipeline_is_finished(MoviePipeline* pipeline);
void movie_pipeline_abort(MoviePipeline* pipeline);

// =============================================================================
// 20. LEVEL SEQUENCES (CINEMATICS)
// =============================================================================

typedef struct LevelSequence LevelSequence;
typedef struct MovieSceneTrack MovieSceneTrack;
typedef struct MovieSceneSection MovieSceneSection;
typedef struct SequencePlayer SequencePlayer;

LevelSequence* level_sequence_create(const char* name, float duration);
void level_sequence_destroy(LevelSequence* seq);
MovieSceneTrack* level_sequence_add_track(LevelSequence* seq, const char* track_type);
MovieSceneSection* movie_scene_add_section(MovieSceneTrack* track, 
                                            float start_time, float end_time);
void movie_scene_section_add_key(MovieSceneSection* section, float time, void* value);

SequencePlayer* sequence_player_create(LevelSequence* sequence);
void sequence_player_destroy(SequencePlayer* player);
void sequence_player_play(SequencePlayer* player);
void sequence_player_pause(SequencePlayer* player);
void sequence_player_stop(SequencePlayer* player);
void sequence_player_set_time(SequencePlayer* player, float time);
float sequence_player_get_current_time(SequencePlayer* player);
bool sequence_player_is_playing(SequencePlayer* player);

#endif // UE_PARITY_STUBS_H
