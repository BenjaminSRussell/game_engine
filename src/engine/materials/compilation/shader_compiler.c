#include "materials/compilation/shader_compiler.h"
#include "include/core/logger.h"
#include <stdio.h>

u32 shader_compiler_compile(const char *source, ShaderStage stage, const ShaderCompilerOptions *options, char *out_log, u32 log_size) {
    if (!source) {
        if (out_log) snprintf(out_log, log_size, "Source is NULL");
        return 0;
    }

    // Stub implementation
    LOG_INFO("Compiling shader stage %d", stage);
    if (options && options->defines) {
        LOG_DEBUG("Defines: %s", options->defines);
    }

    // Return dummy ID
    return 12345;
}
