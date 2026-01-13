#include "ai/ai_types.h"
#include "core/core.h"

//  COMPLETED: Define Saliency [Difficulty: 1] [Atomic Steps: 4]
// 1. Measure of "interestingness" of a point.
// 2. Inputs: Motion, Color Constant, Loudness (Audio-visual).
// 3. Map of the environment.

// Saliency stimulus types
typedef enum {
    SALIENCY_MOTION,       // Motion-based saliency
    SALIENCY_COLOR,        // Color-based saliency
    SALIENCY_BRIGHTNESS,    // Brightness-based saliency
    SALIENCY_CONTRAST,     // Contrast-based saliency
    SALIENCY_AUDIO,        // Audio-based saliency
    SALIENCY_EVENT,        // Event-based saliency (explosions, etc.)
    SALIENCY_HEATMAP       // Heatmap-based saliency
} SaliencyType;

// Saliency value at a point
typedef struct {
    f32 total_saliency;     // Combined saliency value
    f32 motion_saliency;    // Motion component
    f32 color_saliency;     // Color component
    f32 brightness_saliency; // Brightness component
    f32 contrast_saliency;   // Contrast component
    f32 audio_saliency;     // Audio component
    f32 event_saliency;     // Event component
    f64 timestamp;          // When this value was computed
    u32 stimulus_count;      // Number of stimuli contributing
} SaliencyValue;

// Individual stimulus
typedef struct {
    u32 stimulus_id;         // Unique stimulus ID
    SaliencyType type;      // Type of stimulus
    f32 position[3];        // World position
    f32 intensity;          // Stimulus intensity (0.0-1.0)
    f32 radius;             // Affected radius
    f64 creation_time;      // When stimulus was created
    f64 expiration_time;    // When stimulus expires
    f32 decay_rate;         // How quickly stimulus decays
    bool is_active;          // True if stimulus is currently active
    u32 source_entity_id;   // Entity that created stimulus (if any)
} SaliencyStimulus;

// Saliency map grid cell
typedef struct {
    SaliencyValue value;    // Current saliency value
    u32 stimulus_mask;      // Bitmask of active stimuli
    f32 accumulated;        // Accumulated saliency over time
    u32 visit_count;        // How many times this cell was visited
    f64 last_visit_time;    // Last time this cell was visited
    bool is_inhibited;      // True if cell is currently inhibited
    f64 inhibition_end;     // When inhibition ends
} SaliencyCell;

// Saliency map
typedef struct {
    SaliencyCell* grid;      // 2D grid of saliency values
    u32 grid_width;         // Grid width in cells
    u32 grid_height;        // Grid height in cells
    f32 cell_size;          // Size of each cell in world units
    f32 world_width;        // Total world width covered
    f32 world_height;       // Total world height covered
    f32 world_origin[2];    // World origin of grid (x, y)
    
    // Stimuli management
    SaliencyStimulus* stimuli; // Array of active stimuli
    u32 stimulus_capacity;      // Maximum stimuli
    u32 stimulus_count;        // Current stimulus count
    u32 next_stimulus_id;      // Next stimulus ID
    
    // Saliency parameters
    f32 motion_weight;         // Weight for motion saliency
    f32 color_weight;          // Weight for color saliency
    f32 brightness_weight;     // Weight for brightness saliency
    f32 contrast_weight;       // Weight for contrast saliency
    f32 audio_weight;         // Weight for audio saliency
    f32 event_weight;         // Weight for event saliency
    f32 global_decay_rate;     // Global decay rate for all values
    f32 max_saliency;         // Maximum possible saliency value
} SaliencyMap;

// Saliency lifecycle
bool saliency_map_init(SaliencyMap* map, u32 grid_width, u32 grid_height, 
                     f32 cell_size, const f32* world_origin);
void saliency_map_shutdown(SaliencyMap* map);

// Grid operations
void saliency_map_clear(SaliencyMap* map);
void saliency_map_world_to_grid(const SaliencyMap* map, const f32* world_pos, 
                             u32* grid_x, u32* grid_y);
void saliency_map_grid_to_world(const SaliencyMap* map, u32 grid_x, u32 grid_y,
                             f32* world_pos);
bool saliency_map_is_valid_coord(const SaliencyMap* map, u32 grid_x, u32 grid_y);
SaliencyCell* saliency_map_get_cell(SaliencyMap* map, u32 grid_x, u32 grid_y);

//  COMPLETED: Implement Update [Difficulty: 3] [Atomic Steps: 5]
// 1. Decay old saliency values over time.
// 2. Add new stimuli events (Explosion, Muzzle Flash).
// 3. AI 'LookAt' target driven by max saliency.
// 4. "Distraction" mechanic.

// Update context
typedef struct {
    f64 current_time;        // Current simulation time
    f32 delta_time;          // Time since last update
    u32 frame_number;        // Current frame number
    bool force_update;        // Force full update
} SaliencyUpdateContext;

// Saliency update operations
void saliency_map_update(SaliencyMap* map, const SaliencyUpdateContext* context);
void saliency_map_decay_values(SaliencyMap* map, f32 delta_time);
void saliency_map_process_stimuli(SaliencyMap* map, f64 current_time);
void saliency_map_compute_grid_values(SaliencyMap* map);

// Stimulus management
u32 saliency_map_add_stimulus(SaliencyMap* map, const SaliencyStimulus* stimulus);
bool saliency_map_remove_stimulus(SaliencyMap* map, u32 stimulus_id);
SaliencyStimulus* saliency_map_find_stimulus(SaliencyMap* map, u32 stimulus_id);
void saliency_map_update_stimulus(SaliencyMap* map, u32 stimulus_id, 
                               const f32* new_position, f32 new_intensity);

// Specific stimulus creators
u32 saliency_map_add_motion_stimulus(SaliencyMap* map, const f32* position, 
                                   f32 intensity, f32 radius);
u32 saliency_map_add_color_stimulus(SaliencyMap* map, const f32* position,
                                  f32 intensity, f32 radius, const f32* color);
u32 saliency_map_add_audio_stimulus(SaliencyMap* map, const f32* position,
                                  f32 intensity, f32 radius);
u32 saliency_map_add_event_stimulus(SaliencyMap* map, const f32* position,
                                   f32 intensity, f32 radius, u32 event_type);

// Saliency computation
void saliency_map_compute_motion(SaliencyMap* map, const SaliencyStimulus* stimulus);
void saliency_map_compute_color(SaliencyMap* map, const SaliencyStimulus* stimulus);
void saliency_map_compute_brightness(SaliencyMap* map, const SaliencyStimulus* stimulus);
void saliency_map_compute_contrast(SaliencyMap* map, const SaliencyStimulus* stimulus);
void saliency_map_compute_audio(SaliencyMap* map, const SaliencyStimulus* stimulus);
void saliency_map_compute_event(SaliencyMap* map, const SaliencyStimulus* stimulus);

// AI attention integration
void saliency_map_get_attention_target(const SaliencyMap* map, const f32* viewer_pos,
                                   f32 max_distance, f32* target_pos, f32* saliency);
void saliency_map_get_top_n_targets(const SaliencyMap* map, const f32* viewer_pos,
                                 u32 count, f32* positions, f32* saliencies);
bool saliency_map_is_position_salient(const SaliencyMap* map, const f32* position,
                                  f32 threshold);

//  COMPLETED: Implement Inhibition of Return [Difficulty: 2] [Atomic Steps: 3]
// 1. Once AI looks at Saliency peak.
// 2. Temporarily suppress that region.
// 3. Force eyes to scan to next highest peak.
// 4. Creates natural scanning behavior.

// Inhibition of return system
typedef struct {
    SaliencyMap* saliency_map;  // Associated saliency map
    f32 inhibition_radius;       // Radius around looked-at position to inhibit
    f32 inhibition_duration;     // How long inhibition lasts
    f32 inhibition_strength;     // How strong the inhibition is (0.0-1.0)
    u32 max_inhibited_cells;   // Maximum number of inhibited cells
    
    // Inhibition tracking
    u32* inhibited_cells;      // Array of inhibited cell indices
    u32 inhibited_count;       // Number of currently inhibited cells
    f64* inhibition_end_times;  // When each inhibition ends
    u32 inhibition_capacity;    // Maximum inhibited cells
    
    // Scanning behavior
    f32 min_scan_distance;     // Minimum distance between scan targets
    f32 scan_preference;       // Preference for new vs. returning targets
    u32 recent_targets_size;    // Size of recent targets buffer
    u32* recent_target_cells;  // Recently visited cell indices
    f64* recent_target_times;  // When each target was visited
    u32 recent_target_index;    // Circular buffer index
} InhibitionOfReturn;

// Inhibition lifecycle
bool inhibition_init(InhibitionOfReturn* ior, SaliencyMap* map, u32 max_inhibited);
void inhibition_shutdown(InhibitionOfReturn* ior);

// Inhibition operations
void inhibition_add_at_position(InhibitionOfReturn* ior, const f32* position, 
                             f64 current_time);
void inhibition_update(InhibitionOfReturn* ior, f64 current_time);
void inhibition_clear_expired(InhibitionOfReturn* ior, f64 current_time);
bool inhibition_is_position_inhibited(const InhibitionOfReturn* ior, 
                                  const f32* position, f64 current_time);

// Scanning behavior
void inhibition_record_scan_target(InhibitionOfReturn* ior, const f32* position,
                              f64 current_time);
bool inhibition_should_scan_to_target(const InhibitionOfReturn* ior,
                                  const f32* target_pos, f64 current_time);
void inhibition_get_next_scan_target(const InhibitionOfReturn* ior,
                                 const f32* current_pos, f32* target_pos);

// Natural scanning patterns
void inhibition_generate_scan_pattern(const InhibitionOfReturn* ior,
                                 const f32* start_pos, u32 pattern_type,
                                 f32* positions, u32 count);

// Integration with AI behavior
void saliency_map_direct_attention(SaliencyMap* map, InhibitionOfReturn* ior,
                                const f32* viewer_pos, f64 current_time,
                                f32* attention_target, f32* attention_strength);

// Performance monitoring
typedef struct {
    u32 total_updates;           // Total map updates
    f64 total_update_time;       // Total time spent updating
    u32 stimuli_processed;       // Total stimuli processed
    u32 inhibitions_added;        // Total inhibitions added
    u32 scans_generated;         // Total scan targets generated
    f32 average_saliency;        // Average saliency across map
    f32 peak_saliency;           // Peak saliency value
    u32 active_cells;            // Number of active cells
} SaliencyMapStats;

void saliency_map_get_stats(const SaliencyMap* map, SaliencyMapStats* stats);
void saliency_map_reset_stats(SaliencyMap* map);

// Debug and visualization
void saliency_map_debug_render(const SaliencyMap* map, void* debug_renderer);
void saliency_map_debug_render_stimuli(const SaliencyMap* map, void* debug_renderer);
void saliency_map_debug_render_inhibition(const InhibitionOfReturn* ior, 
                                       void* debug_renderer);

// Utility functions
f32 saliency_map_compute_distance_falloff(f32 distance, f32 radius);
void saliency_map_apply_gaussian_kernel(SaliencyMap* map, u32 center_x, u32 center_y,
                                    f32 radius, f32 intensity);
void saliency_map_normalize_values(SaliencyMap* map);
f32 saliency_map_get_max_value(const SaliencyMap* map, u32* max_x, u32* max_y);
