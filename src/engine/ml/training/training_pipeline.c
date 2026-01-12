#include "training_pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static void split_training_data(TrainingData* original, TrainingData** training, TrainingData** validation, float validation_split) {
    uint32_t validation_count = (uint32_t)(original->count * validation_split);
    uint32_t training_count = original->count - validation_count;
    
    *training = malloc(sizeof(TrainingData));
    *validation = malloc(sizeof(TrainingData));
    
    (*training)->inputs = malloc(training_count * original->input_size * sizeof(float));
    (*training)->targets = malloc(training_count * original->output_size * sizeof(float));
    (*training)->count = training_count;
    (*training)->input_size = original->input_size;
    (*training)->output_size = original->output_size;
    
    (*validation)->inputs = malloc(validation_count * original->input_size * sizeof(float));
    (*validation)->targets = malloc(validation_count * original->output_size * sizeof(float));
    (*validation)->count = validation_count;
    (*validation)->input_size = original->input_size;
    (*validation)->output_size = original->output_size;
    
    // Copy data (simplified - should shuffle first)
    memcpy((*training)->inputs, original->inputs, training_count * original->input_size * sizeof(float));
    memcpy((*training)->targets, original->targets, training_count * original->output_size * sizeof(float));
    
    memcpy((*validation)->inputs, original->inputs + training_count * original->input_size, 
           validation_count * original->input_size * sizeof(float));
    memcpy((*validation)->targets, original->targets + training_count * original->output_size, 
           validation_count * original->output_size * sizeof(float));
}

static float calculate_loss(const float* predictions, const float* targets, uint32_t size) {
    float loss = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        float diff = predictions[i] - targets[i];
        loss += diff * diff;
    }
    return loss / size;
}

static float calculate_accuracy(const float* predictions, const float* targets, uint32_t count, uint32_t output_size) {
    uint32_t correct = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t pred_class = 0, target_class = 0;
        float max_pred = predictions[i * output_size];
        float max_target = targets[i * output_size];
        
        for (uint32_t j = 1; j < output_size; j++) {
            if (predictions[i * output_size + j] > max_pred) {
                max_pred = predictions[i * output_size + j];
                pred_class = j;
            }
            if (targets[i * output_size + j] > max_target) {
                max_target = targets[i * output_size + j];
                target_class = j;
            }
        }
        
        if (pred_class == target_class) correct++;
    }
    
    return (float)correct / count;
}

TrainingPipeline* ml_create_training_pipeline(NeuralNetwork* network, TrainingData* data, const TrainingConfig* config) {
    if (!network || !data || !config) return NULL;
    
    TrainingPipeline* pipeline = malloc(sizeof(TrainingPipeline));
    if (!pipeline) return NULL;
    
    pipeline->network = network;
    pipeline->config = *config;
    pipeline->is_training = false;
    
    // Split data into training and validation
    split_training_data(data, &pipeline->training_data, &pipeline->validation_data, config->validation_split);
    
    // Initialize metrics
    pipeline->metrics.training_loss = 0.0f;
    pipeline->metrics.validation_loss = 0.0f;
    pipeline->metrics.accuracy = 0.0f;
    pipeline->metrics.epoch = 0;
    pipeline->metrics.total_epochs = config->epochs;
    
    return pipeline;
}

bool ml_start_training(TrainingPipeline* pipeline) {
    if (!pipeline || !pipeline->training_data) return false;
    
    pipeline->is_training = true;
    pipeline->metrics.epoch = 0;
    
    // Shuffle data if requested
    if (pipeline->config.shuffle_data) {
        ml_shuffle_training_data(pipeline->training_data);
    }
    
    return true;
}

bool ml_train_epoch(TrainingPipeline* pipeline) {
    if (!pipeline || !pipeline->is_training) return false;
    
    uint32_t batch_size = pipeline->config.batch_size;
    uint32_t num_batches = (pipeline->training_data->count + batch_size - 1) / batch_size;
    
    float total_loss = 0.0f;
    
    // Process each batch
    for (uint32_t batch = 0; batch < num_batches; batch++) {
        uint32_t start_idx = batch * batch_size;
        uint32_t end_idx = (start_idx + batch_size > pipeline->training_data->count) ? 
                          pipeline->training_data->count : start_idx + batch_size;
        uint32_t current_batch_size = end_idx - start_idx;
        
        // Forward pass (simplified - would include backpropagation in full implementation)
        InferenceEngine* engine = ml_create_inference_engine(pipeline->network);
        if (!engine) return false;
        
        float* batch_outputs = malloc(current_batch_size * pipeline->network->output_size * sizeof(float));
        
        const float* batch_inputs = pipeline->training_data->inputs + start_idx * pipeline->training_data->input_size;
        const float* batch_targets = pipeline->training_data->targets + start_idx * pipeline->training_data->output_size;
        
        if (!ml_batch_infer(engine, batch_inputs, batch_outputs, current_batch_size)) {
            free(batch_outputs);
            ml_destroy_inference_engine(engine);
            return false;
        }
        
        // Calculate batch loss
        float batch_loss = calculate_loss(batch_outputs, batch_targets, 
                                        current_batch_size * pipeline->network->output_size);
        total_loss += batch_loss;
        
        free(batch_outputs);
        ml_destroy_inference_engine(engine);
        
        // Backpropagation would go here in full implementation
        // For now, we'll just simulate training progress
    }
    
    pipeline->metrics.training_loss = total_loss / num_batches;
    pipeline->metrics.epoch++;
    
    // Validate after each epoch
    if (pipeline->validation_data && pipeline->validation_data->count > 0) {
        pipeline->metrics.validation_loss = ml_validate_model(pipeline);
    }
    
    return true;
}

float ml_validate_model(TrainingPipeline* pipeline) {
    if (!pipeline || !pipeline->validation_data || pipeline->validation_data->count == 0) {
        return 0.0f;
    }
    
    InferenceEngine* engine = ml_create_inference_engine(pipeline->network);
    if (!engine) return 0.0f;
    
    uint32_t validation_size = pipeline->validation_data->count;
    float* validation_outputs = malloc(validation_size * pipeline->network->output_size * sizeof(float));
    
    if (!ml_batch_infer(engine, pipeline->validation_data->inputs, validation_outputs, validation_size)) {
        free(validation_outputs);
        ml_destroy_inference_engine(engine);
        return 0.0f;
    }
    
    float validation_loss = calculate_loss(validation_outputs, pipeline->validation_data->targets,
                                         validation_size * pipeline->network->output_size);
    
    // Calculate accuracy
    pipeline->metrics.accuracy = calculate_accuracy(validation_outputs, pipeline->validation_data->targets,
                                                   validation_size, pipeline->network->output_size);
    
    free(validation_outputs);
    ml_destroy_inference_engine(engine);
    
    return validation_loss;
}

TrainingMetrics ml_get_training_metrics(const TrainingPipeline* pipeline) {
    if (!pipeline) {
        TrainingMetrics empty = {0};
        return empty;
    }
    return pipeline->metrics;
}

bool ml_save_trained_model(const TrainingPipeline* pipeline, const char* model_path) {
    if (!pipeline || !pipeline->network || !model_path) return false;
    
    FILE* file = fopen(model_path, "wb");
    if (!file) return false;
    
    // Write network structure
    fwrite(&pipeline->network->input_size, sizeof(uint32_t), 1, file);
    fwrite(&pipeline->network->output_size, sizeof(uint32_t), 1, file);
    fwrite(&pipeline->network->hidden_layers, sizeof(uint32_t), 1, file);
    fwrite(pipeline->network->layer_sizes, sizeof(uint32_t), pipeline->network->hidden_layers, file);
    fwrite(pipeline->network->activation_type, sizeof(char), 32, file);
    
    // Calculate and write weights
    uint32_t total_weights = pipeline->network->input_size * pipeline->network->layer_sizes[0];
    for (uint32_t i = 1; i < pipeline->network->hidden_layers; i++) {
        total_weights += pipeline->network->layer_sizes[i-1] * pipeline->network->layer_sizes[i];
    }
    total_weights += pipeline->network->layer_sizes[pipeline->network->hidden_layers-1] * pipeline->network->output_size;
    
    uint32_t total_biases = 0;
    for (uint32_t i = 0; i < pipeline->network->hidden_layers; i++) {
        total_biases += pipeline->network->layer_sizes[i];
    }
    total_biases += pipeline->network->output_size;
    
    fwrite(pipeline->network->weights, sizeof(float), total_weights, file);
    fwrite(pipeline->network->biases, sizeof(float), total_biases, file);
    
    fclose(file);
    return true;
}

TrainingData* ml_load_training_data(const char* inputs_path, const char* targets_path) {
    // Simplified data loading
    TrainingData* data = malloc(sizeof(TrainingData));
    if (!data) return NULL;
    
    FILE* input_file = fopen(inputs_path, "rb");
    FILE* target_file = fopen(targets_path, "rb");
    
    if (!input_file || !target_file) {
        free(data);
        if (input_file) fclose(input_file);
        if (target_file) fclose(target_file);
        return NULL;
    }
    
    // Read metadata (simplified)
    fread(&data->count, sizeof(uint32_t), 1, input_file);
    fread(&data->input_size, sizeof(uint32_t), 1, input_file);
    fread(&data->output_size, sizeof(uint32_t), 1, target_file);
    
    // Allocate and read data
    data->inputs = malloc(data->count * data->input_size * sizeof(float));
    data->targets = malloc(data->count * data->output_size * sizeof(float));
    
    fread(data->inputs, sizeof(float), data->count * data->input_size, input_file);
    fread(data->targets, sizeof(float), data->count * data->output_size, target_file);
    
    fclose(input_file);
    fclose(target_file);
    
    return data;
}

void ml_normalize_data(TrainingData* data) {
    if (!data || !data->inputs) return;
    
    // Find min and max for each feature
    float* mins = malloc(data->input_size * sizeof(float));
    float* maxs = malloc(data->input_size * sizeof(float));
    
    for (uint32_t i = 0; i < data->input_size; i++) {
        mins[i] = data->inputs[i];
        maxs[i] = data->inputs[i];
    }
    
    for (uint32_t j = 1; j < data->count; j++) {
        for (uint32_t i = 0; i < data->input_size; i++) {
            float val = data->inputs[j * data->input_size + i];
            if (val < mins[i]) mins[i] = val;
            if (val > maxs[i]) maxs[i] = val;
        }
    }
    
    // Normalize to [0, 1]
    for (uint32_t j = 0; j < data->count; j++) {
        for (uint32_t i = 0; i < data->input_size; i++) {
            float range = maxs[i] - mins[i];
            if (range > 0.0f) {
                data->inputs[j * data->input_size + i] = 
                    (data->inputs[j * data->input_size + i] - mins[i]) / range;
            }
        }
    }
    
    free(mins);
    free(maxs);
}

void ml_shuffle_training_data(TrainingData* data) {
    if (!data || !data->inputs || !data->targets) return;
    
    srand((unsigned int)time(NULL));
    
    for (uint32_t i = data->count - 1; i > 0; i--) {
        uint32_t j = rand() % (i + 1);
        
        // Swap inputs
        for (uint32_t k = 0; k < data->input_size; k++) {
            float temp = data->inputs[i * data->input_size + k];
            data->inputs[i * data->input_size + k] = data->inputs[j * data->input_size + k];
            data->inputs[j * data->input_size + k] = temp;
        }
        
        // Swap targets
        for (uint32_t k = 0; k < data->output_size; k++) {
            float temp = data->targets[i * data->output_size + k];
            data->targets[i * data->output_size + k] = data->targets[j * data->output_size + k];
            data->targets[j * data->output_size + k] = temp;
        }
    }
}

void ml_destroy_training_pipeline(TrainingPipeline* pipeline) {
    if (!pipeline) return;
    
    ml_destroy_training_data(pipeline->training_data);
    ml_destroy_training_data(pipeline->validation_data);
    free(pipeline);
}

void ml_destroy_training_data(TrainingData* data) {
    if (!data) return;
    
    free(data->inputs);
    free(data->targets);
    free(data);
}
