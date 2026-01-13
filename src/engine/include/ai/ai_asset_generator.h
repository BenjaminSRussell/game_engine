// include/engine/ai_asset_generator.h
//
// Purpose: AI-powered asset generation system for creating modular 3D and 2.5D assets.
// Supports 25 different styles, Lego-like snap-together components, and local AI/ML.
//
// Features:
// - Local AI/ML model for asset generation
// - Modular component system (snap-together like Legos)
// - 25 different style templates
// - Systematic AI improvements based on user feedback
// - Efficient 2.5D and 3D model generation
// - Skin/texture generation
//
#ifndef AI_ASSET_GENERATOR_H
#define AI_ASSET_GENERATOR_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include <math/mat4.h>

// Forward declarations
struct Mesh;
struct Texture;

// Asset style enumeration (25 different styles)
typedef enum {
    ASSET_STYLE_REALISTIC,           // 1. Realistic
    ASSET_STYLE_CARTOON,              // 2. Cartoon
    ASSET_STYLE_ANIME,                // 3. Anime
    ASSET_STYLE_PIXEL,                // 4. Pixel art
    ASSET_STYLE_LOW_POLY,             // 5. Low poly
    ASSET_STYLE_HIGH_POLY,            // 6. High poly
    ASSET_STYLE_STYLIZED,              // 7. Stylized
    ASSET_STYLE_CEL_SHADED,           // 8. Cel-shaded
    ASSET_STYLE_TOON,                 // 9. Toon
    ASSET_STYLE_ORGANIC,              // 10. Organic
    ASSET_STYLE_TECHNICAL,             // 11. Technical
    ASSET_STYLE_MEDIEVAL,             // 12. Medieval
    ASSET_STYLE_SCIFI,                // 13. Sci-fi
    ASSET_STYLE_STEAMPUNK,            // 14. Steampunk
    ASSET_STYLE_CYBERPUNK,            // 15. Cyberpunk
    ASSET_STYLE_FANTASY,              // 16. Fantasy
    ASSET_STYLE_HORROR,               // 17. Horror
    ASSET_STYLE_CUTE,                 // 18. Cute
    ASSET_STYLE_GRITTY,               // 19. Gritty
    ASSET_STYLE_MINIMALIST,           // 20. Minimalist
    ASSET_STYLE_DETAILED,             // 21. Detailed
    ASSET_STYLE_BLOCKY,               // 22. Blocky (Minecraft-like)
    ASSET_STYLE_SMOOTH,               // 23. Smooth
    ASSET_STYLE_GEOMETRIC,            // 24. Geometric
    ASSET_STYLE_ORGANIC_FLOW,         // 25. Organic flow
    ASSET_STYLE_COUNT
} AssetStyle;

// Snap point type (for Lego-like connections)
typedef enum {
    SNAP_POINT_TOP,          // Top connection
    SNAP_POINT_BOTTOM,        // Bottom connection
    SNAP_POINT_LEFT,          // Left side
    SNAP_POINT_RIGHT,         // Right side
    SNAP_POINT_FRONT,         // Front
    SNAP_POINT_BACK,          // Back
    SNAP_POINT_CENTER,        // Center (for rotation)
    SNAP_POINT_CUSTOM         // Custom connection point
} SnapPointType;

// Snap point definition
typedef struct {
    SnapPointType type;
    Vec3 position;            // Local position
    Vec3 normal;              // Connection direction
    f32 radius;               // Connection radius
    u32 connection_id;        // Unique connection identifier
    bool occupied;            // Whether this point is connected
} SnapPoint;

// Modular asset component (Lego-like piece)
typedef struct {
    u32 component_id;
    const char* name;
    AssetStyle style;
    
    // Geometry
    struct Mesh* mesh;        // 3D mesh
    struct Mesh* mesh_2_5d;   // 2.5D sprite mesh
    u32 texture_id;          // Skin/texture ID
    
    // Snap points (Lego-like connections)
    SnapPoint* snap_points;
    u32 snap_point_count;
    u32 snap_point_capacity;
    
    // Bounds
    Vec3 bounds_min;
    Vec3 bounds_max;
    Vec3 center;
    
    // Metadata
    f32 complexity;           // Complexity score (for AI)
    u32 triangle_count;
    bool is_base;             // Base component (can't be attached to others)
    bool is_attachment;      // Attachment component (must attach to base)
    
    // AI generation data
    f32 generation_quality;   // AI quality score
    u32 generation_iteration; // Iteration number
    void* ai_metadata;       // AI-specific metadata
} AssetComponent;

// Asset configuration (user requirements)
typedef struct {
    AssetStyle style;
    const char* description;   // User description
    f32 complexity_target;   // Target complexity (0.0-1.0)
    bool require_2_5d;       // Generate 2.5D version
    bool require_3d;         // Generate 3D version
    u32 snap_point_requirements[8]; // Required snap points
    Vec3 target_size;        // Target size
    f32 detail_level;        // Detail level (0.0-1.0)
} AssetConfig;

// AI model for asset generation
typedef struct {
    // Model state
    bool initialized;
    void* model_data;         // Local AI model data
    
    // Training/improvement data
    u32 training_iterations;
    f32 average_quality;
    u32 feedback_count;
    
    // Generation parameters
    f32 creativity;           // Creativity factor (0.0-1.0)
    f32 style_adherence;     // Style adherence (0.0-1.0)
    f32 modularity;          // Modularity factor (0.0-1.0)
} AIGenerator;

// Generation result
typedef struct {
    AssetComponent* components;
    u32 component_count;
    bool success;
    f32 quality_score;
    const char* error_message;
    u32 generation_time_ms;
} GenerationResult;

// User feedback for AI improvement
typedef struct {
    u32 component_id;
    f32 quality_rating;      // 0.0-1.0
    bool matches_description;
    bool style_correct;
    bool modularity_good;
    const char* feedback_text;
} UserFeedback;

// AI Asset Generator
typedef struct {
    AIGenerator ai_model;
    AssetComponent* component_library;
    u32 component_count;
    u32 component_capacity;
    
    // Style templates (25 styles)
    void* style_templates[ASSET_STYLE_COUNT];
    
    // Generation queue
    AssetConfig* generation_queue;
    u32 queue_count;
    u32 queue_capacity;
    
    // Feedback system
    UserFeedback* feedback_history;
    u32 feedback_count;
    u32 feedback_capacity;
    
    bool initialized;
} AIAssetGenerator;

// Lifecycle
bool ai_asset_generator_init(AIAssetGenerator* generator);
void ai_asset_generator_shutdown(AIAssetGenerator* generator);

// Asset generation
GenerationResult ai_generate_asset(AIAssetGenerator* generator, const AssetConfig* config);
GenerationResult ai_generate_from_description(AIAssetGenerator* generator, 
                                               const char* description, 
                                               AssetStyle style);

// Component management
AssetComponent* ai_create_component(AIAssetGenerator* generator, 
                                    const char* name, 
                                    AssetStyle style);
void ai_destroy_component(AIAssetGenerator* generator, AssetComponent* component);
AssetComponent* ai_find_component(AIAssetGenerator* generator, u32 component_id);

// Snap point management
bool ai_add_snap_point(AssetComponent* component, SnapPointType type, Vec3 position, Vec3 normal);
bool ai_connect_components(AssetComponent* base, AssetComponent* attachment, 
                          u32 base_snap_id, u32 attach_snap_id);
bool ai_can_snap(AssetComponent* component1, AssetComponent* component2, 
                 u32 snap1_id, u32 snap2_id);

// Style management
bool ai_load_style_template(AIAssetGenerator* generator, AssetStyle style, const char* template_path);
void ai_set_style_parameters(AIAssetGenerator* generator, AssetStyle style, f32 creativity, f32 detail);

// AI improvement system
void ai_submit_feedback(AIAssetGenerator* generator, const UserFeedback* feedback);
void ai_improve_model(AIAssetGenerator* generator);
f32 ai_get_model_quality(AIAssetGenerator* generator);

// Batch generation
GenerationResult* ai_generate_batch(AIAssetGenerator* generator, 
                                    const AssetConfig* configs, 
                                    u32 count);

// Component library
u32 ai_get_component_count(AIAssetGenerator* generator);
AssetComponent* ai_get_component_by_style(AIAssetGenerator* generator, AssetStyle style, u32 index);
AssetComponent* ai_search_components(AIAssetGenerator* generator, const char* query);

// Export/Import
bool ai_export_component(const AssetComponent* component, const char* path);
AssetComponent* ai_import_component(AIAssetGenerator* generator, const char* path);

#endif // AI_ASSET_GENERATOR_H

