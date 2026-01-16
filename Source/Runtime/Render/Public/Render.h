#ifndef ULTIMATE_ENGINE_RENDER_H
#define ULTIMATE_ENGINE_RENDER_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Opaque types for subsystems
typedef struct RenderDevice RenderDevice;
typedef struct CommandBuffer CommandBuffer;
typedef struct RenderPass RenderPass;
typedef struct MeshRenderer MeshRenderer;
typedef struct SpriteRenderer SpriteRenderer;
typedef struct UIRenderer UIRenderer;

// Defines
#define RENDER_BACKEND_VULKAN 0
#define RENDER_BACKEND_METAL 1

typedef struct RenderSystem {
  RenderDevice *device;
  CommandBuffer *command_buffer;
  RenderPass *main_pass;
  RenderPass *shadow_pass;
  RenderPass *post_process_pass;
  MeshRenderer *mesh_renderer;
  SpriteRenderer *sprite_renderer;
  UIRenderer *ui_renderer;
} RenderSystem;

// Lifecycle
bool Render_Init(void);
void Render_Shutdown(void);
void Render_BeginFrame(void);
void Render_EndFrame(void);

// Debug Drawing
void Render_DrawCube(float x, float y, float z, float size, float r, float g,
                     float b);
void Render_DrawSphere(float x, float y, float z, float radius, float r,
                       float g, float b);
void Render_SetLightDirection(float x, float y, float z);

// Subsystem Creation Wrappers (Stubs or Impl)
RenderDevice *RenderDevice_Create(int backend);
void RenderDevice_Destroy(RenderDevice *device);
void RenderDevice_BeginFrame(RenderDevice *device);
void RenderDevice_Submit(RenderDevice *device, CommandBuffer *cmd);
void RenderDevice_Present(RenderDevice *device);

CommandBuffer *CommandBuffer_Create(void);
void CommandBuffer_Destroy(CommandBuffer *cmd);
void CommandBuffer_Begin(CommandBuffer *cmd);
void CommandBuffer_End(CommandBuffer *cmd);

RenderPass *RenderPass_Create(const char *name);
void RenderPass_Destroy(RenderPass *pass);

MeshRenderer *MeshRenderer_Create(void);
void MeshRenderer_Destroy(MeshRenderer *renderer);

SpriteRenderer *SpriteRenderer_Create(void);
void SpriteRenderer_Destroy(SpriteRenderer *renderer);

UIRenderer *UIRenderer_Create(void);
void UIRenderer_Destroy(UIRenderer *renderer);

#endif // ULTIMATE_ENGINE_RENDER_H
