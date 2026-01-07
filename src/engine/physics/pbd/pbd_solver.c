/**
 * =================================================================================================
 *                    UNIFIED POSITION-BASED DYNAMICS SOLVER  
 * =================================================================================================
 * 
 * PURPOSE: GPU-first unified physics solver treating all phenomena as particles + constraints
 * PERFORMANCE TARGET: 100K+ particles at 60 FPS
 * 
 * APPROACH: Everything (rigid bodies, fluids, cloth, hair) uses same particle representation.
 * Different behaviors emerge from constraint types, not separate solvers.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <renderer/vulkan.h> // Include Vulkan renderer definitions

// Vulkan Global State for Physics (Should ideally be in a struct but sticking to pbd_solver scope)
static VkPipeline g_compute_pipeline = VK_NULL_HANDLE;
static VkPipelineLayout g_pipeline_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayout g_descriptor_set_layout = VK_NULL_HANDLE;
static VkDescriptorPool g_descriptor_pool = VK_NULL_HANDLE;
static VkDescriptorSet g_descriptor_set = VK_NULL_HANDLE;
static VkBuffer g_ssbo_buffer = VK_NULL_HANDLE;
static VkDeviceMemory g_ssbo_memory = VK_NULL_HANDLE;
static bool g_compute_initialized = false;

// Push constants matching shader
typedef struct {
    float dt;
    uint32_t particle_count;
} PushConstants;

#define MAX_PARTICLES 100000
#define MAX_CONSTRAINTS 500000
#define SOLVER_ITERATIONS 5
#define GRAVITY_Y -9.81f

// =================================================================================================
// UNIFIED PARTICLE REPRESENTATION
// =================================================================================================

typedef struct {
    float position[3];       // Current position
    float prev_position[3];  // Previous position (for Verlet integration)
    float velocity[3];       // Derived velocity
    float inv_mass;          // 1/mass (0 = infinite mass/static)
    
    // Material properties
    float friction;          // Surface friction coefficient
    float restitution;       // Bounciness (0-1)
    float density;           // Material density (kg/m³)
    
    // Rendering hint
    uint32_t color;          // RGBA packed
    float radius;            // Particle radius for rendering
    
    bool active;
} PBDParticle;

// =================================================================================================
// CONSTRAINT SYSTEM
// =================================================================================================

typedef enum {
    CONSTRAINT_DISTANCE,     // Fixed distance (rigid bodies, cloth)
    CONSTRAINT_BENDING,      // Angle preservation (cloth, soft bodies)
    CONSTRAINT_VOLUME,       // Volume conservation (fluids, balloons)
    CONSTRAINT_COLLISION,    // Particle-particle or particle-world
    CONSTRAINT_PIN           // Fixed position (anchors)
} ConstraintType;

typedef struct {
    ConstraintType type;
    uint32_t particle_indices[4];  // Max 4 particles per constraint
    float rest_value;               // Rest distance/angle/volume
    float stiffness;                // Constraint strength (0-1)
    bool active;
} PBDConstraint;

// =================================================================================================
// SOLVER STATE
// =================================================================================================

typedef struct {
    PBDParticle* particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    PBDConstraint* constraints;
    uint32_t constraint_count;
    uint32_t constraint_capacity;
    
    float dt;                // Timestep
    uint32_t substeps;       // Solver substeps for stability
    
    // Spatial hashing for collision detection
    uint32_t* spatial_hash;
    uint32_t hash_size;
} PBDSolver;

// =================================================================================================
// INITIALIZATION
// =================================================================================================

PBDSolver* pbd_create(uint32_t max_particles, uint32_t max_constraints) {
    PBDSolver* solver = (PBDSolver*)calloc(1, sizeof(PBDSolver));
    
    solver->particle_capacity = max_particles;
    solver->particles = (PBDParticle*)calloc(max_particles, sizeof(PBDParticle));
    
    solver->constraint_capacity = max_constraints;
    solver->constraints = (PBDConstraint*)calloc(max_constraints, sizeof(PBDConstraint));
    
    solver->dt = 1.0f / 60.0f;
    solver->substeps = 5;
    
    // Spatial hash for broad-phase collision
    solver->hash_size = max_particles * 2;
    solver->spatial_hash = (uint32_t*)calloc(solver->hash_size, sizeof(uint32_t));
    
    return solver;
}

void pbd_destroy(PBDSolver* solver) {
    if (solver) {
        free(solver->particles);
        free(solver->constraints);
        free(solver->spatial_hash);
        free(solver);
    }
}

// =================================================================================================
// PARTICLE MANAGEMENT
// =================================================================================================

uint32_t pbd_add_particle(PBDSolver* solver, float x, float y, float z, float mass) {
    if (solver->particle_count >= solver->particle_capacity) return UINT32_MAX;
    
    uint32_t idx = solver->particle_count++;
    PBDParticle* p = &solver->particles[idx];
    
    p->position[0] = x;
    p->position[1] = y;
    p->position[2] = z;
    
    p->prev_position[0] = x;
    p->prev_position[1] = y;
    p->prev_position[2] = z;
    
    p->velocity[0] = p->velocity[1] = p->velocity[2] = 0.0f;
    p->inv_mass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
    
    p->friction = 0.5f;
    p->restitution = 0.3f;
    p->density = 1000.0f;
    p->radius = 0.05f;
    p->color = 0xFFFFFFFF;
    p->active = true;
    
    return idx;
}

// =================================================================================================
// CONSTRAINT MANAGEMENT
// =================================================================================================

uint32_t pbd_add_distance_constraint(PBDSolver* solver, uint32_t p1, uint32_t p2, float stiffness) {
    if (solver->constraint_count >= solver->constraint_capacity) return UINT32_MAX;
    
    PBDParticle* a = &solver->particles[p1];
    PBDParticle* b = &solver->particles[p2];
    
    // Calculate rest distance
    float dx = b->position[0] - a->position[0];
    float dy = b->position[1] - a->position[1];
    float dz = b->position[2] - a->position[2];
    float rest = sqrtf(dx*dx + dy*dy + dz*dz);
    
    uint32_t idx = solver->constraint_count++;
    PBDConstraint* c = &solver->constraints[idx];
    
    c->type = CONSTRAINT_DISTANCE;
    c->particle_indices[0] = p1;
    c->particle_indices[1] = p2;
    c->rest_value = rest;
    c->stiffness = stiffness;
    c->active = true;
    
    return idx;
}

// =================================================================================================
// PHYSICS UPDATE - Verlet Integration
// =================================================================================================

static void integrate_particles(PBDSolver* solver, float dt) {
    for (uint32_t i = 0; i < solver->particle_count; i++) {
        PBDParticle* p = &solver->particles[i];
        if (!p->active || p->inv_mass == 0.0f) continue;
        
        // Calculate velocity from position delta (Verlet)
        p->velocity[0] = (p->position[0] - p->prev_position[0]) / dt;
        p->velocity[1] = (p->position[1] - p->prev_position[1]) / dt;
        p->velocity[2] = (p->position[2] - p->prev_position[2]) / dt;
        
        // Store previous position
        p->prev_position[0] = p->position[0];
        p->prev_position[1] = p->position[1];
        p->prev_position[2] = p->position[2];
        
        // Apply gravity
        p->velocity[1] += GRAVITY_Y * dt;
        
        // Update position
        p->position[0] += p->velocity[0] * dt;
        p->position[1] += p->velocity[1] * dt;
        p->position[2] += p->velocity[2] * dt;
        
        // Ground plane collision
        if (p->position[1] < 0.0f) {
            p->position[1] = 0.0f;
            p->velocity[1] *= -p->restitution;
        }
    }
}

// =================================================================================================
// CONSTRAINT SOLVING
// =================================================================================================

static void solve_distance_constraint(PBDSolver* solver, PBDConstraint* c) {
    PBDParticle* p1 = &solver->particles[c->particle_indices[0]];
    PBDParticle* p2 = &solver->particles[c->particle_indices[1]];
    
    float dx = p2->position[0] - p1->position[0];
    float dy = p2->position[1] - p1->position[1];
    float dz = p2->position[2] - p1->position[2];
    
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    if (dist < 1e-6f) return;
    
    float delta = (dist - c->rest_value) / dist;
    delta *= c->stiffness;
    
    float inv_mass_sum = p1->inv_mass + p2->inv_mass;
    if (inv_mass_sum < 1e-6f) return;
    
    float correction_scalar = delta / inv_mass_sum;
    
    float correction[3] = {
        dx * correction_scalar,
        dy * correction_scalar,
        dz * correction_scalar
    };
    
    p1->position[0] += correction[0] * p1->inv_mass;
    p1->position[1] += correction[1] * p1->inv_mass;
    p1->position[2] += correction[2] * p1->inv_mass;
    
    p2->position[0] -= correction[0] * p2->inv_mass;
    p2->position[1] -= correction[1] * p2->inv_mass;
    p2->position[2] -= correction[2] * p2->inv_mass;
}

static void solve_constraints(PBDSolver* solver) {
    for (uint32_t iter = 0; iter < SOLVER_ITERATIONS; iter++) {
        for (uint32_t i = 0; i < solver->constraint_count; i++) {
            PBDConstraint* c = &solver->constraints[i];
            if (!c->active) continue;
            
            switch (c->type) {
                case CONSTRAINT_DISTANCE:
                    solve_distance_constraint(solver, c);
                    break;
                // Other constraint types would go here
                default:
                    break;
            }
        }
    }
}

// =================================================================================================
// MAIN UPDATE LOOP
// =================================================================================================

void pbd_update(PBDSolver* solver, float dt) {
    float sub_dt = dt / (float)solver->substeps;
    
    for (uint32_t substep = 0; substep < solver->substeps; substep++) {
        // 1. Apply forces and integrate
        integrate_particles(solver, sub_dt);
        
        // 2. Solve constraints
        solve_constraints(solver);
        
        // 3. Collision detection & response would go here
        // (Spatial hashing + particle-particle constraints)
    }
}

// =================================================================================================
// GPU COMPUTE IMPLEMENTATION (Vulkan)
// =================================================================================================

// Helper to create compute pipeline
void pbd_init_gpu(VulkanRenderer* renderer, PBDSolver* solver) {
    if (g_compute_initialized || !renderer) return;

    VkDevice device = renderer->device;

    // 1. Create Descriptor Set Layout (Binding 0: Particle Buffer)
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = NULL
    };
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding
    };
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &g_descriptor_set_layout) != VK_SUCCESS) {
        return;
    }

    // 2. Create Pipeline Layout
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstants)
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &g_descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &g_pipeline_layout) != VK_SUCCESS) {
        return;
    }

    // 3. Create Compute Pipeline
    // Note: In a real engine, we'd load the SPIR-V file. 
    // For this implementation, we assume the shader module creation is handled externally or we stub it.
    // Since we don't have a SPIR-V loader here, we'll mark as TODO.
    // But to satisfy the linker and logic proceed...
    /*
    VkComputePipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = g_pipeline_layout,
        .stage = ... // Need Shader Module
    };
    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_compute_pipeline);
    */
    
    // 4. Create SSBO for Particles
    VkDeviceSize bufferSize = sizeof(PBDParticle) * solver->particle_capacity;
    vulkan_create_buffer(renderer, bufferSize, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, // Visible for simple debugging update
        &g_ssbo_buffer, &g_ssbo_memory);

    // 5. Create Descriptor Pool & Set
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1
    };
    
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
        .maxSets = 1
    };
    vkCreateDescriptorPool(device, &poolInfo, NULL, &g_descriptor_pool);
    
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = g_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &g_descriptor_set_layout
    };
    vkAllocateDescriptorSets(device, &allocInfo, &g_descriptor_set);
    
    // 6. Update Descriptor Set
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = g_ssbo_buffer,
        .offset = 0,
        .range = bufferSize
    };
    
    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = g_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &bufferInfo
    };
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);

    g_compute_initialized = true;
}

// Dispatch compute shader
void pbd_update_gpu(VulkanRenderer* renderer, PBDSolver* solver, float dt) {
    if (!g_compute_initialized) {
        pbd_init_gpu(renderer, solver);
        // Upload initial data
        void* data;
        vkMapMemory(renderer->device, g_ssbo_memory, 0, sizeof(PBDParticle) * solver->particle_count, 0, &data);
        memcpy(data, solver->particles, sizeof(PBDParticle) * solver->particle_count);
        vkUnmapMemory(renderer->device, g_ssbo_memory);
    }
    
    if (!g_compute_pipeline) return; // Pipeline creation stubbed above

    VkCommandBuffer cmd = renderer->command_buffers[renderer->current_frame]; // Ideally use compute queue cmd buffer

    // Bind pipeline and descriptor set
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_compute_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, g_pipeline_layout, 0, 1, &g_descriptor_set, 0, NULL);
    
    // Push constants
    PushConstants pc = { .dt = dt, .particle_count = solver->particle_count };
    vkCmdPushConstants(cmd, g_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pc);
    
    // Dispatch
    // Local size is 256. Group count = ceil(count / 256)
    uint32_t groupCountX = (solver->particle_count + 255) / 256;
    vkCmdDispatch(cmd, groupCountX, 1, 1);
    
    // Add barrier so graphics/vertex shader waits for compute
    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    };
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 
        0, 1, &barrier, 0, NULL, 0, NULL);
}

// =================================================================================================
// HELPER: Create Rigid Body from Particles
// =================================================================================================

void pbd_create_box(PBDSolver* solver, float cx, float cy, float cz, float size, float stiffness) {
    // Create 8 corner particles
    uint32_t corners[8];
    float h = size * 0.5f;
    
    corners[0] = pbd_add_particle(solver, cx - h, cy - h, cz - h, 1.0f);
    corners[1] = pbd_add_particle(solver, cx + h, cy - h, cz - h, 1.0f);
    corners[2] = pbd_add_particle(solver, cx - h, cy + h, cz - h, 1.0f);
    corners[3] = pbd_add_particle(solver, cx + h, cy + h, cz - h, 1.0f);
    corners[4] = pbd_add_particle(solver, cx - h, cy - h, cz + h, 1.0f);
    corners[5] = pbd_add_particle(solver, cx + h, cy - h, cz + h, 1.0f);
    corners[6] = pbd_add_particle(solver, cx - h, cy + h, cz + h, 1.0f);
    corners[7] = pbd_add_particle(solver, cx + h, cy + h, cz + h, 1.0f);
    
    // Create distance constraints for edges
    uint32_t edges[12][2] = {
        {0,1}, {1,3}, {3,2}, {2,0}, // Bottom face
        {4,5}, {5,7}, {7,6}, {6,4}, // Top face
        {0,4}, {1,5}, {2,6}, {3,7}  // Vertical edges
    };
    
    for (int i = 0; i < 12; i++) {
        pbd_add_distance_constraint(solver, corners[edges[i][0]], corners[edges[i][1]], stiffness);
    }
    
    // Add diagonal constraints for rigidity
    pbd_add_distance_constraint(solver, corners[0], corners[7], stiffness);
    pbd_add_distance_constraint(solver, corners[1], corners[6], stiffness);
}
