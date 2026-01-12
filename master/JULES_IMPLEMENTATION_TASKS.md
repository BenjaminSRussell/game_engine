# Jules Implementation Tasks - Unreal Engine Quality Standards

**Target**: Production-ready Minecraft v2 engine matching Unreal Engine quality  
**Timeline**: Next 4-6 hours of focused implementation  
**Approach**: Systematic completion of critical missing features  

## 🎯 **Phase 1: Critical Foundation (Next 2 Hours)**

### Task 1.1: Matrix Math Implementation (30 minutes)
**Priority**: CRITICAL - Blocks 50+ systems  
**Files**: `src/engine/include/math/mat4.h`, `src/engine/math/mat4.c`  
**Jules Task**: Complete the missing matrix operations

```c
// TODO: Implement these critical matrix functions
Mat4 mat4_multiply(Mat4 a, Mat4 b);           // Basic matrix multiplication
Mat4 mat4_inverse(Mat4 m);                    // Matrix inverse for camera transforms
Mat4 mat4_transpose(Mat4 m);                  // Matrix transpose for normal transforms
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);  // View matrix creation
Mat4 mat4_perspective(float fov, float aspect, float near, float far); // Projection matrix
Mat4 mat4_orthographic(float left, float right, float bottom, float top, float near, float far);
Mat4 mat4_scale(Mat4 m, Vec3 scale);          // Scale transformation
Mat4 mat4_translate(Mat4 m, Vec3 translation); // Translation transformation
Mat4 mat4_rotate(Mat4 m, float angle, Vec3 axis); // Rotation transformation
```

**Acceptance Criteria**:
- [ ] All functions compile without warnings
- [ ] Mathematical correctness verified with test cases
- [ ] Performance comparable to Unreal Engine (within 10%)
- [ ] Integration with existing rendering code successful

**Test Implementation**:
```c
// Quick validation test
void test_matrix_operations() {
    Mat4 identity = mat4_identity();
    Mat4 result = mat4_multiply(identity, identity);
    assert(mat4_equals(result, identity));
    
    Vec3 eye = {0, 0, 10}, center = {0, 0, 0}, up = {0, 1, 0};
    Mat4 view = mat4_look_at(eye, center, up);
    // Verify view matrix properties
}
```

---

### Task 1.2: GPU Buffer Management (45 minutes)
**Priority**: CRITICAL - Blocks entire rendering pipeline  
**Files**: `src/engine/backend/metal/mtl_buffer.m`, `src/engine/backend/vulkan/vk_buffer.c`  
**Jules Task**: Implement proper GPU buffer lifecycle management

**Metal Implementation**:
```objective-c
// TODO: Complete Metal buffer management
- (id<MTLBuffer>)createBufferWithSize:(size_t)size 
                            usage:(MTLBufferUsage)usage 
                        storageMode:(MTLStorageMode)storageMode {
    // Implementation needed
    return nil; // Currently stubbed
}

- (bool)uploadData:(const void*)data 
            toBuffer:(id<MTLBuffer>)buffer 
              offset:(size_t)offset 
                size:(size_t)size {
    // Implementation needed  
    return false; // Currently stubbed
}
```

**Vulkan Implementation**:
```c
// TODO: Complete Vulkan buffer management
VkBuffer vk_buffer_create(VkDevice device, size_t size, VkBufferUsageFlags usage) {
    // Implementation needed
    return VK_NULL_HANDLE; // Currently stubbed
}

bool vk_buffer_upload(VkDevice device, VkBuffer buffer, const void* data, size_t size) {
    // Implementation needed
    return false; // Currently stubbed  
}
```

**Acceptance Criteria**:
- [ ] Buffer creation works for vertex, index, uniform, and storage buffers
- [ ] Data upload handles all buffer types correctly
- [ ] Synchronization between CPU and GPU is proper
- [ ] Memory usage is optimized (no leaks)
- [ ] Performance meets Unreal Engine standards

---

### Task 1.3: Player Attack Function (15 minutes)
**Priority**: CRITICAL - Core gameplay blocker  
**Files**: `src/game/blockgame/player/player_combat.c`  
**Jules Task**: Implement the missing player combat function

```c
// TODO: Complete player attack implementation
int player_attack_entity(EntityID attacker_id, EntityID target_id, float damage) {
    // Currently stubbed - returns 0 always
    
    // Implementation needed:
    // 1. Validate attacker and target entities
    // 2. Check attack range and cooldown
    // 3. Apply damage with proper calculation
    // 4. Trigger combat animations
    // 5. Handle hit detection and effects
    // 6. Return appropriate status code
    
    return 0; // TODO: Replace with real implementation
}
```

**Acceptance Criteria**:
- [ ] Function validates entity existence and state
- [ ] Damage calculation includes weapon stats and modifiers
- [ ] Attack range and cooldown are enforced
- [ ] Combat animations are triggered properly
- [ ] Hit detection works with existing collision system
- [ ] Returns meaningful status codes (SUCCESS, OUT_OF_RANGE, ON_COOLDOWN, etc.)

---

## 🎯 **Phase 2: Advanced Rendering (Next 1.5 Hours)**

### Task 2.1: Lumen GI Integration (45 minutes)
**Priority**: HIGH - Unreal Engine quality feature  
**Files**: `src/engine/rendering/lighting/lumen_gi.m`  
**Jules Task**: Complete Lumen global illumination system

```objective-c
// TODO: Complete Lumen GI voxel clipmap updates
- (void)updateClipmaps:(LumenGISystem*)system 
         withViewOrigin:(Vec3)view_origin 
            viewProjection:(Mat4)view_proj {
    // Currently stubbed implementation
    
    for (int i = 0; i < LUMEN_CLIPMAP_LEVELS; i++) {
        float snap = system->clips[i].voxel_size;
        // TODO: Implement proper voxel snapping
        system->clips[i].center = view_origin; // Placeholder
    }
}

// TODO: Implement radiance cache updates
- (void)updateRadianceCache:(LumenGISystem*)system {
    // Implementation needed for indirect lighting
}

// TODO: Complete voxel injection
- (void)injectVoxels:(LumenGISystem*)system 
           withScene:(Scene*)scene {
    // Implementation needed for scene voxelization
}
```

**Acceptance Criteria**:
- [ ] Voxel clipmaps update correctly with camera movement
- [ ] Radiance cache provides stable indirect lighting
- [ ] Scene voxelization captures all relevant geometry
- [ ] Performance maintains 60+ FPS target
- [ ] Visual quality matches Unreal Engine Lumen

---

### Task 2.2: Ray Tracing Pipeline (30 minutes)
**Priority**: HIGH - Unreal Engine quality feature  
**Files**: `src/engine/rendering/advanced/ray_tracing.c`  
**Jules Task**: Complete ray tracing acceleration structures

```c
// TODO: Implement ray tracing acceleration structure creation
RayTracingAS* ray_tracing_create_acceleration_structure(RayTracingDevice* device) {
    // Currently stubbed - returns NULL
    return NULL; // TODO: Replace with real implementation
}

// TODO: Complete ray tracing shader binding
bool ray_tracing_bind_shaders(RayTracingPipeline* pipeline, Shader* shaders, uint32_t count) {
    // Implementation needed for ray generation, hit, miss shaders
    return false; // Currently stubbed
}

// TODO: Implement ray tracing command recording
void ray_tracing_trace(RayTracingCommandBuffer* cmd, RayTracingPipeline* pipeline, uint32_t width, uint32_t height) {
    // Implementation needed for actual ray tracing
    // Currently empty function
}
```

**Acceptance Criteria**:
- [ ] Acceleration structures (BLAS/TLAS) build correctly
- [ ] Ray tracing shaders bind and execute properly
- [ ] Ray generation, hit, and miss shaders work
- [ ] Performance is optimized for real-time use
- [ ] Integration with existing rendering pipeline successful

---

### Task 2.3: Virtual Texturing (15 minutes)
**Priority**: MEDIUM - Advanced streaming feature  
**Files**: `src/engine/rendering/texturing/virtual_texture.c`  
**Jules Task**: Complete virtual texture streaming system

```c
// TODO: Implement virtual texture tile streaming
bool virtual_texture_stream_tile(VirtualTextureSystem* vt, uint32_t tile_x, uint32_t tile_y, uint32_t mip_level) {
    // Currently stubbed - returns false
    // Implementation needed for runtime texture streaming
    return false;
}

// TODO: Complete virtual texture page table management
void virtual_texture_update_page_table(VirtualTextureSystem* vt, uint32_t page_index, uint32_t physical_address) {
    // Implementation needed for page table updates
}
```

**Acceptance Criteria**:
- [ ] Tile streaming works without visual artifacts
- [ ] Page table updates are efficient
- [ ] Memory usage is optimized for large textures
- [ ] Performance impact is minimal (<1ms per frame)

---

## 🎯 **Phase 3: Physics & AI Systems (Remaining Time)**

### Task 3.1: Vehicle Physics (30 minutes)
**Priority**: HIGH - Complex physics feature  
**Files**: `src/engine/physics/vehicle_physics.c`  
**Jules Task**: Complete vehicle dynamics simulation

```c
// TODO: Implement vehicle suspension physics
void vehicle_update_suspension(Vehicle* vehicle, float delta_time) {
    // Currently stubbed - no implementation
    for (int i = 0; i < 4; i++) {
        // TODO: Calculate suspension forces
        // TODO: Apply tire friction models
        // TODO: Handle wheel contact with ground
    }
}

// TODO: Complete vehicle engine simulation
float vehicle_calculate_engine_torque(Vehicle* vehicle, float throttle, float rpm) {
    // Implementation needed for realistic engine behavior
    return 0.0f; // Currently returns zero torque
}
```

**Acceptance Criteria**:
- [ ] Suspension responds realistically to terrain
- [ ] Tire friction models handle different surfaces
- [ ] Engine torque curve is realistic
- [ ] Vehicle handling feels natural
- [ ] Performance maintains stable 60 FPS

---

### Task 3.2: Behavior Tree AI (20 minutes)
**Priority**: HIGH - Complex AI feature  
**Files**: `src/engine/ai/behavior_tree.c`  
**Jules Task**: Complete behavior tree execution system

```c
// TODO: Implement behavior tree node execution
BehaviorResult behavior_tree_execute_node(BehaviorNode* node, AIContext* context) {
    // Currently stubbed - returns FAILURE always
    switch (node->type) {
        case BEHAVIOR_SEQUENCE:
            // TODO: Execute sequence logic
            return BEHAVIOR_FAILURE;
        case BEHAVIOR_SELECTOR:
            // TODO: Execute selector logic
            return BEHAVIOR_FAILURE;
        case BEHAVIOR_ACTION:
            // TODO: Execute action
            return BEHAVIOR_FAILURE;
        default:
            return BEHAVIOR_FAILURE;
    }
}
```

**Acceptance Criteria**:
- [ ] Sequence nodes execute children in order
- [ ] Selector nodes try alternatives until success
- [ ] Action nodes perform actual AI behaviors
- [ ] Tree execution is efficient and scalable
- [ ] Debugging and visualization tools work

---

## 📊 **Implementation Verification**

### Real-time Progress Tracking
```bash
# Check compilation status every 15 minutes
make test_unreal_features 2>&1 | tail -5

# Run specific system tests
./test_advanced_systems
./test_rendering_pipeline
./test_physics_integration
```

### Quality Gates
1. **Compilation**: Zero warnings, clean build
2. **Functionality**: All stub functions return meaningful values
3. **Integration**: Systems work together without crashes
4. **Performance**: Maintains 60+ FPS target
5. **Memory**: No leaks, efficient allocation

### Jules-Specific Guidelines
- **Code Style**: Follow existing patterns in each file
- **Error Handling**: Always validate inputs, return meaningful error codes
- **Comments**: Add implementation notes for complex logic
- **Testing**: Include basic validation in each function
- **Integration**: Test with existing systems immediately after implementation

## 🎯 **Success Criteria**

By the end of this implementation session:

✅ **Matrix Math**: All operations implemented and tested  
✅ **GPU Buffers**: Proper lifecycle management working  
✅ **Player Combat**: Attack function fully functional  
✅ **Lumen GI**: Basic indirect lighting operational  
✅ **Ray Tracing**: Acceleration structures building  
✅ **Virtual Texturing**: Tile streaming functional  
✅ **Vehicle Physics**: Suspension and engine simulation  
✅ **AI Behavior Trees**: Node execution working  

**Final Validation**: Run comprehensive test suite to verify all new implementations work together as a cohesive Unreal Engine-quality system.