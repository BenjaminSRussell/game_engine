#include "shader_library/vfx/particle_shaders.h"

const char* particle_shader_vert = 
    "#version 450\n"
    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec4 color;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "}\n";

const char* particle_shader_frag =
    "#version 450\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() {\n"
    "    fragColor = vec4(1.0);\n"
    "}\n";

void particle_shaders_init() {}
