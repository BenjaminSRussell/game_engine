#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// BASE INTEGER TYPES
// ============================================================================

// Unsigned integers
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed integers
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Floating point
typedef float f32;
typedef double f64;

// Size type
typedef size_t usize;

// Pointer sized integer
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
typedef i64 intptr;
typedef u64 uintptr;
#else
typedef i32 intptr;
typedef u32 uintptr;
#endif

// ============================================================================
// BOOLEAN AND UTILITY
// ============================================================================

#ifndef __cplusplus
#ifndef bool
// Standard stdbool.h already handles this in modern compilers
#endif
#endif

// Utility macros
#define BIT(x) (1 << (x))
#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define GB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)

// Compiler-specific attributes
#if defined(_MSC_VER)
#define INLINE __forceinline
#define NOINLINE __declspec(noinline)
#define ALIGN(n) __declspec(align(n))
#define PACKED __pragma(pack(push, 1))
#define END_PACKED __pragma(pack(pop))
#else
#define INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define ALIGN(n) __attribute__((aligned(n)))
#define PACKED __attribute__((packed))
#define END_PACKED
#endif

// Debug/Release macros
#ifdef NDEBUG
#define ASSERT(condition) ((void)0)
#define DEBUG_ONLY(code) ((void)0)
#else
#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            __builtin_trap(); \
        } \
    } while(0)
#define DEBUG_ONLY(code) code
#endif

// ============================================================================
// SIMD-ENHANCED MATH VECTOR TYPES
// ============================================================================

// SIMD feature detection and alignment
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define UNIFIED_SIMD_SSE
    #include <immintrin.h>
    typedef __m128 simd_vec_t;
#elif defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
    #define UNIFIED_SIMD_NEON
    #include <arm_neon.h>
    typedef float32x4_t simd_vec_t;
#endif

// 2D Vector (SIMD-aligned when possible)
typedef union ALIGN(16) vec2 {
    struct { f32 x, y; };
    f32 e[2];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t simd; // Only uses x,y components
    #endif
} vec2;

// 3D Vector (SIMD-optimized with padding)
typedef union ALIGN(16) vec3 {
    struct { f32 x, y, z, _w; };
    f32 e[4];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t simd;
    #endif
} vec3;

// 4D Vector (native SIMD)
typedef union ALIGN(16) vec4 {
    struct { f32 x, y, z, w; };
    f32 e[4];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t simd;
    #endif
} vec4;

// Integer vectors
typedef struct {
    i32 x, y;
} ivec2;

typedef struct {
    i32 x, y, z;
} ivec3;

typedef struct {
    i32 x, y, z, w;
} ivec4;

// ============================================================================
// SIMD-ENHANCED MATRIX TYPES
// ============================================================================

// 3x3 Matrix (row-major, SIMD-friendly)
typedef union ALIGN(16) mat3 {
    f32 m[3][3];
    f32 e[9];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t rows[3]; // Each row as SIMD vector
    #endif
} mat3;

// 4x4 Matrix (row-major, SIMD-optimized)
typedef union ALIGN(16) mat4 {
    f32 m[4][4];
    f32 e[16];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t rows[4]; // Each row as SIMD vector
    #endif
} mat4;

// ============================================================================
// SIMD-ENHANCED QUATERNION TYPES
// ============================================================================

// Quaternion (SIMD-compatible)
typedef union ALIGN(16) quat {
    struct { f32 x, y, z, w; };
    f32 e[4];
    #if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
    simd_vec_t simd;
    #endif
} quat;

// ============================================================================
// GEOMETRY TYPES
// ============================================================================

// Axis-Aligned Bounding Box
typedef struct {
    vec3 min;
    vec3 max;
} aabb;

// Plane
typedef struct {
    vec3 normal;
    f32 d; // distance from origin
} plane;

// Frustum
typedef struct {
    plane planes[6]; // left, right, top, bottom, near, far
} frustum;

// Ray
typedef struct {
    vec3 origin;
    vec3 direction;
} ray;

// Sphere
typedef struct {
    vec3 center;
    f32 radius;
} sphere;

// ============================================================================
// TRANSFORM TYPES
// ============================================================================

// 3D Transform
typedef struct {
    vec3 position;
    quat rotation;
    vec3 scale;
} transform3d;

// 2D Transform
typedef struct {
    vec2 position;
    f32 rotation; // in radians
    vec2 scale;
} transform2d;

// ============================================================================
// COLOR TYPES
// ============================================================================

// RGBA Color (0-1 range)
typedef struct {
    f32 r, g, b, a;
} color;

// RGBA Color (0-255 range)
typedef struct {
    u8 r, g, b, a;
} color8;

// ============================================================================
// ENGINE-SPECIFIC TYPES
// ============================================================================

// Entity handle
typedef u32 EntityID;

// Component handle - use ComponentID enum from ecs/component_ids.h

// System handle
typedef u32 SystemID;

// Resource handle
typedef u32 ResourceID;

// Asset handle
typedef u32 AssetID;

// Texture handle
typedef u32 TextureID;

// Mesh handle
typedef u32 MeshID;

// Material handle
typedef u32 MaterialID;

// Shader handle
typedef u32 ShaderID;

// Animation handle
typedef u32 AnimationID;

// Audio handle
typedef u32 AudioID;

// ============================================================================
// MEMORY AND PERFORMANCE TYPES
// ============================================================================

// Memory tags - use MemoryTag enum from core/memory.h

// Time types
typedef f32 Time; // Time in seconds
typedef u64 Timestamp; // Timestamp in nanoseconds

// Frame counter
typedef u64 FrameCount;

// ============================================================================
// INPUT TYPES
// ============================================================================

// Key codes
typedef enum {
    KEY_UNKNOWN = 0,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT = 96,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,
    KEY_F13 = 302,
    KEY_F14 = 303,
    KEY_F15 = 304,
    KEY_F16 = 305,
    KEY_F17 = 306,
    KEY_F18 = 307,
    KEY_F19 = 308,
    KEY_F20 = 309,
    KEY_F21 = 310,
    KEY_F22 = 311,
    KEY_F23 = 312,
    KEY_F24 = 313,
    KEY_KP_0 = 320,
    KEY_KP_1 = 321,
    KEY_KP_2 = 322,
    KEY_KP_3 = 323,
    KEY_KP_4 = 324,
    KEY_KP_5 = 325,
    KEY_KP_6 = 326,
    KEY_KP_7 = 327,
    KEY_KP_8 = 328,
    KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330,
    KEY_KP_DIVIDE = 331,
    KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333,
    KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335,
    KEY_KP_EQUAL = 336,
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_MENU = 348
} KeyCode;

// Mouse buttons
typedef enum {
    MOUSE_BUTTON_1 = 0,
    MOUSE_BUTTON_2 = 1,
    MOUSE_BUTTON_3 = 2,
    MOUSE_BUTTON_4 = 3,
    MOUSE_BUTTON_5 = 4,
    MOUSE_BUTTON_6 = 5,
    MOUSE_BUTTON_7 = 6,
    MOUSE_BUTTON_8 = 7,
    MOUSE_BUTTON_LEFT = MOUSE_BUTTON_1,
    MOUSE_BUTTON_RIGHT = MOUSE_BUTTON_2,
    MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON_3
} MouseButton;

// ============================================================================
// RENDERING TYPES
// ============================================================================

// Vertex format
typedef struct {
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 texcoord0;
    vec2 texcoord1;
    color color;
    ivec4 bone_indices;
    vec4 bone_weights;
} vertex_t;

// Index type
typedef u32 index_t;

// Render state
typedef enum {
    RENDER_STATE_SOLID = 0,
    RENDER_STATE_WIREFRAME,
    RENDER_STATE_POINTS
} RenderState;

// Blend mode
typedef enum {
    BLEND_MODE_OPAQUE = 0,
    BLEND_MODE_ALPHA,
    BLEND_MODE_ADDITIVE,
    BLEND_MODE_MULTIPLY,
    BLEND_MODE_SCREEN
} BlendMode;

// ============================================================================
// PHYSICS TYPES
// ============================================================================

// Physics material
typedef struct {
    f32 friction;
    f32 restitution;
    f32 density;
} physics_material_t;

// Rigid body type
typedef enum {
    RIGID_BODY_STATIC = 0,
    RIGID_BODY_DYNAMIC,
    RIGID_BODY_KINEMATIC
} RigidBodyType;

// ============================================================================
// CONSTANTS
// ============================================================================

#define PI 3.14159265358979323846f
#define PI_2 1.57079632679489661923f
#define PI_4 0.78539816339744830962f
#define TWO_PI 6.28318530717958647692f
#define INV_PI 0.31830988618379067154f
#define SQRT_2 1.41421356237309504880f
#define SQRT_3 1.73205080756887729353f
#define EPSILON 1e-6f
#define DEG_TO_RAD (PI / 180.0f)
#define RAD_TO_DEG (180.0f / PI)

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Min/Max/Clamp
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) MIN(MAX((x), (min)), (max))

// Array size
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Offset of member in struct
#define OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))

// Container of (get struct from member pointer)
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - OFFSET_OF(type, member)))

// Alignment macros
#define ALIGN_UP(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#define ALIGN_DOWN(size, alignment) ((size) & ~((alignment) - 1))
#define IS_ALIGNED(size, alignment) (((size) & ((alignment) - 1)) == 0)

// ============================================================================
// ERROR AND RESULT TYPES
// ============================================================================

// Error codes and Result type - use definitions from core/utils.h

// ============================================================================
// UUID AND IDENTIFICATION TYPES
// ============================================================================

// UUID/GUID type (128-bit unique identifier)
typedef struct {
    u8 bytes[16];
} UUID;

// Resource version for versioning and validation
typedef struct {
    u32 major;
    u32 minor;
    u32 patch;
    u32 build;
} Version;

// ============================================================================
// HANDLE VALIDATION TYPES
// ============================================================================

// Handle generation counter (for versioning handles)
typedef u32 HandleGeneration;

// Versioned handle with generation to detect stale references
typedef struct {
    u32 index;
    HandleGeneration generation;
} VersionedHandle;

// Resource metadata
typedef struct {
    UUID id;
    Version version;
    u64 created_time;
    u64 modified_time;
    char name[256];
} ResourceMetadata;

// ============================================================================
// THREAD SYNCHRONIZATION TYPES
// ============================================================================

// Spinlock state
typedef enum {
    SPINLOCK_UNLOCKED = 0,
    SPINLOCK_LOCKED = 1
} SpinLockState;

// Mutex/Lock handle
typedef void* MutexHandle;

// Read-Write lock handle
typedef void* RWLockHandle;

// Semaphore handle
typedef void* SemaphoreHandle;

// Condition variable handle
typedef void* CondVarHandle;

// Thread handle
typedef void* ThreadHandle;

// ============================================================================
// NETWORK PROTOCOL TYPES
// ============================================================================

// Network address type
typedef enum {
    NET_ADDR_TYPE_INVALID = 0,
    NET_ADDR_TYPE_IPV4 = 1,
    NET_ADDR_TYPE_IPV6 = 2,
    NET_ADDR_TYPE_UNIX = 3
} NetworkAddressType;

// Network endpoint
typedef struct {
    NetworkAddressType type;
    union {
        struct {
            u8 ipv4[4];
            u16 port;
        } ipv4;
        struct {
            u8 ipv6[16];
            u16 port;
        } ipv6;
    } address;
} NetworkEndpoint;

// Packet header flags
typedef enum {
    PACKET_FLAG_COMPRESSED = (1 << 0),
    PACKET_FLAG_ENCRYPTED = (1 << 1),
    PACKET_FLAG_RELIABLE = (1 << 2),
    PACKET_FLAG_ORDERED = (1 << 3),
    PACKET_FLAG_FRAGMENTED = (1 << 4)
} PacketFlags;

// ============================================================================
// EVENT SYSTEM TYPES
// ============================================================================

// Event type
typedef enum {
    EVENT_NONE = 0,
    EVENT_WINDOW_CLOSE = 1,
    EVENT_WINDOW_RESIZE = 2,
    EVENT_KEY_PRESS = 3,
    EVENT_KEY_RELEASE = 4,
    EVENT_MOUSE_MOVE = 5,
    EVENT_MOUSE_BUTTON_PRESS = 6,
    EVENT_MOUSE_BUTTON_RELEASE = 7,
    EVENT_ENTITY_CREATED = 8,
    EVENT_ENTITY_DESTROYED = 9,
    EVENT_COMPONENT_ADDED = 10,
    EVENT_COMPONENT_REMOVED = 11,
    EVENT_COLLISION_START = 12,
    EVENT_COLLISION_END = 13,
    EVENT_GAME_STATE_CHANGED = 14,
    EVENT_PLAYER_SPAWNED = 15,
    EVENT_PLAYER_DIED = 16
} EventType;

// Event structure (extensible)
typedef struct {
    EventType type;
    f32 timestamp;
    void* data;
} Event;

// ============================================================================
// SIMD DETECTION
// ============================================================================

typedef enum {
    SIMD_NONE = 0,
    SIMD_SSE2 = 1,
    SIMD_SSE3 = 2,
    SIMD_SSE4_1 = 4,
    SIMD_SSE4_2 = 8,
    SIMD_AVX = 16,
    SIMD_AVX2 = 32,
    SIMD_NEON_FEATURE = 64
} SimdFeature;

// SIMD utility functions
static INLINE vec2 vec2_make(f32 x, f32 y) {
    vec2 v = {{x, y}};
    return v;
}

static INLINE vec3 vec3_make(f32 x, f32 y, f32 z) {
    vec3 v = {{x, y, z, 0.0f}};
    #if defined(UNIFIED_SIMD_SSE)
        v.simd = _mm_setr_ps(x, y, z, 0.0f);
    #elif defined(UNIFIED_SIMD_NEON)
        f32 data[4] = {x, y, z, 0.0f};
        v.simd = vld1q_f32(data);
    #endif
    return v;
}

static INLINE vec4 vec4_make(f32 x, f32 y, f32 z, f32 w) {
    vec4 v = {{x, y, z, w}};
    #if defined(UNIFIED_SIMD_SSE)
        v.simd = _mm_setr_ps(x, y, z, w);
    #elif defined(UNIFIED_SIMD_NEON)
        f32 data[4] = {x, y, z, w};
        v.simd = vld1q_f32(data);
    #endif
    return v;
}

static INLINE quat quat_make(f32 x, f32 y, f32 z, f32 w) {
    quat q = {{x, y, z, w}};
    #if defined(UNIFIED_SIMD_SSE)
        q.simd = _mm_setr_ps(x, y, z, w);
    #elif defined(UNIFIED_SIMD_NEON)
        f32 data[4] = {x, y, z, w};
        q.simd = vld1q_f32(data);
    #endif
    return q;
}

// SIMD vector operations (when available)
#if defined(UNIFIED_SIMD_SSE) || defined(UNIFIED_SIMD_NEON)
static INLINE vec3 vec3_add(vec3 a, vec3 b) {
    vec3 result;
    #if defined(UNIFIED_SIMD_SSE)
        result.simd = _mm_add_ps(a.simd, b.simd);
    #elif defined(UNIFIED_SIMD_NEON)
        result.simd = vaddq_f32(a.simd, b.simd);
    #endif
    return result;
}

static INLINE vec3 vec3_sub(vec3 a, vec3 b) {
    vec3 result;
    #if defined(UNIFIED_SIMD_SSE)
        result.simd = _mm_sub_ps(a.simd, b.simd);
    #elif defined(UNIFIED_SIMD_NEON)
        result.simd = vsubq_f32(a.simd, b.simd);
    #endif
    return result;
}

static INLINE f32 vec3_dot(vec3 a, vec3 b) {
    #if defined(UNIFIED_SIMD_SSE)
        __m128 mul = _mm_mul_ps(a.simd, b.simd);
        __m128 shuf = _mm_movehdup_ps(mul);
        __m128 sums = _mm_add_ps(mul, shuf);
        shuf = _mm_movehl_ps(shuf, sums);
        sums = _mm_add_ss(sums, shuf);
        return _mm_cvtss_f32(sums);
    #elif defined(UNIFIED_SIMD_NEON)
        float32x4_t mul = vmulq_f32(a.simd, b.simd);
        float32x2_t sum = vadd_f32(vget_low_f32(mul), vget_high_f32(mul));
        sum = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
    #else
        return a.x * b.x + a.y * b.y + a.z * b.z;
    #endif
}
#endif

// Get available SIMD features at runtime
SimdFeature simd_get_available_features(void);
bool simd_is_feature_supported(SimdFeature feature);

// End of types.h
