#ifndef MODEL_SERIALIZATION_H
#define MODEL_SERIALIZATION_H

#include "neural_inference.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char format_name[16];
    char version[8];
    uint32_t checksum;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
} ModelHeader;

typedef struct {
    ModelHeader header;
    NeuralNetwork* network;
    char metadata[256];
    uint8_t* compressed_data;
} SerializedModel;

// Model serialization formats
typedef enum {
    MODEL_FORMAT_BINARY,
    MODEL_FORMAT_JSON,
    MODEL_FORMAT_ONNX,
    MODEL_FORMAT_TFLITE
} ModelFormat;

// Serialization functions
bool ml_serialize_model(const NeuralNetwork* network, const char* file_path, ModelFormat format);
NeuralNetwork* ml_deserialize_model(const char* file_path, ModelFormat format);

// Model compression
bool ml_compress_model_data(const uint8_t* input, uint32_t input_size, 
                           uint8_t** output, uint32_t* output_size);
bool ml_decompress_model_data(const uint8_t* input, uint32_t input_size,
                             uint8_t** output, uint32_t* output_size);

// Model validation
bool ml_validate_model_file(const char* file_path);
bool ml_verify_model_integrity(const NeuralNetwork* network);

// Model metadata
bool ml_save_model_metadata(const NeuralNetwork* network, const char* metadata_path);
bool ml_load_model_metadata(const char* metadata_path, char* metadata, size_t metadata_size);

// Model versioning
typedef struct {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t patch_version;
    char build_date[32];
    char author[64];
} ModelVersion;

bool ml_set_model_version(NeuralNetwork* network, const ModelVersion* version);
ModelVersion ml_get_model_version(const NeuralNetwork* network);

// Cleanup
void ml_destroy_serialized_model(SerializedModel* model);

#ifdef __cplusplus
}
#endif

#endif // MODEL_SERIALIZATION_H
