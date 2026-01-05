// src/game/demo_simple_terrain.c
// Simple Terrain Rendering Demo

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include <core/window.h>
#include <core/logger.h>
#include <math/vec3.h>

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

#define TERRAIN_SIZE 128
#define TERRAIN_SCALE 2.0f
#define HEIGHT_SCALE 10.0f

// -----------------------------------------------------------------------------
// Global State
// -----------------------------------------------------------------------------

typedef struct {
    Window window;
    float heightmap[TERRAIN_SIZE][TERRAIN_SIZE];
    
    // Camera
    float cam_x, cam_y, cam_z;
    float cam_yaw, cam_pitch;
    
    // Input
    float last_mouse_x, last_mouse_y;
    bool first_mouse;
} DemoState;

static DemoState g_demo;

// -----------------------------------------------------------------------------
// Terrain Generation
// -----------------------------------------------------------------------------

float noise(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

float smoothNoise(int x, int y) {
    float corners = (noise(x-1, y-1) + noise(x+1, y-1) + noise(x-1, y+1) + noise(x+1, y+1)) / 16.0f;
    float sides = (noise(x-1, y) + noise(x+1, y) + noise(x, y-1) + noise(x, y+1)) / 8.0f;
    float center = noise(x, y) / 4.0f;
    return corners + sides + center;
}

float interpolate(float a, float b, float x) {
    float ft = x * 3.1415927f;
    float f = (1.0f - cosf(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

float interpolatedNoise(float x, float y) {
    int int_x = (int)x;
    float frac_x = x - int_x;
    int int_y = (int)y;
    float frac_y = y - int_y;
    
    float v1 = smoothNoise(int_x, int_y);
    float v2 = smoothNoise(int_x + 1, int_y);
    float v3 = smoothNoise(int_x, int_y + 1);
    float v4 = smoothNoise(int_x + 1, int_y + 1);
    
    float i1 = interpolate(v1, v2, frac_x);
    float i2 = interpolate(v3, v4, frac_x);
    
    return interpolate(i1, i2, frac_y);
}

float perlinNoise(float x, float y) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < 4; i++) {
        total += interpolatedNoise(x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return total / max_value;
}

void generate_terrain() {
    for (int z = 0; z < TERRAIN_SIZE; z++) {
        for (int x = 0; x < TERRAIN_SIZE; x++) {
            float nx = (float)x / TERRAIN_SIZE;
            float nz = (float)z / TERRAIN_SIZE;
            g_demo.heightmap[z][x] = perlinNoise(nx * 8.0f, nz * 8.0f) * HEIGHT_SCALE;
        }
    }
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------

void render_terrain() {
    glBegin(GL_TRIANGLES);
    
    for (int z = 0; z < TERRAIN_SIZE - 1; z++) {
        for (int x = 0; x < TERRAIN_SIZE - 1; x++) {
            float x0 = (x - TERRAIN_SIZE/2) * TERRAIN_SCALE;
            float x1 = (x + 1 - TERRAIN_SIZE/2) * TERRAIN_SCALE;
            float z0 = (z - TERRAIN_SIZE/2) * TERRAIN_SCALE;
            float z1 = (z + 1 - TERRAIN_SIZE/2) * TERRAIN_SCALE;
            
            float h00 = g_demo.heightmap[z][x];
            float h10 = g_demo.heightmap[z][x+1];
            float h01 = g_demo.heightmap[z+1][x];
            float h11 = g_demo.heightmap[z+1][x+1];
            
            // Color based on height
            float c = 0.3f + h00 / (HEIGHT_SCALE * 2.0f);
            glColor3f(c * 0.6f, c * 0.8f, c * 0.4f);
            
            // Triangle 1
            glVertex3f(x0, h00, z0);
            glVertex3f(x1, h10, z0);
            glVertex3f(x0, h01, z1);
            
            // Triangle 2
            glVertex3f(x1, h10, z0);
            glVertex3f(x1, h11, z1);
            glVertex3f(x0, h01, z1);
        }
    }
    
    glEnd();
}

// -----------------------------------------------------------------------------
// Input
// -----------------------------------------------------------------------------

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    
    if (g_demo.first_mouse) {
        g_demo.last_mouse_x = xpos;
        g_demo.last_mouse_y = ypos;
        g_demo.first_mouse = false;
    }
    
    float xoffset = xpos - g_demo.last_mouse_x;
    float yoffset = g_demo.last_mouse_y - ypos;
    g_demo.last_mouse_x = xpos;
    g_demo.last_mouse_y = ypos;
    
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    g_demo.cam_yaw += xoffset;
    g_demo.cam_pitch += yoffset;
    
    if (g_demo.cam_pitch > 89.0f) g_demo.cam_pitch = 89.0f;
    if (g_demo.cam_pitch < -89.0f) g_demo.cam_pitch = -89.0f;
}

void process_input(float dt) {
    float speed = 20.0f * dt;
    
    // Calculate forward/right vectors
    float yaw_rad = g_demo.cam_yaw * 3.14159f / 180.0f;
    float forward_x = sinf(yaw_rad);
    float forward_z = -cosf(yaw_rad);
    float right_x = cosf(yaw_rad);
    float right_z = sinf(yaw_rad);
    
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_W) == GLFW_PRESS) {
        g_demo.cam_x += forward_x * speed;
        g_demo.cam_z += forward_z * speed;
    }
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_S) == GLFW_PRESS) {
        g_demo.cam_x -= forward_x * speed;
        g_demo.cam_z -= forward_z * speed;
    }
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_A) == GLFW_PRESS) {
        g_demo.cam_x -= right_x * speed;
        g_demo.cam_z -= right_z * speed;
    }
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_D) == GLFW_PRESS) {
        g_demo.cam_x += right_x * speed;
        g_demo.cam_z += right_z * speed;
    }
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_SPACE) == GLFW_PRESS) {
        g_demo.cam_y += speed;
    }
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        g_demo.cam_y -= speed;
    }
    
    if (glfwGetKey(g_demo.window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(g_demo.window.handle, 1);
    }
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    printf("Simple Terrain Demo\n");
    printf("Controls: WASD - Move, Mouse - Look, Space/Shift - Up/Down, ESC - Quit\n");
    
    if (!window_init(&g_demo.window, 1280, 720, "Simple Terrain Demo", false)) {
        return 1;
    }
    
    // Setup mouse input
    glfwSetCursorPosCallback(g_demo.window.handle, mouse_callback);
    glfwSetInputMode(g_demo.window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Initialize camera
    g_demo.cam_x = 0.0f;
    g_demo.cam_y = 30.0f;
    g_demo.cam_z = 0.0f;
    g_demo.cam_yaw = 0.0f;
    g_demo.cam_pitch = -20.0f;
    g_demo.first_mouse = true;
    
    // Generate terrain
    generate_terrain();
    
    // OpenGL setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    double last_time = glfwGetTime();
    
    while (!window_should_close(&g_demo.window)) {
        double current_time = glfwGetTime();
        float dt = (float)(current_time - last_time);
        last_time = current_time;
        
        window_poll_events();
        process_input(dt);
        
        // Render
        int w, h;
        glfwGetFramebufferSize(g_demo.window.handle, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Setup projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)w / (float)h;
        float fov = 60.0f * 3.14159f / 180.0f;
        float f = 1.0f / tanf(fov / 2.0f);
        float zNear = 0.1f, zFar = 1000.0f;
        float proj[16] = {
            f/aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (zFar+zNear)/(zNear-zFar), -1,
            0, 0, (2*zFar*zNear)/(zNear-zFar), 0
        };
        glMultMatrixf(proj);
        
        // Setup view (camera)
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        float pitch_rad = g_demo.cam_pitch * 3.14159f / 180.0f;
        float yaw_rad = g_demo.cam_yaw * 3.14159f / 180.0f;
        float look_x = cosf(pitch_rad) * sinf(yaw_rad);
        float look_y = sinf(pitch_rad);
        float look_z = -cosf(pitch_rad) * cosf(yaw_rad);
        
        // Simple lookAt replacement
        glRotatef(-g_demo.cam_pitch, 1, 0, 0);
        glRotatef(-g_demo.cam_yaw, 0, 1, 0);
        glTranslatef(-g_demo.cam_x, -g_demo.cam_y, -g_demo.cam_z);
        
        // Render terrain
        render_terrain();
        
        window_swap_buffers(&g_demo.window);
    }
    
    window_shutdown(&g_demo.window);
    return 0;
}
