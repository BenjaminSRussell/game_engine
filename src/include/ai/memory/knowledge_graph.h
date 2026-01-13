#ifndef KNOWLEDGE_GRAPH_H
#define KNOWLEDGE_GRAPH_H

#include "engine/include/core/logger.h"
#include <core/types.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Knowledge Graph System - AI knowledge representation and inference

// Forward declarations
typedef struct KnowledgeGraph KnowledgeGraph;
typedef struct KnowledgeNode KnowledgeNode;
typedef struct KnowledgeRelation KnowledgeRelation;
typedef struct KnowledgeQuery KnowledgeQuery;
typedef struct KnowledgeResult KnowledgeResult;

// Node management

// Relation types between entities
typedef enum {
  RELATION_IS_A,        // Entity is a type of another entity
  RELATION_HAS_A,       // Entity has another entity
  RELATION_LIKES,       // Entity likes another entity
  RELATION_HATES,       // Entity hates another entity
  RELATION_AT_LOCATION, // Entity is at a location
  RELATION_OWNS,        // Entity owns another entity
  RELATION_PART_OF,     // Entity is part of another entity
  RELATION_NEAR,        // Entity is near another entity
  RELATION_AVOIDS,      // Entity avoids another entity
  RELATION_PROTECTS,    // Entity protects another entity
  RELATION_FOLLOWS,     // Entity follows another entity
  RELATION_ATTACKS,     // Entity attacks another entity
  RELATION_HELPS,       // Entity helps another entity
  RELATION_KNOWS,       // Entity knows about another entity
  RELATION_SEES,        // Entity sees another entity
  RELATION_HEARS,       // Entity hears another entity
  RELATION_WANTS,       // Entity wants something
  RELATION_NEEDS,       // Entity needs something
  RELATION_COUNT
} RelationType;

// Knowledge node structure
struct KnowledgeNode {
  EntityID entity_id;           // Unique entity identifier
  char name[128];               // Entity name
  char description[256];        // Entity description
  Vec3 last_known_position;     // Last known position
  f64 last_seen_time;           // When entity was last seen
  f32 certainty;                // Certainty level (0.0 - 1.0)
  u32 relation_count;           // Number of relations
  KnowledgeRelation *relations; // Array of relations
  bool is_active;               // Whether entity is currently active
  u32 access_count;             // How often this node is accessed
  f64 creation_time;            // When this knowledge was created
};

// Knowledge relation between entities
struct KnowledgeRelation {
  EntityID subject_id;        // Subject entity ID
  EntityID object_id;         // Object entity ID
  RelationType relation_type; // Type of relation
  f32 certainty;              // Certainty level (0.0 - 1.0)
  f64 creation_time;          // When relation was established
  f64 last_confirmed_time;    // When relation was last confirmed
  u32 confirmation_count;     // How many times this relation was confirmed
  bool is_active;             // Whether relation is currently active
  f32 decay_rate;             // How fast certainty decays over time
};

// Knowledge query structure
struct KnowledgeQuery {
  char query_text[512];       // Natural language query
  EntityID subject_entity;    // Subject entity (0 if unknown)
  RelationType relation_type; // Relation type to search for
  EntityID object_entity;     // Object entity (0 if unknown)
  Vec3 location_hint;         // Location hint for spatial queries
  f32 min_certainty;          // Minimum certainty threshold
  bool include_inactive;      // Include inactive relations
  u32 max_results;            // Maximum number of results
  f64 time_window;            // Time window for temporal queries
};

// Knowledge query result
struct KnowledgeResult {
  KnowledgeNode *nodes;         // Array of matching nodes
  u32 node_count;               // Number of nodes
  KnowledgeRelation *relations; // Array of matching relations
  u32 relation_count;           // Number of relations
  f32 confidence;               // Overall confidence in result
  char explanation[512];        // Explanation of how result was derived
  f64 processing_time;          // Time taken to process query
};

// Knowledge graph configuration
typedef struct {
  u32 max_nodes;              // Maximum number of nodes
  u32 max_relations_per_node; // Maximum relations per node
  f32 default_certainty;      // Default certainty for new knowledge
  f32 decay_rate;             // Global decay rate for old knowledge
  f64 memory_duration;        // How long to remember facts (seconds)
  bool enable_inference;      // Enable automatic inference
  bool enable_learning;       // Enable learning from observations
  bool enable_persistence;    // Enable saving/loading knowledge
} KnowledgeGraphConfig;

// Main Knowledge Graph API
KnowledgeGraph *knowledge_graph_create(const KnowledgeGraphConfig *config);
void knowledge_graph_destroy(KnowledgeGraph *graph);
bool knowledge_graph_initialize(KnowledgeGraph *graph);
void knowledge_graph_shutdown(KnowledgeGraph *graph);

// Node management
KnowledgeNode *knowledge_graph_add_node(KnowledgeGraph *graph,
                                        EntityID entity_id, const char *name,
                                        const char *description);
bool knowledge_graph_remove_node(KnowledgeGraph *graph, EntityID entity_id);
KnowledgeNode *knowledge_graph_get_node(KnowledgeGraph *graph,
                                        EntityID entity_id);
KnowledgeNode *knowledge_graph_find_node_by_name(KnowledgeGraph *graph,
                                                 const char *name);
void knowledge_graph_update_node_position(KnowledgeGraph *graph,
                                          EntityID entity_id,
                                          const Vec3 *position);
void knowledge_graph_update_node_certainty(KnowledgeGraph *graph,
                                           EntityID entity_id, f32 certainty);

// Relation management
bool knowledge_graph_add_relation(KnowledgeGraph *graph, EntityID subject_id,
                                  EntityID object_id,
                                  RelationType relation_type, f32 certainty);
bool knowledge_graph_remove_relation(KnowledgeGraph *graph, EntityID subject_id,
                                     EntityID object_id,
                                     RelationType relation_type);
KnowledgeRelation *knowledge_graph_get_relation(KnowledgeGraph *graph,
                                                EntityID subject_id,
                                                EntityID object_id,
                                                RelationType relation_type);
void knowledge_graph_update_relation_certainty(KnowledgeGraph *graph,
                                               EntityID subject_id,
                                               EntityID object_id,
                                               RelationType relation_type,
                                               f32 certainty);

// Knowledge inference
KnowledgeResult knowledge_graph_query(KnowledgeGraph *graph,
                                      const KnowledgeQuery *query);
KnowledgeResult knowledge_graph_find_path(KnowledgeGraph *graph,
                                          EntityID start_entity,
                                          EntityID end_entity, u32 max_depth);
KnowledgeResult knowledge_graph_get_related_entities(KnowledgeGraph *graph,
                                                     EntityID entity_id,
                                                     RelationType relation_type,
                                                     u32 max_depth);
KnowledgeResult knowledge_graph_infer_facts(KnowledgeGraph *graph,
                                            EntityID entity_id,
                                            u32 inference_depth);

// Fact forgetting and decay
void knowledge_graph_decay_certainty(KnowledgeGraph *graph, f64 delta_time);
void knowledge_graph_remove_old_facts(KnowledgeGraph *graph, f64 current_time);
void knowledge_graph_invalidate_fact(KnowledgeGraph *graph, EntityID subject_id,
                                     EntityID object_id,
                                     RelationType relation_type);
void knowledge_graph_confirm_fact(KnowledgeGraph *graph, EntityID subject_id,
                                  EntityID object_id,
                                  RelationType relation_type);

// Learning and observation
void knowledge_graph_observation(KnowledgeGraph *graph, EntityID observer_id,
                                 EntityID observed_id,
                                 RelationType relation_type,
                                 const Vec3 *location, f32 certainty);
void knowledge_graph_learn_from_interaction(KnowledgeGraph *graph,
                                            EntityID entity1, EntityID entity2,
                                            const char *interaction_type,
                                            f32 outcome_certainty);

// Serialization
bool knowledge_graph_save(KnowledgeGraph *graph, const char *filename);
bool knowledge_graph_load(KnowledgeGraph *graph, const char *filename);
void knowledge_graph_export_to_dot(KnowledgeGraph *graph, const char *filename);

// Utility functions
const char *relation_type_to_string(RelationType relation);
RelationType string_to_relation_type(const char *relation_str);
u32 knowledge_graph_get_node_count(KnowledgeGraph *graph);
u32 knowledge_graph_get_relation_count(KnowledgeGraph *graph);
void knowledge_graph_print_statistics(KnowledgeGraph *graph);
void knowledge_graph_set_debug_mode(KnowledgeGraph *graph, bool enabled);

// Advanced inference functions
bool knowledge_graph_can_infer(KnowledgeGraph *graph, EntityID subject_id,
                               EntityID object_id, RelationType relation_type);
KnowledgeResult knowledge_graph_explain_reasoning(KnowledgeGraph *graph,
                                                  const KnowledgeQuery *query);
void knowledge_graph_propagate_changes(KnowledgeGraph *graph,
                                       EntityID changed_entity);

// Memory management helpers
void knowledge_node_destroy(KnowledgeNode *node);
void knowledge_relation_destroy(KnowledgeRelation *relation);
KnowledgeNode *knowledge_node_clone(const KnowledgeNode *node);
KnowledgeRelation *knowledge_relation_clone(const KnowledgeRelation *relation);

#ifdef __cplusplus
}
#endif

#endif // KNOWLEDGE_GRAPH_H
