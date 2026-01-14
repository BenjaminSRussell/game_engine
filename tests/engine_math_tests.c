#include "engine_coverage_test_framework.h"
#include <math.h>

// Vector math tests
typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y, z, w;
} Vec4;

// Vector operations
static Vec2 vec2_create(float x, float y) {
    Vec2 v = {x, y};
    return v;
}

static Vec3 vec3_create(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static Vec4 vec4_create(float x, float y, float z, float w) {
    Vec4 v = {x, y, z, w};
    return v;
}

static Vec2 vec2_add(Vec2 a, Vec2 b) {
    return vec2_create(a.x + b.x, a.y + b.y);
}

static Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vec4 vec4_add(Vec4 a, Vec4 b) {
    return vec4_create(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

static Vec2 vec2_subtract(Vec2 a, Vec2 b) {
    return vec2_create(a.x - b.x, a.y - b.y);
}

static Vec3 vec3_subtract(Vec3 a, Vec3 b) {
    return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vec4 vec4_subtract(Vec4 a, Vec4 b) {
    return vec4_create(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

static Vec2 vec2_multiply(Vec2 v, float scalar) {
    return vec2_create(v.x * scalar, v.y * scalar);
}

static Vec3 vec3_multiply(Vec3 v, float scalar) {
    return vec3_create(v.x * scalar, v.y * scalar, v.z * scalar);
}

static Vec4 vec4_multiply(Vec4 v, float scalar) {
    return vec4_create(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
}

static float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static float vec2_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

static float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static float vec4_length(Vec4 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

static Vec2 vec2_normalize(Vec2 v) {
    float length = vec2_length(v);
    if (length > 0.0f) {
        return vec2_create(v.x / length, v.y / length);
    }
    return vec2_create(0.0f, 0.0f);
}

static Vec3 vec3_normalize(Vec3 v) {
    float length = vec3_length(v);
    if (length > 0.0f) {
        return vec3_create(v.x / length, v.y / length, v.z / length);
    }
    return vec3_create(0.0f, 0.0f, 0.0f);
}

static Vec4 vec4_normalize(Vec4 v) {
    float length = vec4_length(v);
    if (length > 0.0f) {
        return vec4_create(v.x / length, v.y / length, v.z / length, v.w / length);
    }
    return vec4_create(0.0f, 0.0f, 0.0f, 0.0f);
}

static Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return vec3_create(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// Matrix operations (simplified 4x4)
typedef struct {
    float m[16];
} Mat4;

static Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

static Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    return result;
}

static Vec4 mat4_multiply_vec4(Mat4 m, Vec4 v) {
    return vec4_create(
        m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z + m.m[3] * v.w,
        m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z + m.m[7] * v.w,
        m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z + m.m[11] * v.w,
        m.m[12] * v.x + m.m[13] * v.y + m.m[14] * v.z + m.m[15] * v.w
    );
}

// Math tests
static bool test_vec2_creation(void) {
    Vec2 v = vec2_create(1.0f, 2.0f);
    ASSERT_FLOAT_EQ(1.0f, v.x, 0.0001f);
    ASSERT_FLOAT_EQ(2.0f, v.y, 0.0001f);
    return true;
}

static bool test_vec3_creation(void) {
    Vec3 v = vec3_create(1.0f, 2.0f, 3.0f);
    ASSERT_FLOAT_EQ(1.0f, v.x, 0.0001f);
    ASSERT_FLOAT_EQ(2.0f, v.y, 0.0001f);
    ASSERT_FLOAT_EQ(3.0f, v.z);
    return true;
}

static bool test_vec4_creation(void) {
    Vec4 v = vec4_create(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_FLOAT_EQ(1.0f, v.x, 0.0001f);
    ASSERT_FLOAT_EQ(2.0f, v.y, 0.0001f);
    ASSERT_FLOAT_EQ(3.0f, v.z, 0.0001f);
    ASSERT_FLOAT_EQ(4.0f, v.w, 0.0001f);
    return true;
}

static bool test_vec2_addition(void) {
    Vec2 a = vec2_create(1.0f, 2.0f);
    Vec2 b = vec2_create(3.0f, 4.0f);
    Vec2 result = vec2_add(a, b);
    
    ASSERT_FLOAT_EQ(4.0f, result.x, 0.0001f);
    ASSERT_FLOAT_EQ(6.0f, result.y, 0.0001f);
    return true;
}

static bool test_vec3_addition(void) {
    Vec3 a = vec3_create(1.0f, 2.0f, 3.0f);
    Vec3 b = vec3_create(4.0f, 5.0f, 6.0f);
    Vec3 result = vec3_add(a, b);
    
    ASSERT_FLOAT_EQ(5.0f, result.x, 0.0001f);
    ASSERT_FLOAT_EQ(7.0f, result.y, 0.0001f);
    ASSERT_FLOAT_EQ(9.0f, result.z, 0.0001f);
    return true;
}

static bool test_vec4_addition(void) {
    Vec4 a = vec4_create(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 b = vec4_create(5.0f, 6.0f, 7.0f, 8.0f);
    Vec4 result = vec4_add(a, b);
    
    ASSERT_FLOAT_EQ(6.0f, result.x, 0.0001f);
    ASSERT_FLOAT_EQ(8.0f, result.y, 0.0001f);
    ASSERT_FLOAT_EQ(10.0f, result.z, 0.0001f);
    ASSERT_FLOAT_EQ(12.0f, result.w, 0.0001f);
    return true;
}

static bool test_vec2_subtraction(void) {
    Vec2 a = vec2_create(5.0f, 8.0f);
    Vec2 b = vec2_create(2.0f, 3.0f);
    Vec2 result = vec2_subtract(a, b);
    
    ASSERT_FLOAT_EQ(3.0f, result.x, 0.0001f);
    ASSERT_FLOAT_EQ(5.0f, result.y, 0.0001f);
    return true;
}

static bool test_vec3_subtraction(void) {
    Vec3 a = vec3_create(5.0f, 8.0f, 11.0f);
    Vec3 b = vec3_create(2.0f, 3.0f, 4.0f);
    Vec3 result = vec3_subtract(a, b);
    
    ASSERT_FLOAT_EQ(3.0f, result.x, 0.0001f);
    ASSERT_FLOAT_EQ(5.0f, result.y, 0.0001f);
    ASSERT_FLOAT_EQ(7.0f, result.z, 0.0001f);
    return true;
}

static bool test_vector_multiplication(void) {
    Vec2 v2 = vec2_create(2.0f, 3.0f);
    Vec2 result2 = vec2_multiply(v2, 2.0f);
    ASSERT_FLOAT_EQ(4.0f, result2.x, 0.0001f);
    ASSERT_FLOAT_EQ(6.0f, result2.y, 0.0001f);
    
    Vec3 v3 = vec3_create(2.0f, 3.0f, 4.0f);
    Vec3 result3 = vec3_multiply(v3, 2.0f);
    ASSERT_FLOAT_EQ(4.0f, result3.x, 0.0001f);
    ASSERT_FLOAT_EQ(6.0f, result3.y, 0.0001f);
    ASSERT_FLOAT_EQ(8.0f, result3.z, 0.0001f);
    
    Vec4 v4 = vec4_create(2.0f, 3.0f, 4.0f, 5.0f);
    Vec4 result4 = vec4_multiply(v4, 2.0f);
    ASSERT_FLOAT_EQ(4.0f, result4.x, 0.0001f);
    ASSERT_FLOAT_EQ(6.0f, result4.y, 0.0001f);
    ASSERT_FLOAT_EQ(8.0f, result4.z, 0.0001f);
    ASSERT_FLOAT_EQ(10.0f, result4.w, 0.0001f);
    ASSERT_FLOAT_EQ(8.0f, result4.z);
    ASSERT_FLOAT_EQ(10.0f, result4.w);
    
    return true;
}

static bool test_vector_dot_product(void) {
    Vec2 a2 = vec2_create(1.0f, 2.0f);
    Vec2 b2 = vec2_create(3.0f, 4.0f);
    float dot2 = vec2_dot(a2, b2);
    ASSERT_FLOAT_EQ(11.0f, dot2, 0.0001f); // 1*3 + 2*4 = 11
    
    Vec3 a3 = vec3_create(1.0f, 2.0f, 3.0f);
    Vec3 b3 = vec3_create(4.0f, 5.0f, 6.0f);
    float dot3 = vec3_dot(a3, b3);
    ASSERT_FLOAT_EQ(32.0f, dot3, 0.0001f); // 1*4 + 2*5 + 3*6 = 32 // 1*4 + 2*5 + 3*6 = 32
    
    return true;
}

static bool test_vector_length(void) {
    Vec2 v2 = vec2_create(3.0f, 4.0f);
    float len2 = vec2_length(v2);
    ASSERT_FLOAT_EQ(5.0f, len2, 0.0001f); // sqrt(3^2 + 4^2) = 5
    
    Vec3 v3 = vec3_create(1.0f, 2.0f, 2.0f);
    float len3 = vec3_length(v3);
    ASSERT_FLOAT_EQ(3.0f, len3, 0.0001f); // sqrt(1^2 + 2^2 + 2^2) = 3
    
    return true;
}

static bool test_vector_normalization(void) {
    Vec2 v2 = vec2_create(3.0f, 4.0f);
    Vec2 norm2 = vec2_normalize(v2);
    ASSERT_FLOAT_EQ(0.6f, norm2.x, 0.0001f);
    ASSERT_FLOAT_EQ(0.8f, norm2.y, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, vec2_length(norm2), 0.0001f);
    
    Vec3 v3 = vec3_create(1.0f, 2.0f, 2.0f);
    Vec3 norm3 = vec3_normalize(v3);
    ASSERT_FLOAT_EQ(1.0f/3.0f, norm3.x, 0.0001f);
    ASSERT_FLOAT_EQ(2.0f/3.0f, norm3.y, 0.0001f);
    ASSERT_FLOAT_EQ(2.0f/3.0f, norm3.z, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, vec3_length(norm3), 0.0001f);
    
    // Test zero vector normalization
    Vec2 zero2 = vec2_create(0.0f, 0.0f);
    Vec2 norm_zero2 = vec2_normalize(zero2);
    ASSERT_FLOAT_EQ(0.0f, norm_zero2.x);
    ASSERT_FLOAT_EQ(0.0f, norm_zero2.y);
    
    return true;
}

static bool test_vector_cross_product(void) {
    Vec3 a = vec3_create(1.0f, 0.0f, 0.0f);
    Vec3 b = vec3_create(0.0f, 1.0f, 0.0f);
    Vec3 cross = vec3_cross(a, b);
    
    ASSERT_FLOAT_EQ(0.0f, cross.x, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, cross.y, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, cross.z, 0.0001f);
    
    // Cross product is anti-commutative
    Vec3 cross_reverse = vec3_cross(b, a);
    ASSERT_FLOAT_EQ(0.0f, cross_reverse.x, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, cross_reverse.y, 0.0001f);
    ASSERT_FLOAT_EQ(-1.0f, cross_reverse.z, 0.0001f);
    
    return true;
}

static bool test_matrix_identity(void) {
    Mat4 identity = mat4_identity();
    
    // Check diagonal elements
    ASSERT_FLOAT_EQ(1.0f, identity.m[0]);
    ASSERT_FLOAT_EQ(1.0f, identity.m[5]);
    ASSERT_FLOAT_EQ(1.0f, identity.m[10]);
    ASSERT_FLOAT_EQ(1.0f, identity.m[15]);
    
    // Check off-diagonal elements
    for (int i = 0; i < 16; i++) {
        if (i != 0 && i != 5 && i != 10 && i != 15) {
            ASSERT_FLOAT_EQ(0.0f, identity.m[i]);
        }
    }
    
    return true;
}

static bool test_matrix_multiplication(void) {
    Mat4 identity = mat4_identity();
    Mat4 result = mat4_multiply(identity, identity);
    
    // Identity * Identity = Identity
    for (int i = 0; i < 16; i++) {
        ASSERT_FLOAT_EQ(identity.m[i], result.m[i], 0.0001f);
    }
    
    return true;
}

static bool test_matrix_vector_multiplication(void) {
    Mat4 identity = mat4_identity();
    Vec4 v = vec4_create(1.0f, 2.0f, 3.0f, 4.0f);
    Vec4 result = mat4_multiply_vec4(identity, v);
    
    // Identity * vector = vector
    ASSERT_FLOAT_EQ(v.x, result.x);
    ASSERT_FLOAT_EQ(v.y, result.y);
    ASSERT_FLOAT_EQ(v.z, result.z);
    ASSERT_FLOAT_EQ(v.w, result.w);
    
    return true;
}

static bool test_math_constants(void) {
    // Test common mathematical constants
    ASSERT_FLOAT_EQ(M_PI, 3.14159265358979323846, 0.0001f);
    ASSERT_FLOAT_EQ(M_PI_2, M_PI / 2.0f, 0.0001f);
    ASSERT_FLOAT_EQ(M_PI_4, M_PI / 4.0f, 0.0001f);
    
    return true;
}

static bool test_trigonometric_functions(void) {
    // Test sine and cosine
    ASSERT_FLOAT_EQ(sinf(0.0f), 0.0f, 0.0001f);
    ASSERT_FLOAT_EQ(cosf(0.0f), 1.0f, 0.0001f);
    ASSERT_FLOAT_EQ(sinf(M_PI_2), 1.0f, 0.0001f);
    ASSERT_FLOAT_EQ(cosf(M_PI_2), 0.0f, 0.0001f);
    
    // Test tangent
    ASSERT_FLOAT_EQ(tanf(0.0f), 0.0f, 0.0001f);
    
    return true;
}

static bool test_math_performance(void) {
    const int iterations = 100000;
    uint64_t start_time, end_time, total_time;
    
    // Test vector operations performance
    Vec3 vectors[1000];
    for (int i = 0; i < 1000; i++) {
        vectors[i] = vec3_create((float)i, (float)(i * 2), (float)(i * 3));
    }
    
    MEASURE_PERFORMANCE_START();
    for (int i = 0; i < iterations; i++) {
        Vec3 result = vec3_add(vectors[i % 1000], vectors[(i + 1) % 1000]);
        result = vec3_multiply(result, 0.5f);
        float length = vec3_length(result);
        (void)length; // Avoid unused variable warning
    }
    MEASURE_PERFORMANCE_END(&total_time);
    
    double vector_time_ms = (double)total_time / 1000000.0;
    log_test_message("INFO", "MATH", "Performed %d vector operations in %.2f ms", 
                    iterations, vector_time_ms);
    
    return true;
}

// Main math test suite
TestSuite engine_run_math_tests(void) {
    TestSuite suite = {0};
    strcpy(suite.suite_name, "Math Tests");
    
    const int test_count = 16;
    suite.tests = MALLOC(sizeof(TestResult) * test_count);
    suite.test_count = test_count;
    
    TestCase tests[] = {
        {"Vec2 Creation", test_vec2_creation, "Test Vec2 creation"},
        {"Vec3 Creation", test_vec3_creation, "Test Vec3 creation"},
        {"Vec4 Creation", test_vec4_creation, "Test Vec4 creation"},
        {"Vec2 Addition", test_vec2_addition, "Test Vec2 addition"},
        {"Vec3 Addition", test_vec3_addition, "Test Vec3 addition"},
        {"Vec4 Addition", test_vec4_addition, "Test Vec4 addition"},
        {"Vector Subtraction", test_vec2_subtraction, "Test vector subtraction"},
        {"Vector Multiplication", test_vector_multiplication, "Test vector scalar multiplication"},
        {"Vector Dot Product", test_vector_dot_product, "Test vector dot product"},
        {"Vector Length", test_vector_length, "Test vector length calculation"},
        {"Vector Normalization", test_vector_normalization, "Test vector normalization"},
        {"Vector Cross Product", test_vector_cross_product, "Test vector cross product"},
        {"Matrix Identity", test_matrix_identity, "Test identity matrix creation"},
        {"Matrix Multiplication", test_matrix_multiplication, "Test matrix multiplication"},
        {"Matrix Vector Multiplication", test_matrix_vector_multiplication, "Test matrix-vector multiplication"},
        {"Performance", test_math_performance, "Test math operations performance"}
    };
    
    suite.total_execution_time_ms = get_time_ms();
    
    for (int i = 0; i < test_count; i++) {
        TestResult *result = &suite.tests[i];
        strcpy(result->test_name, tests[i].name);
        strcpy(result->suite_name, suite.suite_name);
        result->assertions_run = 0;
        result->assertions_passed = 0;
        result->error_message[0] = '\0';
        
        g_current_test = result;
        
        double test_start = get_time_ms();
        result->passed = tests[i].function();
        result->execution_time_ms = get_time_ms() - test_start;
        
        if (result->passed) {
            suite.tests_passed++;
            log_test_message("PASS", suite.suite_name, "%s (%.2f ms, %u assertions)", 
                           result->test_name, result->execution_time_ms, result->assertions_passed);
        } else {
            suite.tests_failed++;
            log_test_message("FAIL", suite.suite_name, "%s (%.2f ms): %s", 
                           result->test_name, result->execution_time_ms, result->error_message);
        }
    }
    
    suite.total_execution_time_ms = get_time_ms() - suite.total_execution_time_ms;
    suite.tests_skipped = 0;
    
    g_current_test = NULL;
    
    return suite;
}
