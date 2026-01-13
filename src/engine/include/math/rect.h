#ifndef MATH_RECT_H
#define MATH_RECT_H

#include "engine/include/common.h"
#include "math/vec2.h"
#include <stdbool.h>

typedef struct Rect {
  f32 x;
  f32 y;
  f32 width;
  f32 height;
} Rect;

static inline bool rect_contains(Rect rect, Vec2 point) {
  return point.x >= rect.x && point.x <= rect.x + rect.width &&
         point.y >= rect.y && point.y <= rect.y + rect.height;
}

static inline bool rect_intersects(Rect a, Rect b) {
  return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height &&
         a.y + a.height > b.y;
}

static inline Rect rect_intersection(Rect a, Rect b) {
  f32 x = a.x > b.x ? a.x : b.x;
  f32 y = a.y > b.y ? a.y : b.y;
  f32 r = (a.x + a.width) < (b.x + b.width) ? (a.x + a.width) : (b.x + b.width);
  f32 bo =
      (a.y + a.height) < (b.y + b.height) ? (a.y + a.height) : (b.y + b.height);

  if (x < r && y < bo) {
    return (Rect){x, y, r - x, bo - y};
  }
  return (Rect){0, 0, 0, 0};
}

#endif // MATH_RECT_H
