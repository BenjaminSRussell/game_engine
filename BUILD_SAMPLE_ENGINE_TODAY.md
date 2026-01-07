# Build Running Sample Engine - TODAY

**Goal:** Get a working Metal rendering engine running today
**Approach:** Minimal viable sample with existing code
**Timeline:** ~4-6 hours
**Target:** Render a simple scene with textures and lighting

---

## Current State Assessment

✅ **What We Have:**
- Phase 1: Metal backend (6 critical fixes complete)
- Phase 2: Geometry system (partially implemented)
- Phase 3: Rendering pipeline (partial implementations in multiple formats)
- Rendering subsystems: Camera, Forward/Deferred, Lighting, Materials
- Main entry point (main.c) with full infrastructure
- 2515 implementation files with rendering code

✅ **What We Need:**
- Simple executable that initializes Metal
- Window creation (Metal layer)
- Basic shader compilation
- Mesh rendering
- Minimal scene setup

---

## Build Plan

### STEP 1: Create Minimal Sample App (30 min)
**Goal:** Working Metal window with clear color

Create: `/Users/benjaminrussell/Desktop/Minecraft v2/src/sample_app.m`

Simple Metal app that:
- Creates window (NSWindow)
- Creates Metal device and queue
- Creates CAMetalLayer view
- Clears to color each frame
- Uses our Metal backend

### STEP 2: Wire up Device & Command Buffers (30 min)
**Goal:** Frame loop working

Use existing code from:
- `src/engine/backend/metal/mtl_device.c` - Device creation
- `src/engine/backend/metal/mtl_command_buffer.c` - Command submission
- `src/engine/backend/metal/mtl_sync.m` - Frame synchronization

### STEP 3: Add Basic Shader System (45 min)
**Goal:** Render colored triangle

Use:
- `src/engine/rendering/shaders/` - Shader files
- `src/engine/backend/metal/mtl_shader_compiler.m` - Shader compilation
- Simple vertex/fragment shader

### STEP 4: Add Mesh Rendering (45 min)
**Goal:** Render cube or plane with texture

Use:
- `src/engine/rendering/mesh/` - Mesh loading
- `src/engine/backend/metal/mtl_buffer.c` - Vertex/index buffers
- `src/engine/backend/metal/mtl_texture.c` - Textures

### STEP 5: Add Basic Lighting (45 min)
**Goal:** Simple diffuse + specular lighting

Use:
- `src/engine/rendering/lighting/` - Lighting code
- Forward rendering with directional light

### STEP 6: Integrate with Engine Main (30 min)
**Goal:** Full app running

Hook sample app into main.c

---

## Fastest Path to Something Visual

### Core Files Needed
1. **Window/Device Setup** (~100 lines Objective-C)
   - Create NSWindow
   - Setup Metal layer
   - Create device

2. **Render Loop** (~50 lines)
   - Encode clear pass
   - Submit command buffer
   - Present drawable

3. **Shader System** (~100 lines)
   - Compile simple shader
   - Create render pipeline

4. **Mesh Rendering** (~150 lines)
   - Create triangle mesh
   - Render with pipeline

5. **Lighting** (~100 lines)
   - Simple lighting calculations
   - Material properties

**Total:** ~500 lines of new code

---

## What to NOT Do

❌ Don't build full ECS system
❌ Don't implement advanced features from agents
❌ Don't refactor existing code
❌ Don't try to use dynamic modules
❌ Don't build UI
❌ Don't implement physics
❌ Don't do asset loading (hardcode geometry)

---

## What to DO

✅ Use existing Metal backend (with our 6 fixes)
✅ Create thin wrapper for window/device
✅ Simple hardcoded mesh (triangle)
✅ Minimal shader (flat color then textured)
✅ One light source
✅ Forward rendering only
✅ No fancy features - just render

---

## Compilation Strategy

### Option A: Single File Compilation (Fastest)
Compile everything into one executable:
```bash
cd "/Users/benjaminrussell/Desktop/Minecraft v2"

clang -framework Metal -framework Cocoa -framework QuartzCore \
  -Wall -Wextra \
  src/sample_app.m \
  src/engine/backend/metal/mtl_*.c \
  src/engine/backend/metal/mtl_*.m \
  src/engine/rendering/core/*.c \
  src/engine/rendering/shaders/*.c \
  -o sample_engine
```

### Option B: Use CMake (if exists)
Check if CMakeLists.txt works:
```bash
cd build && cmake .. && make sample_engine
```

### Option C: Minimal Build Script
Create build script that compiles what's needed

---

## Sample App Structure

```c
// src/sample_app.m
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

// 1. Create window
NSWindow* window = [[NSWindow alloc] init...];
NSView* view = [[NSView alloc] init...];

// 2. Setup Metal
id<MTLDevice> device = MTLCreateSystemDefaultDevice();
CAMetalLayer* layer = [CAMetalLayer layer];
layer.device = device;

// 3. Render loop
while (!quit) {
    // Get drawable
    id<CAMetalDrawable> drawable = [layer nextDrawable];

    // Create command buffer
    id<MTLCommandBuffer> cmd = [queue commandBuffer];

    // Encode render pass
    // ... render commands ...

    // Present
    [cmd presentDrawable:drawable];
    [cmd commit];
}
```

---

## Immediate Next Steps

1. **Create src/sample_app.m** with window setup
2. **Test Metal compilation** (clang -framework Metal)
3. **Add basic render loop**
4. **Add shader compilation**
5. **Add triangle mesh**
6. **Add lighting**
7. **Run and see result**

---

## Success Criteria

✅ Window opens
✅ Screen clears to color
✅ Triangle renders
✅ Triangle is lit
✅ Can interact (rotate view)
✅ Runs at 60 FPS
✅ No crashes for 5 minutes

---

## Time Budget

| Phase | Task | Time | Status |
|-------|------|------|--------|
| 1 | Window setup | 30 min | 📝 Ready |
| 2 | Render loop | 30 min | 📝 Ready |
| 3 | Shaders | 45 min | 📝 Ready |
| 4 | Mesh | 45 min | 📝 Ready |
| 5 | Lighting | 45 min | 📝 Ready |
| 6 | Integration | 30 min | 📝 Ready |
| - | **TOTAL** | **225 min (3.75 hours)** | - |

Plus 1-2 hours debugging = **5-6 hours total**

---

## Files to Reference

**Metal Backend (Use our fixes):**
- src/engine/backend/metal/mtl_device.c
- src/engine/backend/metal/mtl_device.h
- src/engine/backend/metal/mtl_command_buffer.c
- src/engine/backend/metal/mtl_buffer.c
- src/engine/backend/metal/mtl_texture.c
- src/engine/backend/metal/mtl_shader_compiler.m

**Rendering Code:**
- src/engine/rendering/shaders/ - Shader files
- src/engine/rendering/camera/ - Camera system
- src/engine/rendering/forward/ - Forward rendering
- src/engine/rendering/framebuffer.c - Framebuffer setup

**Entry Point:**
- src/engine/core/main.c - Already has infrastructure

---

## Why This Works

✅ **Phase 1 fixes make Metal backend stable**
✅ **Most rendering code already exists**
✅ **We only need thin integration layer**
✅ **Can hardcode mesh/shaders for now**
✅ **NSWindow + Metal simple on macOS**
✅ **No external dependencies (Metal is system)**

---

## Expected Result

A macOS application window showing:
- Clear background
- Rendered 3D geometry
- Basic lighting
- Smooth interaction
- 60 FPS

That's a **running game engine sample!**

---

## If Running Out of Time

**Minimum viable:**
- [ ] Clear to color (10 min) ✅ START HERE
- [ ] Triangle render (20 min)
- [ ] Simple shader (15 min)
- [ ] Total: 45 minutes = VISUAL ENGINE

**Nice to have if time:**
- [ ] Lighting
- [ ] Textured geometry
- [ ] Camera controls
- [ ] Smooth animation

---

## Let's Do This

**START:** Create src/sample_app.m with Window/Metal setup
**AIM:** Have window open and clearing to color in 30 minutes
**THEN:** Iterate adding rendering features

Ready to build?
