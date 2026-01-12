#ifndef DEBUG_BVH_VISUALIZATION_H
#define DEBUG_BVH_VISUALIZATION_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;
typedef struct Mat4 Mat4;

// BVH node types
typedef enum {
    BVH_NODE_LEAF,
    BVH_NODE_INTERNAL
} BVHNodeType;

// BVH split strategies
typedef enum {
    BVH_SPLIT_MIDDLE,
    BVH_SPLIT_MEDIAN,
    BVH_SPLIT_SAH, // Surface Area Heuristic
    BVH_SPLIT_OBJECT_MEDIAN,
    BVH_SPLIT_SPATIAL_MEDIAN
} BVHSplitStrategy;

// Axis-aligned bounding box
typedef struct {
    Vec3 min;
    Vec3 max;
} BoundingBox;

// Triangle primitive for BVH
typedef struct {
    Vec3 vertices[3];
    Vec3 normal;
    u32 material_id;
    u32 triangle_id;
} Triangle;

// BVH node
typedef struct BVHNode {
    BoundingBox bounds;
    BVHNodeType type;
    
    union {
        struct {
            struct BVHNode* left;
            struct BVHNode* right;
            u32 split_axis;
        } internal;
        
        struct {
            Triangle* triangles;
            u32 triangle_count;
            u32 first_triangle_index;
        } leaf;
    } data;
    
    // Debug and visualization data
    u32 node_id;
    u32 depth;
    u32 subtree_triangle_count;
    float surface_area;
    Vec4 debug_color;
    bool is_visible;
    bool is_highlighted;
    
    // Performance metrics
    u64 build_time_ns;
    u64 traversal_count;
    u64 intersection_count;
    
} BVHNode;

// BVH tree
typedef struct {
    BVHNode* root;
    BVHNode* nodes;
    u32 node_count;
    u32 node_capacity;
    u32 max_depth;
    
    // Triangle data
    Triangle* triangles;
    u32 triangle_count;
    u32 triangle_capacity;
    
    // Build parameters
    BVHSplitStrategy split_strategy;
    u32 max_triangles_per_leaf;
    u32 max_depth_limit;
    float sah_traversal_cost;
    float sah_intersection_cost;
    
    // Statistics
    u64 total_build_time_ns;
    u64 total_traversal_count;
    u64 total_intersection_count;
    float average_depth;
    float leaf_utilization;
    
    // Visualization settings
    bool show_bounds;
    bool show_hierarchy;
    bool show_leaf_nodes;
    bool show_internal_nodes;
    bool color_by_depth;
    bool color_by_triangle_count;
    Vec4 leaf_color;
    Vec4 internal_color;
    Vec4 highlight_color;
    
} BVHTree;

// BVH traversal stack for ray tracing
typedef struct {
    BVHNode** nodes;
    u32 count;
    u32 capacity;
} BVHTraversalStack;

// Ray structure for intersection testing
typedef struct {
    Vec3 origin;
    Vec3 direction;
    float t_min;
    float t_max;
} Ray;

// Ray-triangle intersection result
typedef struct {
    bool hit;
    float t;
    Vec3 hit_point;
    Vec3 normal;
    Vec2 barycentric;
    u32 triangle_id;
    u32 material_id;
} RayTriangleIntersection;

// BVH debug visualization system
typedef struct {
    BVHTree* bvh_trees;
    u32 tree_count;
    u32 tree_capacity;
    
    // Visualization resources
    u32 bounding_box_shader;
    u32 line_vertex_buffer;
    u32 line_index_buffer;
    u32 triangle_vertex_buffer;
    u32 triangle_index_buffer;
    
    // Debug rendering settings
    bool wireframe_mode;
    bool show_node_ids;
    bool show_statistics;
    bool animate_traversal;
    float line_width;
    Vec4 background_color;
    
    // Traversal animation
    BVHNode* current_traversal_node;
    u32 traversal_step;
    u64 traversal_start_time_ms;
    float traversal_speed;
    
    // Performance tracking
    u64 last_render_time_ms;
    u32 rendered_nodes_last_frame;
    u32 rendered_triangles_last_frame;
    
    // Callbacks
    void (*on_node_selected)(BVHNode* node);
    void (*on_tree_built)(BVHTree* tree);
    void (*on_traversal_completed)(u64 traversal_time_ms);
    
    void* user_data;
} DebugBVHVisualization;

// MARK: - BVH Tree Management

bool bvh_tree_init(BVHTree* tree, u32 max_triangles, u32 max_nodes);
void bvh_tree_shutdown(BVHTree* tree);

bool bvh_tree_build(BVHTree* tree, Triangle* triangles, u32 triangle_count);
bool bvh_tree_build_recursive(BVHTree* tree, BVHNode* node, Triangle* triangles, u32 triangle_count, u32 depth);
void bvh_tree_clear(BVHTree* tree);

// MARK: - BVH Node Operations

BVHNode* bvh_node_create(BVHTree* tree, BVHNodeType type);
void bvh_node_destroy(BVHNode* node);
void bvh_node_calculate_bounds(BVHNode* node, Triangle* triangles, u32 triangle_count);
void bvh_node_merge_bounds(BVHNode* parent, BVHNode* child1, BVHNode* child2);

// MARK: - BVH Splitting Strategies

u32 bvh_split_middle(BVHTree* tree, Triangle* triangles, u32 triangle_count, u32 axis);
u32 bvh_split_median(BVHTree* tree, Triangle* triangles, u32 triangle_count, u32 axis);
u32 bvh_split_sah(BVHTree* tree, Triangle* triangles, u32 triangle_count, u32 axis);
u32 bvh_split_object_median(BVHTree* tree, Triangle* triangles, u32 triangle_count, u32 axis);
u32 bvh_split_spatial_median(BVHTree* tree, Triangle* triangles, u32 triangle_count, u32 axis);

// MARK: - BVH Traversal and Intersection

bool bvh_tree_intersect_ray(BVHTree* tree, const Ray* ray, RayTriangleIntersection* result);
bool bvh_node_intersect_ray(BVHNode* node, const Ray* ray, RayTriangleIntersection* result);
bool bvh_intersect_ray_triangle(const Ray* ray, const Triangle* triangle, RayTriangleIntersection* result);

bool bvh_tree_intersect_aabb(BVHTree* tree, const BoundingBox* box, Triangle** results, u32* result_count);
bool bvh_node_intersect_aabb(BVHNode* node, const BoundingBox* box, Triangle** results, u32* result_count);
bool bvh_intersect_aabb_triangle(const BoundingBox* box, const Triangle* triangle);

// MARK: - BVH Statistics and Analysis

void bvh_tree_calculate_statistics(BVHTree* tree);
void bvh_tree_print_statistics(BVHTree* tree);
void bvh_tree_print_node(BVHNode* node, u32 depth);
void bvh_tree_validate(BVHTree* tree);

float bvh_calculate_surface_area(const BoundingBox* box);
float bvh_calculate_union_surface_area(const BoundingBox* box1, const BoundingBox* box2);
void bvh_merge_boxes(const BoundingBox* box1, const BoundingBox* box2, BoundingBox* result);

// MARK: - Debug Visualization System

bool debug_bvh_visualization_init(DebugBVHVisualization* viz, u32 max_trees);
void debug_bvh_visualization_shutdown(DebugBVHVisualization* viz);

BVHTree* debug_bvh_add_tree(DebugBVHVisualization* viz, u32 max_triangles);
bool debug_bvh_remove_tree(DebugBVHVisualization* viz, BVHTree* tree);
bool debug_bvh_build_tree(DebugBVHVisualization* viz, BVHTree* tree, Triangle* triangles, u32 triangle_count);

// MARK: - BVH Visualization Rendering

void debug_bvh_render(DebugBVHVisualization* viz, const Mat4* view_matrix, const Mat4* projection_matrix);
void debug_bvh_render_tree(DebugBVHVisualization* viz, BVHTree* tree, const Mat4* view_matrix, const Mat4* projection_matrix);
void debug_bvh_render_node(DebugBVHVisualization* viz, BVHNode* node, const Mat4* view_matrix, const Mat4* projection_matrix);

void debug_bvh_render_bounding_box(DebugBVHVisualization* viz, const BoundingBox* box, const Vec4* color, const Mat4* model_matrix);
void debug_bvh_render_triangle(DebugBVHVisualization* viz, const Triangle* triangle, const Vec4* color, const Mat4* model_matrix);

// MARK: - BVH Debug Tools

void debug_bvh_highlight_node(DebugBVHVisualization* viz, BVHNode* node, bool highlight);
void debug_bvh_show_node_path(DebugBVHVisualization* viz, BVHNode* node);
void debug_bvh_animate_ray_traversal(DebugBVHVisualization* viz, BVHTree* tree, const Ray* ray);

BVHNode* debug_bvh_find_node_at_position(DebugBVHVisualization* viz, BVHTree* tree, const Vec3* position);
BVHNode* debug_bvh_find_node_by_id(DebugBVHVisualization* viz, BVHTree* tree, u32 node_id);

// MARK: - BVH Export and Import

bool debug_bvh_export_to_json(DebugBVHVisualization* viz, BVHTree* tree, const char* filename);
bool debug_bvh_import_from_json(DebugBVHVisualization* viz, const char* filename, BVHTree** tree);
bool debug_bvh_export_to_obj(DebugBVHVisualization* viz, BVHTree* tree, const char* filename);

// MARK: - BVH Performance Testing

void debug_bvh_performance_test(DebugBVHVisualization* viz, BVHTree* tree, u32 ray_count);
void debug_bvh_traversal_animation(DebugBVHVisualization* viz, BVHTree* tree, const Ray* ray);

// MARK: - Utility Functions

void debug_bvh_set_visualization_settings(DebugBVHVisualization* viz, bool show_bounds, bool show_hierarchy, 
                                         bool wireframe, float line_width);
void debug_bvh_set_color_scheme(DebugBVHVisualization* viz, bool color_by_depth, bool color_by_triangle_count,
                               const Vec4* leaf_color, const Vec4* internal_color);

void debug_bvh_get_statistics(DebugBVHVisualization* viz, u32* total_nodes, u32* leaf_nodes, 
                             u32* internal_nodes, u32* total_triangles);
void debug_bvh_print_statistics(DebugBVHVisualization* viz);

// MARK: - Configuration

void debug_bvh_set_callbacks(DebugBVHVisualization* viz,
                            void (*on_selected)(BVHNode*),
                            void (*on_built)(BVHTree*),
                            void (*on_traversal)(u64));

// MARK: - Debug Macros

#define DEBUG_BVH_ENABLED 1

#if DEBUG_BVH_ENABLED
    #define DEBUG_BVH_BUILD_TREE(viz, tree, triangles, count) debug_bvh_build_tree(viz, tree, triangles, count)
    #define DEBUG_BVH_RENDER(viz, view, proj) debug_bvh_render(viz, view, proj)
    #define DEBUG_BVH_INTERSECT_RAY(tree, ray, result) bvh_tree_intersect_ray(tree, ray, result)
#else
    #define DEBUG_BVH_BUILD_TREE(viz, tree, triangles, count) false
    #define DEBUG_BVH_RENDER(viz, view, proj)
    #define DEBUG_BVH_INTERSECT_RAY(tree, ray, result) false
#endif

// Global debug BVH visualization instance
extern DebugBVHVisualization* g_debug_bvh_visualization;

#endif // DEBUG_BVH_VISUALIZATION_H
