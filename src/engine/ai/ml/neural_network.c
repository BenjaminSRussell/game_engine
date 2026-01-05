/**
 * NEURAL NETWORK INFERENCE ENGINE
 * Lightweight NN runtime for game AI and asset generation
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  LAYER_DENSE,
  LAYER_CONV2D,
  LAYER_MAXPOOL2D,
  LAYER_RELU,
  LAYER_SIGMOID,
  LAYER_SOFTMAX
} LayerType;

typedef struct {
  float *weights;
  float *biases;
  unsigned int input_size;
  unsigned int output_size;
} DenseLayer;

typedef struct {
  float *kernels;
  float *biases;
  unsigned int kernel_size;
  unsigned int in_channels;
  unsigned int out_channels;
  unsigned int stride;
  unsigned int padding;
} Conv2DLayer;

typedef struct {
  LayerType type;
  void *layer_data;
} NNLayer;

typedef struct {
  NNLayer *layers;
  unsigned int layer_count;
  float *intermediate_buffers[16]; // Max 16 layers
  unsigned int buffer_sizes[16];
} NeuralNetwork;

// Activation functions
static float relu(float x) { return x > 0.0f ? x : 0.0f; }

static float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }

static void softmax(float *values, unsigned int count) {
  float max_val = values[0];
  for (unsigned int i = 1; i < count; i++) {
    if (values[i] > max_val)
      max_val = values[i];
  }

  float sum = 0.0f;
  for (unsigned int i = 0; i < count; i++) {
    values[i] = expf(values[i] - max_val);
    sum += values[i];
  }

  for (unsigned int i = 0; i < count; i++) {
    values[i] /= sum;
  }
}

// Create neural network
NeuralNetwork *nn_create() {
  NeuralNetwork *nn = malloc(sizeof(NeuralNetwork));
  nn->layer_count = 0;
  nn->layers = NULL;
  return nn;
}

// Add dense (fully connected) layer
void nn_add_dense(NeuralNetwork *nn, unsigned int input_size,
                  unsigned int output_size) {
  nn->layer_count++;
  nn->layers = realloc(nn->layers, nn->layer_count * sizeof(NNLayer));

  NNLayer *layer = &nn->layers[nn->layer_count - 1];
  layer->type = LAYER_DENSE;

  DenseLayer *dense = malloc(sizeof(DenseLayer));
  dense->input_size = input_size;
  dense->output_size = output_size;
  dense->weights = malloc(input_size * output_size * sizeof(float));
  dense->biases = calloc(output_size, sizeof(float));

  // Random initialization (Xavier)
  float scale = sqrtf(2.0f / (input_size + output_size));
  for (unsigned int i = 0; i < input_size * output_size; i++) {
    dense->weights[i] = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * scale;
  }

  layer->layer_data = dense;

  // Allocate intermediate buffer
  nn->intermediate_buffers[nn->layer_count - 1] =
      malloc(output_size * sizeof(float));
  nn->buffer_sizes[nn->layer_count - 1] = output_size;
}

// Add activation layer
void nn_add_activation(NeuralNetwork *nn, LayerType activation) {
  nn->layer_count++;
  nn->layers = realloc(nn->layers, nn->layer_count * sizeof(NNLayer));

  NNLayer *layer = &nn->layers[nn->layer_count - 1];
  layer->type = activation;
  layer->layer_data = NULL; // Activations don't need data
}

// Forward pass through dense layer
static void dense_forward(const DenseLayer *layer, const float *input,
                          float *output) {
  for (unsigned int i = 0; i < layer->output_size; i++) {
    float sum = layer->biases[i];
    for (unsigned int j = 0; j < layer->input_size; j++) {
      sum += input[j] * layer->weights[i * layer->input_size + j];
    }
    output[i] = sum;
  }
}

// Run inference
void nn_forward(NeuralNetwork *nn, const float *input, float *output) {
  const float *current_input = input;
  float *current_output = NULL;

  for (unsigned int layer_idx = 0; layer_idx < nn->layer_count; layer_idx++) {
    NNLayer *layer = &nn->layers[layer_idx];
    current_output = nn->intermediate_buffers[layer_idx];

    switch (layer->type) {
    case LAYER_DENSE: {
      DenseLayer *dense = (DenseLayer *)layer->layer_data;
      dense_forward(dense, current_input, current_output);
      break;
    }

    case LAYER_RELU: {
      // In-place ReLU
      unsigned int size = nn->buffer_sizes[layer_idx - 1];
      for (unsigned int i = 0; i < size; i++) {
        current_output[i] = relu(current_input[i]);
      }
      break;
    }

    case LAYER_SIGMOID: {
      unsigned int size = nn->buffer_sizes[layer_idx - 1];
      for (unsigned int i = 0; i < size; i++) {
        current_output[i] = sigmoid(current_input[i]);
      }
      break;
    }

    case LAYER_SOFTMAX: {
      unsigned int size = nn->buffer_sizes[layer_idx - 1];
      memcpy(current_output, current_input, size * sizeof(float));
      softmax(current_output, size);
      break;
    }

    default:
      break;
    }

    current_input = current_output;
  }

  // Copy final output
  if (current_output) {
    unsigned int output_size = nn->buffer_sizes[nn->layer_count - 1];
    memcpy(output, current_output, output_size * sizeof(float));
  }
}

// Load weights from file
int nn_load_weights(NeuralNetwork *nn, const char *file_path) {
  FILE *f = fopen(file_path, "rb");
  if (!f)
    return 0;

  for (unsigned int i = 0; i < nn->layer_count; i++) {
    NNLayer *layer = &nn->layers[i];

    if (layer->type == LAYER_DENSE) {
      DenseLayer *dense = (DenseLayer *)layer->layer_data;
      fread(dense->weights, sizeof(float),
            dense->input_size * dense->output_size, f);
      fread(dense->biases, sizeof(float), dense->output_size, f);
    }
  }

  fclose(f);
  return 1;
}

// Save weights to file
void nn_save_weights(const NeuralNetwork *nn, const char *file_path) {
  FILE *f = fopen(file_path, "wb");
  if (!f)
    return;

  for (unsigned int i = 0; i < nn->layer_count; i++) {
    const NNLayer *layer = &nn->layers[i];

    if (layer->type == LAYER_DENSE) {
      const DenseLayer *dense = (const DenseLayer *)layer->layer_data;
      fwrite(dense->weights, sizeof(float),
             dense->input_size * dense->output_size, f);
      fwrite(dense->biases, sizeof(float), dense->output_size, f);
    }
  }

  fclose(f);
}

void nn_destroy(NeuralNetwork *nn) {
  for (unsigned int i = 0; i < nn->layer_count; i++) {
    NNLayer *layer = &nn->layers[i];

    if (layer->type == LAYER_DENSE) {
      DenseLayer *dense = (DenseLayer *)layer->layer_data;
      free(dense->weights);
      free(dense->biases);
      free(dense);
    }

    if (nn->intermediate_buffers[i]) {
      free(nn->intermediate_buffers[i]);
    }
  }

  free(nn->layers);
  free(nn);
}

// Example: NPC behavior prediction network
NeuralNetwork *nn_create_behavior_predictor() {
  NeuralNetwork *nn = nn_create();

  // Input: [health, distance_to_player, ammo, cover_available, ally_count]
  nn_add_dense(nn, 5, 16);
  nn_add_activation(nn, LAYER_RELU);

  nn_add_dense(nn, 16, 16);
  nn_add_activation(nn, LAYER_RELU);

  // Output: [attack, defend, flee, seek_cover]
  nn_add_dense(nn, 16, 4);
  nn_add_activation(nn, LAYER_SOFTMAX);

  return nn;
}

// Predict NPC behavior
int nn_predict_behavior(NeuralNetwork *nn, float health, float distance,
                        float ammo, float has_cover, float ally_count) {
  float input[5] = {health / 100.0f, distance / 50.0f, ammo / 30.0f, has_cover,
                    ally_count / 5.0f};
  float output[4];

  nn_forward(nn, input, output);

  // Return index of highest probability
  int best_idx = 0;
  float best_prob = output[0];
  for (int i = 1; i < 4; i++) {
    if (output[i] > best_prob) {
      best_prob = output[i];
      best_idx = i;
    }
  }

  return best_idx; // 0=attack, 1=defend, 2=flee, 3=seek_cover
}

// Training (simple SGD - for runtime learning)
void nn_train_step(NeuralNetwork *nn, const float *input, const float *target,
                   float learning_rate) {
  // Forward pass
  float output[16]; // Assume max output size
  nn_forward(nn, input, output);

  // Calculate loss gradient (MSE)
  unsigned int output_size = nn->buffer_sizes[nn->layer_count - 1];
  float *output_grad = malloc(output_size * sizeof(float));

  for (unsigned int i = 0; i < output_size; i++) {
    output_grad[i] = 2.0f * (output[i] - target[i]);
  }

  // Backpropagation (simplified - only for dense layers)
  for (int layer_idx = (int)nn->layer_count - 1; layer_idx >= 0; layer_idx--) {
    NNLayer *layer = &nn->layers[layer_idx];

    if (layer->type == LAYER_DENSE) {
      DenseLayer *dense = (DenseLayer *)layer->layer_data;
      const float *layer_input =
          (layer_idx == 0) ? input : nn->intermediate_buffers[layer_idx - 1];

      // Update weights and biases
      for (unsigned int i = 0; i < dense->output_size; i++) {
        dense->biases[i] -= learning_rate * output_grad[i];

        for (unsigned int j = 0; j < dense->input_size; j++) {
          dense->weights[i * dense->input_size + j] -=
              learning_rate * output_grad[i] * layer_input[j];
        }
      }

      // Calculate input gradient for previous layer
      float *input_grad = malloc(dense->input_size * sizeof(float));
      memset(input_grad, 0, dense->input_size * sizeof(float));

      for (unsigned int i = 0; i < dense->output_size; i++) {
        for (unsigned int j = 0; j < dense->input_size; j++) {
          input_grad[j] +=
              output_grad[i] * dense->weights[i * dense->input_size + j];
        }
      }

      free(output_grad);
      output_grad = input_grad;
    }
  }

  free(output_grad);
}
