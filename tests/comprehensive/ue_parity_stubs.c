/**
 * UNREAL ENGINE PARITY - STUB IMPLEMENTATIONS
 * 
 * These stub implementations allow tests to be compiled and run
 * without the actual engine systems being implemented yet.
 * 
 * Each function returns sensible defaults - when implementing
 * the real system, replace these stubs and the tests will validate
 * correct behavior.
 */

#include "ue_parity_stubs.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// =============================================================================
// NANITE STUBS
// =============================================================================

struct NaniteMesh {
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t cluster_count;
};

NaniteMesh* nanite_mesh_create(void* vertices, uint32_t vertex_count, 
                                uint32_t* indices, uint32_t index_count) {
    NaniteMesh* mesh = calloc(1, sizeof(NaniteMesh));
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    // Estimate clusters: ~10000 triangles per cluster
    uint32_t triangles = index_count / 3;
    mesh->cluster_count = (triangles / 10000u) + 1u;
    return mesh;
}

void nanite_mesh_destroy(NaniteMesh* mesh) { free(mesh); }

uint32_t nanite_mesh_get_cluster_count(NaniteMesh* mesh) {
    return mesh ? mesh->cluster_count : 0;
}

float nanite_get_visible_triangles(NaniteMesh* mesh) {
    return mesh ? (float)(mesh->index_count / 3) : 0.0f;
}

void nanite_render(NaniteRenderData* data, Mat4* view_proj) { /* Stub */ }

// =============================================================================
// LUMEN STUBS
// =============================================================================

struct LumenScene {
    LumenConfig config;
    Color sky_color;
    float sky_intensity;
};

LumenScene* lumen_scene_create(LumenConfig* config) {
    LumenScene* scene = calloc(1, sizeof(LumenScene));
    if (config) scene->config = *config;
    scene->sky_color = (Color){0.5f, 0.7f, 1.0f, 1.0f};
    scene->sky_intensity = 1.0f;
    return scene;
}

void lumen_scene_destroy(LumenScene* scene) { free(scene); }
void lumen_scene_update(LumenScene* scene, float dt) { /* Stub */ }

LumenSample lumen_trace_indirect(LumenScene* scene, Vec3 pos, Vec3 normal) {
    LumenSample sample = {
        .position = pos,
        .normal = normal,
        .indirect_light = {0.1f, 0.1f, 0.15f, 1.0f},
        .ambient_occlusion = 0.8f
    };
    return sample;
}

void lumen_invalidate_region(LumenScene* scene, AABB region) { /* Stub */ }

void lumen_set_sky_light(LumenScene* scene, Color color, float intensity) {
    if (scene) {
        scene->sky_color = color;
        scene->sky_intensity = intensity;
    }
}

// =============================================================================
// VIRTUAL SHADOW MAP STUBS
// =============================================================================

struct VirtualShadowMap {
    VSMConfig config;
};

VirtualShadowMap* vsm_create(VSMConfig* config) {
    VirtualShadowMap* vsm = calloc(1, sizeof(VirtualShadowMap));
    if (config) vsm->config = *config;
    return vsm;
}

void vsm_destroy(VirtualShadowMap* vsm) { free(vsm); }
void vsm_update_light(VirtualShadowMap* vsm, uint32_t id, Mat4* mat) { /* Stub */ }
void vsm_render_shadow_casters(VirtualShadowMap* vsm) { /* Stub */ }

float vsm_sample(VirtualShadowMap* vsm, uint32_t light_id, Vec3 world_pos) {
    return 1.0f; // Fully lit by default
}

// =============================================================================
// TSR STUBS
// =============================================================================

struct TSRState {
    TSRConfig config;
};

TSRState* tsr_create(TSRConfig* config) {
    TSRState* state = calloc(1, sizeof(TSRState));
    if (config) state->config = *config;
    return state;
}

void tsr_destroy(TSRState* state) { free(state); }
void tsr_upscale(TSRState* s, Handle a, Handle b, Handle c, Handle d) { /* Stub */ }
void tsr_reset_history(TSRState* state) { /* Stub */ }

// =============================================================================
// WORLD PARTITION STUBS
// =============================================================================

struct WorldPartition {
    WorldPartitionConfig config;
    Vec3 viewer_pos;
};

WorldPartition* world_partition_create(WorldPartitionConfig* config) {
    WorldPartition* wp = calloc(1, sizeof(WorldPartition));
    if (config) wp->config = *config;
    return wp;
}

void world_partition_destroy(WorldPartition* wp) { free(wp); }

void world_partition_update(WorldPartition* wp, Vec3 viewer_position) {
    if (wp) wp->viewer_pos = viewer_position;
}

bool world_partition_is_cell_loaded(WorldPartition* wp, int x, int y, int z) {
    if (!wp) return false;
    float cell_size = wp->config.cell_size > 0 ? wp->config.cell_size : 5000.0f;
    float cell_center_x = x * cell_size;
    float cell_center_z = z * cell_size;
    float dx = cell_center_x - wp->viewer_pos.x;
    float dz = cell_center_z - wp->viewer_pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    return dist < cell_size * wp->config.loading_range;
}

StreamingLevel* world_partition_get_cell_level(WorldPartition* wp, int x, int y, int z) {
    return NULL; // Stub
}

void world_partition_add_data_layer(WorldPartition* wp, DataLayer* layer) { /* Stub */ }

struct HLOD { uint32_t level_count; };

HLOD* hlod_create(float* lod_distances, uint32_t level_count) {
    HLOD* hlod = calloc(1, sizeof(HLOD));
    hlod->level_count = level_count;
    return hlod;
}

void hlod_build_from_actors(HLOD* hlod, Entity* actors, uint32_t count) { /* Stub */ }
void hlod_destroy(HLOD* hlod) { free(hlod); }

// =============================================================================
// CHAOS STUBS
// =============================================================================

struct DestructibleMesh {
    uint32_t chunk_count;
};

DestructibleMesh* chaos_destructible_create(void* mesh, ChaosDestructionConfig* config) {
    DestructibleMesh* dm = calloc(1, sizeof(DestructibleMesh));
    dm->chunk_count = 1;
    return dm;
}

void chaos_destructible_destroy(DestructibleMesh* dm) { free(dm); }

void chaos_destructible_apply_damage(DestructibleMesh* dm, Vec3 loc, float radius, float damage) {
    if (dm && damage > 50.0f) {
        dm->chunk_count += (uint32_t)(damage / 50.0f);
    }
}

uint32_t chaos_destructible_get_chunk_count(DestructibleMesh* dm) {
    return dm ? dm->chunk_count : 0;
}

struct ChaosCloth { ChaosClothConfig config; };

ChaosCloth* chaos_cloth_create(void* mesh, ChaosClothConfig* config) {
    ChaosCloth* cloth = calloc(1, sizeof(ChaosCloth));
    if (config) cloth->config = *config;
    return cloth;
}

void chaos_cloth_destroy(ChaosCloth* cloth) { free(cloth); }
void chaos_cloth_simulate(ChaosCloth* cloth, float dt) { /* Stub */ }
void chaos_cloth_apply_wind(ChaosCloth* cloth, Vec3 dir, float strength) { /* Stub */ }

struct ChaosVehicle { ChaosVehicleConfig config; float speed; };

ChaosVehicle* chaos_vehicle_create(ChaosVehicleConfig* config) {
    ChaosVehicle* v = calloc(1, sizeof(ChaosVehicle));
    if (config) v->config = *config;
    return v;
}

void chaos_vehicle_destroy(ChaosVehicle* v) { free(v); }
void chaos_vehicle_set_throttle(ChaosVehicle* v, float t) { if (v) v->speed += t * 10; }
void chaos_vehicle_set_steering(ChaosVehicle* v, float s) { /* Stub */ }
void chaos_vehicle_set_brake(ChaosVehicle* v, float b) { if (v && b > 0) v->speed = 0; }
float chaos_vehicle_get_speed_kmh(ChaosVehicle* v) { return v ? v->speed : 0; }

// =============================================================================
// NIAGARA STUBS
// =============================================================================

struct NiagaraEmitter { NiagaraEmitterConfig config; };
struct NiagaraSystem { 
    char name[64]; 
    uint32_t particle_count; 
    NiagaraEmitter* emitters[16];
    uint32_t emitter_count;
};

NiagaraSystem* niagara_system_create(const char* name) {
    NiagaraSystem* sys = calloc(1, sizeof(NiagaraSystem));
    if (name) strncpy(sys->name, name, 63);
    return sys;
}

void niagara_system_destroy(NiagaraSystem* sys) { 
    if (sys) {
        for (uint32_t i = 0; i < sys->emitter_count; i++) {
            free(sys->emitters[i]);
        }
        free(sys); 
    }
}

NiagaraEmitter* niagara_add_emitter(NiagaraSystem* sys, NiagaraEmitterConfig* config) {
    if (!sys || sys->emitter_count >= 16) return NULL;
    NiagaraEmitter* emitter = calloc(1, sizeof(NiagaraEmitter));
    if (config) emitter->config = *config;
    sys->emitters[sys->emitter_count++] = emitter;
    return emitter;
}

void niagara_emitter_add_module(NiagaraEmitter* e, NiagaraModule* m) { /* Stub */ }

void niagara_system_activate(NiagaraSystem* sys, Vec3 position) {
    if (sys) sys->particle_count = 50; // Initial burst
}

void niagara_system_update(NiagaraSystem* sys, float dt) { 
    if (sys && sys->particle_count < 1000) sys->particle_count += 10;
}

uint32_t niagara_get_particle_count(NiagaraSystem* sys) {
    return sys ? sys->particle_count : 0;
}

struct NiagaraDataInterface { int type; };

NiagaraDataInterface* niagara_di_create_mesh_sample(void* mesh) {
    return calloc(1, sizeof(NiagaraDataInterface));
}

NiagaraDataInterface* niagara_di_create_collision_query(void* world) {
    return calloc(1, sizeof(NiagaraDataInterface));
}

NiagaraDataInterface* niagara_di_create_audio_spectrum(void) {
    return calloc(1, sizeof(NiagaraDataInterface));
}

// =============================================================================
// GAS STUBS
// =============================================================================

#define MAX_ATTRIBUTES 32
#define MAX_TAGS 64

struct AbilitySystemComponent {
    Entity owner;
    AttributeDef attributes[MAX_ATTRIBUTES];
    uint32_t attribute_count;
    GameplayTag tags[MAX_TAGS];
    uint32_t tag_count;
    Handle next_handle;
};

AbilitySystemComponent* gas_create(Entity owner) {
    AbilitySystemComponent* asc = calloc(1, sizeof(AbilitySystemComponent));
    asc->owner = owner;
    asc->next_handle = 1;
    return asc;
}

void gas_destroy(AbilitySystemComponent* asc) { free(asc); }

Handle gas_give_ability(AbilitySystemComponent* asc, GameplayAbility* ability) {
    return asc ? asc->next_handle++ : 0;
}

bool gas_try_activate_ability(AbilitySystemComponent* asc, Handle h) { return h != 0; }
void gas_cancel_ability(AbilitySystemComponent* asc, Handle h) { /* Stub */ }
bool gas_can_activate_ability(AbilitySystemComponent* asc, Handle h) { return h != 0; }

Handle gas_apply_effect(AbilitySystemComponent* target, GameplayEffect* effect,
                        AbilitySystemComponent* source, float level) {
    return target ? target->next_handle++ : 0;
}

void gas_remove_effect(AbilitySystemComponent* asc, Handle h) { /* Stub */ }
bool gas_has_effect(AbilitySystemComponent* asc, Handle h) { return h != 0 && h < 100; }

void gas_init_attribute(AbilitySystemComponent* asc, AttributeDef* attr) {
    if (asc && asc->attribute_count < MAX_ATTRIBUTES) {
        asc->attributes[asc->attribute_count++] = *attr;
    }
}

float gas_get_attribute(AbilitySystemComponent* asc, const char* name) {
    if (!asc) return 0.0f;
    for (uint32_t i = 0; i < asc->attribute_count; i++) {
        if (strcmp(asc->attributes[i].name, name) == 0) {
            return asc->attributes[i].current_value;
        }
    }
    return 0.0f;
}

void gas_set_attribute(AbilitySystemComponent* asc, const char* name, float value) {
    if (!asc) return;
    for (uint32_t i = 0; i < asc->attribute_count; i++) {
        if (strcmp(asc->attributes[i].name, name) == 0) {
            asc->attributes[i].current_value = value;
            return;
        }
    }
}

void gas_add_tag(AbilitySystemComponent* asc, GameplayTag tag) {
    if (asc && asc->tag_count < MAX_TAGS) {
        asc->tags[asc->tag_count++] = tag;
    }
}

void gas_remove_tag(AbilitySystemComponent* asc, GameplayTag tag) {
    if (!asc) return;
    for (uint32_t i = 0; i < asc->tag_count; i++) {
        if (asc->tags[i].hash == tag.hash) {
            asc->tags[i] = asc->tags[--asc->tag_count];
            return;
        }
    }
}

bool gas_has_tag(AbilitySystemComponent* asc, GameplayTag tag) {
    if (!asc) return false;
    for (uint32_t i = 0; i < asc->tag_count; i++) {
        if (asc->tags[i].hash == tag.hash) return true;
    }
    return false;
}

bool gas_has_any_tag(AbilitySystemComponent* asc, GameplayTag* tags, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (gas_has_tag(asc, tags[i])) return true;
    }
    return false;
}

bool gas_has_all_tags(AbilitySystemComponent* asc, GameplayTag* tags, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (!gas_has_tag(asc, tags[i])) return false;
    }
    return true;
}

// =============================================================================
// REMAINING STUBS (minimal implementations)
// =============================================================================

// Water
struct WaterBody { WaterBodyConfig config; Vec3* spline; uint32_t spline_count; };
WaterBody* water_body_create(WaterBodyConfig* config) {
    WaterBody* body = calloc(1, sizeof(WaterBody));
    if (config) body->config = *config;
    return body;
}
void water_body_destroy(WaterBody* body) { if (body) { free(body->spline); free(body); } }
void water_body_set_spline(WaterBody* body, Vec3* points, uint32_t count) { /* Stub */ }
float water_body_get_height_at(WaterBody* body, Vec3 pos) {
    return body ? sinf(pos.x * 0.1f) * body->config.wave_amplitude : 0;
}
Vec3 water_body_get_velocity_at(WaterBody* body, Vec3 pos) { return (Vec3){1,0,0}; }
bool water_body_is_underwater(WaterBody* body, Vec3 pos) { return pos.y < 0; }
void water_body_apply_force(WaterBody* body, Vec3 pos, Vec3 force) { /* Stub */ }

// Landscape
struct Landscape { LandscapeConfig config; float* heightmap; };
struct LandscapeLayer { char name[64]; };
Landscape* landscape_create(LandscapeConfig* config) {
    Landscape* l = calloc(1, sizeof(Landscape));
    if (config) l->config = *config;
    return l;
}
void landscape_destroy(Landscape* l) { if (l) { free(l->heightmap); free(l); } }
void landscape_import_heightmap(Landscape* l, float* data, uint32_t w, uint32_t h) { /* Stub */ }
float landscape_get_height_at(Landscape* l, float x, float y) { return 0; }
Vec3 landscape_get_normal_at(Landscape* l, float x, float y) { return (Vec3){0,1,0}; }
LandscapeLayer* landscape_add_layer(Landscape* l, const char* name) { 
    return calloc(1, sizeof(LandscapeLayer)); 
}
void landscape_paint_layer(Landscape* l, LandscapeLayer* layer, float x, float y, float r, float s) {}
uint8_t landscape_get_layer_weight(Landscape* l, LandscapeLayer* layer, float x, float y) { return 128; }

// Foliage
struct FoliageType { FoliageTypeConfig config; };
struct FoliageActor { uint32_t instance_count; };
FoliageType* foliage_type_create(FoliageTypeConfig* config) { 
    FoliageType* t = calloc(1, sizeof(FoliageType));
    if (config) t->config = *config;
    return t;
}
void foliage_type_destroy(FoliageType* type) { free(type); }
FoliageActor* foliage_actor_create(void) { return calloc(1, sizeof(FoliageActor)); }
void foliage_actor_destroy(FoliageActor* actor) { free(actor); }
void foliage_actor_add_instances(FoliageActor* actor, FoliageType* t, Transform* tr, uint32_t count) {
    if (actor) actor->instance_count += count;
}
uint32_t foliage_actor_get_instance_count(FoliageActor* actor) { 
    return actor ? actor->instance_count : 0; 
}
void foliage_actor_remove_instances_in_box(FoliageActor* actor, AABB box) {
    if (actor && actor->instance_count > 10) actor->instance_count -= 10;
}

// PCG
struct PCGGraph { char name[64]; };
struct PCGNode { char type[64]; };
struct PCGData { Vec3* points; uint32_t point_count; };
PCGGraph* pcg_graph_create(const char* name) {
    PCGGraph* g = calloc(1, sizeof(PCGGraph));
    if (name) strncpy(g->name, name, 63);
    return g;
}
void pcg_graph_destroy(PCGGraph* graph) { free(graph); }
PCGNode* pcg_add_node(PCGGraph* graph, const char* type) { 
    return calloc(1, sizeof(PCGNode)); 
}
void pcg_connect(PCGNode* from, const char* output, PCGNode* to, const char* input) {}
PCGData* pcg_execute(PCGGraph* graph, AABB bounds, uint32_t seed) {
    PCGData* data = calloc(1, sizeof(PCGData));
    data->point_count = 100;
    data->points = calloc(100, sizeof(Vec3));
    for (uint32_t i = 0; i < 100; i++) {
        data->points[i].x = bounds.min.x + (float)(i % 10) * 100.0f;
        data->points[i].z = bounds.min.z + (float)(i / 10) * 100.0f;
    }
    return data;
}
void pcg_data_destroy(PCGData* data) { if (data) { free(data->points); free(data); } }
uint32_t pcg_data_get_point_count(PCGData* data) { return data ? data->point_count : 0; }
Vec3 pcg_data_get_point(PCGData* data, uint32_t index) {
    return (data && index < data->point_count) ? data->points[index] : (Vec3){0,0,0};
}

// Mass AI, State Tree, Smart Objects, MetaSounds, Sequences, Movie Render, Input, Control Rig
// (Minimal stubs - actual implementations would be more complex)

struct MassEntityManager { uint32_t entity_count; };
MassEntityManager* mass_create(MassConfig* config) { return calloc(1, sizeof(MassEntityManager)); }
void mass_destroy(MassEntityManager* m) { free(m); }
void mass_register_fragment(MassEntityManager* m, MassFragment* f, size_t s) {}
void mass_register_processor(MassEntityManager* m, MassProcessor* p) {}
Handle mass_spawn_entity(MassEntityManager* m, void* t) { 
    if (m) m->entity_count++; 
    return m ? m->entity_count : 0; 
}
void mass_destroy_entity(MassEntityManager* m, Handle h) { if (m && m->entity_count > 0) m->entity_count--; }
void mass_tick(MassEntityManager* m, float dt) {}
uint32_t mass_get_entity_count(MassEntityManager* m) { return m ? m->entity_count : 0; }

struct StateTree { char name[64]; char current_state[64]; };
struct StateTreeState { char name[64]; };
StateTree* state_tree_create(const char* name) {
    StateTree* t = calloc(1, sizeof(StateTree));
    if (name) strncpy(t->name, name, 63);
    return t;
}
void state_tree_destroy(StateTree* tree) { free(tree); }
StateTreeState* state_tree_add_state(StateTree* t, const char* n, StateTreeState* p) {
    StateTreeState* s = calloc(1, sizeof(StateTreeState));
    if (n) strncpy(s->name, n, 63);
    if (t && t->current_state[0] == 0) strncpy(t->current_state, n, 63);
    return s;
}
void state_tree_add_task(StateTreeState* s, StateTreeTask* t) {}
void state_tree_add_transition(StateTree* t, StateTreeState* f, StateTreeState* to, const char* c) {}
void state_tree_start(StateTree* tree, void* context) {}
void state_tree_tick(StateTree* tree, float dt) {}
const char* state_tree_get_current_state(StateTree* tree) { 
    return tree ? tree->current_state : NULL; 
}

struct SmartObjectSubsystem { int count; };
SmartObjectSubsystem* smart_object_subsystem_create(void) { return calloc(1, sizeof(SmartObjectSubsystem)); }
void smart_object_subsystem_destroy(SmartObjectSubsystem* ss) { free(ss); }
Handle smart_object_register(SmartObjectSubsystem* ss, SmartObjectDefinition* def, Transform t) {
    return ss ? ++ss->count : 0;
}
void smart_object_unregister(SmartObjectSubsystem* ss, Handle handle) {}
SmartObjectSlot* smart_object_find_slot(SmartObjectSubsystem* ss, Vec3 loc, float r, const char* tag) {
    return NULL; // Stub - would return actual slot
}
bool smart_object_claim_slot(SmartObjectSlot* slot, Entity claimer) { return slot != NULL; }
void smart_object_release_slot(SmartObjectSlot* slot, Entity claimer) {}

struct MetaSoundGraph { char name[64]; };
struct MetaSoundNode { char type[64]; };
struct MetaSoundSource { Vec3 position; };
MetaSoundGraph* metasound_graph_create(const char* name) {
    MetaSoundGraph* g = calloc(1, sizeof(MetaSoundGraph));
    if (name) strncpy(g->name, name, 63);
    return g;
}
void metasound_graph_destroy(MetaSoundGraph* graph) { free(graph); }
MetaSoundNode* metasound_add_node(MetaSoundGraph* g, const char* type) { return calloc(1, sizeof(MetaSoundNode)); }
void metasound_connect(MetaSoundNode* from, const char* op, MetaSoundNode* to, const char* ip) {}
MetaSoundSource* metasound_instantiate(MetaSoundGraph* graph, Vec3 position) {
    MetaSoundSource* s = calloc(1, sizeof(MetaSoundSource));
    s->position = position;
    return s;
}
void metasound_set_parameter(MetaSoundSource* source, const char* param, float value) {}
void metasound_trigger(MetaSoundSource* source, const char* trigger_name) {}

struct LevelSequence { char name[64]; float duration; };
struct MovieSceneTrack { char type[64]; };
struct MovieSceneSection { float start, end; };
struct SequencePlayer { LevelSequence* seq; float time; bool playing; };
LevelSequence* level_sequence_create(const char* name, float duration) {
    LevelSequence* s = calloc(1, sizeof(LevelSequence));
    if (name) strncpy(s->name, name, 63);
    s->duration = duration;
    return s;
}
void level_sequence_destroy(LevelSequence* seq) { free(seq); }
MovieSceneTrack* level_sequence_add_track(LevelSequence* seq, const char* type) {
    return calloc(1, sizeof(MovieSceneTrack));
}
MovieSceneSection* movie_scene_add_section(MovieSceneTrack* track, float start, float end) {
    MovieSceneSection* s = calloc(1, sizeof(MovieSceneSection));
    s->start = start;
    s->end = end;
    return s;
}
void movie_scene_section_add_key(MovieSceneSection* section, float time, void* value) {}
SequencePlayer* sequence_player_create(LevelSequence* sequence) {
    SequencePlayer* p = calloc(1, sizeof(SequencePlayer));
    p->seq = sequence;
    return p;
}
void sequence_player_destroy(SequencePlayer* player) { free(player); }
void sequence_player_play(SequencePlayer* player) { if (player) player->playing = true; }
void sequence_player_pause(SequencePlayer* player) { if (player) player->playing = false; }
void sequence_player_stop(SequencePlayer* player) { if (player) { player->playing = false; player->time = 0; } }
void sequence_player_set_time(SequencePlayer* player, float time) { if (player) player->time = time; }
float sequence_player_get_current_time(SequencePlayer* player) { return player ? player->time : 0; }
bool sequence_player_is_playing(SequencePlayer* player) { return player ? player->playing : false; }

struct MoviePipeline { MovieRenderSettings settings; float progress; bool running; };
MoviePipeline* movie_pipeline_create(MovieRenderSettings* settings) {
    MoviePipeline* p = calloc(1, sizeof(MoviePipeline));
    if (settings) p->settings = *settings;
    return p;
}
void movie_pipeline_destroy(MoviePipeline* pipeline) { free(pipeline); }
void movie_pipeline_set_sequence(MoviePipeline* pipeline, void* level_sequence) {}
void movie_pipeline_start_render(MoviePipeline* pipeline) { if (pipeline) pipeline->running = true; }
float movie_pipeline_get_progress(MoviePipeline* pipeline) { return pipeline ? pipeline->progress : 0; }
bool movie_pipeline_is_finished(MoviePipeline* pipeline) { return pipeline && pipeline->progress >= 1.0f; }
void movie_pipeline_abort(MoviePipeline* pipeline) { if (pipeline) pipeline->running = false; }

struct InputAction { char name[64]; };
struct InputMappingContext { char name[64]; int priority; };
InputAction* input_action_create(const char* name) {
    InputAction* a = calloc(1, sizeof(InputAction));
    if (name) strncpy(a->name, name, 63);
    return a;
}
void input_action_destroy(InputAction* action) { free(action); }
void input_action_add_trigger(InputAction* action, InputTrigger* trigger) {}
void input_action_add_modifier(InputAction* action, InputModifier* modifier) {}
InputMappingContext* input_context_create(const char* name) { return calloc(1, sizeof(InputMappingContext)); }
void input_context_destroy(InputMappingContext* ctx) { free(ctx); }
void input_context_bind_action(InputMappingContext* ctx, InputAction* action, uint32_t key) {}
void input_context_set_priority(InputMappingContext* ctx, int priority) { if (ctx) ctx->priority = priority; }
void input_bind_callback(InputAction* action, InputActionCallback callback) {}

struct ControlRig { void* skeleton; };
struct FBIKSolver { void* skeleton; float stiffness; float damping; };
ControlRig* control_rig_create(void* skeleton) { return calloc(1, sizeof(ControlRig)); }
void control_rig_destroy(ControlRig* rig) { free(rig); }
void control_rig_add_effector(ControlRig* rig, IKEffector* effector) {}
void control_rig_solve(ControlRig* rig, float dt) {}
void control_rig_get_pose(ControlRig* rig, Transform* out_pose, uint32_t bone_count) {}
FBIKSolver* fbik_create(void* skeleton) { return calloc(1, sizeof(FBIKSolver)); }
void fbik_destroy(FBIKSolver* solver) { free(solver); }
void fbik_set_root_settings(FBIKSolver* solver, float stiffness, float damping) {
    if (solver) { solver->stiffness = stiffness; solver->damping = damping; }
}
void fbik_solve(FBIKSolver* solver, IKEffector* effectors, uint32_t count) {}
