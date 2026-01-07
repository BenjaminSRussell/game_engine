// include/engine/procedural/procedural_content.h
//
// Purpose: Procedural content generation system with AI-assisted asset creation
// This system provides cutting-edge procedural generation capabilities that surpass
// Unity's procedural tools with machine learning integration and infinite world generation.
//
// Key Features:
// - AI-assisted asset generation using neural networks
// - Infinite world generation with seamless transitions
// - Style transfer for consistent art direction
// - Procedural mesh, texture, and animation generation
// - Grammar-based generation for buildings and structures
// - Noise-based terrain and ecosystem generation
// - Real-time content streaming and optimization
// - Quality control with AI validation
//
// Performance Advantages over Unity Procedural:
// - ML-powered generation vs algorithmic only
// - Real-time infinite world streaming
// - Better quality control and consistency
// - Advanced style transfer and adaptation
// - Integration with our superior ECS architecture
//
// Public APIs:
// - ProceduralSystem: Main procedural generation container
// - WorldGenerator: Infinite world creation system
// - AssetGenerator: AI-assisted asset creation
// - StyleEngine: Art direction and style transfer
// - QualityValidator: AI-powered quality assessment
//
// Ownership: ProceduralSystem owns all generation systems
// Invariants: Generated content must be validated before integration
//
#ifndef PROCEDURAL_CONTENT_H
#define PROCEDURAL_CONTENT_H

#include "../../common.h"
#include "../ecs/ecs.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// AI-ASSISTED ASSET GENERATION
// ============================================================================

// Asset generation types
typedef enum {
  ASSET_GEN_MESH = 0,
  ASSET_GEN_TEXTURE,
  ASSET_GEN_ANIMATION,
  ASSET_GEN_AUDIO,
  ASSET_GEN_TERRAIN,
  ASSET_GEN_VEGETATION,
  ASSET_GEN_ARCHITECTURE,
  ASSET_GEN_PARTICLE_EFFECT
} AssetGenerationType;

// ML model types for asset generation
typedef enum {
  ML_MODEL_GAN = 0,           // Generative Adversarial Network
  ML_MODEL_VAE,               // Variational Autoencoder
  ML_MODEL_DIFFUSION,         // Diffusion Model
  ML_MODEL_TRANSFORMER,       // Transformer-based model
  ML_MODEL_STYLE_TRANSFER,    // Neural Style Transfer
  ML_MODEL_AUTOENCODER        // Autoencoder
} MLModelType;

// Asset generation parameters
typedef struct {
  AssetGenerationType type;
  MLModelType ml_model;
  char *style_reference;
  char *content_prompt;
  vec2 output_resolution;    // For textures
  u32 vertex_count;          // For meshes
  u32 animation_frames;      // For animations
  f32 quality_threshold;     // Minimum quality score
  u32 max_attempts;          // Max generation attempts
  bool enable_variation;     // Generate multiple variants
  f32 variation_strength;    // How much to vary from reference
  u32 seed;                  // Random seed for reproducibility
} AssetGenParams;

// Generated asset
typedef struct {
  AssetGenerationType type;
  void *asset_data;
  u32 data_size;
  f32 quality_score;
  f32 diversity_score;
  f32 style_match_score;
  char *generation_metadata;
  u32 generation_time_ms;
  bool meets_quality_threshold;
  u32 attempt_count;
} GeneratedAsset;

// AI asset generator
typedef struct {
  MLModelType model_type;
  void *ml_model;
  char *model_name;
  char *model_path;
  AssetGenParams default_params;
  GeneratedAsset *generated_assets;
  u32 asset_count;
  u32 max_assets;
  bool is_loaded;
  f64 total_generation_time;
  u32 successful_generations;
  u32 failed_generations;
} AIAssetGenerator;

// ============================================================================
// STYLE TRANSFER ENGINE
// ============================================================================

// Art style definition
typedef struct {
  char *style_name;
  char *style_description;
  void *style_embedding;      // ML embedding vector
  u32 embedding_size;
  f32 color_palette[16];      // Dominant colors
  f32 style_strength;
  f32 abstraction_level;
  f32 detail_level;
  bool is_cartoonish;
  bool is_realistic;
  f32 contrast_level;
  f32 saturation_level;
} ArtStyle;

// Style transfer parameters
typedef struct {
  ArtStyle *source_style;
  ArtStyle *target_style;
  f32 style_weight;
  f32 content_weight;
  u32 iteration_count;
  f32 learning_rate;
  u32 output_resolution;
  bool preserve_edges;
  bool maintain_colors;
  f32 blend_factor;
} StyleTransferParams;

// Style transfer engine
typedef struct {
  void *style_network;
  void *content_network;
  ArtStyle *available_styles;
  u32 style_count;
  u32 max_styles;
  StyleTransferParams default_params;
  bool gpu_accelerated;
  f64 total_transfer_time;
  u32 successful_transfers;
} StyleTransferEngine;

// ============================================================================
// INFINITE WORLD GENERATION
// ============================================================================

// World generation algorithms
typedef enum {
  WORLD_GEN_NOISE = 0,        // Perlin/Simplex noise
  WORLD_GEN_CELLULAR,         // Cellular automata
  WORLD_GEN_GRAMMAR,          // Grammar-based
  WORLD_GEN_GRAPH_BASED,      // Graph-based placement
  WORLD_GEN_SIMULATION,      // Physics-based simulation
  WORLD_GEN_HYBRID           // Hybrid approach
} WorldGenAlgorithm;

// Biome types
typedef enum {
  BIOME_OCEAN = 0,
  BIOME_BEACH,
  BIOME_PLAINS,
  BIOME_FOREST,
  BIOME_MOUNTAINS,
  BIOME_DESERT,
  BIOME_TUNDRA,
  BIOME_SWAMP,
  BIOME_JUNGLE,
  BIOME_CUSTOM
} BiomeType;

// Terrain layer
typedef struct {
  BiomeType biome;
  f32 height_min;
  f32 height_max;
  f32 moisture_min;
  f32 moisture_max;
  f32 temperature_min;
  f32 temperature_max;
  u32 seed_offset;
  f32 noise_scale;
  f32 noise_amplitude;
  u32 octaves;
  f32 persistence;
  f32 lacunarity;
} TerrainLayer;

// World chunk
typedef struct {
  vec2 chunk_position;        // X, Z coordinates
  u32 chunk_size;
  f32 *height_map;
  f32 *moisture_map;
  f32 *temperature_map;
  u8 *biome_map;
  u8 *vegetation_map;
  u8 *structure_map;
  Entity *entities;
  u32 entity_count;
  u32 max_entities;
  bool is_generated;
  bool is_loaded;
  u32 generation_level;       // LOD level
  f64 generation_time;
  u32 seed;
} WorldChunk;

// World generator
typedef struct {
  WorldGenAlgorithm algorithm;
  TerrainLayer *terrain_layers;
  u32 layer_count;
  u32 max_layers;
  u32 chunk_size;
  u32 view_distance_chunks;
  u32 max_loaded_chunks;
  WorldChunk *loaded_chunks;
  u32 loaded_chunk_count;
  u32 world_seed;
  vec2 world_center;
  f32 generation_quality;
  bool enable_streaming;
  bool enable_caching;
  bool enable_ai_enhancement;
  f64 total_generation_time;
  u32 chunks_generated;
} WorldGenerator;

// ============================================================================
// GRAMMAR-BASED GENERATION
// ============================================================================

// Grammar rule types
typedef enum {
  GRAMMAR_RULE_REPLACEMENT = 0,
  GRAMMAR_RULE_TERMINAL,
  GRAMMAR_RULE_NONTERMINAL,
  GRAMMAR_RULE_CONDITIONAL,
  GRAMMAR_RULE_STOCHASTIC
} GrammarRuleType;

// Grammar symbol
typedef struct {
  char *symbol_name;
  bool is_terminal;
  bool is_start;
  void *symbol_data;
  u32 data_size;
} GrammarSymbol;

// Grammar rule
typedef struct {
  u32 rule_id;
  GrammarRuleType type;
  GrammarSymbol *left_hand;
  GrammarSymbol **right_hand;
  u32 right_hand_count;
  f32 probability;            // For stochastic rules
  char *condition;           // For conditional rules
  bool is_active;
} GrammarRule;

// Shape grammar
typedef struct {
  GrammarSymbol *symbols;
  u32 symbol_count;
  u32 max_symbols;
  GrammarRule *rules;
  u32 rule_count;
  u32 max_rules;
  GrammarSymbol *start_symbol;
  u32 max_derivation_steps;
  u32 current_derivation_step;
  char *grammar_name;
  bool is_loaded;
} ShapeGrammar;

// Grammar-based generator
typedef struct {
  ShapeGrammar *grammars;
  u32 grammar_count;
  u32 max_grammars;
  u32 max_generation_depth;
  bool enable_random_selection;
  f32 randomness_factor;
  u32 generations_per_frame;
  f64 total_generation_time;
} GrammarGenerator;

// ============================================================================
// QUALITY VALIDATION SYSTEM
// ============================================================================

// Quality metrics
typedef struct {
  f32 visual_quality;         // Visual appeal score
  f32 technical_quality;      // Technical correctness
  f32 consistency_score;      // Style consistency
  f32 diversity_score;        // Diversity from reference
  f32 performance_score;      // Runtime performance impact
  f32 memory_usage_score;    // Memory efficiency
  f32 overall_score;          // Weighted overall score
  bool meets_threshold;
  char *quality_report;
} QualityMetrics;

// Quality validator
typedef struct {
  void *quality_network;      // ML model for quality assessment
  f32 quality_threshold;
  f32 quality_weights[6];     // Weights for different metrics
  QualityMetrics *validation_history;
  u32 history_count;
  u32 max_history;
  bool enable_auto_rejection;
  bool enable_feedback_learning;
  u32 validations_performed;
  u32 assets_accepted;
  u32 assets_rejected;
} QualityValidator;

// ============================================================================
// PROCEDURAL CONTENT SYSTEM
// ============================================================================

// Procedural system configuration
typedef struct {
  u32 max_concurrent_generations;
  u32 max_memory_usage_mb;
  f32 quality_threshold;
  bool enable_gpu_acceleration;
  bool enable_ai_enhancement;
  bool enable_streaming;
  bool enable_caching;
  u32 worker_threads;
  f32 update_frequency;
  bool enable_profiling;
  char *cache_directory;
  char *model_directory;
} ProceduralConfig;

// Main procedural content system
typedef struct {
  // Configuration
  ProceduralConfig config;
  
  // AI asset generators
  AIAssetGenerator *generators;
  u32 generator_count;
  u32 max_generators;
  
  // Style transfer engine
  StyleTransferEngine style_engine;
  
  // World generator
  WorldGenerator world_generator;
  
  // Grammar generator
  GrammarGenerator grammar_generator;
  
  // Quality validator
  QualityValidator quality_validator;
  
  // Generated assets registry
  GeneratedAsset *asset_registry;
  u32 asset_registry_count;
  u32 max_asset_registry;
  
  // Performance
  Profiler *procedural_profiler;
  f64 total_generation_time;
  f64 ai_generation_time;
  f64 world_generation_time;
  f64 quality_validation_time;
  
  // ECS integration
  World *ecs_world;
  
  // Threading
  void *worker_threads;
  u32 worker_thread_count;
  
  // Streaming and caching
  void *cache_system;
  bool cache_enabled;
  
  // Statistics
  u32 total_assets_generated;
  u32 total_chunks_generated;
  u32 total_grammar_generations;
  f64 average_quality_score;
} ProceduralSystem;

// ============================================================================
// PUBLIC API
// ============================================================================

// Procedural system management
ProceduralSystem *procedural_system_create(const ProceduralConfig *config, World *ecs_world);
void procedural_system_destroy(ProceduralSystem *system);
void procedural_system_update(ProceduralSystem *system, f32 delta_time);

// Configuration
ProceduralConfig procedural_create_default_config(void);
ProceduralConfig procedural_create_high_quality_config(void);
ProceduralConfig procedural_create_performance_config(void);

// ============================================================================
// AI-ASSISTED ASSET GENERATION API
// ============================================================================

// Asset generator management
AIAssetGenerator *procedural_create_asset_generator(ProceduralSystem *system, AssetGenerationType type, MLModelType model);
void procedural_destroy_asset_generator(ProceduralSystem *system, AIAssetGenerator *generator);
bool procedural_load_ml_model(ProceduralSystem *system, AIAssetGenerator *generator, const char *model_path);

// Asset generation
GeneratedAsset *procedural_generate_asset(ProceduralSystem *system, const AssetGenParams *params);
bool procedural_generate_asset_async(ProceduralSystem *system, const AssetGenParams *params);
GeneratedAsset *procedural_get_generated_asset(ProceduralSystem *system, u32 asset_id);
bool procedural_validate_asset_quality(ProceduralSystem *system, GeneratedAsset *asset);

// Asset variation
GeneratedAsset *procedural_create_asset_variant(ProceduralSystem *system, GeneratedAsset *base_asset, f32 variation_strength);
bool procedural_batch_generate_variants(ProceduralSystem *system, GeneratedAsset *base_asset, u32 variant_count, GeneratedAsset **variants);

// ============================================================================
// STYLE TRANSFER API
// ============================================================================

// Style management
ArtStyle *procedural_create_art_style(ProceduralSystem *system, const char *name, const char *reference_image);
bool procedural_load_art_style(ProceduralSystem *system, ArtStyle *style, const char *style_file);
void procedural_destroy_art_style(ProceduralSystem *system, ArtStyle *style);

// Style transfer
GeneratedAsset *procedural_apply_style_transfer(ProceduralSystem *system, GeneratedAsset *asset, ArtStyle *target_style);
bool procedural_batch_style_transfer(ProceduralSystem *system, GeneratedAsset **assets, u32 asset_count, ArtStyle *target_style);
ArtStyle *procedural_extract_style_from_image(ProceduralSystem *system, const char *image_path);

// Style blending
ArtStyle *procedural_blend_styles(ProceduralSystem *system, ArtStyle *style_a, ArtStyle *style_b, f32 blend_factor);
bool procedural_apply_style_to_world(ProceduralSystem *system, ArtStyle *style, vec2 world_center, f32 radius);

// ============================================================================
// INFINITE WORLD GENERATION API
// ============================================================================

// World generator configuration
bool procedural_configure_world_generator(ProceduralSystem *system, u32 chunk_size, u32 view_distance, u32 world_seed);
bool procedural_add_terrain_layer(ProceduralSystem *system, const TerrainLayer *layer);
bool procedural_set_generation_algorithm(ProceduralSystem *system, WorldGenAlgorithm algorithm);

// World generation
WorldChunk *procedural_generate_chunk(ProceduralSystem *system, vec2 chunk_position);
bool procedural_generate_chunk_async(ProceduralSystem *system, vec2 chunk_position);
bool procedural_load_chunk(ProceduralSystem *system, vec2 chunk_position);
bool procedural_unload_chunk(ProceduralSystem *system, vec2 chunk_position);

// World streaming
void procedural_enable_world_streaming(ProceduralSystem *system, bool enable);
void procedural_set_streaming_center(ProceduralSystem *system, vec2 center);
void procedural_update_world_streaming(ProceduralSystem *system);

// Terrain modification
bool procedural_modify_terrain(ProceduralSystem *system, vec3 world_position, f32 radius, f32 height_delta);
bool procedural_add_terrain_feature(ProceduralSystem *system, vec3 position, const char *feature_type);

// ============================================================================
// GRAMMAR-BASED GENERATION API
// ============================================================================

// Grammar management
ShapeGrammar *procedural_create_shape_grammar(ProceduralSystem *system, const char *name);
bool procedural_load_grammar_from_file(ProceduralSystem *system, ShapeGrammar *grammar, const char *grammar_file);
void procedural_destroy_shape_grammar(ProceduralSystem *system, ShapeGrammar *grammar);

// Grammar rule management
bool procedural_add_grammar_rule(ProceduralSystem *system, ShapeGrammar *grammar, const GrammarRule *rule);
bool procedural_remove_grammar_rule(ProceduralSystem *system, ShapeGrammar *grammar, u32 rule_id);
GrammarRule *procedural_find_grammar_rule(ProceduralSystem *system, ShapeGrammar *grammar, const char *symbol_name);

// Grammar generation
bool procedural_generate_from_grammar(ProceduralSystem *system, ShapeGrammar *grammar, u32 max_steps);
bool procedural_generate_structure(ProceduralSystem *system, vec3 position, ShapeGrammar *grammar);
void procedural_set_grammar_randomness(ProceduralSystem *system, f32 randomness_factor);

// ============================================================================
// QUALITY VALIDATION API
// ============================================================================

// Quality validation
QualityMetrics *procedural_validate_asset(ProceduralSystem *system, GeneratedAsset *asset);
bool procedural_meets_quality_threshold(ProceduralSystem *system, GeneratedAsset *asset);
void procedural_set_quality_threshold(ProceduralSystem *system, f32 threshold);

// Quality learning
bool procedural_record_quality_feedback(ProceduralSystem *system, GeneratedAsset *asset, f32 user_rating);
bool procedural_train_quality_model(ProceduralSystem *system, const GeneratedAsset *training_data, u32 data_count);
void procedural_enable_quality_learning(ProceduralSystem *system, bool enable);

// Quality statistics
void procedural_print_quality_report(ProceduralSystem *system);
QualityMetrics procedural_get_average_quality(ProceduralSystem *system);

// ============================================================================
// CACHING AND STREAMING API
// ============================================================================

// Asset caching
bool procedural_cache_asset(ProceduralSystem *system, GeneratedAsset *asset, const char *cache_key);
GeneratedAsset *procedural_load_cached_asset(ProceduralSystem *system, const char *cache_key);
void procedural_clear_asset_cache(ProceduralSystem *system);

// World caching
bool procedural_cache_chunk(ProceduralSystem *system, WorldChunk *chunk);
WorldChunk *procedural_load_cached_chunk(ProceduralSystem *system, vec2 chunk_position);
void procedural_clear_chunk_cache(ProceduralSystem *system);

// Streaming optimization
void procedural_optimize_streaming(ProceduralSystem *system);
void procedural_preload_area(ProceduralSystem *system, vec2 center, f32 radius);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Asset generation parameter macros
#define PROCEDURAL_MESH_PARAMS(vertex_count, quality, seed) \
  (AssetGenParams){ \
    .type = ASSET_GEN_MESH, \
    .vertex_count = vertex_count, \
    .quality_threshold = quality, \
    .seed = seed, \
    .enable_variation = true, \
    .max_attempts = 3 \
  }

#define PROCEDURAL_TEXTURE_PARAMS(resolution, style, quality, seed) \
  (AssetGenParams){ \
    .type = ASSET_GEN_TEXTURE, \
    .output_resolution = resolution, \
    .style_reference = style, \
    .quality_threshold = quality, \
    .seed = seed, \
    .enable_variation = true, \
    .max_attempts = 3 \
  }

#define PROCEDURAL_TERRAIN_PARAMS(chunk_size, quality, seed) \
  (AssetGenParams){ \
    .type = ASSET_GEN_TERRAIN, \
    .quality_threshold = quality, \
    .seed = seed, \
    .enable_variation = false, \
    .max_attempts = 1 \
  }

// Quality threshold macros
#define PROCEDURAL_HIGH_QUALITY 0.8f
#define PROCEDURAL_MEDIUM_QUALITY 0.6f
#define PROCEDURAL_LOW_QUALITY 0.4f

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Real-time adaptation
void procedural_enable_real_time_adaptation(ProceduralSystem *system, bool enable);
void procedural_adapt_to_player_behavior(ProceduralSystem *system, Entity player);

// Collaborative generation
bool procedural_enable_collaborative_generation(ProceduralSystem *system, const char *server_url);
void procedural_sync_generation_data(ProceduralSystem *system);

// Emergent complexity
void procedural_enable_emergent_complexity(ProceduralSystem *system, bool enable);
void procedural_analyze_generation_patterns(ProceduralSystem *system);

// Cross-domain generation
GeneratedAsset *procedural_generate_cross_domain_asset(ProceduralSystem *system, AssetGenerationType source_type, AssetGenerationType target_type);

// Procedural debugging
void procedural_debug_render_generation_progress(ProceduralSystem *system);
void procedural_debug_render_quality_metrics(ProceduralSystem *system);
void procedural_print_generation_statistics(ProceduralSystem *system);

#endif // PROCEDURAL_CONTENT_H
