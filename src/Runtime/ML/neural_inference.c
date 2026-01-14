#include "neural_inference.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float activation_function(float x, const char* type) {
    if (strcmp(type, "relu") == 0) {
        return fmaxf(0.0f, x);
    } else if (strcmp(type, "sigmoid") == 0) {
        return 1.0f / (1.0f + expf(-x));
    } else if (strcmp(type, "tanh") == 0) {
        return tanhf(x);
    }
    return x; // Linear
}

static void forward_pass(const NeuralNetwork* network, const float* input, float* output, float* workspace) {
    uint32_t current_size = network->input_size;
    const float* current_input = input;
    float* current_output = workspace;
    
    // Copy input to first layer
    memcpy(current_output, current_input, current_size * sizeof(float));
    
    // Process each hidden layer
    for (uint32_t layer = 0; layer < network->hidden_layers; layer++) {
        uint32_t next_size = network->layer_sizes[layer];
        const float* weights = network->weights + layer * current_size * next_size;
        const float* biases = network->biases + layer * next_size;
        
        float* next_output = (layer == network->hidden_layers - 1) ? output : current_output + next_size;
        
        // Matrix multiplication + bias + activation
        for (uint32_t j = 0; j < next_size; j++) {
            float sum = biases[j];
            for (uint32_t i = 0; i < current_size; i++) {
                sum += weights[i * next_size + j] * current_input[i];
            }
            next_output[j] = activation_function(sum, network->activation_type);
        }
        
        current_input = current_output;
        current_output = next_output;
        current_size = next_size;
    }
}

InferenceEngine* ml_create_inference_engine(const NeuralNetwork* network) {
    if (!network) return NULL;
    
    InferenceEngine* engine = malloc(sizeof(InferenceEngine));
    if (!engine) return NULL;
    
    engine->network = malloc(sizeof(NeuralNetwork));
    if (!engine->network) {
        free(engine);
        return NULL;
    }
    
    // Deep copy network
    memcpy(engine->network, network, sizeof(NeuralNetwork));
    
    // Allocate buffers
    engine->input_buffer = malloc(network->input_size * sizeof(float));
    engine->output_buffer = malloc(network->output_size * sizeof(float));
    
    // Calculate workspace size (max intermediate layer size)
    uint32_t max_workspace = network->input_size;
    for (uint32_t i = 0; i < network->hidden_layers; i++) {
        if (network->layer_sizes[i] > max_workspace) {
            max_workspace = network->layer_sizes[i];
        }
    }
    engine->workspace = malloc(max_workspace * sizeof(float));
    
    if (!engine->input_buffer || !engine->output_buffer || !engine->workspace) {
        ml_destroy_inference_engine(engine);
        return NULL;
    }
    
    engine->is_initialized = true;
    return engine;
}

bool ml_infer(InferenceEngine* engine, const float* input, float* output) {
    if (!engine || !engine->is_initialized || !input || !output) {
        return false;
    }
    
    forward_pass(engine->network, input, output, engine->workspace);
    return true;
}

bool ml_batch_infer(InferenceEngine* engine, const float* inputs, float* outputs, uint32_t batch_size) {
    if (!engine || !engine->is_initialized || !inputs || !outputs || batch_size == 0) {
        return false;
    }
    
    for (uint32_t i = 0; i < batch_size; i++) {
        const float* input = inputs + i * engine->network->input_size;
        float* output = outputs + i * engine->network->output_size;
        
        if (!ml_infer(engine, input, output)) {
            return false;
        }
    }
    
    return true;
}

NeuralNetwork* ml_load_model(const char* model_path) {
    // Simplified model loading - in production would use proper format
    FILE* file = fopen(model_path, "rb");
    if (!file) return NULL;
    
    NeuralNetwork* network = malloc(sizeof(NeuralNetwork));
    if (!network) {
        fclose(file);
        return NULL;
    }
    
    // Read network structure
    fread(&network->input_size, sizeof(uint32_t), 1, file);
    fread(&network->output_size, sizeof(uint32_t), 1, file);
    fread(&network->hidden_layers, sizeof(uint32_t), 1, file);
    
    network->layer_sizes = malloc(network->hidden_layers * sizeof(uint32_t));
    fread(network->layer_sizes, sizeof(uint32_t), network->hidden_layers, file);
    
    // Read activation type
    fread(network->activation_type, sizeof(char), 32, file);
    
    // Calculate total weights and biases
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    uint32_t total_biases = 0;
    for (uint32_t i = 0; i < network->hidden_layers; i++) {
        total_biases += network->layer_sizes[i];
    }
    total_biases += network->output_size;
    
    // Allocate and read weights and biases
    network->weights = malloc(total_weights * sizeof(float));
    network->biases = malloc(total_biases * sizeof(float));
    
    fread(network->weights, sizeof(float), total_weights, file);
    fread(network->biases, sizeof(float), total_biases, file);
    
    fclose(file);
    return network;
}

void ml_destroy_inference_engine(InferenceEngine* engine) {
    if (!engine) return;
    
    free(engine->input_buffer);
    free(engine->output_buffer);
    free(engine->workspace);
    
    if (engine->network) {
        ml_destroy_neural_network(engine->network);
    }
    
    free(engine);
}

void ml_destroy_neural_network(NeuralNetwork* network) {
    if (!network) return;
    
    free(network->weights);
    free(network->biases);
    free(network->layer_sizes);
    free(network);
}
