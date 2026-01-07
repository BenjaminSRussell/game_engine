#include <Metal/Metal.h>
#include <simd/simd.h>

#define CLOTH_SOLVER_ITERATIONS 4

// Note: Using C struct definitions that match the Metal buffer layout
// These must stay in sync with the shader definitions

typedef struct {
    simd_float3 position;
    float _pad0;
    simd_float3 prev_position;
    float inv_mass;  // 0.0f for pinned particles
    simd_float3 velocity;
    float _pad1;
    simd_float3 normal;
    float _pad2;
    simd_float2 uv;
    simd_float2 _pad3;
} ClothParticle;

typedef struct {
    uint32_t particle_a;
    uint32_t particle_b;
    float rest_length;
    float stiffness;
} ClothConstraint;

typedef struct {
    id<MTLBuffer> particles_buffer;
    id<MTLBuffer> constraints_buffer;
    id<MTLBuffer> vertex_buffer; // Output for rendering
    
    uint32_t particle_count;
    uint32_t constraint_count;
    uint32_t grid_width;
    uint32_t grid_height;
    
    id<MTLComputePipelineState> compute_pipeline;
    id<MTLComputePipelineState> constraint_pipeline;
    id<MTLComputePipelineState> normal_pipeline;
} ClothMesh;

// Forward declarations
void cloth_dispatch_compute(id<MTLComputeCommandEncoder> encoder, id<MTLComputePipelineState> pipeline, uint32_t thread_count);

ClothMesh* cloth_create(id<MTLDevice> device, uint32_t width, uint32_t height, float spacing) {
    ClothMesh* cloth = (ClothMesh*)calloc(1, sizeof(ClothMesh));
    cloth->grid_width = width;
    cloth->grid_height = height;
    cloth->particle_count = width * height;
    
    // Create particles
    size_t particles_size = sizeof(ClothParticle) * cloth->particle_count;
    ClothParticle* initial_particles = (ClothParticle*)malloc(particles_size);
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            ClothParticle* p = &initial_particles[idx];
            
            p->position = simd_make_float3((float)x * spacing, (float)(height - y) * spacing, 0.0f);
            p->prev_position = p->position;
            p->velocity = simd_make_float3(0.0f, 0.0f, 0.0f);
            
            // Pin top row
            if (y == 0) {
                p->inv_mass = 0.0f;
            } else {
                p->inv_mass = 1.0f; // Unit mass
            }
            
            p->normal = simd_make_float3(0.0f, 0.0f, 1.0f);
            p->uv = simd_make_float2((float)x / (width - 1), (float)y / (height - 1));
        }
    }
    
    cloth->particles_buffer = [device newBufferWithBytes:initial_particles length:particles_size options:MTLResourceStorageModeShared];
    free(initial_particles);
    
    // Create constraints
    // Structural (horizontal + vertical) + Shear + Bending
    // For simplicity here, just structural
    uint32_t max_constraints = (width - 1) * height + (height - 1) * width + (width - 1) * (height - 1) * 2;
    ClothConstraint* initial_constraints = (ClothConstraint*)malloc(sizeof(ClothConstraint) * max_constraints);
    uint32_t c_idx = 0;
    
    auto add_constraint = [&](uint32_t a, uint32_t b, float stiffness) {
        ClothConstraint* c = &initial_constraints[c_idx++];
        c->particle_a = a;
        c->particle_b = b;
        c->stiffness = stiffness;
        
        // Calculate rest length from initial positions
        ClothParticle* pA = (ClothParticle*)((char*)cloth->particles_buffer.contents + a * sizeof(ClothParticle));
        ClothParticle* pB = (ClothParticle*)((char*)cloth->particles_buffer.contents + b * sizeof(ClothParticle));
        c->rest_length = simd_distance(pA->position, pB->position);
    };
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t idx = y * width + x;
            
            // Structural Horizontal
            if (x < width - 1) add_constraint(idx, idx + 1, 1.0f);
            // Structural Vertical
            if (y < height - 1) add_constraint(idx, idx + width, 1.0f);
            // Shear
            if (x < width - 1 && y < height - 1) {
                add_constraint(idx, idx + width + 1, 0.8f);
                add_constraint(idx + 1, idx + width, 0.8f);
            }
            // Bending (skip for brevity, but similar logic with stride 2)
        }
    }
    
    cloth->constraint_count = c_idx;
    cloth->constraints_buffer = [device newBufferWithBytes:initial_constraints length:sizeof(ClothConstraint) * c_idx options:MTLResourceStorageModeShared];
    free(initial_constraints);
    
    // Vertex buffer (only position and normal for rendering, usually packed)
    // Here we'll just reuse particle buffer logic or create a separate one if format differs
    // For this implementation, we assume we can bind the particle buffer directly or we copy relevant data.
    // Let's create a dedicated vertex output for simplified rendering integration
    cloth->vertex_buffer = [device newBufferWithLength:sizeof(simd_float3) * 2 * cloth->particle_count options:MTLResourceStorageModePrivate]; // Pos + Normal
    
    return cloth;
}

void cloth_load_pipelines(ClothMesh* cloth, id<MTLLibrary> library) {
    NSError* error = nil;
    
    id<MTLFunction> integrate_fn = [library newFunctionWithName:@"cloth_integrate"];
    cloth->compute_pipeline = [library.device newComputePipelineStateWithFunction:integrate_fn error:&error];
    
    id<MTLFunction> solve_fn = [library newFunctionWithName:@"cloth_solve_constraints"];
    cloth->constraint_pipeline = [library.device newComputePipelineStateWithFunction:solve_fn error:&error];
    
    id<MTLFunction> normals_fn = [library newFunctionWithName:@"cloth_calculate_normals"];
    cloth->normal_pipeline = [library.device newComputePipelineStateWithFunction:normals_fn error:&error];
    
    if (error) {
        printf("Error creating pipeline: %s\n", [[error description] UTF8String]);
    }
}

void cloth_simulate_step(id<MTLComputeCommandEncoder> encoder,
                         ClothMesh* cloth, 
                         float delta_time,
                         simd_float3 gravity, 
                         simd_float3 wind) {
    if (!cloth || !cloth->compute_pipeline) return;

    // 1. Integration Step (Prediction)
    [encoder setComputePipelineState:cloth->compute_pipeline];
    [encoder setBuffer:cloth->particles_buffer offset:0 atIndex:0];
    
    struct {
        float dt;
        float damping;
        simd_float3 gravity;
        float _pad;
    } sim_params;
    
    sim_params.dt = delta_time;
    sim_params.damping = 0.98f;
    sim_params.gravity = gravity;
    
    [encoder setBytes:&sim_params length:sizeof(sim_params) atIndex:2];
    
    cloth_dispatch_compute(encoder, cloth->compute_pipeline, cloth->particle_count);
    
    // 2. Constraint Solving (Iterative)
    [encoder setComputePipelineState:cloth->constraint_pipeline];
    [encoder setBuffer:cloth->particles_buffer offset:0 atIndex:0];
    [encoder setBuffer:cloth->constraints_buffer offset:0 atIndex:1];
    
    for (int i = 0; i < CLOTH_SOLVER_ITERATIONS; i++) {
        // Note: For parallel constraint solving to be stable, we ideally use graph coloring 
        // or atomic operations. Here we assume the shader uses atomics or accepts some race conditions 
        // for visual cloth (PBD is robust).
        cloth_dispatch_compute(encoder, cloth->constraint_pipeline, cloth->constraint_count);
    }
    
    // 3. Collision Handling (TODO: dispatch collision kernel)
    
    // 4. Normal Calculation and Velocity Update
    [encoder setComputePipelineState:cloth->normal_pipeline];
    [encoder setBuffer:cloth->particles_buffer offset:0 atIndex:0];
    [encoder setBuffer:cloth->vertex_buffer offset:0 atIndex:1]; // Output if separating
    [encoder setBytes:&cloth->grid_width length:sizeof(uint32_t) atIndex:2];
    [encoder setBytes:&cloth->grid_height length:sizeof(uint32_t) atIndex:3];
    
    cloth_dispatch_compute(encoder, cloth->normal_pipeline, cloth->particle_count);
}

void cloth_dispatch_compute(id<MTLComputeCommandEncoder> encoder, id<MTLComputePipelineState> pipeline, uint32_t count) {
    NSUInteger threadGroupSize = pipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 256) threadGroupSize = 256;
    
    MTLSize threadgroupSize = MTLSizeMake(threadGroupSize, 1, 1);
    MTLSize gridSize = MTLSizeMake(count, 1, 1);
    
    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
}

void cloth_destroy(ClothMesh* cloth) {
    if (cloth) {
        free(cloth);
    }
}
