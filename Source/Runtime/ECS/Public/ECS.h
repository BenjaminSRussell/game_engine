#ifndef ULTIMATE_ENGINE_ECS_H
#define ULTIMATE_ENGINE_ECS_H

#include "../../Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Entity ID
typedef u32 EntityID;
#define INVALID_ENTITY_ID 0
#define ECS_MAX_ENTITIES 10000

// Component Type
typedef u32 ComponentType;

// Lifecycle
VF_API bool ECS_Init(void);
VF_API void ECS_Shutdown(void);

// Registration
VF_API ComponentType ECS_RegisterComponent(const char *name, u32 size,
                                           u32 alignment);

// Entity Operations
VF_API EntityID ECS_CreateEntity(void);
VF_API void ECS_DestroyEntity(EntityID entity);
VF_API bool ECS_IsEntityValid(EntityID entity);

// Component Operations
VF_API void *ECS_AddComponent(EntityID entity, ComponentType type, void *data);
VF_API void *ECS_GetComponent(EntityID entity, ComponentType type);
VF_API void ECS_RemoveComponent(EntityID entity, ComponentType type);
VF_API bool ECS_HasComponent(EntityID entity, ComponentType type);

// System Execution
VF_API void ECS_Update(float delta_time);
VF_API void ECS_Render(void);

// System Registration
typedef void (*ECSSystemUpdateFunc)(float dt);
typedef void (*ECSSystemRenderFunc)(void);

VF_API void ECS_RegisterSystem(const char *name, ECSSystemUpdateFunc update,
                               ECSSystemRenderFunc render, int order);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_ECS_H
