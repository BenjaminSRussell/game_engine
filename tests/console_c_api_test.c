/*
 * console.c is compiled as C. With ENABLE_IMGUI the render path must call
 * the cimgui C API (not ImGui::) and still execute submitted commands.
 */

#include "console.h"
#include "console_cimgui.h"
#include "core/logging/unified_logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file, int line,
                        const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

static char g_trace[16][32];
static int g_trace_n;
static char g_lines[16][256];
static int g_line_n;
static int g_line_colored[16];
static float g_line_r[16];
static float g_line_g[16];
static float g_line_b[16];
static float g_line_a[16];
static int g_begin_returns;
static int g_close_window;
static int g_open_at_begin;
static float g_child_w;
static float g_child_h;
static int g_child_flags;
static int g_child_window_flags;
static int g_input_flags;
static char g_input_at_call[256];
static int g_focus_calls;
static int g_focus_offset;
static const char *g_submit;
static int g_ping_argc;
static char g_ping_argv0[64];
static char g_ping_argv1[64];

static void fail(const char *msg) {
    int i;
    fprintf(stderr, "FAIL: %s\n", msg);
    fprintf(stderr, "trace:");
    for (i = 0; i < g_trace_n; i++) {
        fprintf(stderr, " %s", g_trace[i]);
    }
    fprintf(stderr, "\n");
    exit(1);
}

static void trace(const char *name) {
    if (g_trace_n >= 16) {
        fail("trace overflow");
    }
    snprintf(g_trace[g_trace_n], sizeof(g_trace[0]), "%s", name);
    g_trace_n++;
}

static void reset_ui(void) {
    g_trace_n = 0;
    g_line_n = 0;
    g_begin_returns = 1;
    g_close_window = 0;
    g_open_at_begin = -1;
    g_child_w = 0.0f;
    g_child_h = 0.0f;
    g_child_flags = -1;
    g_child_window_flags = -1;
    g_input_flags = -1;
    g_input_at_call[0] = '\0';
    g_focus_calls = 0;
    g_focus_offset = 0;
}

static void expect_trace(const char **names, int count) {
    int i;
    if (g_trace_n != count) {
        fail("call count");
    }
    for (i = 0; i < count; i++) {
        if (strcmp(g_trace[i], names[i]) != 0) {
            fail(names[i]);
        }
    }
}

bool igBegin(const char *name, bool *p_open, ImGuiWindowFlags flags) {
    trace("begin");
    if (strcmp(name, "Console") != 0 || flags != 0 || !p_open) {
        fail("igBegin args");
    }
    g_open_at_begin = *p_open ? 1 : 0;
    if (g_close_window) {
        *p_open = false;
    }
    return g_begin_returns ? true : false;
}

void igEnd(void) { trace("end"); }

bool igBeginChild_Str(const char *str_id, const ImVec2_c size, ImGuiChildFlags child_flags,
                      ImGuiWindowFlags window_flags) {
    trace("child");
    if (strcmp(str_id, "ScrollingRegion") != 0) {
        fail("child id");
    }
    g_child_w = size.x;
    g_child_h = size.y;
    g_child_flags = child_flags;
    g_child_window_flags = window_flags;
    return true;
}

void igEndChild(void) { trace("end_child"); }

static void record_line(const char *fmt, va_list ap, int colored, ImVec4_c col) {
    if (g_line_n >= 16) {
        fail("line overflow");
    }
    vsnprintf(g_lines[g_line_n], sizeof(g_lines[0]), fmt, ap);
    g_line_colored[g_line_n] = colored;
    g_line_r[g_line_n] = col.x;
    g_line_g[g_line_n] = col.y;
    g_line_b[g_line_n] = col.z;
    g_line_a[g_line_n] = col.w;
    g_line_n++;
}

void igText(const char *fmt, ...) {
    va_list ap;
    ImVec4_c none = {0};
    trace("text");
    va_start(ap, fmt);
    record_line(fmt, ap, 0, none);
    va_end(ap);
}

void igTextColored(const ImVec4_c col, const char *fmt, ...) {
    va_list ap;
    trace("text_colored");
    va_start(ap, fmt);
    record_line(fmt, ap, 1, col);
    va_end(ap);
}

bool igInputText(const char *label, char *buf, size_t buf_size, ImGuiInputTextFlags flags,
                 ImGuiInputTextCallback callback, void *user_data) {
    trace("input");
    (void)callback;
    (void)user_data;
    if (strcmp(label, "Input") != 0 || buf_size == 0 || !buf) {
        fail("igInputText args");
    }
    snprintf(g_input_at_call, sizeof(g_input_at_call), "%s", buf);
    g_input_flags = flags;
    if (!g_submit) {
        return false;
    }
    snprintf(buf, buf_size, "%s", g_submit);
    g_submit = NULL;
    return true;
}

void igSetKeyboardFocusHere(int offset) {
    trace("focus");
    g_focus_calls++;
    g_focus_offset = offset;
}

static void ping_command(int argc, char **argv) {
    g_ping_argc = argc;
    snprintf(g_ping_argv0, sizeof(g_ping_argv0), "%s", argc > 0 ? argv[0] : "");
    snprintf(g_ping_argv1, sizeof(g_ping_argv1), "%s", argc > 1 ? argv[1] : "");
}

static void expect_line(int index, const char *text, int colored) {
    if (index >= g_line_n || strcmp(g_lines[index], text) != 0 || g_line_colored[index] != colored) {
        fprintf(stderr, "line %d got '%s' colored=%d\n", index,
                index < g_line_n ? g_lines[index] : "(missing)",
                index < g_line_n ? g_line_colored[index] : -1);
        fail("log line");
    }
}

int main(void) {
    const char *collapsed[] = {"begin", "end"};
    const char *draw[] = {"begin", "child", "text", "text_colored", "end_child", "input", "end"};
    const char *submit[] = {"begin", "child", "text", "text_colored", "end_child", "input", "focus", "end"};
    const char *after_ping[] = {"begin", "child", "text", "text_colored", "text", "end_child", "input", "end"};
    const char *after_bad[] = {"begin", "child", "text", "text_colored", "text", "text", "text_colored",
                               "end_child", "input", "end"};
    const char *after_clear[] = {"begin", "child", "end_child", "input", "end"};

    console_init();
    console_register_command("ping", ping_command, "Ping");

    reset_ui();
    console_render();
    expect_trace(NULL, 0);
    if (console_is_visible()) {
        fail("visible before toggle");
    }

    console_toggle();
    if (!console_is_visible()) {
        fail("toggle");
    }

    reset_ui();
    g_begin_returns = 0;
    console_render();
    expect_trace(collapsed, 2);
    if (g_focus_calls != 0 || g_open_at_begin != 1) {
        fail("collapsed window");
    }

    console_print("ok");
    console_print_error("bad");
    reset_ui();
    console_render();
    expect_trace(draw, 7);
    expect_line(0, "ok", 0);
    expect_line(1, "bad", 1);
    if (g_line_r[1] != 1.0f || g_line_g[1] != 0.0f || g_line_b[1] != 0.0f || g_line_a[1] != 1.0f) {
        fail("error color");
    }
    if (g_child_w != 0.0f || g_child_h != -30.0f || g_child_flags != 0 ||
        g_child_window_flags != (1 << 11) || g_input_flags != (1 << 6) || g_focus_calls != 0) {
        fail("widget args");
    }

    reset_ui();
    g_submit = "ping one";
    console_render();
    expect_trace(submit, 8);
    if (g_ping_argc != 2 || strcmp(g_ping_argv0, "ping") != 0 || strcmp(g_ping_argv1, "one") != 0) {
        fail("command dispatch");
    }
    if (g_focus_calls != 1 || g_focus_offset != -1 || g_input_at_call[0] != '\0') {
        fail("focus or input buffer");
    }

    reset_ui();
    console_render();
    expect_trace(after_ping, 8);
    expect_line(0, "ok", 0);
    expect_line(1, "bad", 1);
    expect_line(2, "ping one", 0);
    if (g_line_n != 3) {
        fail("ping echo count");
    }
    if (g_input_at_call[0] != '\0') {
        fail("input not cleared");
    }

    reset_ui();
    g_submit = "nope";
    console_render();
    reset_ui();
    console_render();
    expect_trace(after_bad, 10);
    expect_line(3, "nope", 0);
    expect_line(4, "Unknown command", 1);
    if (g_line_n != 5) {
        fail("unknown command line count");
    }

    reset_ui();
    g_submit = "clear";
    console_render();
    reset_ui();
    console_render();
    expect_trace(after_clear, 5);
    if (g_line_n != 0) {
        fail("clear left log lines");
    }

    reset_ui();
    g_submit = "help";
    console_render();
    reset_ui();
    console_render();
    expect_line(0, "help", 0);
    expect_line(1, "Available commands:", 0);
    expect_line(2, "  help - List all commands", 0);
    expect_line(3, "  clear - Clear console output", 0);
    expect_line(4, "  ping - Ping", 0);
    if (g_line_n != 5) {
        fail("help line count");
    }

    reset_ui();
    g_close_window = 1;
    console_render();
    if (console_is_visible() || g_open_at_begin != 1) {
        fail("close did not clear visibility");
    }
    reset_ui();
    console_render();
    expect_trace(NULL, 0);

    console_shutdown();
    printf("console c api ok\n");
    return 0;
}
