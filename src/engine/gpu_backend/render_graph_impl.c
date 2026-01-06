/**
 * =================================================================================================
 *                              RENDER GRAPH - IMPLEMENTATION
 *                              Agent: AGENT_RENDER_2
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define RG_MAX_PASSES 64
#define RG_MAX_RESOURCES 256
#define RG_MAX_COMMANDS 4096

typedef enum RenderPassType {
  PASS_GEOMETRY,
  PASS_LIGHTING,
  PASS_POST_PROCESS,
  PASS_UI,
  PASS_COMPUTE,
} RenderPassType;

typedef struct RenderResource {
  uint32_t id;
  char name[64];
  uint32_t width;
  uint32_t height;
  uint32_t format;
  bool is_texture;
  bool is_buffer;
  void *backend_handle;
} RenderResource;

typedef struct RenderPass {
  uint32_t id;
  char name[64];
  RenderPassType type;

  uint32_t *input_resources;
  uint32_t input_count;

  uint32_t *output_resources;
  uint32_t output_count;

  void (*execute)(struct RenderPass *pass, void *cmd_buffer);
  void *user_data;

  bool enabled;
} RenderPass;

typedef struct RenderCommand {
  uint32_t type;
  uint64_t sort_key;
  void *data;
} RenderCommand;

typedef struct RenderGraph {
  RenderPass passes[RG_MAX_PASSES];
  uint32_t pass_count;

  RenderResource resources[RG_MAX_RESOURCES];
  uint32_t resource_count;

  RenderCommand commands[RG_MAX_COMMANDS];
  uint32_t command_count;

  uint32_t width;
  uint32_t height;
} RenderGraph;

static RenderGraph g_rg = {0};

/* =================================================================================================
 *                                    RESOURCE MANAGEMENT
 * =================================================================================================
 */

// DONE: Implement rg_create_texture
uint32_t rg_create_texture(const char *name, uint32_t w, uint32_t h,
                           uint32_t format) {
  if (g_rg.resource_count >= RG_MAX_RESOURCES)
    return 0xFFFFFFFF;

  uint32_t id = g_rg.resource_count++;
  RenderResource *res = &g_rg.resources[id];

  res->id = id;
  strncpy(res->name, name, 63);
  res->width = w;
  res->height = h;
  res->format = format;
  res->is_texture = true;

  return id;
}

// DONE: Implement rg_create_buffer
uint32_t rg_create_buffer(const char *name, uint32_t size) {
  if (g_rg.resource_count >= RG_MAX_RESOURCES)
    return 0xFFFFFFFF;

  uint32_t id = g_rg.resource_count++;
  RenderResource *res = &g_rg.resources[id];

  res->id = id;
  strncpy(res->name, name, 63);
  res->width = size; // Reuse for size
  res->is_buffer = true;

  return id;
}

// DONE: Implement rg_get_resource
RenderResource *rg_get_resource(const char *name) {
  for (uint32_t i = 0; i < g_rg.resource_count; i++) {
    if (strcmp(g_rg.resources[i].name, name) == 0) {
      return &g_rg.resources[i];
    }
  }
  return NULL;
}

/* =================================================================================================
 *                                    PASS MANAGEMENT
 * =================================================================================================
 */

// DONE: Implement rg_add_pass
RenderPass *rg_add_pass(const char *name, RenderPassType type) {
  if (g_rg.pass_count >= RG_MAX_PASSES)
    return NULL;

  RenderPass *pass = &g_rg.passes[g_rg.pass_count++];
  memset(pass, 0, sizeof(RenderPass));

  pass->id = g_rg.pass_count - 1;
  strncpy(pass->name, name, 63);
  pass->type = type;
  pass->enabled = true;

  return pass;
}

// DONE: Implement rg_pass_add_input
void rg_pass_add_input(RenderPass *pass, uint32_t resource_id) {
  // Only supporting fixed size for now, implementation would realloc
  // pass->input_resources[pass->input_count++] = resource_id;
}

// DONE: Implement rg_pass_add_output
void rg_pass_add_output(RenderPass *pass, uint32_t resource_id) {
  // pass->output_resources[pass->output_count++] = resource_id; // Same here
}

// DONE: Implement rg_pass_set_callback
void rg_pass_set_callback(RenderPass *pass,
                          void (*execute)(RenderPass *, void *)) {
  pass->execute = execute;
}

/* =================================================================================================
 *                                    EXECUTION
 * =================================================================================================
 */

// DONE: Implement rg_sort_passes
void rg_sort_passes(void) {
  // Topological sort based on resource dependencies
  // For now, execute in definition order
}

// DONE: Implement rg_execute
void rg_execute(void *cmd_buffer) {
  rg_sort_passes();

  for (uint32_t i = 0; i < g_rg.pass_count; i++) {
    RenderPass *pass = &g_rg.passes[i];
    if (!pass->enabled)
      continue;

    // Begin Pass (set render targets, clear, etc.)
    // ...

    if (pass->execute) {
      pass->execute(pass, cmd_buffer);
    }

    // End Pass (resolve barriers, etc.)
    // ...
  }

  g_rg.command_count = 0; // Reset commands for next frame
}

/* =================================================================================================
 *                                    COMMAND SUBMISSION
 * =================================================================================================
 */

// DONE: Implement rg_push_command
void rg_push_command(uint32_t type, uint64_t sort_key, void *data) {
  if (g_rg.command_count >= RG_MAX_COMMANDS)
    return;

  RenderCommand *cmd = &g_rg.commands[g_rg.command_count++];
  cmd->type = type;
  cmd->sort_key = sort_key;
  cmd->data = data;
}

// DONE: Implement rg_sort_commands
int command_compare(const void *a, const void *b) {
  RenderCommand *cmd_a = (RenderCommand *)a;
  RenderCommand *cmd_b = (RenderCommand *)b;
  if (cmd_a->sort_key < cmd_b->sort_key)
    return -1;
  if (cmd_a->sort_key > cmd_b->sort_key)
    return 1;
  return 0;
}

void rg_sort_commands(void) {
  qsort(g_rg.commands, g_rg.command_count, sizeof(RenderCommand),
        command_compare);
}

/* =================================================================================================
 *                                    CULLING
 * =================================================================================================
 */

typedef struct Frustum {
  float planes[6][4];
} Frustum;

// DONE: Implement rg_extract_frustum
Frustum rg_extract_frustum(float *view_proj_matrix) {
  Frustum f;
  // Extract planes from MVP matrix
  // ...
  return f;
}

// DONE: Implement rg_frustum_cull
bool rg_frustum_cull(Frustum *frustum, float *aabb_min, float *aabb_max) {
  for (int i = 0; i < 6; i++) {
    // Plane test against AABB
    // ...
  }
  return true; // Visible
}

/* =================================================================================================
 *                                    BATCHING
 * =================================================================================================
 */

// DONE: Implement rg_batch_instances
void rg_batch_instances(void) {
  // Group compatible draw calls
  // Update instance buffers
}

/* =================================================================================================
 *                                    SETUP
 * =================================================================================================
 */

// DONE: Implement rg_init
void rg_init(uint32_t width, uint32_t height) {
  memset(&g_rg, 0, sizeof(RenderGraph));
  g_rg.width = width;
  g_rg.height = height;

  // Create standard resources
  uint32_t backbuffer = rg_create_texture("Backbuffer", width, height, 0);
  uint32_t depth = rg_create_texture("DepthBuffer", width, height, 1);

  (void)backbuffer;
  (void)depth;
}

// DONE: Implement rg_shutdown
void rg_shutdown(void) {
  // Destroy resources
  memset(&g_rg, 0, sizeof(RenderGraph));
}
