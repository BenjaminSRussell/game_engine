#include "ecs/mass_ecb.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
  CMD_CREATE_ENTITY,
  CMD_DESTROY_ENTITY,
  CMD_ADD_COMPONENT,
  CMD_REMOVE_COMPONENT,
  CMD_SET_COMPONENT
} CommandType;

typedef struct {
  CommandType type;
  Entity entity;
  ECSComponentID component_id;
  void *data; // Copy of data (malloced)
} MassCommand;

struct MassCommandBuffer {
  World *world;
  MassCommand *commands;
  u32 count;
  u32 capacity;
};

MassCommandBuffer *mass_ecb_create(World *world) {
  MassCommandBuffer *buffer = calloc(1, sizeof(MassCommandBuffer));
  buffer->world = world;
  buffer->capacity = 1024;
  buffer->commands = calloc(buffer->capacity, sizeof(MassCommand));
  return buffer;
}

void mass_ecb_destroy(MassCommandBuffer *buffer) {
  if (!buffer)
    return;
  mass_ecb_clear(buffer);
  free(buffer->commands);
  free(buffer);
}

static void ensure_capacity(MassCommandBuffer *buffer) {
  if (buffer->count >= buffer->capacity) {
    buffer->capacity *= 2;
    buffer->commands =
        realloc(buffer->commands, buffer->capacity * sizeof(MassCommand));
  }
}

Entity mass_ecb_create_entity(MassCommandBuffer *buffer) {
  // Note: We cannot return the REAL entity ID yet because it's not created.
  // For mass systems, we usually don't need the ID immediately in the same
  // frame unless for relation. If we need immediate ID, we must lock the world.
  // Here we record a request. The returned entity is a placeholder (INVALID).
  // TODO: Better handling of cross-referencing new entities (e.g. using index).
  ensure_capacity(buffer);
  MassCommand *cmd = &buffer->commands[buffer->count++];
  cmd->type = CMD_CREATE_ENTITY;
  // In a real implementation we might reserve an ID atomically.
  return (Entity){0, 0};
}

void mass_ecb_destroy_entity(MassCommandBuffer *buffer, Entity entity) {
  ensure_capacity(buffer);
  MassCommand *cmd = &buffer->commands[buffer->count++];
  cmd->type = CMD_DESTROY_ENTITY;
  cmd->entity = entity;
}

void mass_ecb_add_component(MassCommandBuffer *buffer, Entity entity,
                            ECSComponentID component_id, const void *data) {
  ensure_capacity(buffer);
  MassCommand *cmd = &buffer->commands[buffer->count++];
  cmd->type = CMD_ADD_COMPONENT;
  cmd->entity = entity;
  cmd->component_id = component_id;

  if (data) {
    // Need component size to copy.
    const ComponentInfo *info =
        ecs_get_component_info(buffer->world, component_id);
    if (info && info->size > 0) {
      cmd->data = malloc(info->size);
      memcpy(cmd->data, data, info->size);
    }
  }
}

void mass_ecb_remove_component(MassCommandBuffer *buffer, Entity entity,
                               ECSComponentID component_id) {
  ensure_capacity(buffer);
  MassCommand *cmd = &buffer->commands[buffer->count++];
  cmd->type = CMD_REMOVE_COMPONENT;
  cmd->entity = entity;
  cmd->component_id = component_id;
}

void mass_ecb_set_component(MassCommandBuffer *buffer, Entity entity,
                            ECSComponentID component_id, const void *data) {
  // Set is same as Add for logic
  mass_ecb_add_component(buffer, entity, component_id, data);
  buffer->commands[buffer->count - 1].type = CMD_SET_COMPONENT;
}

void mass_ecb_playback(MassCommandBuffer *buffer) {
  for (u32 i = 0; i < buffer->count; i++) {
    MassCommand *cmd = &buffer->commands[i];
    switch (cmd->type) {
    case CMD_CREATE_ENTITY: {
      // If we supported returning an ID, we'd map it here.
      ecs_create_entity(buffer->world);
      break;
    }
    case CMD_DESTROY_ENTITY: {
      ecs_destroy_entity(buffer->world, cmd->entity);
      break;
    }
    case CMD_ADD_COMPONENT: {
      ecs_add_component(buffer->world, cmd->entity, cmd->component_id,
                        cmd->data);
      break;
    }
    case CMD_REMOVE_COMPONENT: {
      ecs_remove_component(buffer->world, cmd->entity, cmd->component_id);
      break;
    }
    case CMD_SET_COMPONENT: {
      ecs_set_component(buffer->world, cmd->entity, cmd->component_id,
                        cmd->data);
      break;
    }
    }
  }
  mass_ecb_clear(buffer);
}

void mass_ecb_clear(MassCommandBuffer *buffer) {
  for (u32 i = 0; i < buffer->count; i++) {
    if (buffer->commands[i].data) {
      free(buffer->commands[i].data);
      buffer->commands[i].data = NULL;
    }
  }
  buffer->count = 0;
}
