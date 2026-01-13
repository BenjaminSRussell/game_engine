#ifndef NEURAL_NETWORK_INFERENCE_H
#define NEURAL_NETWORK_INFERENCE_H

void nn_inference_init();
void nn_load_model(const char *model_path);
void nn_predict(float *inputs, int input_size, float *outputs, int output_size);

#endif
