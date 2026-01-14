#pragma once

#include <common.h>

typedef struct ColorPalette {
    char name[32];
    u32 color_count;
    u32* colors; // RGBA8
} ColorPalette;

void color_palette_system_init(void);
void color_palette_system_shutdown(void);
void color_palette_apply(const char* palette_name, float dither_amount);
void color_palette_set_swap_time(float time_of_day);
