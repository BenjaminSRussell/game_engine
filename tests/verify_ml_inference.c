// tests/verify_ml_inference.c
//
// Purpose: Verification of ML Inference Integration
// This test verifies that the ML system can be initialized, models loaded,
// and inference executed using the Custom (Mock) framework.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

// Include the ML Core header
// We will compile with -I src/engine/include
#include "ai/ml/ml_core.h"

// ============================================================================
// STUBS FOR DEPENDENCIES
// ============================================================================

// Memory stubs
void* unified_memory_alloc(size_t size, int strategy, int flags,
                        const char* file, int line, const char* function) {
    return malloc(size);
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    free(ptr);
}

// Logger stubs
void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                     const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[LOG %d:%d] ", level, category);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    printf("=== Starting ML Inference Verification ===\n");

    // 1. Create System
    MLSystem* system = ml_system_create(ML_FRAMEWORK_CUSTOM, ML_DEVICE_CPU);
    if (!system) {
        printf("FAILED: ml_system_create returned NULL\n");
        return 1;
    }
    printf("PASSED: ml_system_create\n");

    // 2. Initialize System
    if (!ml_system_initialize(system)) {
        printf("FAILED: ml_system_initialize returned false\n");
        ml_system_destroy(system);
        return 1;
    }
    printf("PASSED: ml_system_initialize\n");

    // 3. Load Model
    MLModelMetadata metadata = {
        .name = "TestBehaviorModel",
        .version = "1.0",
        .description = "Test model for NPC behavior",
        .type = ML_MODEL_TYPE_BEHAVIOR,
        .framework = ML_FRAMEWORK_CUSTOM,
        .preferred_device = ML_DEVICE_CPU,
        .model_size_bytes = 1024,
        .expected_inference_time_ms = 1.0f
    };

    void* model = ml_load_model(system, "assets/models/behavior.onnx", &metadata);
    if (!model) {
        printf("FAILED: ml_load_model returned NULL\n");
        ml_system_shutdown(system);
        ml_system_destroy(system);
        return 1;
    }
    printf("PASSED: ml_load_model\n");

    // 4. Create Tensors
    u32 dims[] = {1, 10};
    MLTensor* input = ml_create_tensor(system, dims, 2, ML_DATA_TYPE_FLOAT32);
    MLTensor* output = ml_create_tensor(system, dims, 2, ML_DATA_TYPE_FLOAT32);

    if (!input || !output) {
        printf("FAILED: ml_create_tensor\n");
        return 1;
    }
    printf("PASSED: ml_create_tensor\n");

    // Fill input
    float* data = (float*)input->data;
    for (int i = 0; i < 10; i++) data[i] = (float)i;

    // 5. Create Inference Context
    MLInferenceContext* context = ml_create_inference_context(system, model);
    if (!context) {
        printf("FAILED: ml_create_inference_context\n");
        return 1;
    }

    context->inputs = input;
    context->outputs = output;
    context->input_count = 1;
    context->output_count = 1;

    printf("PASSED: ml_create_inference_context\n");

    // 6. Run Inference
    if (!ml_run_inference(system, context)) {
        printf("FAILED: ml_run_inference returned false\n");
        return 1;
    }
    printf("PASSED: ml_run_inference\n");

    // 7. Cleanup
    ml_destroy_tensor(system, input);
    ml_destroy_tensor(system, output);
    ml_unload_model(system, model);
    ml_system_destroy(system);

    printf("=== ML Inference Verification PASSED ===\n");
    return 0;
}
