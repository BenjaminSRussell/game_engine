#ifndef UNREAL_ASSET_IMPORTER_H
#define UNREAL_ASSET_IMPORTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Unreal Engine Package File Magic Numbers
#define UNREAL_PACKAGE_MAGIC 0x9E2A83C1
#define UNREAL_PACKAGE_MAGIC_UE5 0x9E2A83C2

// Unreal Engine Versions
#define UNREAL_VERSION_427 427
#define UNREAL_VERSION_500 500
#define UNREAL_VERSION_503 503
#define UNREAL_VERSION_540 540

// Maximum limits
#define MAX_UNREAL_EXPORTS 10000
#define MAX_UNREAL_IMPORTS 10000
#define MAX_UNREAL_NAME_LENGTH 256
#define MAX_UNREAL_MESH_LODS 8

// Forward declarations
typedef struct UnrealPackage UnrealPackage;
typedef struct UnrealExportTable UnrealExportTable;
typedef struct UnrealImportTable UnrealImportTable;
typedef struct UnrealProperty UnrealProperty;
typedef struct UnrealMeshData UnrealMeshData;
typedef struct UnrealMaterialData UnrealMaterialData;
typedef struct UnrealAnimationData UnrealAnimationData;
typedef struct UnrealBlueprintData UnrealBlueprintData;
typedef struct UnrealTextureData UnrealTextureData;

/**
 * Unreal Dependency Node
 */
typedef struct {
    bool is_export;                 // True for export, false for import
    uint32_t object_index;           // Index in export/import table
    char object_name[MAX_UNREAL_NAME_LENGTH];
    char class_name[MAX_UNREAL_NAME_LENGTH];
    char package_name[MAX_UNREAL_NAME_LENGTH];
    
    uint32_t dependency_count;       // Number of dependencies
    uint32_t* dependencies;          // Array of dependency indices
    bool resolved;                   // Dependency resolution status
} UnrealDependencyNode;

/**
 * Unreal Dependency Graph
 */
typedef struct {
    uint32_t node_count;
    UnrealDependencyNode* nodes;
} UnrealDependencyGraph;

/**
 * Unreal Package File Header
 * Parsed from the beginning of .uasset files
 */
typedef struct {
    uint32_t magic;                    // 0x9E2A83C1 for UE4, 0x9E2A83C2 for UE5
    uint32_t version;                  // Engine version
    uint32_t licensee_version;         // Licensee version
    uint32_t header_size;              // Size of header
    uint32_t package_group;            // Package group
    uint32_t package_flags;            // Package flags
    uint32_t name_count;               // Number of names
    uint32_t name_offset;              // Offset to name table
    uint32_t export_count;             // Number of exports
    uint32_t export_offset;            // Offset to export table
    uint32_t import_count;             // Number of imports
    uint32_t import_offset;            // Offset to import table
    uint32_t depends_offset;           // Offset to dependency table
    uint32_t soft_package_refs_offset; // Offset to soft package references
    uint32_t searchable_names_offset;  // Offset to searchable names
    uint32_t thumbnail_table_offset;  // Offset to thumbnail table
} UnrealPackageHeader;

/**
 * Unreal Name Entry
 * Used for string storage in packages
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    uint32_t name_hash;
    uint16_t flags;
} UnrealNameEntry;

/**
 * Unreal Export Table Entry
 * Describes exported objects
 */
typedef struct {
    char class_name[MAX_UNREAL_NAME_LENGTH];
    char object_name[MAX_UNREAL_NAME_LENGTH];
    uint32_t object_flags;
    uint32_t serial_size;
    uint32_t serial_offset;
    bool is_forced_export;
    bool is_not_for_client;
    bool is_not_for_server;
    uint32_t package_guid[4];
    uint32_t export_flags;
} UnrealExportEntry;

/**
 * Unreal Import Table Entry
 * Describes imported objects
 */
typedef struct {
    char class_package[MAX_UNREAL_NAME_LENGTH];
    char class_name[MAX_UNREAL_NAME_LENGTH];
    char package_name[MAX_UNREAL_NAME_LENGTH];
    char object_name[MAX_UNREAL_NAME_LENGTH];
} UnrealImportEntry;

/**
 * Unreal Property Types
 */
typedef enum {
    UNREAL_PROPERTY_BYTE = 1,
    UNREAL_PROPERTY_INT8 = 2,
    UNREAL_PROPERTY_INT16 = 3,
    UNREAL_PROPERTY_INT = 4,
    UNREAL_PROPERTY_INT64 = 5,
    UNREAL_PROPERTY_FLOAT = 6,
    UNREAL_PROPERTY_DOUBLE = 7,
    UNREAL_PROPERTY_BOOL = 8,
    UNREAL_PROPERTY_STRING = 9,
    UNREAL_PROPERTY_NAME = 10,
    UNREAL_PROPERTY_OBJECT = 11,
    UNREAL_PROPERTY_ARRAY = 12,
    UNREAL_PROPERTY_STRUCT = 13,
    UNREAL_PROPERTY_VECTOR = 14,
    UNREAL_PROPERTY_ROTATOR = 15,
    UNREAL_PROPERTY_TRANSFORM = 16,
    UNREAL_PROPERTY_COLOR = 17,
    UNREAL_PROPERTY_LINEAR_COLOR = 18
} UnrealPropertyType;

/**
 * Unreal Property Descriptor
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    UnrealPropertyType type;
    uint32_t array_dim;
    uint32_t property_flags;
    uint64_t property_ptr;
    size_t data_offset;
    size_t data_size;
} UnrealProperty;

/**
 * Unreal Mesh Vertex
 */
typedef struct {
    float position[3];      // XYZ position
    float normal[3];        // Normal vector
    float tangent[3];       // Tangent vector
    float uv[8][2];         // Multiple UV channels
    float color[4];         // Vertex color (RGBA)
    uint8_t bone_weights[4]; // Bone influence weights
    uint8_t bone_indices[4]; // Bone influence indices
} UnrealVertex;

/**
 * Unreal Mesh Triangle
 */
typedef struct {
    uint32_t indices[3];    // Vertex indices
    uint32_t material_index; // Material index
    uint32_t smoothing_group; // Smoothing group
} UnrealTriangle;

/**
 * Unreal Mesh LOD Level
 */
typedef struct {
    uint32_t vertex_count;
    uint32_t triangle_count;
    UnrealVertex* vertices;
    UnrealTriangle* triangles;
    float screen_size;      // Screen size for LOD transition
} UnrealMeshLOD;

/**
 * Unreal Mesh Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    uint32_t lod_count;
    UnrealMeshLOD lods[MAX_UNREAL_MESH_LODS];
    float bounds_min[3];
    float bounds_max[3];
    bool has_skeleton;
    char skeleton_name[MAX_UNREAL_NAME_LENGTH];
} UnrealMeshData;

/**
 * Unreal Material Node Types
 */
typedef enum {
    UNREAL_MATERIAL_NODE_TEXTURE_SAMPLE = 1,
    UNREAL_MATERIAL_NODE_CONSTANT = 2,
    UNREAL_MATERIAL_NODE_ADD = 3,
    UNREAL_MATERIAL_NODE_MULTIPLY = 4,
    UNREAL_MATERIAL_NODE_LERP = 5,
    UNREAL_MATERIAL_NODE_NORMALIZE = 6,
    UNREAL_MATERIAL_NODE_DOT_PRODUCT = 7,
    UNREAL_MATERIAL_NODE_CROSS_PRODUCT = 8,
    UNREAL_MATERIAL_NODE_PBR = 9
} UnrealMaterialNodeType;

/**
 * Unreal Material Node
 */
typedef struct {
    UnrealMaterialNodeType type;
    char name[MAX_UNREAL_NAME_LENGTH];
    uint32_t input_count;
    uint32_t output_count;
    void* inputs;           // Array of input connections
    void* outputs;          // Array of output connections
    union {
        struct {
            char texture_name[MAX_UNREAL_NAME_LENGTH];
            uint32_t uv_index;
        } texture_sample;
        struct {
            float value[4];
        } constant;
        struct {
            float base_color[3];
            float metallic;
            float roughness;
            float specular;
        } pbr;
    } node_data;
} UnrealMaterialNode;

/**
 * Unreal Material Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    uint32_t node_count;
    UnrealMaterialNode* nodes;
    bool is_physical_material;
    char parent_material[MAX_UNREAL_NAME_LENGTH];
} UnrealMaterialData;

/**
 * Unreal Animation Keyframe
 */
typedef struct {
    float time;
    float value[3];         // For translation/scale
    float rotation[4];      // Quaternion for rotation
} UnrealAnimationKeyframe;

/**
 * Unreal Animation Track
 */
typedef struct {
    char bone_name[MAX_UNREAL_NAME_LENGTH];
    uint32_t keyframe_count;
    UnrealAnimationKeyframe* keyframes;
    bool has_translation;
    bool has_rotation;
    bool has_scale;
} UnrealAnimationTrack;

/**
 * Unreal Animation Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    char skeleton_name[MAX_UNREAL_NAME_LENGTH];
    float duration;
    float frames_per_second;
    uint32_t track_count;
    UnrealAnimationTrack* tracks;
} UnrealAnimationData;

/**
 * Unreal Blueprint Node Types
 */
typedef enum {
    UNREAL_BP_NODE_EVENT = 1,
    UNREAL_BP_NODE_FUNCTION_CALL = 2,
    UNREAL_BP_NODE_VARIABLE_GET = 3,
    UNREAL_BP_NODE_VARIABLE_SET = 4,
    UNREAL_BP_NODE_BRANCH = 5,
    UNREAL_BP_NODE_MATH = 6,
    UNREAL_BP_NODE_COMPARISON = 7,
    UNREAL_BP_NODE_CAST = 8,
    UNREAL_BP_NODE_CONSTRUCT = 9
} UnrealBlueprintNodeType;

/**
 * Unreal Blueprint Node
 */
typedef struct {
    UnrealBlueprintNodeType type;
    char name[MAX_UNREAL_NAME_LENGTH];
    char class_name[MAX_UNREAL_NAME_LENGTH];
    uint32_t input_count;
    uint32_t output_count;
    void* inputs;
    void* outputs;
    float position[2];      // Node position in graph
} UnrealBlueprintNode;

/**
 * Unreal Blueprint Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    char parent_class[MAX_UNREAL_NAME_LENGTH];
    uint32_t node_count;
    UnrealBlueprintNode* nodes;
    uint32_t variable_count;
    void* variables;        // Array of variables
    uint32_t function_count;
    void* functions;        // Array of functions
} UnrealBlueprintData;

/**
 * Unreal Texture Mipmap
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t data_size;
    uint8_t* data;          // Raw texture data (compressed)
} UnrealTextureMipmap;

/**
 * Unreal Texture Data
 */
typedef struct {
    char name[MAX_UNREAL_NAME_LENGTH];
    int32_t width;
    int32_t height;
    int32_t format;         // Pixel format (e.g., PF_DXT1, PF_DXT5, PF_B8G8R8A8)
    int32_t compression;    // Compression settings
    int32_t mipmap_settings;
    uint32_t mipmap_count;
    UnrealTextureMipmap mipmaps[16];
} UnrealTextureData;

/**
 * Unreal Package Context
 * Main structure for managing parsed package data
 */
typedef struct {
    UnrealPackageHeader header;
    UnrealNameEntry* name_table;
    UnrealExportEntry* export_table;
    UnrealImportEntry* import_table;
    uint8_t* package_data;
    size_t package_size;
    char file_path[512];
    bool is_valid;
} UnrealPackage;

// Core API Functions
UnrealPackage* Unreal_LoadPackage(const char* file_path);
void Unreal_FreePackage(UnrealPackage* package);
bool Unreal_ParsePackageHeader(UnrealPackage* package);
bool Unreal_ParseNameTable(UnrealPackage* package);
bool Unreal_ParseExportTable(UnrealPackage* package);
bool Unreal_ParseImportTable(UnrealPackage* package);

// Asset Extraction Functions
UnrealMeshData* Unreal_ExtractStaticMesh(UnrealPackage* package, const char* mesh_name);
UnrealMeshData* Unreal_ExtractSkeletalMesh(UnrealPackage* package, const char* mesh_name);
UnrealMaterialData* Unreal_ExtractMaterial(UnrealPackage* package, const char* material_name);
UnrealTextureData* Unreal_ExtractTexture(UnrealPackage* package, const char* texture_name);
UnrealAnimationData* Unreal_ExtractAnimation(UnrealPackage* package, const char* animation_name);
UnrealBlueprintData* Unreal_ExtractBlueprint(UnrealPackage* package, const char* blueprint_name);

// Dependency Resolution Functions
UnrealDependencyGraph* Unreal_ResolveDependencies(UnrealPackage* package);
bool Unreal_ResolveDependency(UnrealDependencyGraph* graph, uint32_t object_index);
bool Unreal_ValidateDependencyGraph(UnrealDependencyGraph* graph);
void Unreal_FreeDependencyGraph(UnrealDependencyGraph* graph);
bool Unreal_GetDependencyInfo(UnrealDependencyGraph* graph, uint32_t object_index, 
                            uint32_t* dependency_count, uint32_t** dependencies);

// StaticMesh Parsing Helper Functions
bool Unreal_ParseStaticMeshRenderData(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealMeshData* mesh_data);
bool Unreal_ParseStaticMeshCollisionData(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealMeshData* mesh_data);
void Unreal_CalculateMeshBounds(UnrealMeshData* mesh_data);

// SkeletalMesh Parsing Helper Functions
bool Unreal_ParseSkeletalMeshLODData(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealMeshData* mesh_data);
bool Unreal_ParseMaterialReferences(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealMeshData* mesh_data);

// Material Parsing Helper Functions
bool Unreal_ParseMaterialExpressions(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealMaterialData* material_data);

// Texture Parsing Helper Functions
bool Unreal_ParseTexturePlatformData(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealTextureData* texture_data);
void Unreal_FreeTextureData(UnrealTextureData* texture);

// Animation Parsing Helper Functions
bool Unreal_ParseAnimationRawData(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealAnimationData* animation_data);
void Unreal_FreeAnimationData(UnrealAnimationData* animation);

// Property Parsing Functions
bool Unreal_ParseSerializedProperties(UnrealPackage* package, uint8_t* data, size_t data_size, UnrealProperty** properties, uint32_t* property_count);
UnrealPropertyType Unreal_MapPropertyType(const char* type_name);

// Utility Functions
bool Unreal_ConvertCoordinateSystem(float* position, float* normal, float* tangent);
bool Unreal_ValidatePackage(UnrealPackage* package);
const char* Unreal_GetErrorString(void);

// Version Detection
uint32_t Unreal_DetectEngineVersion(UnrealPackage* package);
bool Unreal_IsVersionSupported(uint32_t version);

// Internal helper functions (for use within the module)
static uint32_t read_compressed_uint32_at_offset(uint8_t* data, size_t* offset);
static char* read_fstring_at_offset(uint8_t* data, size_t* offset);

#endif // UNREAL_ASSET_IMPORTER_H
