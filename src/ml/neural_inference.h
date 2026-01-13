#ifndef NEURAL_INFERENCE_H
#define NEURAL_INFERENCE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float* weights;
    float* biases;
    uint32_t input_size;
    uint32_t output_size;
    uint32_t hidden_layers;
    uint32_t* layer_sizes;
    char activation_type[32];
} NeuralNetwork;

typedef struct {
    NeuralNetwork* network;
    float* input_buffer;
    float* output_buffer;
    float* workspace;
    bool is_initialized;
} InferenceEngine;

// Initialize inference engine
InferenceEngine* ml_create_inference_engine(const NeuralNetwork* network);

// Perform inference
bool ml_infer(InferenceEngine* engine, const float* input, float* output);

// Batch inference for multiple inputs
bool ml_batch_infer(InferenceEngine* engine, const float* inputs, float* outputs, uint32_t batch_size);

// Load model from file
NeuralNetwork* ml_load_model(const char* model_path);

// Cleanup
void ml_destroy_inference_engine(InferenceEngine* engine);
void ml_destroy_neural_network(NeuralNetwork* network);

#ifdef __cplusplus
}
#endif

#endif // NEURAL_INFERENCE_H
