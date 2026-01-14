#ifndef TRAINING_PIPELINE_H
#define TRAINING_PIPELINE_H

#include "neural_inference.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float* inputs;
    float* targets;
    uint32_t count;
    uint32_t input_size;
    uint32_t output_size;
} TrainingData;

typedef struct {
    float learning_rate;
    uint32_t epochs;
    uint32_t batch_size;
    float validation_split;
    bool shuffle_data;
    bool early_stopping;
    uint32_t patience;
} TrainingConfig;

typedef struct {
    float training_loss;
    float validation_loss;
    float accuracy;
    uint32_t epoch;
    uint32_t total_epochs;
} TrainingMetrics;

typedef struct {
    NeuralNetwork* network;
    TrainingData* training_data;
    TrainingData* validation_data;
    TrainingConfig config;
    TrainingMetrics metrics;
    bool is_training;
} TrainingPipeline;

// Training pipeline functions
TrainingPipeline* ml_create_training_pipeline(NeuralNetwork* network, TrainingData* data, const TrainingConfig* config);

// Start training process
bool ml_start_training(TrainingPipeline* pipeline);

// Single training epoch
bool ml_train_epoch(TrainingPipeline* pipeline);

// Validate current model
float ml_validate_model(TrainingPipeline* pipeline);

// Get training metrics
TrainingMetrics ml_get_training_metrics(const TrainingPipeline* pipeline);

// Save trained model
bool ml_save_trained_model(const TrainingPipeline* pipeline, const char* model_path);

// Load training data from files
TrainingData* ml_load_training_data(const char* inputs_path, const char* targets_path);

// Data preprocessing
void ml_normalize_data(TrainingData* data);
void ml_shuffle_training_data(TrainingData* data);

// Cleanup
void ml_destroy_training_pipeline(TrainingPipeline* pipeline);
void ml_destroy_training_data(TrainingData* data);

#ifdef __cplusplus
}
#endif

#endif // TRAINING_PIPELINE_H
