#include "framebuffer.h"

#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

struct Framebuffer {
  u32 width;
  u32 height;
  void *color_attachments[FRAMEBUFFER_MAX_COLOR_ATTACHMENTS];
  u32 color_attachment_count;
  void *depth_attachment;
  FramebufferResizeCallback resize_callback;
  void *resize_user_data;
};

static Framebuffer *g_bound_framebuffer = NULL;

Framebuffer *framebuffer_create(u32 width, u32 height) {
  if (width == 0 || height == 0) {
    LOG_ERROR("Framebuffer create failed: invalid size %ux%u", width, height);
    return NULL;
  }

  Framebuffer *fb = (Framebuffer *)calloc(1, sizeof(Framebuffer));
  if (!fb) {
    LOG_ERROR("Framebuffer create failed: out of memory");
    return NULL;
  }

  fb->width = width;
  fb->height = height;
  fb->color_attachment_count = 0;

  return fb;
}

void framebuffer_destroy(Framebuffer *fb) {
  if (!fb) {
    return;
  }

  if (g_bound_framebuffer == fb) {
    g_bound_framebuffer = NULL;
  }

  free(fb);
}

void framebuffer_attach_color(Framebuffer *fb, u32 slot, void *texture) {
  if (!fb || slot >= FRAMEBUFFER_MAX_COLOR_ATTACHMENTS) {
    return;
  }

  fb->color_attachments[slot] = texture;
  if (texture && slot + 1 > fb->color_attachment_count) {
    fb->color_attachment_count = slot + 1;
  }
}

void framebuffer_attach_depth(Framebuffer *fb, void *texture) {
  if (!fb) {
    return;
  }

  fb->depth_attachment = texture;
}

bool framebuffer_validate(Framebuffer *fb) {
  if (!fb) {
    return false;
  }

  if (fb->color_attachment_count == 0 && !fb->depth_attachment) {
    LOG_WARN("Framebuffer validation failed: no attachments");
    return false;
  }

  return true;
}

void framebuffer_bind(Framebuffer *fb) {
  if (!fb) {
    return;
  }

  g_bound_framebuffer = fb;
}

void framebuffer_unbind(void) { g_bound_framebuffer = NULL; }

void framebuffer_clear_color(Framebuffer *fb, f32 r, f32 g, f32 b, f32 a) {
  (void)fb;
  (void)r;
  (void)g;
  (void)b;
  (void)a;
}

void framebuffer_clear_depth(Framebuffer *fb, f32 depth) {
  (void)fb;
  (void)depth;
}

u32 framebuffer_get_width(Framebuffer *fb) { return fb ? fb->width : 0; }

u32 framebuffer_get_height(Framebuffer *fb) { return fb ? fb->height : 0; }

bool framebuffer_resize(Framebuffer *fb, u32 width, u32 height) {
  if (!fb || width == 0 || height == 0) {
    return false;
  }

  u32 old_width = fb->width;
  u32 old_height = fb->height;
  fb->width = width;
  fb->height = height;

  if (fb->resize_callback) {
    fb->resize_callback(fb, old_width, old_height, width, height,
                        fb->resize_user_data);
  }

  return true;
}

void framebuffer_set_resize_callback(Framebuffer *fb,
                                     FramebufferResizeCallback callback,
                                     void *user_data) {
  if (!fb) {
    return;
  }

  fb->resize_callback = callback;
  fb->resize_user_data = user_data;
}

bool framebuffer_needs_resize(Framebuffer *fb) {
  (void)fb;
  return false;
}

f32 framebuffer_get_aspect_ratio(Framebuffer *fb) {
  if (!fb || fb->height == 0) {
    return 0.0f;
  }

  return (f32)fb->width / (f32)fb->height;
}

bool framebuffer_blit(Framebuffer *src, Framebuffer *dst, u32 filter) {
  (void)src;
  (void)dst;
  (void)filter;
  LOG_WARN("Framebuffer blit not implemented");
  return false;
}

bool framebuffer_read_pixels(Framebuffer *fb, u32 slot, u32 x, u32 y,
                             u32 width, u32 height, u32 format, void *data) {
  (void)fb;
  (void)slot;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  (void)format;
  (void)data;
  LOG_WARN("Framebuffer read_pixels not implemented");
  return false;
}

u32 framebuffer_get_color_attachment_count(Framebuffer *fb) {
  return fb ? fb->color_attachment_count : 0;
}

void *framebuffer_get_color_texture(Framebuffer *fb, u32 slot) {
  if (!fb || slot >= FRAMEBUFFER_MAX_COLOR_ATTACHMENTS) {
    return NULL;
  }

  return fb->color_attachments[slot];
}

void *framebuffer_get_depth_texture(Framebuffer *fb) {
  return fb ? fb->depth_attachment : NULL;
}
