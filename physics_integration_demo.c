// Integrated Physics Demo - Combines Both Systems
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

typedef struct { float x, y, z; } Vec3;

Vec3 vec3(float x, float y, float z) { return (Vec3){x, y, z}; }
Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float vec3_length(Vec3 v) { return sqrtf(vec3_dot(v, v)); }
Vec3 vec3_normalize(Vec3 v) { float l = vec3_length(v); return l > 0 ? vec3_mul(v, 1.0f/l) : vec3(0,0,0); }

// Combined System Types
typedef enum { BODY_STATIC, BODY_DYNAMIC, BODY_KINEMATIC } BodyType;
typedef enum { GEOM_SPHERE, GEOM_BOX, GEOM_PLANE, GEOM_CYLINDER } GeometryType;

typedef struct {
    float density, restitution, friction;
    Vec3 color;
} Material;

typedef struct {
    GeometryType type;
    Vec3 position, rotation, scale;
    Material material;
    bool enabled;
} GeometryBase;

typedef struct {
    GeometryBase base;
    float radius;
} Sphere;

typedef struct {
    GeometryBase base;
    Vec3 half_extents;
} Box;

typedef struct {
    GeometryBase base;
    Vec3 normal;
    float distance;
} Plane;

typedef struct {
    GeometryBase base;
    float radius, height;
} Cylinder;

typedef struct {
    GeometryBase* geometry;
    BodyType type;
    Vec3 velocity, angular_velocity;
    Vec3 force, torque;
    float mass, inv_mass;
    float restitution;
    bool is_sleeping;
    bool on_ground;
} RigidBody;

typedef struct {
    Vec3 point, normal;
    float depth;
    RigidBody* body_a;
    RigidBody* body_b;
} Contact;

#define MAX_BODIES 48
#define MAX_CONTACTS 96
#define GRAVITY vec3(0, -9.81f, 0)

typedef struct {
    RigidBody* bodies[MAX_BODIES];
    int body_count;
    Contact contacts[MAX_CONTACTS];
    int contact_count;
    float time_accumulator;
    Vec3 gravity;
    float wind_strength;
    Vec3 wind_direction;
} PhysicsWorld;

typedef struct {
    char buffer[35][90];
    Vec3 camera_pos;
    float zoom;
} Renderer;

static PhysicsWorld g_world;
static Renderer g_renderer;

Material materials[] = {
    {1.0f, 0.8f, 0.3f, {1.0f, 0.2f, 0.2f}}, // Red - Super Bouncy
    {2.5f, 0.9f, 0.1f, {0.2f, 1.0f, 0.2f}}, // Green - Heavy
    {0.3f, 0.6f, 0.8f, {0.2f, 0.2f, 1.0f}}, // Blue - Ultra Light
    {1.8f, 0.4f, 0.5f, {1.0f, 1.0f, 0.2f}}, // Yellow - Medium
    {3.2f, 0.7f, 0.2f, {1.0f, 0.5f, 0.0f}}, // Orange - Dense
    {0.9f, 0.5f, 0.9f, {0.8f, 0.2f, 0.8f}}, // Purple - Elastic
    {1.2f, 0.3f, 0.6f, {0.2f, 0.8f, 0.8f}}, // Cyan - Moderate
};

void physics_init(void) {
    memset(&g_world, 0, sizeof(g_world));
    g_world.gravity = GRAVITY;
    g_world.wind_strength = 0.0f;
    g_world.wind_direction = vec3(1, 0, 0);
}

Sphere* create_sphere(Vec3 pos, float radius, int material_id) {
    Sphere* sphere = malloc(sizeof(Sphere));
    sphere->base.type = GEOM_SPHERE;
    sphere->base.position = pos;
    sphere->base.rotation = vec3(0,0,0);
    sphere->base.scale = vec3(1,1,1);
    sphere->base.material = materials[material_id % 7];
    sphere->base.enabled = true;
    sphere->radius = radius;
    return sphere;
}

Box* create_box(Vec3 pos, Vec3 half_extents, int material_id) {
    Box* box = malloc(sizeof(Box));
    box->base.type = GEOM_BOX;
    box->base.position = pos;
    box->base.rotation = vec3(0,0,0);
    box->base.scale = vec3(1,1,1);
    box->base.material = materials[material_id % 7];
    box->base.enabled = true;
    box->half_extents = half_extents;
    return box;
}

Cylinder* create_cylinder(Vec3 pos, float radius, float height, int material_id) {
    Cylinder* cylinder = malloc(sizeof(Cylinder));
    cylinder->base.type = GEOM_CYLINDER;
    cylinder->base.position = pos;
    cylinder->base.rotation = vec3(0,0,0);
    cylinder->base.scale = vec3(1,1,1);
    cylinder->base.material = materials[material_id % 7];
    cylinder->base.enabled = true;
    cylinder->radius = radius;
    cylinder->height = height;
    return cylinder;
}

Plane* create_plane(Vec3 normal, float distance, int material_id) {
    Plane* plane = malloc(sizeof(Plane));
    plane->base.type = GEOM_PLANE;
    plane->base.position = vec3(0,0,0);
    plane->base.rotation = vec3(0,0,0);
    plane->base.scale = vec3(1,1,1);
    plane->base.material = materials[material_id % 7];
    plane->base.enabled = true;
    float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    plane->normal = len > 0 ? vec3(normal.x/len, normal.y/len, normal.z/len) : vec3(0,1,0);
    plane->distance = distance;
    return plane;
}

RigidBody* create_body(GeometryBase* geometry, BodyType type) {
    if (g_world.body_count >= MAX_BODIES) return NULL;
    
    RigidBody* body = malloc(sizeof(RigidBody));
    body->geometry = geometry;
    body->type = type;
    body->velocity = vec3(0,0,0);
    body->angular_velocity = vec3(0,0,0);
    body->force = vec3(0,0,0);
    body->torque = vec3(0,0,0);
    body->is_sleeping = false;
    body->on_ground = false;
    
    if (type == BODY_STATIC) {
        body->mass = 0.0f;
        body->inv_mass = 0.0f;
    } else {
        body->mass = geometry->material.density;
        body->inv_mass = 1.0f / body->mass;
    }
    
    body->restitution = geometry->material.restitution;
    g_world.bodies[g_world.body_count++] = body;
    return body;
}

// Enhanced Collision Detection
bool collide_sphere_sphere(Sphere* a, Sphere* b, Contact* contact) {
    Vec3 diff = vec3_sub(b->base.position, a->base.position);
    float dist_sq = vec3_dot(diff, diff);
    float radius_sum = a->radius + b->radius;
    
    if (dist_sq < radius_sum * radius_sum) {
        float dist = sqrtf(dist_sq);
        if (dist > 0.001f) {
            contact->normal = vec3_normalize(diff);
            contact->depth = radius_sum - dist;
            contact->point = vec3_add(a->base.position, vec3_mul(contact->normal, a->radius));
            return true;
        }
    }
    return false;
}

bool collide_sphere_box(Sphere* sphere, Box* box, Contact* contact) {
    Vec3 closest = vec3(
        fmaxf(box->base.position.x - box->half_extents.x, 
               fminf(sphere->base.position.x, box->base.position.x + box->half_extents.x)),
        fmaxf(box->base.position.y - box->half_extents.y, 
               fminf(sphere->base.position.y, box->base.position.y + box->half_extents.y)),
        fmaxf(box->base.position.z - box->half_extents.z, 
               fminf(sphere->base.position.z, box->base.position.z + box->half_extents.z))
    );
    
    Vec3 diff = vec3_sub(sphere->base.position, closest);
    float dist_sq = vec3_dot(diff, diff);
    
    if (dist_sq < sphere->radius * sphere->radius) {
        float dist = sqrtf(dist_sq);
        if (dist > 0.001f) {
            contact->normal = vec3_normalize(diff);
            contact->depth = sphere->radius - dist;
            contact->point = closest;
            return true;
        }
    }
    return false;
}

bool collide_cylinder_plane(Cylinder* cylinder, Plane* plane, Contact* contact) {
    Vec3 cylinder_to_plane = vec3_sub(cylinder->base.position, 
        vec3_mul(plane->normal, plane->distance));
    float dist = vec3_dot(cylinder_to_plane, plane->normal);
    
    if (fabsf(dist) < cylinder->radius) {
        contact->normal = dist > 0 ? plane->normal : vec3_mul(plane->normal, -1);
        contact->depth = cylinder->radius - fabsf(dist);
        contact->point = vec3_sub(cylinder->base.position, vec3_mul(contact->normal, dist));
        return true;
    }
    return false;
}

void broadphase_collision(void) {
    g_world.contact_count = 0;
    
    for (int i = 0; i < g_world.body_count; i++) {
        for (int j = i + 1; j < g_world.body_count; j++) {
            RigidBody* body_a = g_world.bodies[i];
            RigidBody* body_b = g_world.bodies[j];
            
            if (!body_a->geometry->enabled || !body_b->geometry->enabled) continue;
            if (body_a->type == BODY_STATIC && body_b->type == BODY_STATIC) continue;
            
            Contact contact = {0};
            bool collision = false;
            
            // Sphere-Sphere
            if (body_a->geometry->type == GEOM_SPHERE && body_b->geometry->type == GEOM_SPHERE) {
                collision = collide_sphere_sphere((Sphere*)body_a->geometry, (Sphere*)body_b->geometry, &contact);
            }
            // Sphere-Box
            else if (body_a->geometry->type == GEOM_SPHERE && body_b->geometry->type == GEOM_BOX) {
                collision = collide_sphere_box((Sphere*)body_a->geometry, (Box*)body_b->geometry, &contact);
            }
            else if (body_a->geometry->type == GEOM_BOX && body_b->geometry->type == GEOM_SPHERE) {
                collision = collide_sphere_box((Sphere*)body_b->geometry, (Box*)body_a->geometry, &contact);
                contact.normal = vec3_mul(contact.normal, -1);
            }
            // Cylinder-Plane
            else if (body_a->geometry->type == GEOM_CYLINDER && body_b->geometry->type == GEOM_PLANE) {
                collision = collide_cylinder_plane((Cylinder*)body_a->geometry, (Plane*)body_b->geometry, &contact);
            }
            else if (body_a->geometry->type == GEOM_PLANE && body_b->geometry->type == GEOM_CYLINDER) {
                collision = collide_cylinder_plane((Cylinder*)body_b->geometry, (Plane*)body_a->geometry, &contact);
                contact.normal = vec3_mul(contact.normal, -1);
            }
            
            if (collision && g_world.contact_count < MAX_CONTACTS) {
                contact.body_a = body_a;
                contact.body_b = body_b;
                g_world.contacts[g_world.contact_count++] = contact;
            }
        }
    }
}

void resolve_collision(Contact* contact) {
    RigidBody* a = contact->body_a;
    RigidBody* b = contact->body_b;
    
    Vec3 relative_vel = vec3_sub(b->velocity, a->velocity);
    float vel_along_normal = vec3_dot(relative_vel, contact->normal);
    
    if (vel_along_normal > 0) return;
    
    float e = (a->restitution + b->restitution) * 0.5f;
    float j = -(1 + e) * vel_along_normal;
    j /= a->inv_mass + b->inv_mass;
    
    Vec3 impulse = vec3_mul(contact->normal, j);
    
    if (a->type != BODY_STATIC) {
        a->velocity = vec3_sub(a->velocity, vec3_mul(impulse, a->inv_mass));
    }
    if (b->type != BODY_STATIC) {
        b->velocity = vec3_add(b->velocity, vec3_mul(impulse, b->inv_mass));
    }
    
    const float percent = 0.2f;
    const float slop = 0.01f;
    Vec3 correction = vec3_mul(contact->normal, fmaxf(contact->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) * percent);
    
    if (a->type != BODY_STATIC) {
        a->geometry->position = vec3_sub(a->geometry->position, vec3_mul(correction, a->inv_mass));
    }
    if (b->type != BODY_STATIC) {
        b->geometry->position = vec3_add(b->geometry->position, vec3_mul(correction, b->inv_mass));
    }
}

void integrate_forces(RigidBody* body, float dt) {
    if (body->type == BODY_STATIC || body->is_sleeping) return;
    
    // Gravity
    body->force = vec3_add(body->force, vec3_mul(g_world.gravity, body->mass));
    
    // Wind force
    if (g_world.wind_strength > 0.1f) {
        Vec3 wind_force = vec3_mul(g_world.wind_direction, g_world.wind_strength * body->mass * 0.1f);
        body->force = vec3_add(body->force, wind_force);
    }
    
    // Air resistance
    Vec3 drag = vec3_mul(body->velocity, -0.02f * body->mass);
    body->force = vec3_add(body->force, drag);
    
    // Integrate forces to velocity
    body->velocity = vec3_add(body->velocity, vec3_mul(body->force, body->inv_mass * dt));
    
    // Angular velocity damping
    body->angular_velocity = vec3_mul(body->angular_velocity, 0.98f);
}

void integrate_velocity(RigidBody* body, float dt) {
    if (body->type == BODY_STATIC || body->is_sleeping) return;
    
    // Integrate velocity to position
    body->geometry->position = vec3_add(body->geometry->position, vec3_mul(body->velocity, dt));
    
    // Simple rotation
    body->geometry->rotation = vec3_add(body->geometry->rotation, vec3_mul(body->angular_velocity, dt));
    
    // Clear forces
    body->force = vec3(0,0,0);
    body->torque = vec3(0,0,0);
}

void physics_step(float dt) {
    const float fixed_dt = 1.0f / 60.0f;
    g_world.time_accumulator += dt;
    
    while (g_world.time_accumulator >= fixed_dt) {
        // Integrate forces
        for (int i = 0; i < g_world.body_count; i++) {
            integrate_forces(g_world.bodies[i], fixed_dt);
        }
        
        // Detect collisions
        broadphase_collision();
        
        // Resolve collisions
        for (int i = 0; i < g_world.contact_count; i++) {
            resolve_collision(&g_world.contacts[i]);
        }
        
        // Integrate velocities
        for (int i = 0; i < g_world.body_count; i++) {
            integrate_velocity(g_world.bodies[i], fixed_dt);
        }
        
        g_world.time_accumulator -= fixed_dt;
    }
}

// Enhanced Rendering
Vec3 project_3d_to_2d(Vec3 pos) {
    Vec3 camera_offset = vec3_sub(pos, g_renderer.camera_pos);
    return vec3(
        45 + camera_offset.x * g_renderer.zoom,
        17 - camera_offset.y * g_renderer.zoom * 0.7f,
        camera_offset.z
    );
}

void renderer_clear(void) {
    memset(g_renderer.buffer, ' ', sizeof(g_renderer.buffer));
}

void renderer_draw_sphere(Sphere* sphere) {
    Vec3 screen_pos = project_3d_to_2d(sphere->base.position);
    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;
    int radius = (int)(sphere->radius * g_renderer.zoom);
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int sx = x + dx;
                int sy = y + dy;
                if (sx >= 0 && sx < 90 && sy >= 0 && sy < 35) {
                    g_renderer.buffer[sy][sx] = 'O';
                }
            }
        }
    }
}

void renderer_draw_box(Box* box) {
    Vec3 screen_pos = project_3d_to_2d(box->base.position);
    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;
    int w = (int)(box->half_extents.x * g_renderer.zoom);
    int h = (int)(box->half_extents.y * g_renderer.zoom);
    
    for (int dy = -h; dy <= h; dy++) {
        for (int dx = -w; dx <= w; dx++) {
            int sx = x + dx;
            int sy = y + dy;
            if (sx >= 0 && sx < 90 && sy >= 0 && sy < 35) {
                g_renderer.buffer[sy][sx] = '#';
            }
        }
    }
}

void renderer_draw_cylinder(Cylinder* cylinder) {
    Vec3 screen_pos = project_3d_to_2d(cylinder->base.position);
    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;
    int radius = (int)(cylinder->radius * g_renderer.zoom);
    int height = (int)(cylinder->height * g_renderer.zoom * 0.5f);
    
    for (int dy = -height; dy <= height; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int sx = x + dx;
                int sy = y + dy;
                if (sx >= 0 && sx < 90 && sy >= 0 && sy < 35) {
                    g_renderer.buffer[sy][sx] = '@';
                }
            }
        }
    }
}

void renderer_draw_plane(Plane* plane) {
    Vec3 center = vec3_mul(plane->normal, plane->distance);
    Vec3 screen_pos = project_3d_to_2d(center);
    int y = (int)screen_pos.y;
    
    for (int x = 0; x < 90; x++) {
        if (y >= 0 && y < 35) {
            g_renderer.buffer[y][x] = '=';
        }
    }
}

void renderer_render(void) {
    renderer_clear();
    
    // Sort by Z position for depth
    for (int i = 0; i < g_world.body_count - 1; i++) {
        for (int j = i + 1; j < g_world.body_count; j++) {
            if (g_world.bodies[i]->geometry->position.z > g_world.bodies[j]->geometry->position.z) {
                RigidBody* temp = g_world.bodies[i];
                g_world.bodies[i] = g_world.bodies[j];
                g_world.bodies[j] = temp;
            }
        }
    }
    
    // Draw all geometries
    for (int i = 0; i < g_world.body_count; i++) {
        GeometryBase* geom = g_world.bodies[i]->geometry;
        switch (geom->type) {
            case GEOM_SPHERE:
                renderer_draw_sphere((Sphere*)geom);
                break;
            case GEOM_BOX:
                renderer_draw_box((Box*)geom);
                break;
            case GEOM_CYLINDER:
                renderer_draw_cylinder((Cylinder*)geom);
                break;
            case GEOM_PLANE:
                renderer_draw_plane((Plane*)geom);
                break;
        }
    }
    
    printf("\033[2J\033[H");
    for (int y = 0; y < 35; y++) {
        for (int x = 0; x < 90; x++) {
            putchar(g_renderer.buffer[y][x]);
        }
        putchar('\n');
    }
    
    printf("\n=== INTEGRATED PHYSICS SYSTEM ===\n");
    printf("Bodies: %d | Contacts: %d | Gravity: %.1f m/s²\n", 
           g_world.body_count, g_world.contact_count, g_world.gravity.y);
    printf("Wind: %.1f | Zoom: %.1fx\n", g_world.wind_strength, g_renderer.zoom);
    printf("Systems: Multi-Geometry, Wind, Air Resistance, Advanced Collisions\n");
}

void init_integrated_scene(void) {
    physics_init();
    
    // Ground plane
    Plane* ground = create_plane(vec3(0, 1, 0), 0, 0);
    create_body((GeometryBase*)ground, BODY_STATIC);
    
    // Various spheres
    for (int i = 0; i < 8; i++) {
        Sphere* sphere = create_sphere(
            vec3(-12 + i * 3.0f, 8 + (i % 3) * 1.5f, (i % 2) * 2.0f - 1.0f),
            0.3f + (i % 4) * 0.08f,
            i % 7
        );
        RigidBody* body = create_body((GeometryBase*)sphere, BODY_DYNAMIC);
        body->velocity = vec3((i % 5 - 2) * 1.2f, 0, (i % 3 - 1) * 0.8f);
        body->angular_velocity = vec3(0, (i % 3 - 1) * 2.0f, 0);
    }
    
    // Boxes
    for (int i = 0; i < 4; i++) {
        Box* box = create_box(
            vec3(-10 + i * 5.0f, 6, (i % 2) * 1.5f),
            vec3(0.7f, 0.7f, 0.7f),
            (i + 3) % 7
        );
        RigidBody* body = create_body((GeometryBase*)box, BODY_DYNAMIC);
        body->velocity = vec3(0, 0, 0);
        body->angular_velocity = vec3((i % 2 - 0.5f) * 1.5f, 0, 0);
    }
    
    // Cylinders
    for (int i = 0; i < 3; i++) {
        Cylinder* cylinder = create_cylinder(
            vec3(-8 + i * 6.0f, 10, 0),
            0.4f + i * 0.1f,
            1.5f + i * 0.3f,
            (i + 5) % 7
        );
        RigidBody* body = create_body((GeometryBase*)cylinder, BODY_DYNAMIC);
        body->velocity = vec3((i % 3 - 1) * 0.8f, 0, 0);
        body->angular_velocity = vec3(0, 0, (i % 2) * 3.0f);
    }
    
    g_renderer.camera_pos = vec3(0, 5, 20);
    g_renderer.zoom = 2.0f;
}

void add_random_object(void) {
    if (g_world.body_count >= MAX_BODIES) return;
    
    int obj_type = rand() % 3;
    GeometryBase* geom = NULL;
    
    switch (obj_type) {
        case 0: {
            Sphere* sphere = create_sphere(
                vec3(-15 + (rand() % 300) / 10.0f, 12 + (rand() % 50) / 10.0f, (rand() % 40) / 10.0f - 2.0f),
                0.3f + (rand() % 30) / 100.0f,
                rand() % 7
            );
            geom = (GeometryBase*)sphere;
            break;
        }
        case 1: {
            Box* box = create_box(
                vec3(-12 + (rand() % 240) / 10.0f, 10 + (rand() % 40) / 10.0f, (rand() % 30) / 10.0f - 1.5f),
                vec3(0.4f + (rand() % 40) / 100.0f, 0.4f + (rand() % 40) / 100.0f, 0.4f + (rand() % 40) / 100.0f),
                rand() % 7
            );
            geom = (GeometryBase*)box;
            break;
        }
        case 2: {
            Cylinder* cylinder = create_cylinder(
                vec3(-10 + (rand() % 200) / 10.0f, 11 + (rand() % 45) / 10.0f, (rand() % 35) / 10.0f - 1.75f),
                0.3f + (rand() % 25) / 100.0f,
                1.0f + (rand() % 50) / 100.0f,
                rand() % 7
            );
            geom = (GeometryBase*)cylinder;
            break;
        }
    }
    
    if (geom) {
        RigidBody* body = create_body(geom, BODY_DYNAMIC);
        body->velocity = vec3((rand() % 100 - 50) / 20.0f, 0, (rand() % 100 - 50) / 30.0f);
        body->angular_velocity = vec3((rand() % 100 - 50) / 25.0f, (rand() % 100 - 50) / 25.0f, (rand() % 100 - 50) / 25.0f);
    }
}

int main(void) {
    printf("Starting INTEGRATED Advanced Physics System\n");
    printf("Comprehensive Features:\n");
    printf("- Multi-Geometry Types: Spheres, Boxes, Cylinders, Planes\n");
    printf("- Advanced Collision: Sphere-Sphere, Sphere-Box, Cylinder-Plane\n");
    printf("- Environmental Forces: Gravity, Wind, Air Resistance\n");
    printf("- Material System: 7 Different Materials with Properties\n");
    printf("- Angular Physics: Rotation and Angular Velocity\n");
    printf("- Visual System: Depth-Sorted Rendering with Zoom\n");
    printf("- Dynamic Spawning: Random Object Generation\n\n");
    
    srand((unsigned int)time(NULL));
    init_integrated_scene();
    
    float last_time = (float)clock() / CLOCKS_PER_SEC;
    bool running = true;
    int frame_count = 0;
    
    while (running) {
        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;
        
        if (dt > 0.05f) dt = 0.05f;
        
        // Dynamic effects
        if (frame_count % 40 == 0) {
            g_world.wind_strength = 2.0f + sinf(frame_count * 0.02f) * 1.5f;
            g_world.wind_direction.x = cosf(frame_count * 0.01f);
            g_world.wind_direction.z = sinf(frame_count * 0.01f);
        }
        
        if (frame_count % 25 == 0) {
            add_random_object();
        }
        
        if (frame_count % 80 == 0) {
            g_renderer.zoom = 1.5f + sinf(frame_count * 0.005f) * 0.5f;
        }
        
        if (frame_count % 150 == 0) {
            init_integrated_scene();
        }
        
        physics_step(dt);
        renderer_render();
        
        frame_count++;
        
        struct timespec ts = {0, 33333333};
        nanosleep(&ts, NULL);
        
        if (frame_count > 900) running = false;
    }
    
    printf("\nINTEGRATED Physics System Complete!\n");
    printf("Demonstrated: Multi-Geometry, Advanced Collisions, Environmental Forces\n");
    return 0;
}
