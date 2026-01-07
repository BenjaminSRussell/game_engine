#include "ai/memory/knowledge_graph.h"
#include <core/memory.h>
#include <core/threading/mutex.h>
#include <core/time_system.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ✅ COMPLETED: Define Knowledge Node structure with EntityID, RelationType, and certainty
// ✅ COMPLETED: Implement Knowledge Graph Inference for query processing and fact propagation
// ✅ COMPLETED: Implement Forgotten Facts with decay and event invalidation

// Internal structures
struct KnowledgeGraph {
    KnowledgeGraphConfig config;
    
    // Node storage
    KnowledgeNode* nodes;
    u32 node_count;
    u32 node_capacity;
    EntityID next_entity_id;
    
    // Relation storage
    KnowledgeRelation* relations;
    u32 relation_count;
    u32 relation_capacity;
    
    // Indexing for fast lookup
    struct {
        EntityID* entity_to_node_index;
        u32 capacity;
    } node_index;
    
    struct {
        // Hash table for relation lookup: subject_id * relation_type + object_id
        u32* relation_hash_table;
        u32 hash_capacity;
    } relation_index;
    
    // Inference cache
    struct {
        KnowledgeQuery* queries;
        KnowledgeResult* results;
        u32 count;
        u32 capacity;
        f64 cache_duration;
    } inference_cache;
    
    // Timing and decay
    f64 current_time;
    f64 last_decay_time;
    
    // Thread safety
    Mutex* graph_mutex;
    
    // Debug and statistics
    bool debug_mode;
    u32 queries_processed;
    u32 inferences_made;
    f64 total_query_time;
};

// Hash function for relation indexing
static u32 hash_relation(EntityID subject, RelationType relation, EntityID object) {
    u64 combined = ((u64)subject << 32) | ((u64)relation << 16) | (u64)object;
    return (u32)(combined ^ (combined >> 32));
}

// Main Knowledge Graph implementation
KnowledgeGraph* knowledge_graph_create(const KnowledgeGraphConfig* config) {
    KnowledgeGraph* graph = MALLOC_AI(sizeof(KnowledgeGraph));
    if (!graph) return NULL;
    
    memset(graph, 0, sizeof(KnowledgeGraph));
    
    if (config) {
        graph->config = *config;
    } else {
        // Default configuration
        graph->config.max_nodes = 10000;
        graph->config.max_relations_per_node = 50;
        graph->config.default_certainty = 0.5f;
        graph->config.decay_rate = 0.001f;
        graph->config.memory_duration = 3600.0; // 1 hour
        graph->config.enable_inference = true;
        graph->config.enable_learning = true;
        graph->config.enable_persistence = true;
    }
    
    // Allocate node storage
    graph->node_capacity = graph->config.max_nodes;
    graph->nodes = MALLOC_AI(sizeof(KnowledgeNode) * graph->node_capacity);
    if (!graph->nodes) {
        memory_free(graph);
        return NULL;
    }
    
    // Allocate relation storage
    graph->relation_capacity = graph->config.max_nodes * graph->config.max_relations_per_node;
    graph->relations = MALLOC_AI(sizeof(KnowledgeRelation) * graph->relation_capacity);
    if (!graph->relations) {
        memory_free(graph->nodes);
        memory_free(graph);
        return NULL;
    }
    
    // Allocate indexing structures
    graph->node_index.capacity = graph->node_capacity * 2;
    graph->node_index.entity_to_node_index = MALLOC_AI(sizeof(EntityID) * graph->node_index.capacity);
    
    graph->relation_index.hash_capacity = graph->relation_capacity * 2;
    graph->relation_index.relation_hash_table = MALLOC_AI(sizeof(u32) * graph->relation_index.hash_capacity);
    
    if (!graph->node_index.entity_to_node_index || !graph->relation_index.relation_hash_table) {
        if (graph->nodes) memory_free(graph->nodes);
        if (graph->relations) memory_free(graph->relations);
        if (graph->node_index.entity_to_node_index) memory_free(graph->node_index.entity_to_node_index);
        if (graph->relation_index.relation_hash_table) memory_free(graph->relation_index.relation_hash_table);
        memory_free(graph);
        return NULL;
    }
    
    // Initialize hash table
    memset(graph->relation_index.relation_hash_table, 0xFF, sizeof(u32) * graph->relation_index.hash_capacity);
    
    // Allocate inference cache
    graph->inference_cache.capacity = 100;
    graph->inference_cache.queries = MALLOC_AI(sizeof(KnowledgeQuery) * graph->inference_cache.capacity);
    graph->inference_cache.results = MALLOC_AI(sizeof(KnowledgeResult) * graph->inference_cache.capacity);
    graph->inference_cache.cache_duration = 30.0; // 30 seconds
    
    graph->graph_mutex = mutex_create(false, "KnowledgeGraph");
    graph->next_entity_id = 1;
    graph->current_time = time_get_high_res_time();
    
    return graph;
}

void knowledge_graph_destroy(KnowledgeGraph* graph) {
    if (!graph) return;
    
    // Cleanup nodes
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].relations) {
            memory_free(graph->nodes[i].relations);
        }
    }
    
    // Cleanup inference cache
    for (u32 i = 0; i < graph->inference_cache.count; i++) {
        if (graph->inference_cache.results[i].nodes) {
            memory_free(graph->inference_cache.results[i].nodes);
        }
        if (graph->inference_cache.results[i].relations) {
            memory_free(graph->inference_cache.results[i].relations);
        }
    }
    
    // Free allocated memory
    if (graph->nodes) memory_free(graph->nodes);
    if (graph->relations) memory_free(graph->relations);
    if (graph->node_index.entity_to_node_index) memory_free(graph->node_index.entity_to_node_index);
    if (graph->relation_index.relation_hash_table) memory_free(graph->relation_index.relation_hash_table);
    if (graph->inference_cache.queries) memory_free(graph->inference_cache.queries);
    if (graph->inference_cache.results) memory_free(graph->inference_cache.results);
    if (graph->graph_mutex) mutex_destroy(graph->graph_mutex);
    
    memory_free(graph);
}

bool knowledge_graph_initialize(KnowledgeGraph* graph) {
    if (!graph) return false;
    
    mutex_lock(graph->graph_mutex);
    
    // Initialize hash tables
    memset(graph->node_index.entity_to_node_index, 0xFF, sizeof(EntityID) * graph->node_index.capacity);
    memset(graph->relation_index.relation_hash_table, 0xFF, sizeof(u32) * graph->relation_index.hash_capacity);
    
    graph->current_time = time_get_high_res_time();
    graph->last_decay_time = graph->current_time;
    
    mutex_unlock(graph->graph_mutex);
    return true;
}

void knowledge_graph_shutdown(KnowledgeGraph* graph) {
    if (!graph) return;
    
    // Save knowledge if persistence is enabled
    if (graph->config.enable_persistence) {
        knowledge_graph_save(graph, "knowledge_graph.dat");
    }
}

// Node management
KnowledgeNode* knowledge_graph_add_node(KnowledgeGraph* graph, EntityID entity_id, 
                                      const char* name, const char* description) {
    if (!graph || !name || graph->node_count >= graph->node_capacity) return NULL;
    
    mutex_lock(graph->graph_mutex);
    
    // Check if node already exists
    if (entity_id != 0 && knowledge_graph_get_node(graph, entity_id)) {
        mutex_unlock(graph->graph_mutex);
        return NULL;
    }
    
    // Generate new entity ID if not provided
    if (entity_id == 0) {
        entity_id = graph->next_entity_id++;
    }
    
    // Create new node
    KnowledgeNode* node = &graph->nodes[graph->node_count];
    memset(node, 0, sizeof(KnowledgeNode));
    
    node->entity_id = entity_id;
    strncpy(node->name, name, sizeof(node->name) - 1);
    if (description) {
        strncpy(node->description, description, sizeof(node->description) - 1);
    }
    node->certainty = graph->config.default_certainty;
    node->creation_time = graph->current_time;
    node->last_seen_time = graph->current_time;
    node->is_active = true;
    
    // Allocate relations array
    node->relations = MALLOC_AI(sizeof(KnowledgeRelation) * graph->config.max_relations_per_node);
    if (!node->relations) {
        mutex_unlock(graph->graph_mutex);
        return NULL;
    }
    
    // Update index
    u32 index = entity_id % graph->node_index.capacity;
    graph->node_index.entity_to_node_index[index] = graph->node_count;
    
    graph->node_count++;
    
    mutex_unlock(graph->graph_mutex);
    return node;
}

KnowledgeNode* knowledge_graph_get_node(KnowledgeGraph* graph, EntityID entity_id) {
    if (!graph || entity_id == 0) return NULL;
    
    mutex_lock(graph->graph_mutex);
    
    u32 index = entity_id % graph->node_index.capacity;
    u32 node_index = graph->node_index.entity_to_node_index[index];
    
    if (node_index == 0xFFFFFFFF || node_index >= graph->node_count) {
        mutex_unlock(graph->graph_mutex);
        return NULL;
    }
    
    KnowledgeNode* node = &graph->nodes[node_index];
    if (node->entity_id != entity_id) {
        // Hash collision - search linearly
        for (u32 i = 0; i < graph->node_count; i++) {
            if (graph->nodes[i].entity_id == entity_id) {
                node = &graph->nodes[i];
                break;
            }
        }
    }
    
    node->access_count++;
    mutex_unlock(graph->graph_mutex);
    return node;
}

// Relation management
bool knowledge_graph_add_relation(KnowledgeGraph* graph, EntityID subject_id, 
                                EntityID object_id, RelationType relation_type, f32 certainty) {
    if (!graph || subject_id == 0 || object_id == 0 || graph->relation_count >= graph->relation_capacity) {
        return false;
    }
    
    mutex_lock(graph->graph_mutex);
    
    // Check if relation already exists
    if (knowledge_graph_get_relation(graph, subject_id, object_id, relation_type)) {
        mutex_unlock(graph->graph_mutex);
        return false;
    }
    
    // Get subject and object nodes
    KnowledgeNode* subject = knowledge_graph_get_node(graph, subject_id);
    KnowledgeNode* object = knowledge_graph_get_node(graph, object_id);
    
    if (!subject || !object) {
        mutex_unlock(graph->graph_mutex);
        return false;
    }
    
    // Create new relation
    KnowledgeRelation* relation = &graph->relations[graph->relation_count];
    relation->subject_id = subject_id;
    relation->object_id = object_id;
    relation->relation_type = relation_type;
    relation->certainty = certainty;
    relation->creation_time = graph->current_time;
    relation->last_confirmed_time = graph->current_time;
    relation->confirmation_count = 1;
    relation->is_active = true;
    relation->decay_rate = graph->config.decay_rate;
    
    // Add to subject node's relations
    if (subject->relation_count < graph->config.max_relations_per_node) {
        subject->relations[subject->relation_count++] = *relation;
    }
    
    // Update hash table
    u32 hash = hash_relation(subject_id, relation_type, object_id);
    u32 index = hash % graph->relation_index.hash_capacity;
    graph->relation_index.relation_hash_table[index] = graph->relation_count;
    
    graph->relation_count++;
    
    // Trigger inference if enabled
    if (graph->config.enable_inference) {
        knowledge_graph_propagate_changes(graph, subject_id);
    }
    
    mutex_unlock(graph->graph_mutex);
    return true;
}

KnowledgeRelation* knowledge_graph_get_relation(KnowledgeGraph* graph, EntityID subject_id, 
                                             EntityID object_id, RelationType relation_type) {
    if (!graph || subject_id == 0 || object_id == 0) return NULL;
    
    mutex_lock(graph->graph_mutex);
    
    u32 hash = hash_relation(subject_id, relation_type, object_id);
    u32 index = hash % graph->relation_index.hash_capacity;
    u32 relation_index = graph->relation_index.relation_hash_table[index];
    
    if (relation_index == 0xFFFFFFFF || relation_index >= graph->relation_count) {
        mutex_unlock(graph->graph_mutex);
        return NULL;
    }
    
    KnowledgeRelation* relation = &graph->relations[relation_index];
    
    // Verify relation matches
    if (relation->subject_id != subject_id || 
        relation->object_id != object_id || 
        relation->relation_type != relation_type) {
        
        // Hash collision - search linearly
        relation = NULL;
        for (u32 i = 0; i < graph->relation_count; i++) {
            KnowledgeRelation* r = &graph->relations[i];
            if (r->subject_id == subject_id && 
                r->object_id == object_id && 
                r->relation_type == relation_type) {
                relation = r;
                break;
            }
        }
    }
    
    mutex_unlock(graph->graph_mutex);
    return relation;
}

// Knowledge inference
KnowledgeResult knowledge_graph_query(KnowledgeGraph* graph, const KnowledgeQuery* query) {
    KnowledgeResult result = {0};
    if (!graph || !query) return result;
    
    f64 start_time = time_get_high_res_time();
    
    mutex_lock(graph->graph_mutex);
    
    // Check inference cache first
    for (u32 i = 0; i < graph->inference_cache.count; i++) {
        if (strcmp(graph->inference_cache.queries[i].query_text, query->query_text) == 0 &&
            (graph->current_time - graph->inference_cache.queries[i].time_window) < graph->inference_cache.cache_duration) {
            
            result = graph->inference_cache.results[i];
            mutex_unlock(graph->graph_mutex);
            return result;
        }
    }
    
    // Process query based on type
    if (query->subject_entity != 0 && query->relation_type != RELATION_COUNT && query->object_entity != 0) {
        // Direct relation query
        KnowledgeRelation* relation = knowledge_graph_get_relation(graph, query->subject_entity, 
                                                              query->object_entity, query->relation_type);
        if (relation && relation->certainty >= query->min_certainty) {
            result.relations = MALLOC_AI(sizeof(KnowledgeRelation));
            if (result.relations) {
                result.relations[0] = *relation;
                result.relation_count = 1;
                result.confidence = relation->certainty;
                strcpy(result.explanation, "Direct relation found in knowledge graph");
            }
        }
    } else if (query->subject_entity != 0) {
        // Entity-based query - find all relations for subject
        KnowledgeNode* node = knowledge_graph_get_node(graph, query->subject_entity);
        if (node) {
            result.relation_count = 0;
            for (u32 i = 0; i < node->relation_count; i++) {
                if (node->relations[i].certainty >= query->min_certainty &&
                    (query->relation_type == RELATION_COUNT || node->relations[i].relation_type == query->relation_type)) {
                    result.relation_count++;
                }
            }
            
            if (result.relation_count > 0) {
                result.relations = MALLOC_AI(sizeof(KnowledgeRelation) * result.relation_count);
                if (result.relations) {
                    u32 index = 0;
                    for (u32 i = 0; i < node->relation_count; i++) {
                        if (node->relations[i].certainty >= query->min_certainty &&
                            (query->relation_type == RELATION_COUNT || node->relations[i].relation_type == query->relation_type)) {
                            result.relations[index++] = node->relations[i];
                        }
                    }
                    result.confidence = 0.8f; // High confidence for entity-based queries
                    strcpy(result.explanation, "Found relations for subject entity");
                }
            }
        }
    } else {
        // Inference-based query - try to infer answer
        result = knowledge_graph_infer_facts(graph, query->subject_entity, 3);
        if (result.relation_count == 0) {
            strcpy(result.explanation, "No matching knowledge found and inference failed");
            result.confidence = 0.1f;
        }
    }
    
    // Cache result
    if (graph->inference_cache.count < graph->inference_cache.capacity) {
        graph->inference_cache.queries[graph->inference_cache.count] = *query;
        graph->inference_cache.results[graph->inference_cache.count] = result;
        graph->inference_cache.count++;
    }
    
    graph->queries_processed++;
    
    mutex_unlock(graph->graph_mutex);
    
    result.processing_time = time_get_high_res_time() - start_time;
    graph->total_query_time += result.processing_time;
    
    return result;
}

KnowledgeResult knowledge_graph_infer_facts(KnowledgeGraph* graph, EntityID entity_id, u32 inference_depth) {
    KnowledgeResult result = {0};
    if (!graph || entity_id == 0 || inference_depth == 0) return result;
    
    KnowledgeNode* entity = knowledge_graph_get_node(graph, entity_id);
    if (!entity) return result;
    
    // Collect all possible inferences
    u32 max_inferences = 100;
    KnowledgeRelation* inferred_relations = MALLOC_AI(sizeof(KnowledgeRelation) * max_inferences);
    if (!inferred_relations) return result;
    
    u32 inference_count = 0;
    
    // Rule-based inference
    for (u32 i = 0; i < entity->relation_count && inference_count < max_inferences; i++) {
        KnowledgeRelation* relation = &entity->relations[i];
        
        // Transitive relations: A -> B -> C implies A -> C
        if (relation->relation_type == RELATION_AT_LOCATION || 
            relation->relation_type == RELATION_PART_OF ||
            relation->relation_type == RELATION_NEAR) {
            
            KnowledgeNode* intermediate = knowledge_graph_get_node(graph, relation->object_id);
            if (intermediate) {
                for (u32 j = 0; j < intermediate->relation_count && inference_count < max_inferences; j++) {
                    KnowledgeRelation* next_relation = &intermediate->relations[j];
                    
                    // Create transitive inference
                    if (next_relation->relation_type == relation->relation_type) {
                        KnowledgeRelation* inferred = &inferred_relations[inference_count++];
                        inferred->subject_id = entity_id;
                        inferred->object_id = next_relation->object_id;
                        inferred->relation_type = relation->relation_type;
                        inferred->certainty = relation->certainty * next_relation->certainty * 0.8f; // Decay for inference
                        inferred->creation_time = graph->current_time;
                        inferred->last_confirmed_time = graph->current_time;
                        inferred->confirmation_count = 0; // Inferred, not confirmed
                        inferred->is_active = true;
                        inferred->decay_rate = graph->config.decay_rate * 2.0f; // Faster decay for inferred facts
                    }
                }
            }
        }
        
        // Contradiction detection and resolution
        if (relation->relation_type == RELATION_LIKES) {
            // If A likes B and B hates A, reduce certainty
            KnowledgeRelation* contradiction = knowledge_graph_get_relation(graph, relation->object_id, 
                                                                   entity_id, RELATION_HATES);
            if (contradiction) {
                relation->certainty *= 0.5f;
                contradiction->certainty *= 0.5f;
            }
        }
    }
    
    // Prepare result
    if (inference_count > 0) {
        result.relations = MALLOC_AI(sizeof(KnowledgeRelation) * inference_count);
        if (result.relations) {
            memcpy(result.relations, inferred_relations, sizeof(KnowledgeRelation) * inference_count);
            result.relation_count = inference_count;
            result.confidence = 0.6f; // Lower confidence for inferred facts
            strcpy(result.explanation, "Facts inferred through transitive relations and contradiction resolution");
        }
        graph->inferences_made += inference_count;
    }
    
    memory_free(inferred_relations);
    return result;
}

// Fact forgetting and decay
void knowledge_graph_decay_certainty(KnowledgeGraph* graph, f64 delta_time) {
    if (!graph || delta_time <= 0.0) return;
    
    mutex_lock(graph->graph_mutex);
    
    f32 decay_factor = expf(-graph->config.decay_rate * (f32)delta_time);
    
    // Decay node certainty
    for (u32 i = 0; i < graph->node_count; i++) {
        KnowledgeNode* node = &graph->nodes[i];
        f64 time_since_seen = graph->current_time - node->last_seen_time;
        
        if (time_since_seen > graph->config.memory_duration) {
            node->certainty *= decay_factor;
            
            // Deactivate very old nodes
            if (node->certainty < 0.1f) {
                node->is_active = false;
            }
        }
    }
    
    // Decay relation certainty
    for (u32 i = 0; i < graph->relation_count; i++) {
        KnowledgeRelation* relation = &graph->relations[i];
        f64 time_since_confirmed = graph->current_time - relation->last_confirmed_time;
        
        if (time_since_confirmed > graph->config.memory_duration) {
            relation->certainty *= decay_factor;
            
            // Deactivate very old relations
            if (relation->certainty < 0.1f) {
                relation->is_active = false;
            }
        }
    }
    
    graph->last_decay_time = graph->current_time;
    
    mutex_unlock(graph->graph_mutex);
}

void knowledge_graph_invalidate_fact(KnowledgeGraph* graph, EntityID subject_id, 
                                   EntityID object_id, RelationType relation_type) {
    if (!graph || subject_id == 0 || object_id == 0) return;
    
    mutex_lock(graph->graph_mutex);
    
    KnowledgeRelation* relation = knowledge_graph_get_relation(graph, subject_id, object_id, relation_type);
    if (relation) {
        relation->certainty = 0.0f;
        relation->is_active = false;
    }
    
    mutex_unlock(graph->graph_mutex);
}

void knowledge_graph_confirm_fact(KnowledgeGraph* graph, EntityID subject_id, 
                               EntityID object_id, RelationType relation_type) {
    if (!graph || subject_id == 0 || object_id == 0) return;
    
    mutex_lock(graph->graph_mutex);
    
    KnowledgeRelation* relation = knowledge_graph_get_relation(graph, subject_id, object_id, relation_type);
    if (relation) {
        relation->certainty = fminf(1.0f, relation->certainty + 0.1f);
        relation->last_confirmed_time = graph->current_time;
        relation->confirmation_count++;
        relation->is_active = true;
    }
    
    mutex_unlock(graph->graph_mutex);
}

// Utility functions
const char* relation_type_to_string(RelationType relation) {
    static const char* names[] = {
        "IS_A", "HAS_A", "LIKES", "HATES", "AT_LOCATION", "OWNS", "PART_OF",
        "NEAR", "AVOIDS", "PROTECTS", "FOLLOWS", "ATTACKS", "HELPS",
        "KNOWS", "SEES", "HEARS", "WANTS", "NEEDS"
    };
    
    if (relation >= 0 && relation < RELATION_COUNT) {
        return names[relation];
    }
    return "UNKNOWN";
}

RelationType string_to_relation_type(const char* relation_str) {
    if (!relation_str) return RELATION_COUNT;
    
    if (strcmp(relation_str, "IS_A") == 0) return RELATION_IS_A;
    if (strcmp(relation_str, "HAS_A") == 0) return RELATION_HAS_A;
    if (strcmp(relation_str, "LIKES") == 0) return RELATION_LIKES;
    if (strcmp(relation_str, "HATES") == 0) return RELATION_HATES;
    if (strcmp(relation_str, "AT_LOCATION") == 0) return RELATION_AT_LOCATION;
    if (strcmp(relation_str, "OWNS") == 0) return RELATION_OWNS;
    if (strcmp(relation_str, "PART_OF") == 0) return RELATION_PART_OF;
    if (strcmp(relation_str, "NEAR") == 0) return RELATION_NEAR;
    if (strcmp(relation_str, "AVOIDS") == 0) return RELATION_AVOIDS;
    if (strcmp(relation_str, "PROTECTS") == 0) return RELATION_PROTECTS;
    if (strcmp(relation_str, "FOLLOWS") == 0) return RELATION_FOLLOWS;
    if (strcmp(relation_str, "ATTACKS") == 0) return RELATION_ATTACKS;
    if (strcmp(relation_str, "HELPS") == 0) return RELATION_HELPS;
    if (strcmp(relation_str, "KNOWS") == 0) return RELATION_KNOWS;
    if (strcmp(relation_str, "SEES") == 0) return RELATION_SEES;
    if (strcmp(relation_str, "HEARS") == 0) return RELATION_HEARS;
    if (strcmp(relation_str, "WANTS") == 0) return RELATION_WANTS;
    if (strcmp(relation_str, "NEEDS") == 0) return RELATION_NEEDS;
    
    return RELATION_COUNT;
}

void knowledge_graph_print_statistics(KnowledgeGraph* graph) {
    if (!graph) return;
    
    mutex_lock(graph->graph_mutex);
    
    u32 active_nodes = 0;
    u32 active_relations = 0;
    f32 avg_certainty = 0.0f;
    
    for (u32 i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].is_active) active_nodes++;
        avg_certainty += graph->nodes[i].certainty;
    }
    
    for (u32 i = 0; i < graph->relation_count; i++) {
        if (graph->relations[i].is_active) active_relations++;
    }
    
    if (graph->node_count > 0) avg_certainty /= graph->node_count;
    
    printf("Knowledge Graph Statistics:\n");
    printf("  Nodes: %u total, %u active\n", graph->node_count, active_nodes);
    printf("  Relations: %u total, %u active\n", graph->relation_count, active_relations);
    printf("  Average Certainty: %.3f\n", avg_certainty);
    printf("  Queries Processed: %u\n", graph->queries_processed);
    printf("  Inferences Made: %u\n", graph->inferences_made);
    printf("  Average Query Time: %.3f ms\n", 
           graph->queries_processed > 0 ? (graph->total_query_time / graph->queries_processed) * 1000.0 : 0.0);
    
    mutex_unlock(graph->graph_mutex);
}
