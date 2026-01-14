#ifndef DEBUG_3D_TEXT_RENDERER_H
#define DEBUG_3D_TEXT_RENDERER_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;
typedef struct Mat4 Mat4;

// Text rendering styles
typedef enum {
    TEXT_STYLE_NORMAL,
    TEXT_STYLE_BOLD,
    TEXT_STYLE_ITALIC,
    TEXT_STYLE_OUTLINE,
    TEXT_STYLE_SHADOW,
    TEXT_STYLE_GLOW
} TextStyle;

// Text alignment
typedef enum {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
} TextAlignment;

// Text anchor points
typedef enum {
    TEXT_ANCHOR_TOP_LEFT,
    TEXT_ANCHOR_TOP_CENTER,
    TEXT_ANCHOR_TOP_RIGHT,
    TEXT_ANCHOR_MIDDLE_LEFT,
    TEXT_ANCHOR_MIDDLE_CENTER,
    TEXT_ANCHOR_MIDDLE_RIGHT,
    TEXT_ANCHOR_BOTTOM_LEFT,
    TEXT_ANCHOR_BOTTOM_CENTER,
    TEXT_ANCHOR_BOTTOM_RIGHT
} TextAnchor;

// Font metrics
typedef struct {
    u32 font_size;
    float line_height;
    float character_width;
    float character_height;
    u32 atlas_width;
    u32 atlas_height;
    u32 characters_per_row;
    u32 character_count;
} FontMetrics;

// Text rendering properties
typedef struct {
    char text[512];
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Vec4 color;
    Vec4 outline_color;
    Vec4 shadow_color;
    Vec4 glow_color;
    
    FontMetrics* font;
    TextStyle style;
    TextAlignment alignment;
    TextAnchor anchor;
    
    float outline_width;
    float shadow_offset_x;
    float shadow_offset_y;
    float glow_intensity;
    
    bool visible;
    bool billboard;
    bool depth_test;
    bool face_camera;
    
    u32 layer; // Rendering layer
    float alpha;
} Text3D;

// Text batch for efficient rendering
typedef struct {
    Text3D* texts;
    u32 text_count;
    u32 capacity;
    
    // Rendering state
    bool needs_update;
    u32 vertex_buffer;
    u32 index_buffer;
    u32 texture_atlas;
    
    // Bounding box for culling
    Vec3 min_bounds;
    Vec3 max_bounds;
} TextBatch;

// 3D Text renderer
typedef struct {
    // Font management
    FontMetrics* fonts;
    u32 font_count;
    u32 font_capacity;
    
    // Text batches
    TextBatch* batches;
    u32 batch_count;
    u32 batch_capacity;
    
    // Rendering resources
    u32 shader_program;
    u32 vertex_array;
    u32 texture_atlas;
    
    // Default font
    FontMetrics* default_font;
    
    // Global settings
    bool enable_depth_testing;
    bool enable_face_camera;
    float global_alpha;
    Vec4 global_color;
    
    // Performance tracking
    u32 rendered_texts_last_frame;
    u32 total_vertices;
    u32 total_indices;
    
    // Callbacks
    void (*on_text_created)(Text3D* text);
    void (*on_text_destroyed)(Text3D* text);
    void (*on_font_loaded)(FontMetrics* font);
    
    void* user_data;
} Debug3DTextRenderer;

// MARK: - 3D Text Renderer Management

bool debug_text_renderer_init(Debug3DTextRenderer* renderer, u32 max_fonts, u32 max_batches);
void debug_text_renderer_shutdown(Debug3DTextRenderer* renderer);

// MARK: - Font Management

FontMetrics* debug_text_load_font(Debug3DTextRenderer* renderer, const char* font_path, u32 font_size);
FontMetrics* debug_text_load_system_font(Debug3DTextRenderer* renderer, const char* font_name, u32 font_size);
bool debug_text_unload_font(Debug3DTextRenderer* renderer, FontMetrics* font);
FontMetrics* debug_text_get_font(Debug3DTextRenderer* renderer, const char* font_name, u32 font_size);

// MARK: - Text Creation and Management

Text3D* debug_text_create(Debug3DTextRenderer* renderer, const char* text, const Vec3* position, 
                         FontMetrics* font);
bool debug_text_destroy(Debug3DTextRenderer* renderer, Text3D* text);
bool debug_text_update_text(Debug3DTextRenderer* renderer, Text3D* text, const char* new_text);
bool debug_text_set_position(Debug3DTextRenderer* renderer, Text3D* text, const Vec3* position);
bool debug_text_set_color(Debug3DTextRenderer* renderer, Text3D* text, const Vec4* color);
bool debug_text_set_visible(Debug3DTextRenderer* renderer, Text3D* text, bool visible);

// MARK: - Text Styling

bool debug_text_set_style(Debug3DTextRenderer* renderer, Text3D* text, TextStyle style);
bool debug_text_set_alignment(Debug3DTextRenderer* renderer, Text3D* text, TextAlignment alignment);
bool debug_text_set_anchor(Debug3DTextRenderer* renderer, Text3D* text, TextAnchor anchor);
bool debug_text_set_outline(Debug3DTextRenderer* renderer, Text3D* text, float width, const Vec4* color);
bool debug_text_set_shadow(Debug3DTextRenderer* renderer, Text3D* text, float offset_x, float offset_y, const Vec4* color);
bool debug_text_set_glow(Debug3DTextRenderer* renderer, Text3D* text, float intensity, const Vec4* color);

// MARK: - Text Batching

TextBatch* debug_text_create_batch(Debug3DTextRenderer* renderer, FontMetrics* font, u32 capacity);
bool debug_text_add_to_batch(Debug3DTextRenderer* renderer, TextBatch* batch, Text3D* text);
bool debug_text_remove_from_batch(Debug3DTextRenderer* renderer, TextBatch* batch, Text3D* text);
bool debug_text_update_batch(Debug3DTextRenderer* renderer, TextBatch* batch);

// MARK: - Rendering

void debug_text_render(Debug3DTextRenderer* renderer, const Mat4* view_matrix, const Mat4* projection_matrix);
void debug_text_render_batch(Debug3DTextRenderer* renderer, TextBatch* batch, const Mat4* view_matrix, const Mat4* projection_matrix);
void debug_text_render_text(Debug3DTextRenderer* renderer, Text3D* text, const Mat4* view_matrix, const Mat4* projection_matrix);

// MARK: - Utility Functions

void debug_text_set_global_settings(Debug3DTextRenderer* renderer, bool depth_test, bool face_camera, float alpha);
void debug_text_set_default_font(Debug3DTextRenderer* renderer, FontMetrics* font);

void debug_text_get_text_bounds(Text3D* text, Vec3* min_bounds, Vec3* max_bounds);
bool debug_text_is_visible(Debug3DTextRenderer* renderer, Text3D* text, const Mat4* view_projection_matrix);

// MARK: - Debug Drawing Helpers

Text3D* debug_text_draw_vector(Debug3DTextRenderer* renderer, const Vec3* start, const Vec3* end, const char* label);
Text3D* debug_text_draw_point(Debug3DTextRenderer* renderer, const Vec3* position, const char* label);
Text3D* debug_text_draw_coordinate_system(Debug3DTextRenderer* renderer, const Vec3* position, float scale);
Text3D* debug_text_draw_bounding_box(Debug3DTextRenderer* renderer, const Vec3* min_bounds, const Vec3* max_bounds, const char* label);

// MARK: - Performance and Statistics

void debug_text_get_statistics(Debug3DTextRenderer* renderer, u32* total_texts, u32* visible_texts, 
                              u32* total_vertices, u32* total_batches);
void debug_text_print_statistics(Debug3DTextRenderer* renderer);

// MARK: - Configuration

void debug_text_set_callbacks(Debug3DTextRenderer* renderer,
                             void (*on_created)(Text3D*),
                             void (*on_destroyed)(Text3D*),
                             void (*on_font_loaded)(FontMetrics*));

// MARK: - Platform-specific implementations

bool platform_load_font_atlas(const char* font_path, u32 font_size, u8** atlas_data, u32* atlas_width, u32* atlas_height);
bool platform_load_system_font(const char* font_name, u32 font_size, u8** atlas_data, u32* atlas_width, u32* atlas_height);
bool platform_get_character_metrics(const char* font_path, u32 font_size, char character, float* width, float* height, float* advance);

// MARK: - Debug Macros

#define DEBUG_TEXT_ENABLED 1

#if DEBUG_TEXT_ENABLED
    #define DEBUG_TEXT_CREATE(renderer, text, pos, font) debug_text_create(renderer, text, pos, font)
    #define DEBUG_TEXT_DRAW_VECTOR(renderer, start, end, label) debug_text_draw_vector(renderer, start, end, label)
    #define DEBUG_TEXT_DRAW_POINT(renderer, pos, label) debug_text_draw_point(renderer, pos, label)
    #define DEBUG_TEXT_DRAW_COORDS(renderer, pos, scale) debug_text_draw_coordinate_system(renderer, pos, scale)
#else
    #define DEBUG_TEXT_CREATE(renderer, text, pos, font) NULL
    #define DEBUG_TEXT_DRAW_VECTOR(renderer, start, end, label) NULL
    #define DEBUG_TEXT_DRAW_POINT(renderer, pos, label) NULL
    #define DEBUG_TEXT_DRAW_COORDS(renderer, pos, scale) NULL
#endif

// Global debug text renderer instance
extern Debug3DTextRenderer* g_debug_text_renderer;

#endif // DEBUG_3D_TEXT_RENDERER_H
