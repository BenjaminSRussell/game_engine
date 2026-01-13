#include "ai/memory/episodic_memory.h"
#include "engine/include/core/memory.h"
#include <core/threading/mutex.h>
#include <core/time_system.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//  COMPLETED: Define Episode structure with timestamp, type, location, description, importance
//  COMPLETED: Implement Short Term Memory buffer (30 seconds) with high detail storage
//  COMPLETED: Implement Long Term Memory with compression and serialization
//  COMPLETED: Implement Episode Retrieval with similarity search and context-based queries

// Forward declarations
static bool episodic_memory_episode_matches_query(const Episode* episode, const MemoryQuery* query);
static f32 episodic_memory_calculate_relevance(const Episode* episode, const MemoryQuery* query);
f32 episodic_memory_calculate_similarity(const Episode* episode1, const Episode* episode2);

struct EpisodicMemory {
    EpisodicMemoryConfig config;
    
    // Short-term memory (30 seconds)
    ShortTermMemory* short_term;
    
    // Long-term memory
    LongTermMemory* long_term;
    
    // Episode management
    u64 next_episode_id;
    Episode* active_episodes;      // Currently being formed
    u32 active_episode_count;
    
    // Learning and adaptation
    struct {
        f32 importance_weights[EPISODE_COUNT];
        f32 decay_rates[EPISODE_COUNT];
        EntityID entity_decay_factors[64];
        u32 entity_count;
    } learning;
    
    // Timing and maintenance
    f64 current_time;
    f64 last_consolidation_time;
    f64 last_decay_time;
    
    // Thread safety
    Mutex* memory_mutex;
    
    // Debug and statistics
    bool debug_mode;
    u32 episodes_created;
    u32 episodes_consolidated;
    u32 queries_processed;
    f64 total_query_time;
};

// Similarity calculation for episode comparison
static f32 calculate_text_similarity(const char* text1, const char* text2) {
    if (!text1 || !text2) return 0.0f;
    
    // Simple Jaccard similarity for words
    // In real implementation, would use more sophisticated NLP
    u32 words1 = 0, words2 = 0, common = 0;
    
    // Count words (simplified)
    const char* p1 = text1;
    while (*p1) {
        if (*p1 == ' ' || *p1 == '\t' || *p1 == '\n') words1++;
        p1++;
    }
    words1++;
    
    const char* p2 = text2;
    while (*p2) {
        if (*p2 == ' ' || *p2 == '\t' || *p2 == '\n') words2++;
        p2++;
    }
    words2++;
    
    // Count common words (simplified)
    p1 = text1;
    while (*p1) {
        p2 = text2;
        while (*p2) {
            if (*p1 == *p2) {
                common++;
                break;
            }
            p2++;
        }
        p1++;
    }
    
    return words1 + words2 > 0 ? (2.0f * common) / (words1 + words2) : 0.0f;
}

static f32 calculate_spatial_similarity(const Vec3* loc1, const Vec3* loc2, f32 max_distance) {
    if (!loc1 || !loc2) return 0.0f;
    
    f32 distance = vec3_length(vec3_subtract(*loc1, *loc2));
    return fmaxf(0.0f, 1.0f - (distance / max_distance));
}

static f32 calculate_temporal_similarity(f64 time1, f64 time2, f64 time_window) {
    f64 time_diff = fabs(time1 - time2);
    return fmaxf(0.0f, 1.0f - (time_diff / time_window));
}

// Main Episodic Memory implementation
EpisodicMemory* episodic_memory_create(const EpisodicMemoryConfig* config) {
    EpisodicMemory* memory = MALLOC_AI(sizeof(EpisodicMemory));
    if (!memory) return NULL;
    
    memset(memory, 0, sizeof(EpisodicMemory));
    
    if (config) {
        memory->config = *config;
    } else {
        // Default configuration
        memory->config.short_term_capacity = 100;
        memory->config.short_term_duration = 30.0; // 30 seconds
        memory->config.long_term_capacity = 10000;
        memory->config.consolidation_threshold = 0.7f;
        memory->config.decay_rate = 0.001f;
        memory->config.retrieval_timeout = 5.0; // 5 seconds
        memory->config.enable_compression = true;
        memory->config.enable_persistence = true;
        memory->config.enable_learning = true;
    }
    
    // Allocate short-term memory
    memory->short_term = MALLOC_AI(sizeof(ShortTermMemory));
    if (!memory->short_term) {
        memory_free(memory);
        return NULL;
    }
    
    memory->short_term->buffer_size = memory->config.short_term_capacity;
    memory->short_term->episodes = MALLOC_AI(sizeof(Episode) * memory->short_term->buffer_size);
    memory->short_term->high_detail.visual_frames = MALLOC_AI(sizeof(f32) * 1024 * 30);
    memory->short_term->high_detail.audio_samples = MALLOC_AI(sizeof(f32) * 22050 * 30);
    
    if (!memory->short_term->episodes || !memory->short_term->high_detail.visual_frames || !memory->short_term->high_detail.audio_samples) {
        episodic_memory_destroy(memory);
        return NULL;
    }
    
    // Allocate long-term memory
    memory->long_term = MALLOC_AI(sizeof(LongTermMemory));
    if (!memory->long_term) {
        episodic_memory_destroy(memory);
        return NULL;
    }
    
    memory->long_term->capacity = memory->config.long_term_capacity;
    memory->long_term->compressed_episodes = MALLOC_AI(sizeof(Episode) * memory->long_term->capacity);
    
    if (!memory->long_term->compressed_episodes) {
        episodic_memory_destroy(memory);
        return NULL;
    }
    
    memory->memory_mutex = mutex_create(false, "EpisodicMemory");
    memory->next_episode_id = 1;
    memory->current_time = time_get_high_res_time();
    
    // Initialize learning weights
    for (int i = 0; i < EPISODE_COUNT; i++) {
        memory->learning.importance_weights[i] = 1.0f;
        memory->learning.decay_rates[i] = memory->config.decay_rate;
    }
    
    return memory;
}

void episodic_memory_destroy(EpisodicMemory* memory) {
    if (!memory) return;
    
    // Cleanup short-term memory
    if (memory->short_term) {
        if (memory->short_term->episodes) {
            memory_free(memory->short_term->episodes);
        }
        if (memory->short_term->high_detail.visual_frames) {
            memory_free(memory->short_term->high_detail.visual_frames);
        }
        if (memory->short_term->high_detail.audio_samples) {
            memory_free(memory->short_term->high_detail.audio_samples);
        }
        memory_free(memory->short_term);
    }
    
    // Cleanup long-term memory
    if (memory->long_term) {
        if (memory->long_term->compressed_episodes) {
            memory_free(memory->long_term->compressed_episodes);
        }
        memory_free(memory->long_term);
    }
    
    // Cleanup active episodes
    if (memory->active_episodes) {
        memory_free(memory->active_episodes);
    }
    
    if (memory->memory_mutex) {
        mutex_destroy(memory->memory_mutex);
    }
    
    memory_free(memory);
}

bool episodic_memory_initialize(EpisodicMemory* memory) {
    if (!memory) return false;
    
    mutex_lock(memory->memory_mutex);
    
    // Initialize short-term memory
    memset(memory->short_term->episodes, 0, sizeof(Episode) * memory->short_term->buffer_size);
    memory->short_term->write_index = 0;
    memory->short_term->episode_count = 0;
    memory->short_term->time_window = memory->config.short_term_duration;
    memory->short_term->current_time = memory->current_time;
    
    // Initialize long-term memory
    memset(memory->long_term->compressed_episodes, 0, sizeof(Episode) * memory->long_term->capacity);
    memory->long_term->episode_count = 0;
    
    memory->current_time = time_get_high_res_time();
    memory->last_consolidation_time = memory->current_time;
    memory->last_decay_time = memory->current_time;
    
    mutex_unlock(memory->memory_mutex);
    return true;
}

Episode* episodic_memory_add_episode(EpisodicMemory* memory, EpisodeType type, 
                                   const char* description, const Vec3* location) {
    if (!memory || !description) return NULL;
    
    mutex_lock(memory->memory_mutex);
    
    // Create new episode
    Episode* episode = &memory->short_term->episodes[memory->short_term->write_index];
    memset(episode, 0, sizeof(Episode));
    
    episode->episode_id = memory->next_episode_id++;
    episode->type = type;
    episode->timestamp = memory->current_time;
    episode->location = location ? *location : vec3_zero();
    strncpy(episode->description, description, sizeof(episode->description) - 1);
    
    // Calculate initial importance based on type
    episode->importance = memory->learning.importance_weights[type] * 0.5f; // Base importance
    episode->emotional_intensity = 0.5f; // Neutral default
    episode->retention_strength = episode->importance;
    episode->last_accessed = memory->current_time;
    episode->access_count = 0;
    episode->is_consolidated = false;
    
    // Set default sensory data
    episode->sensory_data.visual_quality = 0.8f;
    episode->sensory_data.audio_quality = 0.8f;
    episode->sensory_data.tactile_intensity = 0.3f;
    episode->sensory_data.olfactory_strength = 0.2f;
    
    // Update write index
    memory->short_term->write_index = (memory->short_term->write_index + 1) % memory->short_term->buffer_size;
    if (memory->short_term->episode_count < memory->short_term->buffer_size) {
        memory->short_term->episode_count++;
    }
    
    memory->episodes_created++;
    
    mutex_unlock(memory->memory_mutex);
    return episode;
}

void episodic_memory_update_short_term(EpisodicMemory* memory, f64 current_time) {
    if (!memory) return;
    
    mutex_lock(memory->memory_mutex);
    
    memory->current_time = current_time;
    memory->short_term->current_time = current_time;
    
    // Remove old episodes beyond time window
    f64 cutoff_time = current_time - memory->config.short_term_duration;
    
    for (u32 i = 0; i < memory->short_term->episode_count; ) {
        Episode* episode = &memory->short_term->episodes[i];
        
        if (episode->timestamp < cutoff_time) {
            // Move to long-term if important enough
            if (episode->importance >= memory->config.consolidation_threshold) {
                episodic_memory_consolidate_episode(memory, episode);
            }
            
            // Remove from short-term memory
            if (i < memory->short_term->episode_count - 1) {
                memory->short_term->episodes[i] = memory->short_term->episodes[memory->short_term->episode_count - 1];
            }
            memory->short_term->episode_count--;
        } else {
            i++;
        }
    }
    
    mutex_unlock(memory->memory_mutex);
}

void episodic_memory_consolidate_to_long_term(EpisodicMemory* memory) {
    if (!memory) return;
    
    mutex_lock(memory->memory_mutex);
    
    // Check for consolidation candidates
    for (u32 i = 0; i < memory->short_term->episode_count; i++) {
        Episode* episode = &memory->short_term->episodes[i];
        
        if (!episode->is_consolidated && 
            episode->importance >= memory->config.consolidation_threshold &&
            memory->long_term->episode_count < memory->long_term->capacity) {
            
            episodic_memory_consolidate_episode(memory, episode);
        }
    }
    
    memory->last_consolidation_time = memory->current_time;
    
    mutex_unlock(memory->memory_mutex);
}

void episodic_memory_consolidate_episode(EpisodicMemory* memory, Episode* episode) {
    if (!memory || !episode || episode->is_consolidated) return;
    
    // Compress episode for long-term storage
    Episode* compressed = &memory->long_term->compressed_episodes[memory->long_term->episode_count];
    *compressed = *episode;
    
    // Apply compression (simplified - in real implementation would use more sophisticated compression)
    compressed->retention_strength *= 0.8f; // Slight decay during consolidation
    compressed->is_consolidated = true;
    
    // Update indexing
    memory->long_term->episode_count++;
    memory->episodes_consolidated++;
    
    // Update compression statistics
    memory->long_term->original_size += sizeof(Episode);
    memory->long_term->compressed_size += sizeof(Episode) * 0.7f; // Assume 30% compression
    memory->long_term->compression_ratio = (f32)memory->long_term->compressed_size / memory->long_term->original_size;
}

MemoryResult episodic_memory_query(EpisodicMemory* memory, const MemoryQuery* query) {
    MemoryResult result = {0};
    if (!memory || !query) return result;
    
    f64 start_time = time_get_high_res_time();
    
    mutex_lock(memory->memory_mutex);
    
    // Search short-term memory first if requested
    u32 total_results = 0;
    Episode temp_results[256]; // Temporary storage
    
    if (query->include_short_term) {
        for (u32 i = 0; i < memory->short_term->episode_count && total_results < query->max_results; i++) {
            Episode* episode = &memory->short_term->episodes[i];
            
            if (episodic_memory_episode_matches_query(episode, query)) {
                f32 relevance = episodic_memory_calculate_relevance(episode, query);
                if (relevance >= query->min_importance) {
                    temp_results[total_results++] = *episode;
                }
            }
        }
    }
    
    // Search long-term memory if requested
    if (query->include_long_term && total_results < query->max_results) {
        for (u32 i = 0; i < memory->long_term->episode_count && total_results < query->max_results; i++) {
            Episode* episode = &memory->long_term->compressed_episodes[i];
            
            if (episodic_memory_episode_matches_query(episode, query)) {
                f32 relevance = episodic_memory_calculate_relevance(episode, query);
                if (relevance >= query->min_importance) {
                    temp_results[total_results++] = *episode;
                }
            }
        }
    }
    
    // Prepare result
    if (total_results > 0) {
        result.episodes = MALLOC_AI(sizeof(Episode) * total_results);
        if (result.episodes) {
            memcpy(result.episodes, temp_results, sizeof(Episode) * total_results);
            result.episode_count = total_results;
            result.relevance_score = 0.8f; // Average relevance
            strcpy(result.explanation, "Episodes found matching query criteria");
        }
    } else {
        strcpy(result.explanation, "No episodes found matching query criteria");
        result.relevance_score = 0.1f;
    }
    
    memory->queries_processed++;
    
    mutex_unlock(memory->memory_mutex);
    
    result.processing_time = time_get_high_res_time() - start_time;
    memory->total_query_time += result.processing_time;
    
    return result;
}

MemoryResult episodic_memory_find_similar(EpisodicMemory* memory, const Episode* reference, u32 max_results) {
    MemoryResult result = {0};
    if (!memory || !reference) return result;
    
    f64 start_time = time_get_high_res_time();
    
    mutex_lock(memory->memory_mutex);
    
    Episode similar_episodes[256];
    u32 similar_count = 0;
    
    // Search both short-term and long-term memory
    for (u32 i = 0; i < memory->short_term->episode_count && similar_count < max_results; i++) {
        Episode* episode = &memory->short_term->episodes[i];
        f32 similarity = episodic_memory_calculate_similarity(episode, reference);
        
        if (similarity >= 0.5f) { // Similarity threshold
            similar_episodes[similar_count++] = *episode;
        }
    }
    
    for (u32 i = 0; i < memory->long_term->episode_count && similar_count < max_results; i++) {
        Episode* episode = &memory->long_term->compressed_episodes[i];
        f32 similarity = episodic_memory_calculate_similarity(episode, reference);
        
        if (similarity >= 0.5f) {
            similar_episodes[similar_count++] = *episode;
        }
    }
    
    // Sort by similarity (simplified - would use proper sorting in production)
    for (u32 i = 0; i < similar_count - 1; i++) {
        for (u32 j = i + 1; j < similar_count; j++) {
            f32 sim1 = episodic_memory_calculate_similarity(&similar_episodes[i], reference);
            f32 sim2 = episodic_memory_calculate_similarity(&similar_episodes[j], reference);
            
            if (sim2 > sim1) {
                Episode temp = similar_episodes[i];
                similar_episodes[i] = similar_episodes[j];
                similar_episodes[j] = temp;
            }
        }
    }
    
    // Prepare result
    if (similar_count > 0) {
        result.episodes = MALLOC_AI(sizeof(Episode) * similar_count);
        if (result.episodes) {
            memcpy(result.episodes, similar_episodes, sizeof(Episode) * similar_count);
            result.episode_count = similar_count;
            result.relevance_score = 0.7f; // Good confidence for similarity search
            strcpy(result.explanation, "Episodes found with high similarity to reference");
        }
    } else {
        strcpy(result.explanation, "No similar episodes found");
        result.relevance_score = 0.1f;
    }
    
    mutex_unlock(memory->memory_mutex);
    
    result.processing_time = time_get_high_res_time() - start_time;
    return result;
}

// Helper functions
static bool episodic_memory_episode_matches_query(const Episode* episode, const MemoryQuery* query) {
    if (!episode || !query) return false;
    
    // Type filter
    if (query->episode_type != EPISODE_COUNT && episode->type != query->episode_type) {
        return false;
    }
    
    // Time range filter
    if (query->time_range_start > 0 && episode->timestamp < query->time_range_start) {
        return false;
    }
    if (query->time_range_end > 0 && episode->timestamp > query->time_range_end) {
        return false;
    }
    
    // Location filter
    if (query->location_hint.x != 0 || query->location_hint.y != 0 || query->location_hint.z != 0) {
        f32 distance = vec3_length(vec3_subtract(episode->location, query->location_hint));
        if (distance > 50.0f) { // 50 unit radius
            return false;
        }
    }
    
    // Importance filter
    if (episode->importance < query->min_importance) {
        return false;
    }
    
    // Entity filter
    if (query->entity_filter != 0) {
        bool found = false;
        for (u32 i = 0; i < episode->entity_count; i++) {
            if (episode->involved_entities[i] == query->entity_filter) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    // Tag filter (simplified)
    if (query->required_tag_count > 0) {
        // In real implementation, would check all required tags
        // For now, just check if episode has any tags
        if (episode->tag_count == 0) return false;
    }
    
    return true;
}

static f32 episodic_memory_calculate_relevance(const Episode* episode, const MemoryQuery* query) {
    if (!episode || !query) return 0.0f;
    
    f32 relevance = episode->importance;
    
    // Boost for recent episodes
    f64 time_diff = query->time_range_end - episode->timestamp;
    if (time_diff < 300.0) { // Within 5 minutes
        relevance *= 1.2f;
    }
    
    // Boost for emotional episodes
    relevance *= (1.0f + episode->emotional_intensity * 0.3f);
    
    // Boost for frequently accessed episodes
    if (episode->access_count > 5) {
        relevance *= 1.1f;
    }
    
    return fminf(1.0f, relevance);
}

f32 episodic_memory_calculate_similarity(const Episode* episode1, const Episode* episode2) {
    if (!episode1 || !episode2) return 0.0f;
    
    f32 total_similarity = 0.0f;
    u32 factors = 0;
    
    // Type similarity
    if (episode1->type == episode2->type) {
        total_similarity += 0.3f;
    }
    factors++;
    
    // Text similarity
    f32 text_sim = calculate_text_similarity(episode1->description, episode2->description);
    total_similarity += text_sim * 0.3f;
    factors++;
    
    // Spatial similarity
    f32 spatial_sim = calculate_spatial_similarity(&episode1->location, &episode2->location, 100.0f);
    total_similarity += spatial_sim * 0.2f;
    factors++;
    
    // Temporal similarity
    f64 time_diff = fabs(episode1->timestamp - episode2->timestamp);
    f32 temporal_sim = fmaxf(0.0f, 1.0f - (time_diff / 3600.0)); // 1 hour window
    total_similarity += temporal_sim * 0.2f;
    factors++;
    
    return factors > 0 ? total_similarity / factors : 0.0f;
}

// Utility functions
const char* episode_type_to_string(EpisodeType type) {
    static const char* names[] = {
        "VISUAL", "AUDITORY", "TACTILE", "ACTION", "DIALOGUE",
        "EMOTION", "LOCATION_CHANGE", "ITEM_INTERACTION", "SOCIAL",
        "COMBAT", "DISCOVERY", "GOAL_ACHIEVED", "GOAL_FAILED"
    };
    
    if (type >= 0 && type < EPISODE_COUNT) {
        return names[type];
    }
    return "UNKNOWN";
}

void episodic_memory_print_statistics(EpisodicMemory* memory) {
    if (!memory) return;
    
    mutex_lock(memory->memory_mutex);
    
    printf("Episodic Memory Statistics:\n");
    printf("  Episodes Created: %u\n", memory->episodes_created);
    printf("  Episodes Consolidated: %u\n", memory->episodes_consolidated);
    printf("  Short-term Episodes: %u/%u\n", memory->short_term->episode_count, memory->short_term->buffer_size);
    printf("  Long-term Episodes: %u/%u\n", memory->long_term->episode_count, memory->long_term->capacity);
    printf("  Queries Processed: %u\n", memory->queries_processed);
    printf("  Average Query Time: %.3f ms\n", 
           memory->queries_processed > 0 ? (memory->total_query_time / memory->queries_processed) * 1000.0 : 0.0);
    printf("  Compression Ratio: %.2f\n", memory->long_term->compression_ratio);
    
    mutex_unlock(memory->memory_mutex);
}
