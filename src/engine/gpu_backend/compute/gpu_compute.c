/**
 * GPU COMPUTE INFRASTRUCTURE - C API for Shader Dispatch
 * OpenGL/Vulkan abstraction for compute shader execution
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

// ============================================================================
// COMPUTE SHADER MANAGEMENT
// ============================================================================

typedef struct {
    unsigned int program_id;
    unsigned int shader_id;
    const char* source;
} ComputeShader;

ComputeShader* compute_shader_create(const char* source) {
    ComputeShader* cs = calloc(1, sizeof(ComputeShader));
    cs->source = source;
    
    // Compile shader
    cs->shader_id = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs->shader_id, 1, &source, NULL);
    glCompileShader(cs->shader_id);
    
    // Check compilation
    int success;
    glGetShaderiv(cs->shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(cs->shader_id, 512, NULL, info_log);
        fprintf(stderr, "Compute Shader Compilation Error: %s\n", info_log);
        free(cs);
        return NULL;
    }
    
    // Link program
    cs->program_id = glCreateProgram();
    glAttachShader(cs->program_id, cs->shader_id);
    glLinkProgram(cs->program_id);
    
    glGetProgramiv(cs->program_id, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(cs->program_id, 512, NULL, info_log);
        fprintf(stderr, "Compute Program Link Error: %s\n", info_log);
        glDeleteShader(cs->shader_id);
        free(cs);
        return NULL;
    }
    
    return cs;
}

void compute_shader_dispatch(ComputeShader* cs, int groups_x, int groups_y, int groups_z) {
    glUseProgram(cs->program_id);
    glDispatchCompute(groups_x, groups_y, groups_z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void compute_shader_destroy(ComputeShader* cs) {
    if (!cs) return;
    glDeleteProgram(cs->program_id);
    glDeleteShader(cs->shader_id);
    free(cs);
}

// ============================================================================
// COMPUTE BUFFER MANAGEMENT
// ============================================================================

typedef struct {
    unsigned int buffer_id;
    size_t size;
    void* mapped_ptr;
} ComputeBuffer;

ComputeBuffer* compute_buffer_create(size_t size, void* initial_data) {
    ComputeBuffer* cb = calloc(1, sizeof(ComputeBuffer));
    cb->size = size;
    
    glGenBuffers(1, &cb->buffer_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cb->buffer_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, initial_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    return cb;
}

void compute_buffer_bind(ComputeBuffer* cb, int binding_point) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, cb->buffer_id);
}

void* compute_buffer_map(ComputeBuffer* cb) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cb->buffer_id);
    cb->mapped_ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    return cb->mapped_ptr;
}

void compute_buffer_unmap(ComputeBuffer* cb) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cb->buffer_id);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    cb->mapped_ptr = NULL;
}

void compute_buffer_read(ComputeBuffer* cb, void* dest) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cb->buffer_id);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, cb->size, dest);
}

void compute_buffer_destroy(ComputeBuffer* cb) {
    if (!cb) return;
    glDeleteBuffers(1, &cb->buffer_id);
    free(cb);
}

// ============================================================================
// HIGH-LEVEL GPU PHYSICS INTEGRATION
// ============================================================================

typedef struct {
    ComputeShader* navier_stokes_shader;
    ComputeShader* flip_shader;
    ComputeShader* sph_shader;
    ComputeShader* particle_shader;
    ComputeShader* shallow_water_shader;
    ComputeShader* broadphase_shader;
    
    // Buffers
    ComputeBuffer* velocity_buffers[3];
    ComputeBuffer* density_buffer;
    ComputeBuffer* pressure_buffer;
    ComputeBuffer* particle_buffer;
    
    int grid_size;
    int particle_count;
} GPUPhysicsContext;

GPUPhysicsContext* gpu_physics_init(int grid_size, int max_particles) {
    GPUPhysicsContext* ctx = calloc(1, sizeof(GPUPhysicsContext));
    ctx->grid_size = grid_size;
    ctx->particle_count = max_particles;
    
    // Load shaders from external file
    extern const char* NAVIER_STOKES_COMPUTE_SHADER;
    extern const char* FLIP_P2G_SHADER;
    extern const char* SPH_DENSITY_SHADER;
    extern const char* PARTICLE_UPDATE_SHADER;
    extern const char* SHALLOW_WATER_SHADER;
    extern const char* GPU_BROADPHASE_SHADER;
    
    ctx->navier_stokes_shader = compute_shader_create(NAVIER_STOKES_COMPUTE_SHADER);
    ctx->flip_shader = compute_shader_create(FLIP_P2G_SHADER);
    ctx->sph_shader = compute_shader_create(SPH_DENSITY_SHADER);
    ctx->particle_shader = compute_shader_create(PARTICLE_UPDATE_SHADER);
    ctx->shallow_water_shader = compute_shader_create(SHALLOW_WATER_SHADER);
    ctx->broadphase_shader = compute_shader_create(GPU_BROADPHASE_SHADER);
    
    // Create buffers
    size_t grid_buffer_size = grid_size * grid_size * grid_size * sizeof(float);
    ctx->velocity_buffers[0] = compute_buffer_create(grid_buffer_size, NULL);
    ctx->velocity_buffers[1] = compute_buffer_create(grid_buffer_size, NULL);
    ctx->velocity_buffers[2] = compute_buffer_create(grid_buffer_size, NULL);
    ctx->density_buffer = compute_buffer_create(grid_buffer_size, NULL);
    ctx->pressure_buffer = compute_buffer_create(grid_buffer_size, NULL);
    
    size_t particle_buffer_size = max_particles * (sizeof(float) * 6 + sizeof(float) * 2);
    ctx->particle_buffer = compute_buffer_create(particle_buffer_size, NULL);
    
    return ctx;
}

void gpu_physics_update_fluids(GPUPhysicsContext* ctx, float dt) {
    // Bind buffers
    compute_buffer_bind(ctx->velocity_buffers[0], 0);
    compute_buffer_bind(ctx->velocity_buffers[1], 1);
    compute_buffer_bind(ctx->velocity_buffers[2], 2);
    compute_buffer_bind(ctx->density_buffer, 3);
    compute_buffer_bind(ctx->pressure_buffer, 4);
    
    // Set uniforms
    glUseProgram(ctx->navier_stokes_shader->program_id);
    glUniform1i(glGetUniformLocation(ctx->navier_stokes_shader->program_id, "grid_size"), ctx->grid_size);
    glUniform1f(glGetUniformLocation(ctx->navier_stokes_shader->program_id, "dt"), dt);
    glUniform1f(glGetUniformLocation(ctx->navier_stokes_shader->program_id, "viscosity"), 0.001f);
    
    // Dispatch (grid_size/8 work groups, 8x8x8 local size)
    int groups = (ctx->grid_size + 7) / 8;
    compute_shader_dispatch(ctx->navier_stokes_shader, groups, groups, groups);
}

void gpu_physics_update_particles(GPUPhysicsContext* ctx, float dt) {
    compute_buffer_bind(ctx->particle_buffer, 0);
    
    glUseProgram(ctx->particle_shader->program_id);
    glUniform1i(glGetUniformLocation(ctx->particle_shader->program_id, "particle_count"), ctx->particle_count);
    glUniform1f(glGetUniformLocation(ctx->particle_shader->program_id, "dt"), dt);
    glUniform3f(glGetUniformLocation(ctx->particle_shader->program_id, "gravity"), 0.0f, -9.81f, 0.0f);
    
    // Dispatch (particle_count/256 work groups, 256 local size)
    int groups = (ctx->particle_count + 255) / 256;
    compute_shader_dispatch(ctx->particle_shader, groups, 1, 1);
}

void gpu_physics_destroy(GPUPhysicsContext* ctx) {
    if (!ctx) return;
    
    compute_shader_destroy(ctx->navier_stokes_shader);
    compute_shader_destroy(ctx->flip_shader);
    compute_shader_destroy(ctx->sph_shader);
    compute_shader_destroy(ctx->particle_shader);
    compute_shader_destroy(ctx->shallow_water_shader);
    compute_shader_destroy(ctx->broadphase_shader);
    
    for (int i = 0; i < 3; i++) compute_buffer_destroy(ctx->velocity_buffers[i]);
    compute_buffer_destroy(ctx->density_buffer);
    compute_buffer_destroy(ctx->pressure_buffer);
    compute_buffer_destroy(ctx->particle_buffer);
    
    free(ctx);
}

// GPU COMPUTE INFRASTRUCTURE COMPLETE
// Full OpenGL compute shader integration with buffer management
