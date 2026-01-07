# Detailed TODO Checklist: All 200+ Tasks

## Legend
- ⏳ Not Started
- 🔄 In Progress
- ✅ Completed
- ⚠️ Blocked/On Hold

---

## PHASE 1: VERTEX FORMAT FOUNDATION (15 TODOs)

### 1.1 Core Format Definition System (8 TODOs)

- [ ] **1.1.1** ⏳ Create `vertex_format.h` with type definitions
  - [ ] Define `vertex_attribute_type_t` enum (FLOAT, FLOAT2, FLOAT3, INT, etc)
  - [ ] Define `vertex_attribute_t` struct (name, type, offset, optional flag)
  - [ ] Define `vertex_format_t` struct (attributes[], count, stride, hash)
  - [ ] Add validation macros

- [ ] **1.1.2** ⏳ Create `vertex_format.c` with basic implementation
  - [ ] Implement `vertex_format_create(name)` → allocate and initialize
  - [ ] Implement `vertex_format_destroy()` → free memory, cleanup
  - [ ] Implement `vertex_format_clone()` → deep copy format
  - [ ] Add memory tracking for statistics

- [ ] **1.1.3** ⏳ Add attribute management functions
  - [ ] Implement `vertex_format_add_attribute()` → add attribute to format
  - [ ] Implement `vertex_format_get_attribute_by_name()` → lookup by name
  - [ ] Implement `vertex_format_get_attribute_by_index()` → lookup by index
  - [ ] Validate no duplicate names

- [ ] **1.1.4** ⏳ Add stride calculation
  - [ ] Implement `vertex_format_calculate_stride()` → compute total stride
  - [ ] Handle alignment requirements (16-byte boundaries)
  - [ ] Calculate and store padding automatically
  - [ ] Test with various attribute combinations

- [ ] **1.1.5** ⏳ Add basic validation
  - [ ] Implement `vertex_format_validate()` → return validation_result_t
  - [ ] Check for overlapping attributes
  - [ ] Check stride is reasonable (< 1024 bytes)
  - [ ] Check alignment is power of 2

- [ ] **1.1.6** ⏳ Add format hashing
  - [ ] Implement `vertex_format_calculate_hash()` → hash64
  - [ ] Ensure consistent hash for same format
  - [ ] Use for format caching and deduplication
  - [ ] Test hash collision resistance

- [ ] **1.1.7** ⏳ Add format comparison
  - [ ] Implement `vertex_format_equals()` → compare two formats
  - [ ] Implement `vertex_format_is_compatible()` → check substitutability
  - [ ] Handle attribute reordering
  - [ ] Document compatibility rules

- [ ] **1.1.8** ⏳ Add statistics tracking
  - [ ] Track total formats created
  - [ ] Track total memory used
  - [ ] Track lookup/comparison operations
  - [ ] Implement `vertex_format_print_statistics()`

**Subtotal Phase 1.1: 8 TODOs**

---

### 1.2 Standard Format Definitions (5 TODOs)

- [ ] **1.2.1** ⏳ Implement P3 format (12 bytes)
  - [ ] Create `VERTEX_FORMAT_P3` constant
  - [ ] Define: Position (vec3) @ offset 0 = 12 bytes
  - [ ] Total stride: 12 bytes
  - [ ] Verify Metal compatibility

- [ ] **1.2.2** ⏳ Implement P3N3 format (24 bytes)
  - [ ] Create `VERTEX_FORMAT_P3N3` constant
  - [ ] Define: Position (vec3) @ offset 0 = 12 bytes
  - [ ] Define: Normal (vec3) @ offset 12 = 12 bytes
  - [ ] Total stride: 24 bytes

- [ ] **1.2.3** ⏳ Implement P3N3U2 format (32 bytes)
  - [ ] Create `VERTEX_FORMAT_P3N3U2` constant
  - [ ] Define: Position (vec3) @ offset 0 = 12 bytes
  - [ ] Define: Normal (vec3) @ offset 12 = 12 bytes
  - [ ] Define: TexCoord0 (vec2) @ offset 24 = 8 bytes
  - [ ] Total stride: 32 bytes (with 4-byte padding)

- [ ] **1.2.4** ⏳ Create standard format table
  - [ ] Create array of pre-defined format_t structs
  - [ ] Implement `vertex_format_get_standard(type)` → returns format
  - [ ] Ensure formats are immutable (const)
  - [ ] Add format count constant

- [ ] **1.2.5** ⏳ Add format utilities
  - [ ] Implement `vertex_format_get_name(enum)` → return string name
  - [ ] Implement `vertex_format_get_standard()` → return format descriptor
  - [ ] Implement `vertex_format_get_stride(enum)` → quick stride lookup
  - [ ] Create format enumeration iterator

**Subtotal Phase 1.2: 5 TODOs**

---

### 1.3 Metal Integration (2 TODOs)

- [ ] **1.3.1** ⏳ Create Metal conversion
  - [ ] Implement `vertex_format_to_metal_descriptor()` → metal_vertex_descriptor_t
  - [ ] Map vertex_attribute_type_t to metal_vertex_format_t
  - [ ] Set up buffer layout from format
  - [ ] Set stride and buffer indices

- [ ] **1.3.2** ⏳ Implement GPU binding
  - [ ] Implement `metal_bind_vertex_format()`
  - [ ] Bind format to render encoder
  - [ ] Set vertex buffers using format layout
  - [ ] Validate format compatibility with pipeline
  - [ ] Handle multiple vertex buffers

**Subtotal Phase 1.3: 2 TODOs**

---

### Phase 1 Build & Testing (Not in TODO count but required)

- [ ] Create CMakeLists.txt for vertex/ subsystem
- [ ] Update parent CMakeLists.txt with new sources
- [ ] Verify compilation (no errors or warnings)
- [ ] Create unit tests (vertex_format_test.c)
- [ ] Test all 3 standard formats
- [ ] Test format validation
- [ ] Test Metal integration
- [ ] Verify no memory leaks

---

## PHASE 2: COMPRESSION & OPTIMIZATION (40 TODOs)

### 2.1 Normal Compression (10 TODOs)

- [ ] **2.1.1** ⏳ Implement Oct16 encoding
  - [ ] Function: `normal_to_oct(const float normal[3]) → packed_normal_oct_t`
  - [ ] Map 3D normal to 2D octahedral map
  - [ ] Quantize to int16 range
  - [ ] Handle all octants correctly

- [ ] **2.1.2** ⏳ Implement Oct16 decoding
  - [ ] Function: `oct_to_normal(const packed_normal_oct_t*) → float[3]`
  - [ ] Reverse octahedral mapping
  - [ ] Normalize result
  - [ ] Verify accuracy

- [ ] **2.1.3** ⏳ Add compression quality metrics
  - [ ] Calculate RMSE (root mean squared error) for batch
  - [ ] Calculate max error
  - [ ] Calculate average error
  - [ ] Generate quality report

- [ ] **2.1.4** ⏳ Create Metal decompression shader
  - [ ] Implement `octahedral_decode()` in Metal
  - [ ] Test in compute shader
  - [ ] Verify accuracy matches CPU version
  - [ ] Benchmark performance

- [ ] **2.1.5** ⏳ Add spheremap as alternative
  - [ ] Function: `normal_to_spheremap()` → uint16[2]
  - [ ] Function: `spheremap_to_normal()` → float[3]
  - [ ] Compare quality vs Oct16
  - [ ] Provide both options

- [ ] **2.1.6** ⏳ Implement SIMD optimization
  - [ ] Batch compress multiple normals efficiently
  - [ ] Use SSE2/NEON if available
  - [ ] Fallback to scalar
  - [ ] Benchmark speedup

- [ ] **2.1.7** ⏳ Add error analysis tools
  - [ ] Create visualization of compression artifacts
  - [ ] Generate quality reports per normal
  - [ ] Suggest precision improvements
  - [ ] Show before/after comparison

- [ ] **2.1.8** ⏳ Create test suite
  - [ ] Test vectors with known results
  - [ ] Verify round-trip accuracy (compress → decompress)
  - [ ] Edge cases (zero normal, normalized, etc)
  - [ ] Random batch testing

- [ ] **2.1.9** ⏳ Add shader code generation
  - [ ] Generate decompression code for shaders
  - [ ] Format as #include-able header
  - [ ] Support multiple precision levels
  - [ ] Document usage

- [ ] **2.1.10** ⏳ Documentation
  - [ ] Explain algorithm with diagrams
  - [ ] Provide usage examples
  - [ ] Document quality/performance tradeoffs
  - [ ] List limitations

**Subtotal Phase 2.1: 10 TODOs**

---

### 2.2 Position Quantization (8 TODOs)

- [ ] **2.2.1** ⏳ Implement position quantization
  - [ ] Function: `position_quantize(const float[3], params) → packed_position_t`
  - [ ] Map float position to quantized range
  - [ ] Handle underflow/overflow
  - [ ] Support variable precision (16, 20, 24 bits)

- [ ] **2.2.2** ⏳ Implement position dequantization
  - [ ] Function: `position_dequantize(const packed_position_t*, params) → float[3]`
  - [ ] Reverse quantization mapping
  - [ ] Handle precision correctly

- [ ] **2.2.3** ⏳ Calculate optimal quantization parameters
  - [ ] Function: `position_calculate_params(positions[], count) → params`
  - [ ] Find optimal bounds from vertex data
  - [ ] Target different precision levels
  - [ ] Minimize quantization error

- [ ] **2.2.4** ⏳ Add error metrics
  - [ ] Estimate quantization error for given precision
  - [ ] Generate error reports
  - [ ] Suggest precision levels based on error tolerance
  - [ ] Visualize error distribution

- [ ] **2.2.5** ⏳ Metal decompression
  - [ ] Implement decompression in vertex shader
  - [ ] Test accuracy matches CPU
  - [ ] Benchmark GPU cost
  - [ ] Profile memory savings vs decompression cost

- [ ] **2.2.6** ⏳ Support variable precision
  - [ ] Allow 16, 20, 24 bit quantization
  - [ ] Auto-select precision from mesh bounds
  - [ ] Store precision in format
  - [ ] Validate precision value

- [ ] **2.2.7** ⏳ Add visualization
  - [ ] Show quantized vs original positions side-by-side
  - [ ] Highlight high-error areas in heatmap
  - [ ] Interactive precision slider
  - [ ] Real-time error metric updates

- [ ] **2.2.8** ⏳ Test suite
  - [ ] Edge cases (very small/large scales)
  - [ ] Precision loss verification
  - [ ] Round-trip accuracy with various precisions
  - [ ] Mesh-specific testing

**Subtotal Phase 2.2: 8 TODOs**

---

### 2.3 Additional Compression (8 TODOs)

- [ ] **2.3.1** ⏳ Implement UV compression (float16)
  - [ ] Function: `uv_pack(const float[2]) → packed_uv_t`
  - [ ] Function: `uv_unpack(const packed_uv_t*) → float[2]`
  - [ ] Handle repeat UVs correctly
  - [ ] Test accuracy

- [ ] **2.3.2** ⏳ Implement color compression (uint8 RGBA)
  - [ ] Function: `color_pack(const float[4]) → packed_color_t`
  - [ ] Function: `color_unpack(const packed_color_t*) → float[4]`
  - [ ] Quantize float[0,1] to uint8[0,255]
  - [ ] Test with various color values

- [ ] **2.3.3** ⏳ Implement tangent compression
  - [ ] Function: `tangent_pack()` → stores tangent + handedness
  - [ ] Reconstruct binormal in shader from normal + tangent
  - [ ] Verify TBN reconstruction
  - [ ] Test normal mapping quality

- [ ] **2.3.4** ⏳ Create format optimizer
  - [ ] Analyze format for compression opportunities
  - [ ] Suggest compression techniques per attribute
  - [ ] Estimate memory savings
  - [ ] Estimate quality loss

- [ ] **2.3.5** ⏳ Add format optimization tool
  - [ ] Function: `vertex_format_suggest_compression()`
  - [ ] Function: `vertex_format_apply_compression()`
  - [ ] Generate optimized format variant
  - [ ] Provide conversion functions

- [ ] **2.3.6** ⏳ Metal shader helpers
  - [ ] Collection of decompression functions in Metal
  - [ ] Include as metal library
  - [ ] Provide as #include header
  - [ ] Document all functions

- [ ] **2.3.7** ⏳ Quality assessment
  - [ ] Visual comparison tool
  - [ ] Numerical error metrics
  - [ ] Per-attribute analysis
  - [ ] Generate quality reports

- [ ] **2.3.8** ⏳ Benchmarking
  - [ ] Measure memory size reduction percentage
  - [ ] Measure bandwidth improvement
  - [ ] Measure decompression GPU cost
  - [ ] Calculate effective speedup

**Subtotal Phase 2.3: 8 TODOs**

---

### 2.4 Advanced Standard Formats (10 TODOs)

- [ ] **2.4.1** ⏳ Implement P3N3T4U2U2 (Full PBR - 48 bytes)
  - [ ] Position (vec3) @ offset 0 = 12 bytes
  - [ ] Normal (oct16) @ offset 12 = 4 bytes (compressed from vec3)
  - [ ] Tangent (oct16) @ offset 16 = 4 bytes (compressed from vec3)
  - [ ] TexCoord0 (vec2) @ offset 20 = 8 bytes
  - [ ] TexCoord1 (vec2) @ offset 28 = 8 bytes
  - [ ] Padding = 12 bytes
  - [ ] Total stride: 48 bytes (vs 60 without compression)
  - [ ] Create constant and register format

- [ ] **2.4.2** ⏳ Implement Animated format
  - [ ] Based on P3N3T4U2U2 (48 bytes)
  - [ ] BoneIndices (vec4 uint16) @ offset 48 = 8 bytes
  - [ ] BoneWeights (vec4) @ offset 56 = 16 bytes
  - [ ] Total stride: 72 bytes
  - [ ] Add bone data validation functions

- [ ] **2.4.3** ⏳ Implement Terrain format
  - [ ] Position (vec3) @ offset 0 = 12 bytes
  - [ ] Normal (oct16) @ offset 12 = 4 bytes
  - [ ] Height (float) @ offset 16 = 4 bytes
  - [ ] Blend weights (vec4 uint8) @ offset 20 = 4 bytes
  - [ ] Texture indices (vec4 uint8) @ offset 24 = 4 bytes
  - [ ] Total stride: 28 bytes
  - [ ] Add terrain-specific functions

- [ ] **2.4.4** ⏳ Implement Particle format
  - [ ] Position (vec3) @ offset 0 = 12 bytes
  - [ ] Velocity (vec3) @ offset 12 = 12 bytes
  - [ ] Lifetime (float) @ offset 24 = 4 bytes
  - [ ] Size (float) @ offset 28 = 4 bytes
  - [ ] Total stride: 32 bytes
  - [ ] Add particle system integration functions

- [ ] **2.4.5** ⏳ Add format converters
  - [ ] Implement P3 → P3N3 converter
  - [ ] Implement P3N3 → P3N3U2 converter
  - [ ] Implement P3N3U2 → PBR converter
  - [ ] Implement PBR → Animated converter
  - [ ] Handle data loss appropriately

- [ ] **2.4.6** ⏳ Implement batch conversion
  - [ ] Process multiple vertices efficiently
  - [ ] Handle compression during conversion
  - [ ] Parallel processing for large meshes
  - [ ] Progress tracking for long conversions

- [ ] **2.4.7** ⏳ Add validation for new formats
  - [ ] Verify all formats have correct stride
  - [ ] Check alignment within stride
  - [ ] Ensure Metal compatibility
  - [ ] Test with sample meshes

- [ ] **2.4.8** ⏳ Implement format discovery
  - [ ] Determine optimal format for mesh analysis
  - [ ] Analyze mesh characteristics (bounds, normals, UVs, etc)
  - [ ] Recommend best format for quality/size tradeoff
  - [ ] Provide detailed analysis report

- [ ] **2.4.9** ⏳ Create converter utilities
  - [ ] Helpers for common conversions
  - [ ] Error handling for invalid conversions
  - [ ] Progress callbacks for UI integration
  - [ ] Memory-efficient streaming conversion

- [ ] **2.4.10** ⏳ Test all formats
  - [ ] Create diverse test meshes
  - [ ] Verify rendering correctness for each format
  - [ ] Compare memory vs quality for each
  - [ ] Document performance characteristics

**Subtotal Phase 2.4: 10 TODOs**

---

### 2.5 Format Validation & Reflection (4 TODOs)

- [ ] **2.5.1** ⏳ Implement comprehensive validation
  - [ ] Stride validation against Metal limits
  - [ ] Offset/overlap detection
  - [ ] Alignment enforcement (16-byte boundaries)
  - [ ] GPU format compatibility checking
  - [ ] Size limit checking (max 256 bytes)
  - [ ] Packing efficiency analysis
  - [ ] Generate detailed error messages
  - [ ] Provide optimization suggestions

- [ ] **2.5.2** ⏳ Implement shader reflection
  - [ ] Extract vertex input signature from compiled Metal shader
  - [ ] Match format attributes to shader input variables
  - [ ] Detect missing attributes (shader expects but format doesn't have)
  - [ ] Detect extra attributes (format has but shader doesn't use)
  - [ ] Type mismatch detection
  - [ ] Generate compatibility reports
  - [ ] Suggest attribute reordering

- [ ] **2.5.3** ⏳ Add compatibility matrix
  - [ ] Cross-reference all formats and all shaders
  - [ ] Pre-compute at shader compilation time
  - [ ] Cache results for fast lookup
  - [ ] Provide `is_format_compatible_with_shader()` query
  - [ ] Generate matrix visualization

- [ ] **2.5.4** ⏳ Generate diagnostics
  - [ ] Detailed error messages for validation failures
  - [ ] Optimization suggestions for inefficient formats
  - [ ] Visualization of format layout (ASCII diagram)
  - [ ] Memory analysis with padding waste calculation
  - [ ] Generate HTML/text reports

**Subtotal Phase 2.5: 4 TODOs**

---

**PHASE 2 TOTAL: 40 TODOs**

---

## PHASE 3: INSTANCING FOUNDATION (25 TODOs)

### 3.1 Instance Data Management (12 TODOs)

- [ ] **3.1.1** ⏳ Define instance_data_t structure
  - [ ] 4x4 transform matrix (64 bytes)
  - [ ] 4x4 previous frame transform (64 bytes)
  - [ ] Custom data vec4[2] (32 bytes)
  - [ ] Material ID (4 bytes)
  - [ ] Visibility mask (4 bytes)
  - [ ] Padding to 256-byte align (8 bytes)
  - [ ] Total: 176 bytes per instance
  - [ ] Add comments for shader access

- [ ] **3.1.2** ⏳ Create instance_buffer_t struct
  - [ ] CPU-side data array (instance_data_t*)
  - [ ] GPU buffer handle (metal_buffer_t*)
  - [ ] Count of active instances (uint32_t)
  - [ ] Capacity for growth (uint32_t)
  - [ ] Dirty flag tracking (bool)
  - [ ] Free list/sparse array support (optional)

- [ ] **3.1.3** ⏳ Implement instance_buffer_create()
  - [ ] Allocate CPU array for instances
  - [ ] Create GPU buffer with appropriate size
  - [ ] Initialize metadata (count=0, capacity=max)
  - [ ] Handle creation failure gracefully
  - [ ] Return initialized buffer or NULL

- [ ] **3.1.4** ⏳ Implement instance_buffer_destroy()
  - [ ] Free CPU memory
  - [ ] Release GPU buffer
  - [ ] Cleanup synchronization primitives
  - [ ] Verify no leaks

- [ ] **3.1.5** ⏳ Implement instance_buffer_allocate()
  - [ ] Add instance to buffer at next free slot
  - [ ] Copy initial data from parameter
  - [ ] Assign and return index/handle
  - [ ] Handle capacity growth if needed
  - [ ] Mark buffer as dirty

- [ ] **3.1.6** ⏳ Implement instance_buffer_update()
  - [ ] Update specific instance data
  - [ ] Mark as dirty for GPU upload
  - [ ] Support partial updates (specific fields)
  - [ ] Bounds checking on index

- [ ] **3.1.7** ⏳ Implement instance_buffer_free()
  - [ ] Mark instance as free (for sparse allocation)
  - [ ] Support reuse of freed slots
  - [ ] Maintain sparse allocation efficiently
  - [ ] Optional: swap with last active instance

- [ ] **3.1.8** ⏳ Implement instance_buffer_flush_gpu()
  - [ ] Upload dirty instances to GPU buffer
  - [ ] Handle double-buffering for pipelined rendering
  - [ ] Perform per-frame GPU synchronization
  - [ ] Support async upload with callback

- [ ] **3.1.9** ⏳ Add iterator support
  - [ ] Iterate over only active instances
  - [ ] Skip freed/inactive instances efficiently
  - [ ] Thread-safe iteration if needed
  - [ ] Support reverse iteration

- [ ] **3.1.10** ⏳ Add batch operations
  - [ ] `instance_buffer_allocate_batch()` - allocate multiple at once
  - [ ] `instance_buffer_update_batch()` - update multiple at once
  - [ ] `instance_buffer_free_batch()` - free multiple at once
  - [ ] Efficient bulk operations

- [ ] **3.1.11** ⏳ Add statistics tracking
  - [ ] Active instance count
  - [ ] Free instance count
  - [ ] Total memory usage (CPU + GPU)
  - [ ] GPU bandwidth measurements
  - [ ] Frame timing for upload

- [ ] **3.1.12** ⏳ Add validation and debugging
  - [ ] Bounds checking on all operations
  - [ ] Handle validation (generation counters)
  - [ ] Debug output functions
  - [ ] Memory leak detection
  - [ ] Corruption detection

**Subtotal Phase 3.1: 12 TODOs**

---

### 3.2 Instance Pooling (13 TODOs)

- [ ] **3.2.1** ⏳ Design instance pool structure
  - [ ] Free list for allocation tracking
  - [ ] Generation counters for use-after-free detection
  - [ ] Capacity management and growth
  - [ ] Active instance tracking
  - [ ] Statistics collection

- [ ] **3.2.2** ⏳ Implement instance_pool_create()
  - [ ] Initialize free list
  - [ ] Allocate capacity (pools can be large)
  - [ ] Set up generation counters
  - [ ] Initialize statistics
  - [ ] Return pool handle or pointer

- [ ] **3.2.3** ⏳ Implement instance_pool_destroy()
  - [ ] Free all allocated data structures
  - [ ] Flush pending operations
  - [ ] Final statistics output
  - [ ] Verify no leaks

- [ ] **3.2.4** ⏳ Implement instance_pool_allocate()
  - [ ] Allocate from free list
  - [ ] Generate unique handle with generation counter
  - [ ] Prevent use-after-free bugs
  - [ ] Return unique handle

- [ ] **3.2.5** ⏳ Implement instance_pool_free()
  - [ ] Return handle to free list
  - [ ] Increment generation counter
  - [ ] Maintain pool statistics
  - [ ] Clear instance data (optional security)

- [ ] **3.2.6** ⏳ Implement instance_pool_get()
  - [ ] Look up instance by handle
  - [ ] Validate generation counter
  - [ ] Detect use-after-free
  - [ ] Return instance data pointer or NULL

- [ ] **3.2.7** ⏳ Implement instance_pool_update()
  - [ ] Update instance data by handle
  - [ ] Batch updates for efficiency
  - [ ] Validation and error checking

- [ ] **3.2.8** ⏳ Add pool statistics functions
  - [ ] `instance_pool_get_active_count()` - number of allocated instances
  - [ ] `instance_pool_get_free_count()` - number available
  - [ ] `instance_pool_get_utilization()` - percentage used
  - [ ] `instance_pool_get_memory_usage()` - total bytes

- [ ] **3.2.9** ⏳ Add pool validation
  - [ ] `instance_pool_is_valid_handle()` - check handle validity
  - [ ] Corruption detection
  - [ ] Integrity checking
  - [ ] Generate debug reports

- [ ] **3.2.10** ⏳ Add automatic growth
  - [ ] Detect when pool capacity limit reached
  - [ ] Grow pool dynamically
  - [ ] Maintain performance (reasonable growth factor)
  - [ ] Communicate growth to user

- [ ] **3.2.11** ⏳ Add thread-safety (if needed)
  - [ ] Lock-free data structures or mutex protection
  - [ ] Atomic generation counters
  - [ ] Safe concurrent access
  - [ ] Benchmark contention

- [ ] **3.2.12** ⏳ Add pool iteration
  - [ ] Iterate over active instances
  - [ ] Support filtering (by property)
  - [ ] Safe removal during iteration

- [ ] **3.2.13** ⏳ Test suite
  - [ ] Allocate/free many instances rapidly
  - [ ] Use-after-free detection validation
  - [ ] Memory leak verification
  - [ ] Stress testing with many concurrent operations

**Subtotal Phase 3.2: 13 TODOs**

---

**PHASE 3 TOTAL: 25 TODOs**

---

## PHASE 4: GPU-DRIVEN RENDERING (35 TODOs)

### 4.1 Indirect Rendering (12 TODOs)

- [ ] **4.1.1** ⏳ Define indirect draw arguments struct
  - [ ] vertex_count - vertices per draw
  - [ ] instance_count - instances per draw
  - [ ] vertex_start - starting vertex index
  - [ ] base_instance - starting instance index
  - [ ] Match MTLIndirectRenderCommand layout

- [ ] **4.1.2** ⏳ Implement indirect_draw_buffer_t
  - [ ] CPU-side argument array (mtl_indirect_draw_indexed_t*)
  - [ ] GPU buffer for arguments
  - [ ] GPU counter buffer (for dynamic draw counts)
  - [ ] Metadata (count, capacity, flags)

- [ ] **4.1.3** ⏳ Implement indirect_draw_buffer_create()
  - [ ] Allocate CPU array
  - [ ] Create GPU buffers (arguments + counter)
  - [ ] Initialize counters
  - [ ] Set up double buffering

- [ ] **4.1.4** ⏳ Implement indirect_draw_buffer_destroy()
  - [ ] Free CPU and GPU resources
  - [ ] Cleanup synchronization

- [ ] **4.1.5** ⏳ Implement indirect_draw_buffer_reset()
  - [ ] Clear all arguments
  - [ ] Reset counter to 0
  - [ ] Prepare for new frame

- [ ] **4.1.6** ⏳ Implement indirect_draw_buffer_add()
  - [ ] Add single draw argument
  - [ ] Bounds checking
  - [ ] Error handling for overflow

- [ ] **4.1.7** ⏳ Implement indirect_draw_buffer_add_batch()
  - [ ] Add multiple arguments efficiently
  - [ ] Batch copy
  - [ ] Less overhead than individual adds

- [ ] **4.1.8** ⏳ Implement indirect_draw_buffer_flush_gpu()
  - [ ] Copy arguments to GPU buffer
  - [ ] Handle frame pipelining
  - [ ] Manage GPU counter
  - [ ] Synchronization

- [ ] **4.1.9** ⏳ Implement multi-draw dispatch
  - [ ] `indirect_draw_buffer_dispatch()` - execute all indirect draws
  - [ ] Use Metal's indirect rendering API
  - [ ] Support dynamic draw counts from GPU counter
  - [ ] Proper command encoding

- [ ] **4.1.10** ⏳ Add GPU counter support
  - [ ] Read counter value from GPU
  - [ ] CPU-GPU synchronization points
  - [ ] Async readback option
  - [ ] Prevent stalls

- [ ] **4.1.11** ⏳ Add statistics & debugging
  - [ ] Track draw call count
  - [ ] Track instance count
  - [ ] Memory usage
  - [ ] Print draw arguments for inspection

- [ ] **4.1.12** ⏳ Add error handling
  - [ ] Validate argument values
  - [ ] Detect buffer overflows
  - [ ] Provide meaningful error messages
  - [ ] Graceful degradation

**Subtotal Phase 4.1: 12 TODOs**

---

### 4.2 Compute-Based Culling (12 TODOs)

- [ ] **4.2.1** ⏳ Write frustum culling compute kernel
  - [ ] Load instance transforms and bounds
  - [ ] Load camera frustum planes
  - [ ] Test bounds against frustum
  - [ ] Output visibility (0 or 1)
  - [ ] Optimize for GPU parallelism

- [ ] **4.2.2** ⏳ Implement sphere/AABB bounds checking
  - [ ] Per-instance bounding sphere
  - [ ] Frustum-sphere intersection test
  - [ ] AABB alternative option
  - [ ] Early rejection optimization
  - [ ] Vectorize operations for speed

- [ ] **4.2.3** ⏳ Add LOD selection logic
  - [ ] Distance-based LOD calculation
  - [ ] Set LOD index per instance (0-3)
  - [ ] Output to results buffer
  - [ ] Support variable LOD count

- [ ] **4.2.4** ⏳ Implement occlusion culling basics
  - [ ] HZB texture sampling (optional advanced feature)
  - [ ] Conservative depth testing
  - [ ] Fallback to frustum-only culling
  - [ ] Document accuracy vs performance

- [ ] **4.2.5** ⏳ Optimize work distribution
  - [ ] Grid-based spatial partitioning
  - [ ] Load balancing across threadgroups
  - [ ] Cache coherence optimization
  - [ ] Profile and tune threadgroup size

- [ ] **4.2.6** ⏳ Create culling output structure
  - [ ] Results buffer (visibility flags)
  - [ ] Counter buffer (visible count)
  - [ ] Metadata and statistics
  - [ ] Support multiple culling methods

- [ ] **4.2.7** ⏳ Implement culling dispatch
  - [ ] Calculate optimal threadgroup count
  - [ ] Dispatch compute job to GPU
  - [ ] Synchronize results
  - [ ] Handle GPU errors

- [ ] **4.2.8** ⏳ Add frustum update each frame
  - [ ] Update frustum planes from camera
  - [ ] Efficient matrix transformation
  - [ ] Double-buffering for frame pipelining
  - [ ] Validation and debugging

- [ ] **4.2.9** ⏳ Add statistics tracking
  - [ ] Count visible instances
  - [ ] Count culled instances
  - [ ] Culling efficiency ratio
  - [ ] GPU timing per pass

- [ ] **4.2.10** ⏳ Implement error handling
  - [ ] Bounds validation
  - [ ] Counter overflow detection
  - [ ] Kernel error detection
  - [ ] Error recovery/fallback

- [ ] **4.2.11** ⏳ Add debugging visualization
  - [ ] Render frustum as wireframe
  - [ ] Visualize culled instances (different color)
  - [ ] Heatmap of work distribution
  - [ ] Performance overlay

- [ ] **4.2.12** ⏳ Benchmarking & profiling
  - [ ] Measure culling time
  - [ ] Scalability testing (100K, 1M instances)
  - [ ] Compare to CPU culling
  - [ ] Memory bandwidth analysis

**Subtotal Phase 4.2: 12 TODOs**

---

### 4.3 Compaction & Sorting (11 TODOs)

- [ ] **4.3.1** ⏳ Implement parallel compaction algorithm
  - [ ] Scan-based (prefix sum) algorithm
  - [ ] Work-efficient GPU algorithm
  - [ ] Optimal for large instance counts
  - [ ] References: GPU Gems 3 & 4

- [ ] **4.3.2** ⏳ Write compaction compute kernel
  - [ ] Load culling results (visibility flags)
  - [ ] Compact visible instances to front of list
  - [ ] Generate indirect draw arguments from compacted list
  - [ ] Update counter with visible count

- [ ] **4.3.3** ⏳ Add material-based sorting
  - [ ] Primary sort key: material ID
  - [ ] Secondary sort key: instance ID
  - [ ] Radix sort or GPU-based quicksort
  - [ ] Minimize state changes during rendering

- [ ] **4.3.4** ⏳ Write material sorting kernel
  - [ ] Implement radix sort key extraction
  - [ ] Implement radix sort passes
  - [ ] Handle variable material count
  - [ ] Optimize for GPU cache

- [ ] **4.3.5** ⏳ Implement load balancing
  - [ ] Detect material imbalance
  - [ ] Redistribute work evenly across materials
  - [ ] Minimize empty groups
  - [ ] Improve cache efficiency

- [ ] **4.3.6** ⏳ Optimize for cache coherence
  - [ ] Sort by spatial locality when possible
  - [ ] Improve memory access patterns
  - [ ] Prefetching optimization
  - [ ] Profile memory bandwidth improvement

- [ ] **4.3.7** ⏳ Add compaction result structure
  - [ ] Compacted instance list
  - [ ] Indirect draw arguments (1 per material)
  - [ ] Material ranges and metadata
  - [ ] Statistics and profiling data

- [ ] **4.3.8** ⏳ Implement compaction dispatch
  - [ ] Calculate dispatch size from instance count
  - [ ] Handle dynamic counts from GPU
  - [ ] Synchronize results
  - [ ] Error handling

- [ ] **4.3.9** ⏳ Add statistics collection
  - [ ] Compaction efficiency (how well compacted)
  - [ ] Material distribution analysis
  - [ ] Work distribution statistics
  - [ ] Performance metrics

- [ ] **4.3.10** ⏳ Add performance profiling
  - [ ] Per-stage timing (scan, sort, compact)
  - [ ] Bottleneck identification
  - [ ] Optimization recommendations
  - [ ] Detailed timing reports

- [ ] **4.3.11** ⏳ Add validation
  - [ ] Verify no duplicate instances in compacted list
  - [ ] Verify all visible instances present
  - [ ] Verify correct sorting order
  - [ ] Bounds checking on arrays

**Subtotal Phase 4.3: 11 TODOs**

---

### 4.4 Integration & Pipeline (10 TODOs)

- [ ] **4.4.1** ⏳ Create dispatch size calculator
  - [ ] Calculate threadgroup count from instance count
  - [ ] Handle edge cases (1 instance, etc)
  - [ ] Occupancy optimization
  - [ ] Respect device limits

- [ ] **4.4.2** ⏳ Implement GPU counter reading
  - [ ] Async readback from GPU counter
  - [ ] GPU-CPU synchronization
  - [ ] Fallback to fixed counts if unavailable
  - [ ] Minimize stalls

- [ ] **4.4.3** ⏳ Add CPU-GPU sync points
  - [ ] Proper command buffer flushing
  - [ ] Correct encoding order (cull → compact → render)
  - [ ] Double-buffering for frame pipelining
  - [ ] Maintain 60+ FPS smoothness

- [ ] **4.4.4** ⏳ Implement error handling
  - [ ] Counter overflow detection
  - [ ] Buffer overflow checks
  - [ ] Shader compilation errors
  - [ ] Graceful fallback to CPU rendering

- [ ] **4.4.5** ⏳ Add validation layer
  - [ ] Verify dispatch sizes
  - [ ] Check buffer sizes sufficient
  - [ ] Detect shader errors (Metal validation)
  - [ ] Memory coherency checks

- [ ] **4.4.6** ⏳ Add debugging/visualization
  - [ ] HUD display of real-time statistics
  - [ ] Visual batch visualization
  - [ ] Frustum and culling visualization
  - [ ] Error highlighting and messages

- [ ] **4.4.7** ⏳ Add performance metrics
  - [ ] Per-stage GPU timing (profiler integration)
  - [ ] Memory bandwidth measurement
  - [ ] GPU occupancy calculation
  - [ ] Frame-to-frame consistency

- [ ] **4.4.8** ⏳ Integrate with render loop
  - [ ] Update pipeline stage ordering
  - [ ] Proper command buffer encoding sequence
  - [ ] Frame pipelining support (N-frame buffering)
  - [ ] Synchronization with other render passes

- [ ] **4.4.9** ⏳ Add fallback paths
  - [ ] CPU-based culling as fallback
  - [ ] Disable indirect rendering on older hardware
  - [ ] Performance-safe mode (conservative)
  - [ ] Graceful feature degradation

- [ ] **4.4.10** ⏳ Create example usage & documentation
  - [ ] Simple test scene with 100K instances
  - [ ] Configurable parameters (frustum, LOD, etc)
  - [ ] Visualization tools and debug overlays
  - [ ] Code examples and usage guide

**Subtotal Phase 4.4: 10 TODOs**

---

**PHASE 4 TOTAL: 35 TODOs**

---

## PHASE 5: PERFORMANCE POLISH (20 TODOs)

### 5.1 Memory Optimization (8 TODOs)

- [ ] **5.1.1** ⏳ Analyze vertex attribute layout
  - [ ] Measure cache misses (theoretical)
  - [ ] Identify memory bandwidth hotspots
  - [ ] Generate layout recommendations
  - [ ] Provide optimization priorities

- [ ] **5.1.2** ⏳ Implement SoA (Structure-of-Arrays) variant
  - [ ] Create separate arrays per attribute
  - [ ] Compare performance to AoS (Array-of-Structures)
  - [ ] Benchmark memory bandwidth improvement
  - [ ] Document tradeoffs (cache coherence vs bandwidth)

- [ ] **5.1.3** ⏳ Optimize instance data layout
  - [ ] Analyze instance_data_t field ordering
  - [ ] Reorder for cache efficiency
  - [ ] Align to cache line boundaries (64-byte)
  - [ ] SIMD-friendly packing
  - [ ] Reduce false sharing

- [ ] **5.1.4** ⏳ Add memory prefetching hints
  - [ ] Hint to hardware prefetcher
  - [ ] Optimize stride patterns for prefetching
  - [ ] Test improvement with profiler
  - [ ] Document effectiveness

- [ ] **5.1.5** ⏳ Implement cache locality analysis
  - [ ] Measure spatial coherence
  - [ ] Identify hot access patterns
  - [ ] Generate heatmaps
  - [ ] Suggest memory reorganization

- [ ] **5.1.6** ⏳ Add memory profiling
  - [ ] Track bandwidth usage (bytes per frame)
  - [ ] Identify bottleneck components
  - [ ] Compare formats and layouts
  - [ ] Generate memory analysis reports

- [ ] **5.1.7** ⏳ Optimize GPU memory layout
  - [ ] Align buffers to 256-byte boundaries (for alignment requirements)
  - [ ] Coalesce small allocations
  - [ ] Reduce memory fragmentation
  - [ ] Profile improvements

- [ ] **5.1.8** ⏳ Benchmark memory improvements
  - [ ] Measure memory bandwidth before/after optimization
  - [ ] Compare FPS improvement
  - [ ] Profile individual components (culling, sorting, rendering)
  - [ ] Document findings

**Subtotal Phase 5.1: 8 TODOs**

---

### 5.2 State Change Minimization (6 TODOs)

- [ ] **5.2.1** ⏳ Implement state change tracking
  - [ ] Track pipeline state changes
  - [ ] Monitor buffer bindings
  - [ ] Count texture/sampler changes
  - [ ] Measure cost of each state change type

- [ ] **5.2.2** ⏳ Create command reordering algorithm
  - [ ] Group commands by material (same pipeline)
  - [ ] Group by vertex format
  - [ ] Minimize state changes
  - [ ] Maintain correctness (depth ordering, etc)

- [ ] **5.2.3** ⏳ Implement batch optimization
  - [ ] Merge compatible batches
  - [ ] Reorder batches for state coherence
  - [ ] Reduce draw call count
  - [ ] Profile state change savings

- [ ] **5.2.4** ⏳ Add state caching
  - [ ] Cache last-bound state (pipeline, buffers, etc)
  - [ ] Skip redundant state bindings
  - [ ] Avoid unnecessary Metal calls
  - [ ] Measure overhead savings

- [ ] **5.2.5** ⏳ Generate optimization reports
  - [ ] Show current state change count
  - [ ] Recommend reordering opportunities
  - [ ] Visualize batch groupings
  - [ ] Export analysis data

- [ ] **5.2.6** ⏳ Benchmark improvements
  - [ ] Measure draw call overhead (CPU time per draw)
  - [ ] Compare before/after optimization
  - [ ] Profile state change cost
  - [ ] FPS improvement measurement

**Subtotal Phase 5.2: 6 TODOs**

---

### 5.3 GPU Utilization (4 TODOs)

- [ ] **5.3.1** ⏳ Implement occupancy calculator
  - [ ] Calculate waves per threadgroup
  - [ ] Measure available parallelism
  - [ ] Suggest better threadgroup sizes
  - [ ] Compare to theoretical maximum

- [ ] **5.3.2** ⏳ Add bandwidth measurement
  - [ ] Estimate theoretical max bandwidth
  - [ ] Measure actual bandwidth achieved
  - [ ] Identify bandwidth-limited stages
  - [ ] Generate bandwidth reports

- [ ] **5.3.3** ⏳ Implement latency hiding analysis
  - [ ] Calculate wave occupancy
  - [ ] Identify pipeline stalls
  - [ ] Suggest ways to hide latency
  - [ ] Recommend kernel changes

- [ ] **5.3.4** ⏳ Add performance tuning
  - [ ] Optimize threadgroup size
  - [ ] Tune work distribution
  - [ ] Profile improvements with profiler
  - [ ] Document optimal settings per GPU

**Subtotal Phase 5.3: 4 TODOs**

---

### 5.4 Profiling & Diagnostics (2 TODOs)

- [ ] **5.4.1** ⏳ Implement GPU timing infrastructure
  - [ ] Per-stage GPU timing (cull, sort, render)
  - [ ] Pipeline stall detection
  - [ ] Memory stall detection
  - [ ] Generate detailed timing timeline

- [ ] **5.4.2** ⏳ Create profiling dashboard
  - [ ] Real-time statistics display
  - [ ] Frame-by-frame graph view
  - [ ] Performance alerts (FPS drops, etc)
  - [ ] Export profiling data

**Subtotal Phase 5.4: 2 TODOs**

---

**PHASE 5 TOTAL: 20 TODOs**

---

## SUMMARY

| Phase | Name | TODOs | Status |
|-------|------|-------|--------|
| 1 | Vertex Format Foundation | 15 | ⏳ Not Started |
| 2 | Compression & Optimization | 40 | ⏳ Not Started |
| 3 | Instancing Foundation | 25 | ⏳ Not Started |
| 4 | GPU-Driven Rendering | 35 | ⏳ Not Started |
| 5 | Performance Polish | 20 | ⏳ Not Started |
| **TOTAL** | | **135** | |

---

## Usage Instructions

### How to Use This Checklist

1. **Print or bookmark this file** for reference during implementation
2. **Check off items as you complete them** (change `[ ]` to `[x]`)
3. **Use phase breakdowns** to organize work
4. **Cross-reference with IMPLEMENTATION_ROADMAP.md** for detailed specifications
5. **Update progress regularly** to track completion

### Tracking Progress

- **Phase 1: Week 1-2** - Foundation work
- **Phase 2: Week 3-5** - Compression and advanced formats
- **Phase 3: Week 6-7** - Instancing system
- **Phase 4: Week 8-10** - GPU-driven rendering
- **Phase 5: Week 11-12** - Optimization and polish

### Completion Criteria

Mark a task as complete (✅) only when:
- Code is written and compiles
- Tests pass
- No memory leaks
- Performance validated
- Documentation updated

---

## Notes

- **Total Tasks:** ~135 detailed TODOs
- **Estimated Time:** 8-12 weeks for one developer
- **Priority:** Phase 1 is critical; later phases depend on it
- **Testing:** Should be done continuously, not deferred
- **Documentation:** Update as you go, not at the end

---

**Last Updated:** 2026-01-07
**Status:** Ready to begin Phase 1
