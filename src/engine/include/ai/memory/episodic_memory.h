#ifndef EPISODIC_MEMORY_H
#define EPISODIC_MEMORY_H

#include <core/types.h>
#include "engine/include/core/logger.h"
#include <math/vec3.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Episodic Memory System - AI memory for personal experiences and events

// Forward declarations
typedef struct EpisodicMemory EpisodicMemory;
typedef struct Episode Episode;
typedef struct MemoryQuery MemoryQuery;
typedef struct MemoryResult MemoryResult;
typedef struct ShortTermMemory ShortTermMemory;
typedef struct LongTermMemory LongTermMemory;

// Episode types
typedef enum {
    EPISODE_VISUAL,           // Visual observation
    EPISODE_AUDITORY,         // Sound heard
    EPISODE_TACTILE,          // Physical contact
    EPISODE_ACTION,            // Action performed
    EPISODE_DIALOGUE,          // Conversation/dialogue
    EPISODE_EMOTION,          // Emotional state change
    EPISODE_LOCATION_CHANGE,    // Moved to new location
    EPISODE_ITEM_INTERACTION,  // Interaction with object
    EPISODE_SOCIAL,           // Social interaction
    EPISODE_COMBAT,           // Combat encounter
    EPISODE_DISCOVERY,         // Discovery of new information
    EPISODE_GOAL_ACHIEVED,    // Goal completed
    EPISODE_GOAL_FAILED,       // Goal failed
    EPISODE_COUNT
} EpisodeType;

// Episode structure
struct Episode {
    u64 episode_id;             // Unique episode identifier
    EpisodeType type;           // Type of episode
    f64 timestamp;             // When episode occurred
    Vec3 location;              // Where episode occurred
    char description[512];      // Episode description
    f32 importance;            // Importance score (0.0 - 1.0)
    f32 emotional_intensity;     // Emotional intensity (0.0 - 1.0)
    
    // Context information
    EntityID involved_entities[16]; // Entities involved
    u32 entity_count;              // Number of entities
    char tags[32][64];           // Searchable tags
    u32 tag_count;                 // Number of tags
    
    // Memory consolidation
    f32 retention_strength;      // How well this is remembered
    f64 last_accessed;          // When this was last recalled
    u32 access_count;            // How many times recalled
    bool is_consolidated;        // Whether moved to long-term memory
    
    // Sensory details
    struct {
        f32 visual_quality;       // Visual clarity (0.0 - 1.0)
        f32 audio_quality;       // Audio clarity (0.0 - 1.0)
        f32 tactile_intensity;    // Touch intensity (0.0 - 1.0)
        f32 olfactory_strength;   // Smell strength (0.0 - 1.0)
    } sensory_data;
    
    // Temporal context
    f64 duration;               // Episode duration
    Episode* related_episodes[8]; // Related episodes
    u32 related_count;          // Number of related episodes
};

// Short-term memory buffer (30 seconds)
struct ShortTermMemory {
    Episode* episodes;           // Circular buffer of episodes
    u32 buffer_size;           // Buffer capacity
    u32 write_index;           // Current write position
    u32 episode_count;         // Number of episodes in buffer
    f64 time_window;          // Time window (30 seconds)
    f64 current_time;         // Current simulation time
    
    // High-detail storage
    struct {
        f32* visual_frames;     // Visual frame data
        f32* audio_samples;     // Audio sample data
        u32 frame_count;        // Number of frames
        u32 sample_count;       // Number of samples
        u32 sample_rate;        // Audio sample rate
    } high_detail;
};

// Long-term memory with compression
struct LongTermMemory {
    Episode* compressed_episodes;  // Compressed episode storage
    u32 episode_count;            // Number of episodes
    u32 capacity;                 // Maximum capacity
    
    // Indexing for fast retrieval
    struct {
        EpisodeType* type_index[EPISODE_COUNT];
        u32 type_counts[EPISODE_COUNT];
        char** tag_index;
        u32 tag_count;
    } indexing;
    
    // Compression statistics
    f32 compression_ratio;         // Memory compression ratio
    u64 original_size;            // Original uncompressed size
    u64 compressed_size;          // Compressed size
};

// Memory query for retrieval
struct MemoryQuery {
    char query_text[256];         // Natural language query
    EpisodeType episode_type;      // Type filter (EPISODE_COUNT for all)
    Vec3 location_hint;           // Location hint for spatial queries
    f64 time_range_start;        // Time range start
    f64 time_range_end;          // Time range end
    f32 min_importance;          // Minimum importance threshold
    char required_tags[16][64];   // Required tags
    u32 required_tag_count;       // Number of required tags
    EntityID entity_filter;        // Filter by specific entity
    u32 max_results;             // Maximum results to return
    bool include_short_term;      // Include short-term memory
    bool include_long_term;       // Include long-term memory
    f32 similarity_threshold;     // Minimum similarity for fuzzy matching
};

// Memory retrieval result
struct MemoryResult {
    Episode* episodes;            // Array of matching episodes
    u32 episode_count;           // Number of episodes
    f32 relevance_score;         // Overall relevance score
    char explanation[256];        // Explanation of how results were found
    f64 processing_time;         // Time taken to process query
    bool has_more_results;        // Whether more results are available
};

// Episodic memory configuration
typedef struct {
    u32 short_term_capacity;      // Short-term memory capacity (episodes)
    f64 short_term_duration;      // Short-term memory duration (seconds)
    u32 long_term_capacity;       // Long-term memory capacity (episodes)
    f32 consolidation_threshold;   // Importance threshold for consolidation
    f32 decay_rate;              // Memory decay rate
    f64 retrieval_timeout;        // Maximum time for retrieval
    bool enable_compression;       // Enable memory compression
    bool enable_persistence;      // Enable saving/loading memories
    bool enable_learning;         // Enable learning from experiences
} EpisodicMemoryConfig;

// Main Episodic Memory API
EpisodicMemory* episodic_memory_create(const EpisodicMemoryConfig* config);
void episodic_memory_destroy(EpisodicMemory* memory);
bool episodic_memory_initialize(EpisodicMemory* memory);
void episodic_memory_shutdown(EpisodicMemory* memory);

// Episode management
Episode* episodic_memory_add_episode(EpisodicMemory* memory, EpisodeType type, 
                                   const char* description, const Vec3* location);
bool episodic_memory_remove_episode(EpisodicMemory* memory, u64 episode_id);
Episode* episodic_memory_get_episode(EpisodicMemory* memory, u64 episode_id);
void episodic_memory_update_episode_importance(EpisodicMemory* memory, u64 episode_id, f32 importance);
void episodic_memory_add_entity_to_episode(EpisodicMemory* memory, u64 episode_id, EntityID entity_id);
void episodic_memory_add_tag_to_episode(EpisodicMemory* memory, u64 episode_id, const char* tag);

// Short-term memory management
void episodic_memory_update_short_term(EpisodicMemory* memory, f64 current_time);
Episode* episodic_memory_get_recent_episodes(EpisodicMemory* memory, u32 count);
void episodic_memory_consolidate_to_long_term(EpisodicMemory* memory);

// Long-term memory management
bool episodic_memory_compress_episodes(EpisodicMemory* memory);
void episodic_memory_decay_memories(EpisodicMemory* memory, f64 delta_time);
bool episodic_memory_save_long_term(EpisodicMemory* memory, const char* filename);
bool episodic_memory_load_long_term(EpisodicMemory* memory, const char* filename);

// Memory retrieval
MemoryResult episodic_memory_query(EpisodicMemory* memory, const MemoryQuery* query);
MemoryResult episodic_memory_find_similar(EpisodicMemory* memory, const Episode* reference, u32 max_results);
MemoryResult episodic_memory_get_by_time_range(EpisodicMemory* memory, f64 start_time, f64 end_time);
MemoryResult episodic_memory_get_by_location(EpisodicMemory* memory, const Vec3* location, f32 radius);

// Similarity search and context-based queries
f32 episodic_memory_calculate_similarity(const Episode* episode1, const Episode* episode2);
MemoryResult episodic_memory_contextual_search(EpisodicMemory* memory, const Vec3* current_location, 
                                           const EpisodeType* recent_types, u32 type_count);
MemoryResult episodic_memory_find_related_episodes(EpisodicMemory* memory, u64 episode_id, u32 max_depth);

// Learning and adaptation
void episodic_memory_learn_from_experience(EpisodicMemory* memory, const Episode* experience, f32 outcome);
void episodic_memory_update_importance_weights(EpisodicMemory* memory, const EpisodeType* types, 
                                           const f32* weights, u32 count);
void episodic_memory_adapt_decay_rates(EpisodicMemory* memory, EntityID entity_id, f32 adaptation_factor);

// Utility functions
const char* episode_type_to_string(EpisodeType type);
EpisodeType string_to_episode_type(const char* type_str);
u32 episodic_memory_get_episode_count(EpisodicMemory* memory);
u32 episodic_memory_get_short_term_count(EpisodicMemory* memory);
u32 episodic_memory_get_long_term_count(EpisodicMemory* memory);
void episodic_memory_print_statistics(EpisodicMemory* memory);
void episodic_memory_set_debug_mode(EpisodicMemory* memory, bool enabled);

// Advanced retrieval functions
bool episodic_memory_has_been_here_before(EpisodicMemory* memory, const Vec3* location, f32 tolerance);
MemoryResult episodic_memory_find_goal_related_episodes(EpisodicMemory* memory, const char* goal_description);
Episode* episodic_memory_get_most_important_episode(EpisodicMemory* memory, EpisodeType type, f64 time_range);

// Memory consolidation helpers
bool episodic_memory_should_consolidate(const Episode* episode, f32 threshold);
void episodic_memory_consolidate_episode(EpisodicMemory* memory, Episode* episode);
Episode* episodic_memory_summarize_episodes(const Episode* episodes, u32 count);

#ifdef __cplusplus
}
#endif

#endif // EPISODIC_MEMORY_H
