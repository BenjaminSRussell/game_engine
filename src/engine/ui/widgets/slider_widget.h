#ifndef UI_SLIDER_WIDGET_H
#define UI_SLIDER_WIDGET_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Slider Widget
typedef struct SliderWidget SliderWidget;

typedef void (*SliderValueChangedCallback)(SliderWidget* slider, float value, void* user_data);

struct SliderWidget {
    Widget widget; // Base class

    float value;
    float min_value;
    float max_value;
    float step; // 0 for continuous
    bool vertical;

    // Visuals
    float track_thickness;
    float thumb_size;
    Vec4 track_color;
    Vec4 thumb_color;
    Vec4 active_track_color;

    // Callback
    SliderValueChangedCallback on_value_changed;
    void* callback_user_data;

    // Internal state
    bool dragging;
};

// API
SliderWidget* slider_widget_create(const char* name);
void slider_widget_set_value(SliderWidget* slider, float value);
void slider_widget_set_range(SliderWidget* slider, float min, float max);
void slider_widget_set_step(SliderWidget* slider, float step);
void slider_widget_set_vertical(SliderWidget* slider, bool vertical);
void slider_widget_set_callback(SliderWidget* slider, SliderValueChangedCallback callback, void* user_data);
float slider_widget_get_value(const SliderWidget* slider);

#ifdef __cplusplus
}
#endif

#endif // UI_SLIDER_WIDGET_H
