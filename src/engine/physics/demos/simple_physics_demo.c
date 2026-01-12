// Simple Physics Demo - Standalone demonstration without complex dependencies
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

// Simple 2D physics structures
typedef struct {
    float x, y;
} Vec2;

typedef struct {
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    float radius;
    float mass;
    float color[3];
} Ball;

typedef struct {
    Vec2 position;
    Vec2 size;
    float color[3];
} Box;

#define MAX_BALLS 20
#define GRAVITY -9.81f
#define DAMPING 0.999f
#define RESTITUTION 0.8f

static Ball balls[MAX_BALLS];
static int ball_count = 0;
static Box walls[4];
static bool running = true;
static float time_accumulator = 0.0f;
static const float FIXED_TIMESTEP = 1.0f / 60.0f; // 60 FPS physics

// Simple math functions
Vec2 vec2_add(Vec2 a, Vec2 b) { return (Vec2){a.x + b.x, a.y + b.y}; }
Vec2 vec2_sub(Vec2 a, Vec2 b) { return (Vec2){a.x - b.x, a.y - b.y}; }
Vec2 vec2_mul(Vec2 v, float s) { return (Vec2){v.x * s, v.y * s}; }
float vec2_length(Vec2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
float vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
Vec2 vec2_normalize(Vec2 v) { 
    float len = vec2_length(v); 
    return len > 0 ? (Vec2){v.x / len, v.y / len} : (Vec2){0, 0}; 
}

void init_demo(void) {
    printf("Initializing Simple Physics Demo\n");
    
    // Clear existing balls
    ball_count = 0;
    
    // Create walls
    walls[0] = (Box){(Vec2){0, -5}, (Vec2){20, 0.5}, {0.5, 0.5, 0.5}}; // Bottom
    walls[1] = (Box){(Vec2){0, 15}, (Vec2){20, 0.5}, {0.5, 0.5, 0.5}}; // Top
    walls[2] = (Box){(Vec2){-1, 0}, (Vec2){0.5, 15}, {0.5, 0.5, 0.5}}; // Left
    walls[3] = (Box){(Vec2){19.5, 0}, (Vec2){0.5, 15}, {0.5, 0.5, 0.5}}; // Right
    
    // Create initial balls
    for (int i = 0; i < 10; i++) {
        balls[ball_count] = (Ball){
            .position = (Vec2){2.0f + i * 1.5f, 10.0f + (i % 3) * 0.5f},
            .velocity = (Vec2){0, 0},
            .acceleration = (Vec2){0, GRAVITY},
            .radius = 0.3f + (i % 3) * 0.1f,
            .mass = 1.0f + (i % 3) * 0.5f,
            .color = {
                0.2f + (i % 3) * 0.3f,
                0.4f + (i % 2) * 0.3f,
                0.8f - (i % 4) * 0.2f
            }
        };
        ball_count++;
    }
    
    // Add some moving balls
    for (int i = 0; i < 5; i++) {
        balls[ball_count] = (Ball){
            .position = (Vec2){5.0f + i * 2.0f, 12.0f},
            .velocity = (Vec2){(i % 3 - 1) * 2.0f, 0},
            .acceleration = (Vec2){0, GRAVITY},
            .radius = 0.25f,
            .mass = 0.8f,
            .color = {0.9f, 0.3f, 0.3f}
        };
        ball_count++;
    }
}

void update_physics(float dt) {
    // Fixed timestep for stability
    time_accumulator += dt;
    
    while (time_accumulator >= FIXED_TIMESTEP) {
        float fixed_dt = FIXED_TIMESTEP;
        
        // Update ball physics
        for (int i = 0; i < ball_count; i++) {
            // Apply gravity
            balls[i].acceleration.y = GRAVITY;
            
            // Update velocity and position (Euler integration)
            balls[i].velocity = vec2_add(balls[i].velocity, vec2_mul(balls[i].acceleration, fixed_dt));
            balls[i].velocity = vec2_mul(balls[i].velocity, DAMPING); // Apply damping
            balls[i].position = vec2_add(balls[i].position, vec2_mul(balls[i].velocity, fixed_dt));
        }
        
        // Handle collisions
        // Ball-to-ball collisions
        for (int i = 0; i < ball_count; i++) {
            for (int j = i + 1; j < ball_count; j++) {
                Vec2 diff = vec2_sub(balls[j].position, balls[i].position);
                float dist = vec2_length(diff);
                float min_dist = balls[i].radius + balls[j].radius;
                
                if (dist < min_dist && dist > 0.001f) {
                    // Collision detected
                    Vec2 normal = vec2_normalize(diff);
                    
                    // Separate balls
                    float overlap = min_dist - dist;
                    Vec2 separation = vec2_mul(normal, overlap * 0.5f);
                    balls[i].position = vec2_sub(balls[i].position, separation);
                    balls[j].position = vec2_add(balls[j].position, separation);
                    
                    // Calculate relative velocity
                    Vec2 relative_vel = vec2_sub(balls[j].velocity, balls[i].velocity);
                    float vel_along_normal = vec2_dot(relative_vel, normal);
                    
                    // Don't resolve if velocities are separating
                    if (vel_along_normal > 0) continue;
                    
                    // Calculate impulse
                    float impulse = 2 * vel_along_normal / (1/balls[i].mass + 1/balls[j].mass);
                    Vec2 impulse_vec = vec2_mul(normal, impulse * RESTITUTION);
                    
                    // Apply impulse
                    balls[i].velocity = vec2_add(balls[i].velocity, vec2_mul(impulse_vec, balls[j].mass));
                    balls[j].velocity = vec2_sub(balls[j].velocity, vec2_mul(impulse_vec, balls[i].mass));
                }
            }
        }
        
        // Ball-to-wall collisions
        for (int i = 0; i < ball_count; i++) {
            for (int w = 0; w < 4; w++) {
                Box wall = walls[w];
                
                // Simple AABB collision
                float closest_x = fmaxf(wall.position.x, fminf(balls[i].position.x, wall.position.x + wall.size.x));
                float closest_y = fmaxf(wall.position.y, fminf(balls[i].position.y, wall.position.y + wall.size.y));
                
                float dist_x = balls[i].position.x - closest_x;
                float dist_y = balls[i].position.y - closest_y;
                float dist_sq = dist_x * dist_x + dist_y * dist_y;
                
                if (dist_sq < balls[i].radius * balls[i].radius) {
                    // Collision with wall
                    float dist = sqrtf(dist_sq);
                    if (dist > 0.001f) {
                        Vec2 normal = (Vec2){dist_x / dist, dist_y / dist};
                        
                        // Push ball out of wall
                        float overlap = balls[i].radius - dist;
                        balls[i].position = vec2_add(balls[i].position, vec2_mul(normal, overlap));
                        
                        // Reflect velocity
                        float vel_along_normal = vec2_dot(balls[i].velocity, normal);
                        if (vel_along_normal < 0) {
                            balls[i].velocity = vec2_sub(balls[i].velocity, vec2_mul(normal, 2 * vel_along_normal * RESTITUTION));
                        }
                    }
                }
            }
        }
        
        time_accumulator -= FIXED_TIMESTEP;
    }
}

void render_ascii(void) {
    // Clear screen
    printf("\033[2J\033[H");
    
    // Render walls
    for (int w = 0; w < 4; w++) {
        Box wall = walls[w];
        for (int y = 0; y < wall.size.y; y++) {
            for (int x = 0; x < wall.size.x; x++) {
                int screen_x = (int)(wall.position.x + x);
                int screen_y = 20 - (int)(wall.position.y + y); // Flip Y for screen coordinates
                
                if (screen_x >= 0 && screen_x < 80 && screen_y >= 0 && screen_y < 20) {
                    printf("\033[48;5;%dm%s", 40 + (int)(wall.color[0] * 3), " ");
                }
            }
        }
    }
    
    // Render balls
    for (int i = 0; i < ball_count; i++) {
        int screen_x = (int)balls[i].position.x;
        int screen_y = 20 - (int)balls[i].position.y;
        
        if (screen_x >= 0 && screen_x < 80 && screen_y >= 0 && screen_y < 20) {
            // Simple color coding based on ball properties
            int color_code = 41 + (int)(balls[i].color[0] * 3);
            printf("\033[%dm●\033[0m", color_code);
        }
    }
    
    // Render UI
    printf("\n\033[0mSimple Physics Demo\n");
    printf("Balls: %d | FPS: ~60\n", ball_count);
    printf("Controls: Space=Add ball, R=Reset, Q=Quit\n");
}

void handle_input(void) {
    // Simple input handling (would use proper input system in real app)
    static bool space_pressed = false;
    
    // Add new ball on space
    if (!space_pressed && rand() % 100 < 2) { // Simulate space press
        if (ball_count < MAX_BALLS) {
            balls[ball_count] = (Ball){
                .position = (Vec2){5.0f + (rand() % 100) / 10.0f, 13.0f},
                .velocity = (Vec2){(rand() % 200 - 100) / 50.0f, 0},
                .acceleration = (Vec2){0, GRAVITY},
                .radius = 0.2f + (rand() % 100) / 200.0f,
                .mass = 0.5f + (rand() % 100) / 200.0f,
                .color = {
                    (rand() % 100) / 100.0f,
                    (rand() % 100) / 100.0f,
                    (rand() % 100) / 100.0f
                }
            };
            ball_count++;
        }
        space_pressed = true;
    }
    
    // Reset on 'R'
    if (rand() % 1000 < 1) { // Simulate R press occasionally
        init_demo();
    }
    
    // Quit on 'Q'
    if (rand() % 2000 < 1) { // Simulate Q press occasionally
        running = false;
    }
}

int main(int argc, char* argv[]) {
    printf("Starting Simple Physics Demo\n");
    printf("This demo showcases 2D physics simulation with:\n");
    printf("- Gravity and acceleration\n");
    printf("- Ball-to-ball collisions\n");
    printf("- Ball-to-wall collisions\n");
    printf("- Conservation of momentum\n");
    printf("- Energy dissipation\n\n");
    
    srand((unsigned int)time(NULL));
    
    init_demo();
    
    float last_time = (float)clock() / CLOCKS_PER_SEC;
    
    while (running) {
        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;
        
        // Limit timestep to prevent instability
        if (dt > 0.1f) dt = 0.1f;
        
        handle_input();
        update_physics(dt);
        render_ascii();
        
        // Simple frame rate limiting
        struct timespec ts = {0, 16666667}; // ~60 FPS
        nanosleep(&ts, NULL);
    }
    
    printf("\nPhysics demo ended. Goodbye!\n");
    return 0;
}
