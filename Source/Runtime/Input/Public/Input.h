#ifndef ULTIMATE_ENGINE_INPUT_H
#define ULTIMATE_ENGINE_INPUT_H

#include <stdbool.h>

bool Input_Init(void);
void Input_Shutdown(void);
void Input_Update(float delta_time);

// Key states
#define KEY_ESCAPE 256
#define KEY_W 87
#define KEY_A 65
#define KEY_S 83
#define KEY_D 68
#define KEY_SPACE 32
#define KEY_UP 265
#define KEY_DOWN 264
#define KEY_LEFT 263
#define KEY_RIGHT 262

bool Input_IsKeyPressed(int key_code);
bool Input_IsKeyDown(int key_code);
bool Input_IsKeyReleased(int key_code);
bool Input_IsMouseButtonPressed(int button);
bool Input_IsMouseButtonPressed(int button);
void Input_GetMousePosition(float *x, float *y);

// Internal Event Handlers (Called by Platform)
void Input_OnKey(int key, bool pressed);
void Input_OnMouseButton(int button, bool pressed);
void Input_OnMouseMove(float x, float y);

#endif // ULTIMATE_ENGINE_INPUT_H
