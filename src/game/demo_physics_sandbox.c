// src/game/demo_physics_sandbox.c
// Standalone Physics Sandbox Demo (Bypassing Engine Monolith)

#include <stdio.h>
#include <unistd.h> // for sleep/usleep if needed
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#include <math.h>

#include <core/window.h>
#include <core/logger.h>
#include <physics/physics.h>
#include <math/vec3.h>
#include <math/quat.h>

// -----------------------------------------------------------------------------
// Constants & Globals
// -----------------------------------------------------------------------------

#define MAX_BODIES 200

typedef struct {
    Window window;
    PhysicsWorld *phys;
    RigidBody *bodies[MAX_BODIES];
    int body_count;
    bool paused;
} DemoState;

static DemoState g_demo;

// -----------------------------------------------------------------------------
// Scene Setup
// -----------------------------------------------------------------------------

void spawn_box(Vec3 pos, Vec3 size, float mass, bool is_static) {
    if (g_demo.body_count >= MAX_BODIES) return;
    
    RigidBody *b = rigid_body_create(is_static ? BODY_TYPE_STATIC : BODY_TYPE_DYNAMIC, pos);
    Collider *c = collider_create_box(vec3(size.x*0.5f, size.y*0.5f, size.z*0.5f)); // Half-extents
    rigid_body_attach_collider(b, c);
    
    if (!is_static) {
        rigid_body_set_mass(b, mass);
        // rigid_body_set_inertia_tensor(b, ...); // Auto-calc or dummy
    }
    
    rigid_body_set_restitution(b, 0.4f);
    rigid_body_set_friction(b, 0.5f);
    
    physics_world_add_body(g_demo.phys, b);
    g_demo.bodies[g_demo.body_count++] = b;
}

void spawn_sphere(Vec3 pos, float radius, float mass) {
    if (g_demo.body_count >= MAX_BODIES) return;
    
    RigidBody *b = rigid_body_create(BODY_TYPE_DYNAMIC, pos);
    Collider *c = collider_create_sphere(radius);
    rigid_body_attach_collider(b, c);
    rigid_body_set_mass(b, mass);
    rigid_body_set_restitution(b, 0.7f);
    
    physics_world_add_body(g_demo.phys, b);
    g_demo.bodies[g_demo.body_count++] = b;
}

void setup_scene() {
    // Ground
    spawn_box(vec3(0, -1, 0), vec3(50, 2, 50), 0, true);
    
    // Stack of boxes
    int rows = 6;
    float size = 1.0f;
    float gap = 0.05f;
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j <= i; j++) {
             float x = (j * (size + gap)) - (i * (size + gap) * 0.5f);
             float y = 0.5f + i * (size + gap) + size*0.5f; // Start above ground
             spawn_box(vec3(x, y, 0), vec3(size, size, size), 2.0f, false);
        }
    }
}

// -----------------------------------------------------------------------------
// Rendering Helpers
// -----------------------------------------------------------------------------

void draw_cube(Vec3 pos, Quat rot, Vec3 size, Vec3 color) {
    glPushMatrix();
    
    glTranslatef(pos.x, pos.y, pos.z);
    
    // Quat to Axis Angle
    float angle = 2.0f * acosf(rot.w);
    float s = sqrtf(1.0f - rot.w * rot.w);
    if (s < 0.001f) {
        // No rotation
    } else {
        glRotatef(angle * 180.0f / 3.14159f, rot.x / s, rot.y / s, rot.z / s);
    }
    
    glScalef(size.x, size.y, size.z);
    
    glColor3f(color.x, color.y, color.z);
    
    // Simple Cube (Immediate mode)
    glBegin(GL_QUADS);
    // Top
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f); glVertex3f( 0.5f, 0.5f, -0.5f);
    // Bottom
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f( 0.5f, -0.5f, -0.5f);
    glVertex3f( 0.5f, -0.5f, 0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    // Front
    glVertex3f(-0.5f, -0.5f, 0.5f); glVertex3f( 0.5f, -0.5f, 0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, 0.5f);
    // Back
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f( 0.5f, 0.5f, -0.5f); glVertex3f( 0.5f, -0.5f, -0.5f);
    // Left
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f); glVertex3f(-0.5f, 0.5f, -0.5f);
    // Right
    glVertex3f( 0.5f, -0.5f, -0.5f); glVertex3f( 0.5f, 0.5f, -0.5f);
    glVertex3f( 0.5f, 0.5f, 0.5f); glVertex3f( 0.5f, -0.5f, 0.5f);
    glEnd();
    
    // Wireframe outline
    glColor3f(0,0,0);
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // Re-draw cube lines... omitted for brevity, solid is fine.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glPopMatrix();
}

void draw_sphere(Vec3 pos, float radius) {
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    
    // Draw approx sphere as a point or icosahedron? 
    // Just a small cube for now to represent center
    glColor3f(1, 0, 0);
    float r = radius;
    glBegin(GL_QUADS);
    // Minimal box representation of sphere
    glVertex3f(-r, r, 0); glVertex3f(r, r, 0); glVertex3f(r, -r, 0); glVertex3f(-r, -r, 0);
    glEnd();
    
    glPopMatrix();
}


// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------

int main() {
    printf("Initializing Demo...\n");
    
    if (!window_init(&g_demo.window, 1280, 720, "Physics Sandbox (Standalone)", false)) {
        return 1;
    }
    
    PhysicsConfig pconf = { {0, -9.81f, 0}, 1.0f/60.0f, 10, 4 };
    g_demo.phys = physics_world_create(pconf);
    
    setup_scene();
    
    // GL Setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    while (!window_should_close(&g_demo.window)) {
        window_poll_events();
        
        // Input (GLFW direct if engine input not valid)
        if (glfwGetKey(g_demo.window.handle, GLFW_KEY_SPACE) == GLFW_PRESS) {
             spawn_sphere(vec3(0, 10, 0), 1.0f, 5.0f);
             usleep(100000); // Debounce
        }
        
        // Update
        if (!g_demo.paused) {
            physics_world_step(g_demo.phys, 1.0f/60.0f);
        }
        
        // Render
        int w, h;
        glfwGetFramebufferSize(g_demo.window.handle, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)w / (float)h;
        // gluPerspective(45, aspect, 0.1, 100) -> Manual frustum
        float fov = 45.0f * 3.14159f / 180.0f;
        float f = 1.0f / tanf(fov/2);
        float zNear = 0.1f, zFar = 100.0f;
        float m[16] = {
            f/aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, (zFar+zNear)/(zNear-zFar), -1,
            0, 0, (2*zFar*zNear)/(zNear-zFar), 0
        };
        glMultMatrixf(m);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Camera (0, 5, 20) looking at (0, 5, 0)
        glTranslatef(0, -5, -20); 
        
        // Draw Ground
        // (Handled by bodies iteration)
        
        for (int i = 0; i < g_demo.body_count; i++) {
            RigidBody *b = g_demo.bodies[i];
            Collider *collider = rigid_body_get_collider(b);
            if (!collider) continue;
            
            Vec3 pos = rigid_body_get_position(b);
            Quat rot = rigid_body_get_rotation(b);
            bool sleeping = rigid_body_is_sleeping(b);
            
            if (collider_get_type(collider) == COLLIDER_TYPE_BOX) {
                 f32 hx, hy, hz;
                 collider_get_box_half_extents(collider, &hx, &hy, &hz);
                 Vec3 size = vec3(hx*2, hy*2, hz*2);
                 draw_cube(pos, rot, size, sleeping ? vec3(0.4,0.4,0.4) : vec3(0.7, 0.3, 0.3));
            } else {
                 f32 radius = collider_get_sphere_radius(collider);
                draw_sphere(pos, radius);
            }
        }
        
        window_swap_buffers(&g_demo.window);
    }
    
    physics_world_destroy(g_demo.phys);
    window_shutdown(&g_demo.window);
    
    return 0;
}
