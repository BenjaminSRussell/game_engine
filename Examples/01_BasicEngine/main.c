#define GL_SILENCE_DEPRECATION
#include "../../Source/Runtime/Audio/Public/Audio.h"
#include "../../Source/Runtime/Audio/Public/AudioHelper.h"
#include "../../Source/Runtime/Core/Public/core_types.h"
#include "../../Source/Runtime/ECS/Public/ECS.h"
#include "../../Source/Runtime/ECS/components/Audio_Component.h"
#include "../../Source/Runtime/ECS/components/Components_Registration.h"
#include "../../Source/Runtime/ECS/components/Particle_Component.h"
#include "../../Source/Runtime/ECS/components/Physics_Component.h"
#include "../../Source/Runtime/ECS/components/Transform_Component.h"
#include "../../Source/Runtime/Input/Public/Input.h"
#include "../../Source/Runtime/Platform/Public/Platform.h"
#include "../../Source/Runtime/Render/Public/Render.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Forward declarations
void PhysicsSystem_Register(void);
void ParticleSystem_Register(void);
void AudioSystem_Register(void);

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Helper to spawn a physics object
EntityID SpawnPhysicsObject(float x, float y, float z, ShapeType shape,
                            float radius, float r, float g, float b,
                            float restitution) {
  EntityID id = ECS_CreateEntity();
  TransformComponent tc;
  tc.position.x = x;
  tc.position.y = y;
  tc.position.z = z;
  tc.rotation.x = 0;
  tc.rotation.y = 0;
  tc.rotation.z = 0;
  tc.scale.x = 1;
  tc.scale.y = 1;
  tc.scale.z = 1;
  tc.color.x = r;
  tc.color.y = g;
  tc.color.z = b;
  tc.shape = shape;
  ECS_AddComponent(id, COMPONENT_TRANSFORM, &tc);

  PhysicsComponent pc;
  pc.velocity.x = 0;
  pc.velocity.y = 0;
  pc.velocity.z = 0;
  pc.angular_velocity.x = 0;
  pc.angular_velocity.y = 0;
  pc.angular_velocity.z = 0;
  pc.mass = 1.0f;
  pc.radius = radius;
  pc.restitution = restitution;
  pc.active = true;
  ECS_AddComponent(id, COMPONENT_PHYSICS, &pc);
  return id;
}

// Build a tower of cubes
void BuildTower(float base_x, float base_z, int height) {
  for (int i = 0; i < height; i++) {
    float y = 0.4f + i * 0.85f;
    float hue = (float)i / height;
    SpawnPhysicsObject(base_x, y, base_z, SHAPE_CUBE, 0.4f, hue, 1.0f - hue,
                       0.5f, 0.3f);
  }
}

// Build a domino chain
void BuildDominoChain(float start_x, float start_z, int count) {
  for (int i = 0; i < count; i++) {
    float x = start_x + i * 1.2f;
    float hue = (float)i / count;
    EntityID domino = SpawnPhysicsObject(x, 0.8f, start_z, SHAPE_CUBE, 0.3f,
                                         0.8f, hue, 1.0f - hue, 0.2f);
    // Make them tall and thin (scale)
    TransformComponent *t =
        (TransformComponent *)ECS_GetComponent(domino, COMPONENT_TRANSFORM);
    if (t) {
      t->scale.x = 0.3f;
      t->scale.y = 1.5f;
      t->scale.z = 0.8f;
    }
  }
}

// Spawn a wrecking ball (sphere on a chain)
EntityID SpawnWreckingBall(float x, float y, float z) {
  return SpawnPhysicsObject(x, y, z, SHAPE_SPHERE, 1.0f, 0.3f, 0.3f, 0.3f,
                            0.8f);
}

// Spawn particle burst
void SpawnImpactBurst(float x, float y, float z) {
  for (int i = 0; i < 12; i++) {
    EntityID p = ECS_CreateEntity();
    TransformComponent tc;
    tc.position.x = x;
    tc.position.y = y;
    tc.position.z = z;
    tc.rotation.x = 0;
    tc.rotation.y = 0;
    tc.rotation.z = 0;
    tc.scale.x = 0.2f;
    tc.scale.y = 0.2f;
    tc.scale.z = 0.2f;
    tc.color.x = 1;
    tc.color.y = 1;
    tc.color.z = 1;
    tc.shape = SHAPE_CUBE;
    ECS_AddComponent(p, COMPONENT_TRANSFORM, &tc);

    ParticleComponent pc;
    pc.velocity.x = ((float)rand() / (float)RAND_MAX - 0.5f) * 15.0f;
    pc.velocity.y = ((float)rand() / (float)RAND_MAX) * 12.0f;
    pc.velocity.z = ((float)rand() / (float)RAND_MAX - 0.5f) * 15.0f;
    pc.max_lifetime = 0.8f + ((float)rand() / (float)RAND_MAX) * 0.4f;
    pc.lifetime = pc.max_lifetime;
    pc.color[0] = 0.8f + (float)rand() / (float)RAND_MAX * 0.2f;
    pc.color[1] = 0.5f + (float)rand() / (float)RAND_MAX * 0.5f;
    pc.color[2] = 0.2f;
    ECS_AddComponent(p, COMPONENT_PARTICLE, &pc);
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  srand(time(NULL));
  printf("=== Ultimate Engine: Advanced Physics Demo ===\n");
  printf("Features: Collision, Rotation, Lighting, Structures\n\n");

  if (!Platform_Init())
    return 1;
  if (!Input_Init())
    return 1;
  if (!Render_Init())
    return 1;
  if (!Audio_Init())
    return 1;
  if (!ECS_Init())
    return 1;

  // Register Components & Systems
  Components_RegisterAll();
  PhysicsSystem_Register();
  ParticleSystem_Register();
  AudioSystem_Register();

  // Generate Test Sound (Thud @ 100Hz, 0.5s)
  int thud_buffer = Audio_GenerateTestSound(100, 0.5f);

  // Set lighting direction
  Render_SetLightDirection(0.5f, -1.0f, 0.3f);

  // Create Camera
  EntityID camera = ECS_CreateEntity();
  TransformComponent cam_trans;
  cam_trans.position.x = 0;
  cam_trans.position.y = 8;
  cam_trans.position.z = 25;
  cam_trans.rotation.x = 0.3f;
  cam_trans.rotation.y = 0;
  cam_trans.rotation.z = 0;
  cam_trans.scale.x = 1;
  cam_trans.scale.y = 1;
  cam_trans.scale.z = 1;
  cam_trans.color.x = 1;
  cam_trans.color.y = 1;
  cam_trans.color.z = 1;
  cam_trans.shape = SHAPE_CUBE;
  ECS_AddComponent(camera, COMPONENT_TRANSFORM, &cam_trans);

  AudioListenerComponent cam_listener = {.active = true};
  ECS_AddComponent(camera, COMPONENT_AUDIO_LISTENER, &cam_listener);

  // Build initial scene
  printf("Building Tower...\n");
  BuildTower(-5, 0, 8);

  printf("Building Domino Chain...\n");
  BuildDominoChain(-10, -5, 12);

  printf("Spawning Wrecking Ball...\n");
  EntityID wrecking_ball = SpawnWreckingBall(5, 10, 0);
  // Give it initial velocity
  PhysicsComponent *wb_phys =
      (PhysicsComponent *)ECS_GetComponent(wrecking_ball, COMPONENT_PHYSICS);
  if (wb_phys) {
    wb_phys->velocity.x = -8.0f;
    wb_phys->mass = 5.0f; // Heavy!
  }

  printf("\n=== Controls ===\n");
  printf("WASD: Move camera\n");
  printf("Q/E: Move camera up/down\n");
  printf("1: Spawn cube\n");
  printf("2: Spawn sphere\n");
  printf("3: Rebuild tower\n");
  printf("4: Spawn wrecking ball\n");
  printf("5: Tip first domino\n");
  printf("ESC: Exit\n\n");

  float timer = 0;
  int frame_count = 0;

  while (!Platform_RequestedExit()) {
    float dt = 0.016f;
    Platform_Update(dt);
    Input_Update(dt);
    timer += dt;
    frame_count++;

    if (Input_IsKeyDown(256))
      break; // ESC

    // Camera control
    TransformComponent *tc =
        (TransformComponent *)ECS_GetComponent(camera, COMPONENT_TRANSFORM);
    if (tc) {
      float speed = 12.0f * dt;
      if (Input_IsKeyDown(87))
        tc->position.z -= speed; // W
      if (Input_IsKeyDown(83))
        tc->position.z += speed; // S
      if (Input_IsKeyDown(65))
        tc->position.x -= speed; // A
      if (Input_IsKeyDown(68))
        tc->position.x += speed; // D
      if (Input_IsKeyDown(81))
        tc->position.y += speed; // Q
      if (Input_IsKeyDown(69))
        tc->position.y -= speed; // E
    }

    // Interactive spawning
    static bool key_pressed[6] = {false};

    if (Input_IsKeyDown(49) && !key_pressed[1]) { // 1
      SpawnPhysicsObject(0, 15, 0, SHAPE_CUBE, 0.5f, (float)rand() / RAND_MAX,
                         (float)rand() / RAND_MAX, (float)rand() / RAND_MAX,
                         0.5f);
      key_pressed[1] = true;
    } else if (!Input_IsKeyDown(49))
      key_pressed[1] = false;

    if (Input_IsKeyDown(50) && !key_pressed[2]) { // 2
      SpawnPhysicsObject(0, 15, 0, SHAPE_SPHERE, 0.6f, (float)rand() / RAND_MAX,
                         (float)rand() / RAND_MAX, (float)rand() / RAND_MAX,
                         0.7f);
      key_pressed[2] = true;
    } else if (!Input_IsKeyDown(50))
      key_pressed[2] = false;

    if (Input_IsKeyDown(51) && !key_pressed[3]) { // 3
      BuildTower(-5, 0, 8);
      key_pressed[3] = true;
    } else if (!Input_IsKeyDown(51))
      key_pressed[3] = false;

    if (Input_IsKeyDown(52) && !key_pressed[4]) { // 4
      EntityID wb = SpawnWreckingBall(10, 12, 0);
      PhysicsComponent *p =
          (PhysicsComponent *)ECS_GetComponent(wb, COMPONENT_PHYSICS);
      if (p) {
        p->velocity.x = -10.0f;
        p->mass = 5.0f;
      }

      // Add sound to wrecking ball (looping humming noise)
      // Note: thud_buffer is local to main, so let's regenerate or make global.
      // For simplicity/demo speed, we'll generate a new one here or assume we
      // have it. Better: Generate hum buffer once at start.
      int hum_buffer = Audio_GenerateTestSound(50, 2.0f);
      AudioSourceComponent audio = {0};
      audio.buffer_id = hum_buffer;
      audio.pitch = 1.0f;
      audio.gain = 0.8f;
      audio.loop = true;
      audio.is_3d = true;
      audio.play_on_start = true;
      ECS_AddComponent(wb, COMPONENT_AUDIO_SOURCE, &audio);

      key_pressed[4] = true;
    } else if (!Input_IsKeyDown(52))
      key_pressed[4] = false;

    if (Input_IsKeyDown(53) && !key_pressed[5]) { // 5
      // Find first domino and tip it
      for (EntityID id = 0; id < ECS_MAX_ENTITIES; id++) {
        if (!ECS_IsEntityValid(id))
          continue;
        if (!ECS_HasComponent(id, COMPONENT_PHYSICS))
          continue;
        TransformComponent *t =
            (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
        PhysicsComponent *p =
            (PhysicsComponent *)ECS_GetComponent(id, COMPONENT_PHYSICS);
        if (t && p && t->position.x < -9.0f && t->scale.y > 1.0f) {
          p->velocity.x = 3.0f;
          p->angular_velocity.z = 2.0f;
          break;
        }
      }
      key_pressed[5] = true;
    } else if (!Input_IsKeyDown(53))
      key_pressed[5] = false;

    // Update Systems
    ECS_Update(dt);

    // Render
    Render_BeginFrame();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov = 60.0f;
    float aspect = 1280.0f / 720.0f;
    float size = 0.1f * tanf(fov * 0.5f * 3.14159f / 180.0f);
    glFrustum(-size * aspect, size * aspect, -size, size, 0.1f, 150.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (tc) {
      glRotatef(-tc->rotation.x * 57.2958f, 1, 0, 0);
      glRotatef(-tc->rotation.y * 57.2958f, 0, 1, 0);
      glTranslatef(-tc->position.x, -tc->position.y, -tc->position.z);
    }

    // Draw Floor (large grey plane)
    Render_DrawCube(0, -0.5f, 0, 60.0f, 0.25f, 0.25f, 0.25f);

    // Draw all physics objects
    for (EntityID id = 0; id < ECS_MAX_ENTITIES; id++) {
      if (!ECS_IsEntityValid(id))
        continue;
      if (!ECS_HasComponent(id, COMPONENT_PHYSICS))
        continue;
      if (ECS_HasComponent(id, COMPONENT_PARTICLE))
        continue;

      TransformComponent *t =
          (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
      PhysicsComponent *p =
          (PhysicsComponent *)ECS_GetComponent(id, COMPONENT_PHYSICS);

      if (t && p && p->active) {
        glPushMatrix();
        glTranslatef(t->position.x, t->position.y, t->position.z);
        glRotatef(t->rotation.x * 57.2958f, 1, 0, 0);
        glRotatef(t->rotation.y * 57.2958f, 0, 1, 0);
        glRotatef(t->rotation.z * 57.2958f, 0, 0, 1);

        if (t->shape == SHAPE_SPHERE) {
          Render_DrawSphere(0, 0, 0, p->radius, t->color.x, t->color.y,
                            t->color.z);
        } else {
          Render_DrawCube(0, 0, 0, p->radius * 2.0f, t->color.x, t->color.y,
                          t->color.z);
        }

        glPopMatrix();
      }
    }

    Render_EndFrame();

    // Status update every 5 seconds
    if (frame_count % 300 == 0) {
      int active_count = 0;
      for (EntityID id = 0; id < ECS_MAX_ENTITIES; id++) {
        if (ECS_IsEntityValid(id) && ECS_HasComponent(id, COMPONENT_PHYSICS))
          active_count++;
      }
      printf("[%.1fs] Active physics objects: %d\n", timer, active_count);
    }
  }

  printf("\nShutting down...\n");
  ECS_Shutdown();
  Audio_Shutdown();
  Render_Shutdown();
  Input_Shutdown();
  Platform_Shutdown();
  return 0;
}
