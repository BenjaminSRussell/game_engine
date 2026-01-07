# Phase 2: Geometry & Directory Reorganization (100+ Tasks Per Agent)

---

## Agent 2.1: Directory Reorganization & Project Structure (~120 TODOs)

### Objective
Reorganize 154 flat directories into ~20 logical subsystems with proper hierarchy and clear ownership.

### Critical Implementation Tasks

**Directory Planning & Mapping (30 TODOs)**
- [ ] Audit all 154 current directories
- [ ] Categorize by functionality (core, geometry, lighting, materials, etc.)
- [ ] Create mapping document (old → new paths)
- [ ] Plan migration strategy
- [ ] Identify circular dependencies
- [ ] Plan include paths
- [ ] Design header search order
- [ ] Plan CMake structure
- [ ] Version compatibility strategy
- [ ] Documentation migration plan

**Core System Directories (15 TODOs)**
- [ ] Create `core/` with command, sync, resource subdirs
- [ ] Create `backend/metal/` for Metal API
- [ ] Create `platform/` for OS integration
- [ ] Create `memory/` for allocation management
- [ ] Move device files to `backend/metal/`
- [ ] Move command files to `core/command/`
- [ ] Move sync files to `core/sync/`
- [ ] Verify no circular dependencies
- [ ] Update all includes

**Geometry Subsystem (15 TODOs)**
- [ ] Create `geometry/mesh/`, `geometry/vertex/`, `geometry/lod/`
- [ ] Create `geometry/bvh/`, `geometry/cluster/`, `geometry/nanite/`
- [ ] Create `geometry/instancing/`
- [ ] Move mesh files
- [ ] Move vertex files
- [ ] Move LOD files
- [ ] Move BVH files
- [ ] Verify mesh system integrity

**Lighting Subsystem (15 TODOs)**
- [ ] Create `lighting/direct/`, `lighting/shadows/`, `lighting/gi/`
- [ ] Create `lighting/volumetric/`
- [ ] Move light source files
- [ ] Move shadow mapping files
- [ ] Move probe files
- [ ] Move volumetric files
- [ ] Update light manager includes

**Materials & Rendering (15 TODOs)**
- [ ] Create `materials/pbr/`, `materials/instances/`
- [ ] Create `shading/brdf/`, `shading/compiler/`
- [ ] Create `rendering/render_graph/`, `rendering/gbuffer/`
- [ ] Create `rendering/forward/`, `rendering/deferred/`
- [ ] Move material files
- [ ] Move shader files
- [ ] Move rendering files
- [ ] Update all dependencies

**Effects & Post-Processing (15 TODOs)**
- [ ] Create `effects/particles/`, `effects/decals/`
- [ ] Create `postprocess/taa/`, `postprocess/bloom/`
- [ ] Create `environment/ocean/`, `environment/atmosphere/`
- [ ] Move particle files
- [ ] Move effect files
- [ ] Move post-processing files
- [ ] Verify shader includes

**Character & Animation (10 TODOs)**
- [ ] Create `character/animation/`, `character/skeleton/`
- [ ] Create `character/cloth/`, `character/hair/`
- [ ] Move animation files
- [ ] Move cloth files
- [ ] Move hair files

**Assets & Tools (10 TODOs)**
- [ ] Create `assets/import/`, `assets/processing/`
- [ ] Create `editor/gizmos/`, `editor/debug/`, `editor/profiling/`
- [ ] Move asset import files
- [ ] Move editor files

**File Relocation & Migration (15 TODOs)**
- [ ] Copy all files to new locations
- [ ] Update relative includes
- [ ] Update absolute includes
- [ ] Fix #include guards
- [ ] Remove old directories
- [ ] Create migration log
- [ ] Verify no orphaned files
- [ ] Test build succeeds

**CMake & Build System (10 TODOs)**
- [ ] Update CMakeLists.txt with new structure
- [ ] Create per-subsystem CMakeLists.txt
- [ ] Update include paths
- [ ] Verify all targets build
- [ ] Test incremental builds
- [ ] Check compile times
- [ ] Verify link order

### Success Criteria
- [ ] All files migrated to new hierarchy
- [ ] No broken includes
- [ ] Build succeeds without warnings
- [ ] Clear ownership of each subsystem
- [ ] Dependencies are acyclic

---

## Agent 2.2: Mesh System Core (~100 TODOs)

### Objective
Implement mesh data structures, loading, and basic manipulation.

### Core Implementation Tasks

**Mesh Data Structures (25 TODOs)**
- [ ] Define vertex_t structure (position, normal, uv, tangent)
- [ ] Define mesh_t structure (vertices, indices, materials)
- [ ] Define submesh_t for multi-material meshes
- [ ] Define mesh_lod_t for LOD variants
- [ ] Mesh bounds calculations (AABB)
- [ ] Mesh transformation matrices
- [ ] Material slot management
- [ ] Mesh flags (dynamic, instanced, etc.)
- [ ] Mesh statistics (triangle count, etc.)
- [ ] Mesh validation

**Mesh Creation & Destruction (15 TODOs)**
- [ ] Create mesh from vertices/indices
- [ ] Allocate GPU buffers
- [ ] Create vertex buffer
- [ ] Create index buffer
- [ ] Copy data to GPU
- [ ] Destroy mesh and release GPU memory
- [ ] Reference counting

**Mesh Bounds & Calculations (15 TODOs)**
- [ ] Calculate AABB
- [ ] Calculate bounding sphere
- [ ] Calculate centroid
- [ ] Update bounds after transform
- [ ] OBB calculation
- [ ] Convex hull approximation
- [ ] Mesh statistics
- [ ] Validation

**Mesh Deformation (15 TODOs)**
- [ ] Morphing/blend shapes
- [ ] Skeletal deformation support
- [ ] GPU-based skinning setup
- [ ] Deformation limits
- [ ] Performance optimization

**Material Binding (15 TODOs)**
- [ ] Assign materials to submeshes
- [ ] Material slot validation
- [ ] Material parameter binding
- [ ] Per-instance material overrides
- [ ] Material array textures

**Mesh Optimization (15 TODOs)**
- [ ] Vertex deduplification
- [ ] Index optimization (cache coherence)
- [ ] Texture coordinate optimization
- [ ] Normal packing
- [ ] Tangent space computation
- [ ] Compression considerations

### Success Criteria
- [ ] Meshes create and destroy properly
- [ ] GPU memory allocated correctly
- [ ] All calculations accurate
- [ ] No memory leaks
- [ ] Ready for rendering

---

## Agent 2.3: Vertex Formats & Attributes (~100 TODOs)

### Objective
Implement flexible vertex attribute system with format definition and GPU binding.

### Core Implementation Tasks

**Vertex Format Definition (30 TODOs)**
- [ ] Define format descriptor structure
- [ ] Support variable attribute count
- [ ] Attribute offset calculation
- [ ] Stride calculation
- [ ] Format hashing
- [ ] Format validation
- [ ] Format caching
- [ ] Format statistics
- [ ] Alignment enforcement
- [ ] Platform-specific formats

**Standard Vertex Formats (15 TODOs)**
- [ ] Position only (P3)
- [ ] Position + Normal (P3N3)
- [ ] Position + Normal + UV (P3N3U2)
- [ ] Full PBR format (P3N3T4U2U2 + material data)
- [ ] Animated format (add bone indices/weights)
- [ ] Terrain format (heightmap, blending)
- [ ] Particle format (position, velocity, lifetime)

**GPU Vertex Binding (20 TODOs)**
- [ ] Create Metal vertex descriptor
- [ ] Bind vertex buffers to GPU
- [ ] Bind per-instance attributes
- [ ] Set attribute formats
- [ ] Set buffer stride
- [ ] Set buffer step rate
- [ ] Validate against pipeline

**Attribute Compression (20 TODOs)**
- [ ] Normal packing (oct encoding, 16-bit)
- [ ] Tangent space packing
- [ ] Position quantization
- [ ] Texture coordinate compression
- [ ] Color quantization
- [ ] Decompression in shader

**Format Reflection & Queries (15 TODOs)**
- [ ] Query format attributes
- [ ] Query attribute offset
- [ ] Query stride
- [ ] Get attribute by name
- [ ] Format compatibility checking
- [ ] Format size calculation

### Success Criteria
- [ ] All vertex formats supported
- [ ] Compression working
- [ ] GPU binding correct
- [ ] Performance optimal

---

## Agent 2.4: LOD System (~100 TODOs)

### Objective
Implement level-of-detail generation, selection, and transitions.

### Core Implementation Tasks

**LOD Generation (25 TODOs)**
- [ ] Generate LOD 0-4 variants
- [ ] Progressive decimation
- [ ] Error thresholding
- [ ] Triangle count targets
- [ ] Geometry preservation
- [ ] Normal smoothing
- [ ] UV preservation
- [ ] Material preservation
- [ ] Seam handling

**LOD Selection (25 TODOs)**
- [ ] Screen-space error calculation
- [ ] Distance-based selection
- [ ] Hysteresis for stability
- [ ] Prediction for frame N+1
- [ ] Camera velocity factoring
- [ ] Multiple viewer support
- [ ] Performance targets
- [ ] Dynamic thresholds

**LOD Transitions (20 TODOs)**
- [ ] Crossfade between LODs
- [ ] Fade duration control
- [ ] Seamless transitions
- [ ] Alpha blending
- [ ] Temporal dithering
- [ ] Popping prevention

**LOD Management (20 TODOs)**
- [ ] LOD chain storage
- [ ] Memory budgeting
- [ ] Streaming LODs
- [ ] LOD statistics
- [ ] Performance profiling
- [ ] Validation

**GPU LOD Selection (10 TODOs)**
- [ ] GPU-driven LOD selection
- [ ] Compute shader LOD decision
- [ ] Result readback

### Success Criteria
- [ ] LODs generate correctly
- [ ] Selection works smoothly
- [ ] Transitions seamless
- [ ] Memory efficient

---

## Agent 2.5: Spatial Acceleration & BVH (~100 TODOs)

### Objective
Implement BVH construction, traversal, and GPU culling.

### Core Implementation Tasks

**BVH Structure & Building (35 TODOs)**
- [ ] BVH node structure
- [ ] BVH leaf structure
- [ ] Building algorithm (binary split)
- [ ] SAH (Surface Area Heuristic) cost
- [ ] Top-down construction
- [ ] Parallel construction
- [ ] AABB computation
- [ ] Tree balance evaluation
- [ ] Validation

**BVH Traversal (20 TODOs)**
- [ ] Single ray traversal
- [ ] Frustum traversal
- [ ] AABB traversal
- [ ] Sphere intersection
- [ ] Closest hit queries
- [ ] All hits queries

**GPU-Based Culling (30 TODOs)**
- [ ] Upload BVH to GPU
- [ ] Compute shader culling
- [ ] Frustum culling kernels
- [ ] Occlusion culling setup
- [ ] Result output buffer
- [ ] Visible list generation
- [ ] Statistics
- [ ] Performance optimization

**Incremental Updates (15 TODOs)**
- [ ] Dynamic BVH updates
- [ ] Node refit
- [ ] Tree reconstruction
- [ ] Temporal coherence

### Success Criteria
- [ ] BVH builds correctly
- [ ] Culling effective
- [ ] GPU operations fast
- [ ] Memory efficient

---

## Agent 2.6: Instancing & GPU-Driven Rendering (~100 TODOs)

### Objective
Implement instancing system with indirect rendering and GPU-driven batching.

### Core Implementation Tasks

**Instance Data Management (25 TODOs)**
- [ ] Instance buffer structure
- [ ] Per-instance transforms
- [ ] Per-instance material parameters
- [ ] Per-instance visibility flags
- [ ] Dynamic instance updates
- [ ] Instance pooling
- [ ] Buffer management
- [ ] Update strategy

**Indirect Rendering Setup (25 TODOs)**
- [ ] Indirect draw arguments structure
- [ ] Indirect buffer creation
- [ ] Indirect draw dispatch
- [ ] Multi-draw indirect
- [ ] Argument generation
- [ ] Reset between frames

**GPU-Driven Batching (30 TODOs)**
- [ ] Instance culling compute
- [ ] Dispatch size calculation
- [ ] Compaction of results
- [ ] Material sorting
- [ ] Batch grouping
- [ ] Work distribution
- [ ] Load balancing
- [ ] Statistics

**Performance Optimization (20 TODOs)**
- [ ] Reduce state changes
- [ ] Minimize draw calls
- [ ] Optimize memory access
- [ ] Cache coherence
- [ ] Profiling tools

### Success Criteria
- [ ] Instancing works correctly
- [ ] Indirect rendering functional
- [ ] GPU-driven efficient
- [ ] Draw call count reduced

---

## Phase 2 Integration Checklist

All agents must verify:
- [ ] Directory structure consistent
- [ ] All includes updated
- [ ] Build succeeds
- [ ] No circular dependencies
- [ ] ~600 TODOs completed
- [ ] Ready for Phase 3

