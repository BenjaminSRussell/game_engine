#include "render_test_framework.h"
#include "../math/vec3.h"
#include "../math/vec4.h"
#include "../math/mat4.h"

bool test_vec3_add(void) {
    vec3_t a = vec3_set(1.0f, 2.0f, 3.0f);
    vec3_t b = vec3_set(4.0f, 5.0f, 6.0f);
    vec3_t res = vec3_add(a, b);
    
    RENDER_TEST_ASSERT_FLOAT_EQ(res.x, 5.0f, 0.0001f);
    RENDER_TEST_ASSERT_FLOAT_EQ(res.y, 7.0f, 0.0001f);
    RENDER_TEST_ASSERT_FLOAT_EQ(res.z, 9.0f, 0.0001f);
    return true;
}

bool test_vec4_dot(void) {
    vec4_t a = vec4_set(1.0f, 2.0f, 3.0f, 4.0f);
    vec4_t b = vec4_set(5.0f, 6.0f, 7.0f, 8.0f);
    float dot = vec4_dot(a, b);
    
    // 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
    RENDER_TEST_ASSERT_FLOAT_EQ(dot, 70.0f, 0.0001f);
    return true;
}

bool test_mat4_mul(void) {
    mat4_t a = mat4_identity();
    mat4_t b = mat4_translate(1.0f, 2.0f, 3.0f);
    mat4_t res = mat4_mul(a, b);
    
    RENDER_TEST_ASSERT_FLOAT_EQ(res.e[3][0], 1.0f, 0.0001f);
    RENDER_TEST_ASSERT_FLOAT_EQ(res.e[3][1], 2.0f, 0.0001f);
    RENDER_TEST_ASSERT_FLOAT_EQ(res.e[3][2], 3.0f, 0.0001f);
    return true;
}

void register_math_tests(uint32_t suite_id) {
    render_test_register(suite_id, "Vector 3 Addition", test_vec3_add);
    render_test_register(suite_id, "Vector 4 Dot Product", test_vec4_dot);
    render_test_register(suite_id, "Matrix 4 Multiplication", test_mat4_mul);
}

int main(void) {
    render_test_framework_init();
    uint32_t suite = render_test_suite_create("Math Library Tests");
    register_math_tests(suite);
    render_test_run_all();
    render_test_framework_shutdown();
    return 0;
}
