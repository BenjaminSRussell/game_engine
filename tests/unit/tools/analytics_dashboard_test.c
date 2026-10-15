/*
 * Bar and line draw calls from analytics_dashboard.c. Issue #195.
 * Compiles analytics_dashboard.c directly and records every primitive a chart
 * emits through the draw backend. Does not link Engine, which does not build
 * on origin/main.
 */

#include "tools/analytics_dashboard.h"

#include "core/logging/unified_logger.h"
#include "core/utils.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// analytics_dashboard.c references these symbols. Log output is discarded: the
// test checks draw calls, not log text.
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

u64 get_current_time_ms(void) { return 0; }

#define MAX_CALLS 16

typedef struct {
  float x0, y0, x1, y1;
  Vec4 color;
} LineCall;

typedef struct {
  float x, y, width, height;
  Vec4 color;
} RectCall;

static LineCall g_lines[MAX_CALLS];
static RectCall g_rects[MAX_CALLS];
static u32 g_line_count;
static u32 g_rect_count;
static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static Vec4 rgba(float r, float g, float b, float a) {
  Vec4 v;
  v.x = r;
  v.y = g;
  v.z = b;
  v.w = a;
  return v;
}

static bool approx_equal(float actual, float expected) {
  return fabsf(actual - expected) < 0.01f;
}

static bool same_color(Vec4 a, Vec4 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

static void record_line(void *user_data, float x0, float y0, float x1, float y1,
                        Vec4 color) {
  (void)user_data;
  if (g_line_count < MAX_CALLS) {
    LineCall *call = &g_lines[g_line_count];
    call->x0 = x0;
    call->y0 = y0;
    call->x1 = x1;
    call->y1 = y1;
    call->color = color;
  }
  g_line_count++;
}

static void record_rect(void *user_data, float x, float y, float width,
                        float height, Vec4 color) {
  (void)user_data;
  if (g_rect_count < MAX_CALLS) {
    RectCall *call = &g_rects[g_rect_count];
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
    call->color = color;
  }
  g_rect_count++;
}

static void reset_calls(void) {
  memset(g_lines, 0, sizeof(g_lines));
  memset(g_rects, 0, sizeof(g_rects));
  g_line_count = 0;
  g_rect_count = 0;
}

static void attach_recorder(void) {
  AnalyticsDrawBackend backend;
  memset(&backend, 0, sizeof(backend));
  backend.draw_line = record_line;
  backend.draw_rect = record_rect;
  analytics_dashboard_set_draw_backend(&backend);
  reset_calls();
}

static bool rect_matches(const RectCall *r, float x, float y, float width,
                         float height, Vec4 color) {
  return approx_equal(r->x, x) && approx_equal(r->y, y) &&
         approx_equal(r->width, width) && approx_equal(r->height, height) &&
         same_color(r->color, color);
}

static bool line_matches(const LineCall *l, float x0, float y0, float x1,
                         float y1, Vec4 color) {
  return approx_equal(l->x0, x0) && approx_equal(l->y0, y0) &&
         approx_equal(l->x1, x1) && approx_equal(l->y1, y1) &&
         same_color(l->color, color);
}

// Only series primitives are wanted, so grid, axes and legend are turned off.
static ChartData *make_chart(AnalyticsDashboard *dash, ChartType type,
                             u32 capacity) {
  ChartData *chart =
      analytics_dashboard_create_chart(dash, type, "test", capacity, "x", "y");
  if (chart) {
    chart->show_grid = false;
    chart->show_axes = false;
    chart->show_legend = false;
  }
  return chart;
}

// Values 2, 4, 1 on the 400x200 bar area (x from 50, baseline y = 250).
// bar_width = 400 / (3 * 1.5) = 88.889 and spacing = 44.444, so bar i starts at
// x = 50 + i * 133.333 + 44.444. Heights are 100, 200 and 50.
static void test_bars_emit_one_rect_each_with_layout(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 2, 1), "dashboard init");
  ChartData *chart = make_chart(&dash, CHART_TYPE_BAR, 4);
  EXPECT(chart != NULL, "bar chart created");

  Vec4 red = rgba(1.0f, 0.0f, 0.0f, 1.0f);
  Vec4 green = rgba(0.0f, 1.0f, 0.0f, 1.0f);
  Vec4 blue = rgba(0.0f, 0.0f, 1.0f, 1.0f);
  EXPECT(analytics_chart_add_point(chart, 0.0f, 2.0f, "a", red), "add bar a");
  EXPECT(analytics_chart_add_point(chart, 1.0f, 4.0f, "b", green), "add bar b");
  EXPECT(analytics_chart_add_point(chart, 2.0f, 1.0f, "c", blue), "add bar c");

  attach_recorder();
  analytics_dashboard_render_chart(chart);

  EXPECT(g_line_count == 0, "bar chart emits no lines");
  EXPECT(g_rect_count == 3, "bar chart emits one rect per bar");
  EXPECT(rect_matches(&g_rects[0], 94.444f, 150.0f, 88.889f, 100.0f, red),
         "bar 0 rect");
  EXPECT(rect_matches(&g_rects[1], 227.778f, 50.0f, 88.889f, 200.0f, green),
         "bar 1 rect");
  EXPECT(rect_matches(&g_rects[2], 361.111f, 200.0f, 88.889f, 50.0f, blue),
         "bar 2 rect");

  analytics_dashboard_shutdown(&dash);
}

// {0, -3, 2}: only the positive bar gets a rect, at full height because 2 is
// the scale maximum. {-1, -2}: no rect, because the scale starts at zero and
// no value is positive.
static void test_bars_skip_non_positive_values(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 4, 1), "dashboard init");
  Vec4 gray = rgba(0.5f, 0.5f, 0.5f, 1.0f);
  Vec4 blue = rgba(0.0f, 0.0f, 1.0f, 1.0f);

  ChartData *mixed = make_chart(&dash, CHART_TYPE_BAR, 4);
  EXPECT(mixed != NULL, "mixed bar chart created");
  EXPECT(analytics_chart_add_point(mixed, 0.0f, 0.0f, "zero", gray),
         "add zero value");
  EXPECT(analytics_chart_add_point(mixed, 1.0f, -3.0f, "neg", gray),
         "add negative value");
  EXPECT(analytics_chart_add_point(mixed, 2.0f, 2.0f, "pos", blue),
         "add positive value");

  attach_recorder();
  analytics_dashboard_render_chart(mixed);
  EXPECT(g_rect_count == 1, "only the positive bar is drawn");
  // Bar 2 of 3: x = 50 + 2 * 133.333 + 44.444 = 361.111, height = 200.
  EXPECT(rect_matches(&g_rects[0], 361.111f, 50.0f, 88.889f, 200.0f, blue),
         "positive bar rect");

  ChartData *negative = make_chart(&dash, CHART_TYPE_BAR, 4);
  EXPECT(negative != NULL, "negative bar chart created");
  EXPECT(analytics_chart_add_point(negative, 0.0f, -1.0f, "a", gray),
         "add first negative value");
  EXPECT(analytics_chart_add_point(negative, 1.0f, -2.0f, "b", gray),
         "add second negative value");

  reset_calls();
  analytics_dashboard_render_chart(negative);
  EXPECT(g_rect_count == 0, "all-negative data draws no bars");

  analytics_dashboard_shutdown(&dash);
}

// (0,0) (1,2) (2,1) (3,4): the x range is 3 and the y range is 4, so the points
// map to (50,250) (183.333,150) (316.667,200) (450,50). Each segment joins two
// consecutive points and takes the color of its starting point.
static void test_line_segments_join_consecutive_points(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 2, 1), "dashboard init");
  ChartData *chart = make_chart(&dash, CHART_TYPE_LINE, 4);
  EXPECT(chart != NULL, "line chart created");

  Vec4 c0 = rgba(1.0f, 0.0f, 0.0f, 1.0f);
  Vec4 c1 = rgba(0.0f, 1.0f, 0.0f, 1.0f);
  Vec4 c2 = rgba(0.0f, 0.0f, 1.0f, 1.0f);
  Vec4 c3 = rgba(1.0f, 1.0f, 0.0f, 1.0f);
  EXPECT(analytics_chart_add_point(chart, 0.0f, 0.0f, NULL, c0), "add point 0");
  EXPECT(analytics_chart_add_point(chart, 1.0f, 2.0f, NULL, c1), "add point 1");
  EXPECT(analytics_chart_add_point(chart, 2.0f, 1.0f, NULL, c2), "add point 2");
  EXPECT(analytics_chart_add_point(chart, 3.0f, 4.0f, NULL, c3), "add point 3");

  attach_recorder();
  analytics_dashboard_render_chart(chart);

  EXPECT(g_rect_count == 0, "line chart emits no rects");
  EXPECT(g_line_count == 3, "one segment per adjacent pair of points");
  EXPECT(line_matches(&g_lines[0], 50.0f, 250.0f, 183.333f, 150.0f, c0),
         "segment 0");
  EXPECT(line_matches(&g_lines[1], 183.333f, 150.0f, 316.667f, 200.0f, c1),
         "segment 1");
  EXPECT(line_matches(&g_lines[2], 316.667f, 200.0f, 450.0f, 50.0f, c2),
         "segment 2");

  analytics_dashboard_shutdown(&dash);
}

static void test_line_with_fewer_than_two_points_emits_nothing(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 2, 1), "dashboard init");
  ChartData *chart = make_chart(&dash, CHART_TYPE_LINE, 4);
  EXPECT(chart != NULL, "line chart created");
  Vec4 white = rgba(1.0f, 1.0f, 1.0f, 1.0f);

  attach_recorder();
  analytics_dashboard_render_chart(chart);
  EXPECT(analytics_chart_add_point(chart, 0.0f, 1.0f, NULL, white),
         "add single point");
  analytics_dashboard_render_chart(chart);
  EXPECT(g_line_count == 0 && g_rect_count == 0,
         "fewer than two points draw nothing");

  analytics_dashboard_shutdown(&dash);
}

// Rendering with no backend set must not crash and must emit nothing.
static void test_render_without_backend_does_not_crash(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 2, 1), "dashboard init");
  ChartData *bars = make_chart(&dash, CHART_TYPE_BAR, 2);
  ChartData *lines = make_chart(&dash, CHART_TYPE_LINE, 2);
  EXPECT(bars != NULL && lines != NULL, "charts created");
  Vec4 white = rgba(1.0f, 1.0f, 1.0f, 1.0f);
  EXPECT(analytics_chart_add_point(bars, 0.0f, 1.0f, NULL, white), "add bar");
  EXPECT(analytics_chart_add_point(lines, 0.0f, 1.0f, NULL, white),
         "add line point 0");
  EXPECT(analytics_chart_add_point(lines, 1.0f, 2.0f, NULL, white),
         "add line point 1");

  reset_calls();
  analytics_dashboard_render_chart(bars);
  analytics_dashboard_render_chart(lines);
  EXPECT(g_line_count == 0 && g_rect_count == 0, "no backend, no calls");

  analytics_dashboard_shutdown(&dash);
}

// Passing NULL must detach the backend, so later renders emit nothing.
static void test_detached_backend_receives_no_calls(void) {
  AnalyticsDashboard dash;
  EXPECT(analytics_dashboard_init(&dash, 2, 1), "dashboard init");
  ChartData *bars = make_chart(&dash, CHART_TYPE_BAR, 2);
  EXPECT(bars != NULL, "bar chart created");
  Vec4 white = rgba(1.0f, 1.0f, 1.0f, 1.0f);
  EXPECT(analytics_chart_add_point(bars, 0.0f, 1.0f, NULL, white), "add bar");

  attach_recorder();
  analytics_dashboard_render_chart(bars);
  EXPECT(g_rect_count == 1, "attached backend receives the bar");

  analytics_dashboard_set_draw_backend(NULL);
  reset_calls();
  analytics_dashboard_render_chart(bars);
  EXPECT(g_rect_count == 0, "detached backend receives no rects");

  analytics_dashboard_shutdown(&dash);
}

int main(void) {
  test_render_without_backend_does_not_crash();
  test_bars_emit_one_rect_each_with_layout();
  test_bars_skip_non_positive_values();
  test_line_segments_join_consecutive_points();
  test_line_with_fewer_than_two_points_emits_nothing();
  test_detached_backend_receives_no_calls();

  if (g_failed != 0) {
    fprintf(stderr, "%d analytics_dashboard chart check(s) failed\n", g_failed);
    return 1;
  }
  printf("analytics_dashboard bar and line draw calls: all checks passed\n");
  return 0;
}
