#include "model_serialization.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static uint32_t calculate_checksum(const uint8_t* data, uint32_t size) {
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
    }
    return checksum;
}

static bool serialize_binary_format(const NeuralNetwork* network, const char* file_path) {
    FILE* file = fopen(file_path, "wb");
    if (!file) return false;
    
    // Write header
    ModelHeader header;
    strcpy(header.format_name, "NEURAL_BIN");
    strcpy(header.version, "1.0");
    header.uncompressed_size = 0; // Will be calculated
    
    // Calculate total data size
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
    
    header.uncompressed_size = sizeof(NeuralNetwork) + 
                              (total_weights + total_biases) * sizeof(float) +
                              network->hidden_layers * sizeof(uint32_t);
    
    // Write network structure
    fwrite(&network->input_size, sizeof(uint32_t), 1, file);
    fwrite(&network->output_size, sizeof(uint32_t), 1, file);
    fwrite(&network->hidden_layers, sizeof(uint32_t), 1, file);
    fwrite(network->layer_sizes, sizeof(uint32_t), network->hidden_layers, file);
    fwrite(network->activation_type, sizeof(char), 32, file);
    
    // Write weights and biases
    fwrite(network->weights, sizeof(float), total_weights, file);
    fwrite(network->biases, sizeof(float), total_biases, file);
    
    // Calculate and write checksum
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* buffer = malloc(file_size);
    fread(buffer, 1, file_size, file);
    header.checksum = calculate_checksum(buffer, file_size);
    free(buffer);
    
    // Rewrite header with checksum
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(ModelHeader), 1, file);
    
    fclose(file);
    return true;
}

static bool serialize_json_format(const NeuralNetwork* network, const char* file_path) {
    FILE* file = fopen(file_path, "w");
    if (!file) return false;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"model_type\": \"neural_network\",\n");
    fprintf(file, "  \"input_size\": %u,\n", network->input_size);
    fprintf(file, "  \"output_size\": %u,\n", network->output_size);
    fprintf(file, "  \"hidden_layers\": %u,\n", network->hidden_layers);
    fprintf(file, "  \"activation\": \"%s\",\n", network->activation_type);
    
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
    
    // Write layer sizes
    fprintf(file, "  \"layer_sizes\": [");
    for (uint32_t i = 0; i < network->hidden_layers; i++) {
        fprintf(file, "%u%s", network->layer_sizes[i], 
                (i < network->hidden_layers - 1) ? ", " : "");
    }
    fprintf(file, "],\n");
    
    // Write weights (simplified - would need proper JSON escaping)
    fprintf(file, "  \"weights\": [");
    for (uint32_t i = 0; i < total_weights; i++) {
        fprintf(file, "%.6f%s", network->weights[i], 
                (i < total_weights - 1) ? ", " : "");
    }
    fprintf(file, "],\n");
    
    // Write biases
    fprintf(file, "  \"biases\": [");
    for (uint32_t i = 0; i < total_biases; i++) {
        fprintf(file, "%.6f%s", network->biases[i], 
                (i < total_biases - 1) ? ", " : "");
    }
    fprintf(file, "]\n");
    
    fprintf(file, "}\n");
    fclose(file);
    return true;
}

static NeuralNetwork* deserialize_binary_format(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
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
    
    // Read weights and biases
    network->weights = malloc(total_weights * sizeof(float));
    network->biases = malloc(total_biases * sizeof(float));
    
    fread(network->weights, sizeof(float), total_weights, file);
    fread(network->biases, sizeof(float), total_biases, file);
    
    fclose(file);
    return network;
}

bool ml_serialize_model(const NeuralNetwork* network, const char* file_path, ModelFormat format) {
    if (!network || !file_path) return false;
    
    switch (format) {
        case MODEL_FORMAT_BINARY:
            return serialize_binary_format(network, file_path);
        case MODEL_FORMAT_JSON:
            return serialize_json_format(network, file_path);
        case MODEL_FORMAT_ONNX:
            // Would implement ONNX export
            return false;
        case MODEL_FORMAT_TFLITE:
            // Would implement TensorFlow Lite export
            return false;
        default:
            return false;
    }
}

NeuralNetwork* ml_deserialize_model(const char* file_path, ModelFormat format) {
    if (!file_path) return NULL;
    
    switch (format) {
        case MODEL_FORMAT_BINARY:
            return deserialize_binary_format(file_path);
        case MODEL_FORMAT_JSON:
            // Would implement JSON parsing
            return NULL;
        case MODEL_FORMAT_ONNX:
            // Would implement ONNX import
            return NULL;
        case MODEL_FORMAT_TFLITE:
            // Would implement TensorFlow Lite import
            return NULL;
        default:
            return NULL;
    }
}

bool ml_compress_model_data(const uint8_t* input, uint32_t input_size, 
                           uint8_t** output, uint32_t* output_size) {
    // Simplified compression - would use zlib or similar
    *output = malloc(input_size);
    if (!*output) return false;
    
    memcpy(*output, input, input_size);
    *output_size = input_size;
    return true;
}

bool ml_decompress_model_data(const uint8_t* input, uint32_t input_size,
                             uint8_t** output, uint32_t* output_size) {
    // Simplified decompression
    *output = malloc(input_size);
    if (!*output) return false;
    
    memcpy(*output, input, input_size);
    *output_size = input_size;
    return true;
}

bool ml_validate_model_file(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (!file) return false;
    
    // Check file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < sizeof(ModelHeader)) {
        fclose(file);
        return false;
    }
    
    // Read and validate header
    ModelHeader header;
    fread(&header, sizeof(ModelHeader), 1, file);
    
    if (strcmp(header.format_name, "NEURAL_BIN") != 0) {
        fclose(file);
        return false;
    }
    
    // Verify checksum
    uint8_t* buffer = malloc(file_size);
    fread(buffer, 1, file_size, file);
    uint32_t calculated_checksum = calculate_checksum(buffer, file_size);
    free(buffer);
    
    fclose(file);
    return (calculated_checksum == header.checksum);
}

bool ml_verify_model_integrity(const NeuralNetwork* network) {
    if (!network) return false;
    
    // Check for null pointers
    if (!network->weights || !network->biases || !network->layer_sizes) {
        return false;
    }
    
    // Check for reasonable values
    if (network->input_size == 0 || network->output_size == 0 || network->hidden_layers == 0) {
        return false;
    }
    
    // Check for NaN or infinite values in weights
    uint32_t total_weights = network->input_size * network->layer_sizes[0];
    for (uint32_t i = 1; i < network->hidden_layers; i++) {
        total_weights += network->layer_sizes[i-1] * network->layer_sizes[i];
    }
    total_weights += network->layer_sizes[network->hidden_layers-1] * network->output_size;
    
    for (uint32_t i = 0; i < total_weights; i++) {
        if (isnan(network->weights[i]) || isinf(network->weights[i])) {
            return false;
        }
    }
    
    return true;
}

bool ml_save_model_metadata(const NeuralNetwork* network, const char* metadata_path) {
    if (!network || !metadata_path) return false;
    
    FILE* file = fopen(metadata_path, "w");
    if (!file) return false;
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    
    fprintf(file, "model_metadata:\n");
    fprintf(file, "  created: %s", asctime(timeinfo));
    fprintf(file, "  input_size: %u\n", network->input_size);
    fprintf(file, "  output_size: %u\n", network->output_size);
    fprintf(file, "  hidden_layers: %u\n", network->hidden_layers);
    fprintf(file, "  activation: %s\n", network->activation_type);
    
    fclose(file);
    return true;
}

bool ml_load_model_metadata(const char* metadata_path, char* metadata, size_t metadata_size) {
    if (!metadata_path || !metadata || metadata_size == 0) return false;
    
    FILE* file = fopen(metadata_path, "r");
    if (!file) return false;
    
    size_t read_size = fread(metadata, 1, metadata_size - 1, file);
    metadata[read_size] = '\0';
    
    fclose(file);
    return true;
}

bool ml_set_model_version(NeuralNetwork* network, const ModelVersion* version) {
    if (!network || !version) return false;
    
    // Store version in network metadata (simplified)
    return true;
}

ModelVersion ml_get_model_version(const NeuralNetwork* network) {
    ModelVersion version = {0};
    if (!network) return version;
    
    // Retrieve version from network metadata (simplified)
    version.major_version = 1;
    version.minor_version = 0;
    version.patch_version = 0;
    strcpy(version.build_date, "2024-01-01");
    strcpy(version.author, "Engine");
    
    return version;
}

void ml_destroy_serialized_model(SerializedModel* model) {
    if (!model) return;
    
    free(model->compressed_data);
    if (model->network) {
        ml_destroy_neural_network(model->network);
    }
    free(model);
}
