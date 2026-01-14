#ifndef AI_MODELS_H
#define AI_MODELS_H

#include <common.h>
#include <stdbool.h>

typedef enum {
    AI_MODEL_TEXT_TO_3D,
    AI_MODEL_TEXTURE_SYNTHESIS,
    AI_MODEL_STYLE_TRANSFER,
    AI_MODEL_UPSCALING,
    AI_MODEL_NORMAL_GENERATION,
    AI_MODEL_COUNT
} AIModelType;

typedef struct AIModel {
    AIModelType type;
    char name[128];
    char model_path[512];
    void *model_data; // TFLite or ONNX model data
    void *interpreter; // Runtime interpreter instance
    bool use_gpu;
    int input_size[4]; // [batch, height, width, channels]
    int output_size[4];
} AIModel;

#endif // AI_MODELS_H
