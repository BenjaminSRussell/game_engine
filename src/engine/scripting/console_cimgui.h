#ifndef CONSOLE_CIMGUI_H
#define CONSOLE_CIMGUI_H

/*
 * C subset of the cimgui API (Dear ImGui 1.92) used by console.c.
 * imgui.h is C++ and cannot be included from this translation unit.
 * A cimgui build provides the symbols; tests provide stubs.
 */

#include <stdbool.h>
#include <stddef.h>

typedef struct ImVec2_c {
    float x;
    float y;
} ImVec2_c;

typedef struct ImVec4_c {
    float x;
    float y;
    float z;
    float w;
} ImVec4_c;

typedef int ImGuiWindowFlags;
typedef int ImGuiChildFlags;
typedef int ImGuiInputTextFlags;

typedef struct ImGuiInputTextCallbackData ImGuiInputTextCallbackData;
typedef int (*ImGuiInputTextCallback)(ImGuiInputTextCallbackData *data);

#define ImGuiWindowFlags_HorizontalScrollbar (1 << 11)
#define ImGuiChildFlags_None 0
#define ImGuiInputTextFlags_EnterReturnsTrue (1 << 6)

bool igBegin(const char *name, bool *p_open, ImGuiWindowFlags flags);
void igEnd(void);
bool igBeginChild_Str(const char *str_id, const ImVec2_c size, ImGuiChildFlags child_flags,
                      ImGuiWindowFlags window_flags);
void igEndChild(void);
void igText(const char *fmt, ...);
void igTextColored(const ImVec4_c col, const char *fmt, ...);
bool igInputText(const char *label, char *buf, size_t buf_size, ImGuiInputTextFlags flags,
                 ImGuiInputTextCallback callback, void *user_data);
void igSetKeyboardFocusHere(int offset);

#endif /* CONSOLE_CIMGUI_H */
