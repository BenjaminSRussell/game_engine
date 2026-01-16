#include "Public/Render.h"
#include <math.h>
#include <stdio.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Stub sub-managers for now
void *g_render_device = NULL;

bool Render_Init(void) {
  printf("Initializing Renderer (OpenGL)...\n");
  // In a real engine, we'd load function pointers (glad/glew) here.
  // On macOS, system OpenGL 4.1 is available directly.

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  return true;
}

void Render_Shutdown(void) { printf("Shutting down Renderer...\n"); }

void Render_BeginFrame(void) {
  // Cornflower Blue
  glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Render_EndFrame(void) {
  // Swap buffers is handled by Platform_Update
}

void Render_DrawCube(float x, float y, float z, float size, float r, float g,
                     float b) {
  float s = size * 0.5f;
  glColor3f(r, g, b);

  glPushMatrix();
  glTranslatef(x, y, z);

  glBegin(GL_QUADS);
  // Front face
  glVertex3f(-s, -s, s);
  glVertex3f(s, -s, s);
  glVertex3f(s, s, s);
  glVertex3f(-s, s, s);
  // Back face
  glVertex3f(-s, -s, -s);
  glVertex3f(-s, s, -s);
  glVertex3f(s, s, -s);
  glVertex3f(s, -s, -s);
  // Top face
  glVertex3f(-s, s, -s);
  glVertex3f(-s, s, s);
  glVertex3f(s, s, s);
  glVertex3f(s, s, -s);
  // Bottom face
  glVertex3f(-s, -s, -s);
  glVertex3f(s, -s, -s);
  glVertex3f(s, -s, s);
  glVertex3f(-s, -s, s);
  // Right face
  glVertex3f(s, -s, -s);
  glVertex3f(s, s, -s);
  glVertex3f(s, s, s);
  glVertex3f(s, -s, s);
  // Left face
  glVertex3f(-s, -s, -s);
  glVertex3f(-s, -s, s);
  glVertex3f(-s, s, s);
  glVertex3f(-s, s, -s);
  glEnd();

  glPopMatrix();
}

// Global light direction for simple lighting
static float g_light_dir[3] = {0.5f, -1.0f, 0.3f}; // Normalized in init

void Render_SetLightDirection(float x, float y, float z) {
  // Normalize
  float len = sqrtf(x * x + y * y + z * z);
  if (len > 0.001f) {
    g_light_dir[0] = x / len;
    g_light_dir[1] = y / len;
    g_light_dir[2] = z / len;
  }
}

void Render_DrawSphere(float x, float y, float z, float radius, float r,
                       float g, float b) {
  glPushMatrix();
  glTranslatef(x, y, z);

  // Draw sphere using quad strips (simple tessellation)
  int slices = 16;
  int stacks = 12;

  for (int i = 0; i < stacks; i++) {
    float lat0 = 3.14159f * (-0.5f + (float)i / stacks);
    float lat1 = 3.14159f * (-0.5f + (float)(i + 1) / stacks);
    float z0 = sinf(lat0);
    float zr0 = cosf(lat0);
    float z1 = sinf(lat1);
    float zr1 = cosf(lat1);

    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; j++) {
      float lng = 2.0f * 3.14159f * (float)j / slices;
      float x_val = cosf(lng);
      float y_val = sinf(lng);

      // Vertex 1
      float nx0 = x_val * zr0;
      float ny0 = y_val * zr0;
      float nz0 = z0;
      float light0 = fmaxf(0.2f, -(nx0 * g_light_dir[0] + ny0 * g_light_dir[1] +
                                   nz0 * g_light_dir[2]));
      glColor3f(r * light0, g * light0, b * light0);
      glVertex3f(radius * nx0, radius * ny0, radius * nz0);

      // Vertex 2
      float nx1 = x_val * zr1;
      float ny1 = y_val * zr1;
      float nz1 = z1;
      float light1 = fmaxf(0.2f, -(nx1 * g_light_dir[0] + ny1 * g_light_dir[1] +
                                   nz1 * g_light_dir[2]));
      glColor3f(r * light1, g * light1, b * light1);
      glVertex3f(radius * nx1, radius * ny1, radius * nz1);
    }
    glEnd();
  }

  glPopMatrix();
}
