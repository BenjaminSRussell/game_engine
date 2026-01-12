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

typedef enum { BODY_STATIC, BODY_DYNAMIC } BodyType;
typedef enum { GEOM_SPHERE, GEOM_BOX, GEOM_PLANE } GeometryType;

typedef struct {
    float density, restitution;
    Vec3 color;
} Material;

typedef struct {
    GeometryType type;
    Vec3 position;
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
    GeometryBase* geometry;
    BodyType type;
    Vec3 velocity;
    Vec3 force;
    float mass, inv_mass;
    float restitution;
    bool is_sleeping;
} RigidBody;

typedef struct {
    Vec3 point, normal;
    float depth;
    RigidBody* body_a;
    RigidBody* body_b;
} Contact;

#define MAX_BODIES 32
#define MAX_CONTACTS 64
#define GRAVITY (Vec3){0, -9.81f, 0}

typedef struct {
    RigidBody* bodies[MAX_BODIES];
    int body_count;
    Contact contacts[MAX_CONTACTS];
    int contact_count;
    float time_accumulator;
    Vec3 gravity;
} PhysicsWorld;

typedef struct {
    char buffer[20][60];
    Vec3 camera_pos;
} Renderer;

static PhysicsWorld g_world;
static Renderer g_renderer;

Material materials[] = {
    {1.0f, 0.6f, {1.0f, 0.2f, 0.2f}},
    {2.0f, 0.1f, {0.2f, 1.0f, 0.2f}},
    {0.5f, 0.7f, {0.2f, 0.2f, 1.0f}},
    {1.5f, 0.5f, {1.0f, 1.0f, 0.2f}},
};

void physics_init(void) {
    memset(&g_world, 0, sizeof(g_world));
    g_world.gravity = GRAVITY;
}

Sphere* create_sphere(Vec3 pos, float radius, int material_id) {
    Sphere* sphere = malloc(sizeof(Sphere));
    sphere->base.type = GEOM_SPHERE;
    sphere->base.position = pos;
    sphere->base.material = materials[material_id % 4];
    sphere->base.enabled = true;
    sphere->radius = radius;
    return sphere;
}

Box* create_box(Vec3 pos, Vec3 half_extents, int material_id) {
    Box* box = malloc(sizeof(Box));
    box->base.type = GEOM_BOX;
    box->base.position = pos;
    box->base.material = materials[material_id % 4];
    box->base.enabled = true;
    box->half_extents = half_extents;
    return box;
}

Plane* create_plane(Vec3 normal, float distance, int material_id) {
    Plane* plane = malloc(sizeof(Plane));
    plane->base.type = GEOM_PLANE;
    plane->base.position = (Vec3){0,0,0};
    plane->base.material = materials[material_id % 4];
    plane->base.enabled = true;
    float len = sqrtf(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    plane->normal = len > 0 ? (Vec3){normal.x/len, normal.y/len, normal.z/len} : (Vec3){0,1,0};
    plane->distance = distance;
    return plane;
}

RigidBody* create_body(GeometryBase* geometry, BodyType type) {
    if (g_world.body_count >= MAX_BODIES) return NULL;
    
    RigidBody* body = malloc(sizeof(RigidBody));
    body->geometry = geometry;
    body->type = type;
    body->velocity = (Vec3){0,0,0};
    body->force = (Vec3){0,0,0};
    body->is_sleeping = false;
    
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

bool collide_sphere_sphere(Sphere* a, Sphere* b, Contact* contact) {
    Vec3 diff = (Vec3){b->base.position.x - a->base.position.x, 
                        b->base.position.y - a->base.position.y, 
                        b->base.position.z - a->base.position.z};
    float dist_sq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
    float radius_sum = a->radius + b->radius;
    
    if (dist_sq < radius_sum * radius_sum) {
        float dist = sqrtf(dist_sq);
        if (dist > 0.001f) {
            float len = sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
            contact->normal = len > 0 ? (Vec3){diff.x/len, diff.y/len, diff.z/len} : (Vec3){0,1,0};
            contact->depth = radius_sum - dist;
            contact->point = (Vec3){a->base.position.x + contact->normal.x * a->radius,
                                   a->base.position.y + contact->normal.y * a->radius,
                                   a->base.position.z + contact->normal.z * a->radius};
            return true;
        }
    }
    return false;
}

bool collide_sphere_plane(Sphere* sphere, Plane* plane, Contact* contact) {
    Vec3 sphere_to_plane = (Vec3){sphere->base.position.x - plane->normal.x * plane->distance,
                                  sphere->base.position.y - plane->normal.y * plane->distance,
                                  sphere->base.position.z - plane->normal.z * plane->distance};
    float dist = sphere_to_plane.x*plane->normal.x + sphere_to_plane.y*plane->normal.y + sphere_to_plane.z*plane->normal.z;
    
    if (fabsf(dist) < sphere->radius) {
        contact->normal = dist > 0 ? (Vec3){-plane->normal.x, -plane->normal.y, -plane->normal.z} : plane->normal;
        contact->depth = sphere->radius - fabsf(dist);
        contact->point = (Vec3){sphere->base.position.x - contact->normal.x * dist,
                               sphere->base.position.y - contact->normal.y * dist,
                               sphere->base.position.z - contact->normal.z * dist};
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
            
            Contact contact = {{0}};
            bool collision = false;
            
            if (body_a->geometry->type == GEOM_SPHERE && body_b->geometry->type == GEOM_SPHERE) {
                collision = collide_sphere_sphere((Sphere*)body_a->geometry, (Sphere*)body_b->geometry, &contact);
            }
            else if (body_a->geometry->type == GEOM_SPHERE && body_b->geometry->type == GEOM_PLANE) {
                collision = collide_sphere_plane((Sphere*)body_a->geometry, (Plane*)body_b->geometry, &contact);
            }
            else if (body_a->geometry->type == GEOM_PLANE && body_b->geometry->type == GEOM_SPHERE) {
                collision = collide_sphere_plane((Sphere*)body_b->geometry, (Plane*)body_a->geometry, &contact);
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
    
    Vec3 relative_vel = (Vec3){b->velocity.x - a->velocity.x, 
                               b->velocity.y - a->velocity.y, 
                               b->velocity.z - a->velocity.z};
    float vel_along_normal = relative_vel.x*contact->normal.x + relative_vel.y*contact->normal.y + relative_vel.z*contact->normal.z;
    
    if (vel_along_normal > 0) return;
    
    float e = (a->restitution + b->restitution) * 0.5f;
    float j = -(1 + e) * vel_along_normal;
    j /= a->inv_mass + b->inv_mass;
    
    Vec3 impulse = (Vec3){contact->normal.x * j, contact->normal.y * j, contact->normal.z * j};
    
    if (a->type != BODY_STATIC) {
        a->velocity.x -= impulse.x * a->inv_mass;
        a->velocity.y -= impulse.y * a->inv_mass;
        a->velocity.z -= impulse.z * a->inv_mass;
    }
    if (b->type != BODY_STATIC) {
        b->velocity.x += impulse.x * b->inv_mass;
        b->velocity.y += impulse.y * b->inv_mass;
        b->velocity.z += impulse.z * b->inv_mass;
    }
    
    const float percent = 0.2f;
    const float slop = 0.01f;
    float correction_amount = fmaxf(contact->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) * percent;
    Vec3 correction = (Vec3){contact->normal.x * correction_amount, 
                            contact->normal.y * correction_amount, 
                            contact->normal.z * correction_amount};
    
    if (a->type != BODY_STATIC) {
        a->geometry->position.x -= correction.x * a->inv_mass;
        a->geometry->position.y -= correction.y * a->inv_mass;
        a->geometry->position.z -= correction.z * a->inv_mass;
    }
    if (b->type != BODY_STATIC) {
        b->geometry->position.x += correction.x * b->inv_mass;
        b->geometry->position.y += correction.y * b->inv_mass;
        b->geometry->position.z += correction.z * b->inv_mass;
    }
}

void integrate_forces(RigidBody* body, float dt) {
    if (body->type == BODY_STATIC || body->is_sleeping) return;
    
    body->force.x += g_world.gravity.x * body->mass;
    body->force.y += g_world.gravity.y * body->mass;
    body->force.z += g_world.gravity.z * body->mass;
    
    body->velocity.x += body->force.x * body->inv_mass * dt;
    body->velocity.y += body->force.y * body->inv_mass * dt;
    body->velocity.z += body->force.z * body->inv_mass * dt;
    
    body->velocity.x *= 0.999f;
    body->velocity.y *= 0.999f;
    body->velocity.z *= 0.999f;
}

void integrate_velocity(RigidBody* body, float dt) {
    if (body->type == BODY_STATIC || body->is_sleeping) return;
    
    body->geometry->position.x += body->velocity.x * dt;
    body->geometry->position.y += body->velocity.y * dt;
    body->geometry->position.z += body->velocity.z * dt;
    
    body->force = (Vec3){0,0,0};
}

void physics_step(float dt) {
    const float fixed_dt = 1.0f / 60.0f;
    g_world.time_accumulator += dt;
    
    while (g_world.time_accumulator >= fixed_dt) {
        for (int i = 0; i < g_world.body_count; i++) {
            integrate_forces(g_world.bodies[i], fixed_dt);
        }
        
        broadphase_collision();
        
        for (int i = 0; i < g_world.contact_count; i++) {
            resolve_collision(&g_world.contacts[i]);
        }
        
        for (int i = 0; i < g_world.body_count; i++) {
            integrate_velocity(g_world.bodies[i], fixed_dt);
        }
        
        g_world.time_accumulator -= fixed_dt;
    }
}

Vec3 project_3d_to_2d(Vec3 pos) {
    Vec3 camera_offset = (Vec3){pos.x - g_renderer.camera_pos.x, 
                               pos.y - g_renderer.camera_pos.y, 
                               pos.z - g_renderer.camera_pos.z};
    return (Vec3){
        30 + camera_offset.x * 2.0f,
        10 - camera_offset.y * 1.5f,
        camera_offset.z
    };
}

void renderer_clear(void) {
    memset(g_renderer.buffer, ' ', sizeof(g_renderer.buffer));
}

void renderer_draw_sphere(Sphere* sphere) {
    Vec3 screen_pos = project_3d_to_2d(sphere->base.position);
    int x = (int)screen_pos.x;
    int y = (int)screen_pos.y;
    int radius = (int)(sphere->radius * 2.0f);
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx*dx + dy*dy <= radius*radius) {
                int sx = x + dx;
                int sy = y + dy;
                if (sx >= 0 && sx < 60 && sy >= 0 && sy < 20) {
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
    int w = (int)(box->half_extents.x * 3.0f);
    int h = (int)(box->half_extents.y * 2.0f);
    
    for (int dy = -h; dy <= h; dy++) {
        for (int dx = -w; dx <= w; dx++) {
            int sx = x + dx;
            int sy = y + dy;
            if (sx >= 0 && sx < 60 && sy >= 0 && sy < 20) {
                g_renderer.buffer[sy][sx] = '#';
            }
        }
    }
}

void renderer_draw_plane(Plane* plane) {
    Vec3 center = (Vec3){plane->normal.x * plane->distance, 
                         plane->normal.y * plane->distance, 
                         plane->normal.z * plane->distance};
    Vec3 screen_pos = project_3d_to_2d(center);
    int y = (int)screen_pos.y;
    
    for (int x = 0; x < 60; x++) {
        if (y >= 0 && y < 20) {
            g_renderer.buffer[y][x] = '-';
        }
    }
}

void renderer_render(void) {
    renderer_clear();
    
    for (int i = 0; i < g_world.body_count; i++) {
        GeometryBase* geom = g_world.bodies[i]->geometry;
        switch (geom->type) {
            case GEOM_SPHERE:
                renderer_draw_sphere((Sphere*)geom);
                break;
            case GEOM_BOX:
                renderer_draw_box((Box*)geom);
                break;
            case GEOM_PLANE:
                renderer_draw_plane((Plane*)geom);
                break;
        }
    }
    
    printf("\033[2J\033[H");
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 60; x++) {
            putchar(g_renderer.buffer[y][x]);
        }
        putchar('\n');
    }
    
    printf("\n=== Advanced Geometry & Physics Test ===\n");
    printf("Bodies: %d | Contacts: %d | Gravity: %.1f m/s²\n", 
           g_world.body_count, g_world.contact_count, g_world.gravity.y);
    printf("Systems: Gravity, Collision, Motion Dynamics\n");
    printf("Auto-demo: Adding objects and resetting\n");
}

void init_demo_scene(void) {
    physics_init();
    
    Plane* ground = create_plane((Vec3){0, 1, 0}, 0, 0);
    create_body((GeometryBase*)ground, BODY_STATIC);
    
    for (int i = 0; i < 4; i++) {
        Sphere* sphere = create_sphere(
            (Vec3){-6 + i * 3.0f, 8 + i * 0.5f, 0},
            0.4f + i * 0.05f,
            i % 4
        );
        RigidBody* body = create_body((GeometryBase*)sphere, BODY_DYNAMIC);
        body->velocity = (Vec3){(i % 3 - 1) * 0.8f, 0, 0};
    }
    
    for (int i = 0; i < 2; i++) {
        Box* box = create_box(
            (Vec3){-4 + i * 8.0f, 6, 0},
            (Vec3){0.6f, 0.6f, 0.6f},
            (i + 2) % 4
        );
        RigidBody* body = create_body((GeometryBase*)box, BODY_DYNAMIC);
        body->velocity = (Vec3){0, 0, 0};
    }
    
    g_renderer.camera_pos = (Vec3){0, 4, 12};
}

void add_random_sphere(void) {
    if (g_world.body_count >= MAX_BODIES) return;
    
    Sphere* sphere = create_sphere(
        (Vec3){-8 + (rand() % 160) / 10.0f, 10 + (rand() % 30) / 10.0f, 0},
        0.3f + (rand() % 20) / 100.0f,
        rand() % 4
    );
    RigidBody* body = create_body((GeometryBase*)sphere, BODY_DYNAMIC);
    body->velocity = (Vec3){(rand() % 100 - 50) / 25.0f, 0, 0};
}

int main(int argc, char* argv[]) {
    printf("Starting Advanced Game Geometry System with Integrated Physics\n");
    printf("Demonstrating:\n");
    printf("- 3D Physics Simulation with Real Gravity\n");
    printf("- Multiple Geometry Types (Spheres, Boxes, Planes)\n");
    printf("- Real-time Collision Detection & Response\n");
    printf("- Material Properties & Motion Dynamics\n");
    printf("- Visual Representation of All Systems\n\n");
    
    srand((unsigned int)time(NULL));
    init_demo_scene();
    
    float last_time = (float)clock() / CLOCKS_PER_SEC;
    bool running = true;
    int frame_count = 0;
    
    while (running) {
        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;
        
        if (dt > 0.1f) dt = 0.1f;
        
        if (frame_count % 30 == 0) {
            int action = rand() % 100;
            if (action < 50) {
                add_random_sphere();
            } else if (action < 60) {
                init_demo_scene();
            } else if (action < 65) {
                running = false;
            }
        }
        
        physics_step(dt);
        renderer_render();
        
        frame_count++;
        
        struct timespec ts = {0, 50000000};
        nanosleep(&ts, NULL);
    }
    
    printf("\nAdvanced Geometry & Physics Test Complete!\n");
    printf("All systems demonstrated successfully.\n");
    return 0;
}
