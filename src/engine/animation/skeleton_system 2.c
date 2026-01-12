#include "math/mat4.h"
#include "math/vec3.h"
#include "math/quat.h"

static Mat4 create_transform_matrix(const Vec3* position, const Quat* rotation, const Vec3* scale) {
    Mat4 translation = mat4_translate(*position);
    Mat4 rotation_matrix = quat_to_mat4(*rotation);
    Mat4 scale_matrix = mat4_scale(*scale);
    
    Mat4 combined = mat4_mul(rotation_matrix, scale_matrix);
    return mat4_mul(translation, combined);
}
