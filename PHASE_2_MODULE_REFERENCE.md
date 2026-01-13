# Phase 2 - Module Reference & Integration Guide

## Quick Module Locations

### Export System
```
src/engine/assets/io/export/
├── model_exporter.h              (Header - 243 lines)
├── model_exporter.c              (Implementation - 632 lines)
├── asset_export_manager.c        (Existing - enhanced with docs)
├── asset_export_processor.c      (Existing - enhanced with docs)
└── ... (other existing export modules)
```

### Compression System
```
src/engine/assets/io/compression/
├── compression_wrapper.h         (Header - 104 lines)
├── compression_wrapper.c         (Implementation - 530 lines)
└── (Integrates with LZ4/ZSTD if available)
```

### Networking System
```
src/engine/network/
├── connection_manager.h          (Header - 166 lines)
├── connection_manager.c          (Implementation - 548 lines)
├── state_sync.h                  (Header - 229 lines)
└── state_sync.c                  (Implementation - 664 lines)
```

---

## Module Integration Map

### 1. Asset Export Pipeline

**Purpose**: Export 3D models in multiple formats with material data

**Dependencies**:
- Standard C library (stdlib, string, stdio, math)
- Time functions (time.h)

**Headers to Include**:
```c
#include "assets/io/export/model_exporter.h"
```

**Basic Usage**:
```c
// Create exporter
ModelExporter* exporter = model_exporter_create();

// Setup export options
ExportOptions options = {
    .format = MODEL_EXPORT_FORMAT_GLTF,
    .compression = MODEL_COMPRESSION_NONE,
    .embed_textures = true,
    .export_materials = true,
    .optimize_mesh = true,
    .scale_factor = 1.0f,
};

// Export mesh
int result = model_exporter_export_mesh(
    exporter,
    &mesh_data,
    "output/mesh.glb",
    &options
);

// Cleanup
model_exporter_destroy(exporter);
```

**Supported Formats**:
- glTF 2.0 (GLB binary) - `.glb`
- Autodesk FBX - `.fbx`
- Wavefront OBJ - `.obj`
- Stanford PLY - `.ply`

**Key Features**:
- ✅ Complete PBR material export
- ✅ Normal/tangent generation
- ✅ Animation keyframe export
- ✅ Skeletal animation support
- ✅ Progress callbacks
- ✅ Export statistics

---

### 2. Compression System

**Purpose**: Compress/decompress data with automatic algorithm selection

**Dependencies**:
- Standard C library (stdlib, string, stdio)
- LZ4 (optional, conditional compilation)
- ZSTD (optional, conditional compilation)

**Headers to Include**:
```c
#include "assets/io/compression/compression_wrapper.h"
```

**Basic Usage**:
```c
// Option 1: Compress data buffer
CompressionOptions options = {
    .algorithm = COMPRESSION_ALGORITHM_AUTO,
    .level = COMPRESSION_LEVEL_DEFAULT,
};

void* compressed_data = NULL;
size_t compressed_size = 0;

int result = compression_compress(
    input_data,
    input_size,
    &compressed_data,
    &compressed_size,
    &options
);

// Option 2: Compress file to file
compression_compress_file("input.bin", "output.bin", &options);

// Option 3: Decompress (auto-detects format)
void* decompressed_data = NULL;
size_t decompressed_size = 0;

compression_decompress(
    compressed_data,
    compressed_size,
    &decompressed_data,
    &decompressed_size
);

// Cleanup
free(compressed_data);
free(decompressed_data);
```

**Supported Algorithms**:
- **LZ4**: Fast compression (~400 MB/s)
- **ZSTD**: Better compression ratio
- **Auto**: Selects based on data size

**Compression Levels**:
- `COMPRESSION_LEVEL_FAST` (1) - Fastest, lowest ratio
- `COMPRESSION_LEVEL_DEFAULT` (3) - Balanced
- `COMPRESSION_LEVEL_HIGH` (9) - Slower, better ratio
- `COMPRESSION_LEVEL_MAXIMUM` (22) - Slowest, best ratio

**Key Features**:
- ✅ Auto-detection on decompression
- ✅ File-to-file operations
- ✅ Size estimation
- ✅ Compression statistics
- ✅ Thread-safe

---

### 3. Networking System

**Purpose**: Establish and maintain network connections with state synchronization

**Dependencies**:
- Standard C library (stdlib, string, stdio, math)
- POSIX sockets (sys/socket.h, netinet/in.h, arpa/inet.h)
- Threading (pthread.h)
- Time functions (time.h)
- File control (fcntl.h)
- Errno handling (errno.h)

**Headers to Include**:
```c
#include "network/connection_manager.h"
#include "network/state_sync.h"
```

#### A. Connection Manager

**Basic Usage - Client**:
```c
// Create connection manager
ConnectionManager* client = connection_manager_connect(32);

// Connect to server
ConnectionID conn = connection_manager_connect(
    client,
    "192.168.1.100",  // Server IP
    7777,              // Server port
    5000               // Timeout in ms
);

// Setup callbacks
connection_manager_set_packet_received_callback(
    client,
    on_packet_received,
    user_data
);

// Main loop
while (game_running) {
    // Update connections (check timeouts, receive packets)
    connection_manager_update(client, delta_time);

    // Receive packets
    Packet* packets = NULL;
    uint32_t packet_count = 0;
    connection_manager_receive_packets(client, conn, &packets, &packet_count);

    // Process packets...

    // Free packets
    connection_manager_free_packets(packets, packet_count);
}

// Cleanup
connection_manager_disconnect(client, conn);
connection_manager_destroy(client);
```

**Basic Usage - Server**:
```c
// Create server
ConnectionManager* server = connection_manager_create(32);
connection_manager_create_server(server, "0.0.0.0", 7777);

// Accept connections and process packets (same as client)
```

**Packet Structure**:
```c
Packet packet = {
    .data = buffer,
    .size = buffer_size,
    .type = PACKET_TYPE_MOVEMENT,
    .delivery_type = PACKET_DELIVERY_UNRELIABLE,
    .sequence_number = 0,
};

connection_manager_send_packet(client, conn, &packet);
```

**Key Features**:
- ✅ UDP-based transport
- ✅ Connection state management
- ✅ Packet queuing (reliable & unreliable)
- ✅ Sequence numbering
- ✅ Timeout handling
- ✅ Statistics (bandwidth, latency)
- ✅ Server support

#### B. State Synchronization

**Basic Usage**:
```c
// Create synchronizer
StateSynchronizer* sync = state_sync_create(1024);

// Register entities
state_sync_register_entity(sync, player_entity_id);
state_sync_register_entity(sync, enemy_entity_id);

// Update local entity states
Vec3 position = {10.0f, 5.0f, 0.0f};
state_sync_update_position(sync, player_entity_id, &position);

Vec3 velocity = {1.0f, 0.0f, 0.0f};
state_sync_update_velocity(sync, player_entity_id, &velocity);

// Create snapshot for transmission
StateSnapshot* snapshot = NULL;
state_sync_create_snapshot(
    sync,
    &snapshot,
    STATE_CHANGE_POSITION | STATE_CHANGE_VELOCITY
);

// Compress and send snapshot via connection_manager
DeltaCompressed* compressed = NULL;
state_sync_delta_compress_snapshot(sync, snapshot, &compressed);

Packet pkt = {
    .data = compressed->data,
    .size = compressed->size,
    .type = PACKET_TYPE_STATE_UPDATE,
    .delivery_type = PACKET_DELIVERY_UNRELIABLE,
};

connection_manager_send_packet(manager, conn, &pkt);

// On remote side: receive and apply state
StateSnapshot* remote_snapshot = NULL;
state_sync_delta_decompress(sync, received_compressed, &remote_snapshot);
state_sync_apply_snapshot(sync, remote_snapshot);

// Get interpolated state for rendering
InterpolatedState interp_state = {0};
state_sync_get_interpolated_state(sync, remote_entity_id, 0.5f, &interp_state);

// Use interp_state.position for rendering

// Cleanup
state_sync_free_snapshot(snapshot);
state_sync_free_delta_compressed(compressed);
state_sync_destroy(sync);
```

**Key Features**:
- ✅ Entity registration/unregistration
- ✅ Multi-property updates (position, rotation, velocity, animation)
- ✅ Snapshot creation and batching
- ✅ Delta compression for bandwidth savings
- ✅ Interpolation for smooth movement
- ✅ Prediction for extrapolation
- ✅ Custom property support
- ✅ Update rate control

---

## Complete Integration Example

```c
#include "assets/io/export/model_exporter.h"
#include "assets/io/compression/compression_wrapper.h"
#include "network/connection_manager.h"
#include "network/state_sync.h"

// 1. Export a model
void export_game_assets() {
    ModelExporter* exporter = model_exporter_create();

    ExportOptions opts = {
        .format = MODEL_EXPORT_FORMAT_GLTF,
        .export_materials = true,
    };

    model_exporter_export_mesh(exporter, &player_mesh, "player.glb", &opts);
    model_exporter_destroy(exporter);
}

// 2. Setup networking
void init_networking() {
    // Create managers
    connection_manager = connection_manager_create(32);
    state_sync = state_sync_create(1024);

    // Register game entities
    state_sync_register_entity(state_sync, local_player_id);
    state_sync_register_entity(state_sync, remote_player_id);
}

// 3. Send player state each frame
void update_and_broadcast() {
    // Update local player state
    state_sync_update_position(state_sync, local_player_id, &player_pos);
    state_sync_update_velocity(state_sync, local_player_id, &player_vel);

    // Create and send snapshot
    StateSnapshot* snapshot = NULL;
    state_sync_create_snapshot(
        state_sync,
        &snapshot,
        STATE_CHANGE_POSITION | STATE_CHANGE_VELOCITY
    );

    // Compress for transmission
    DeltaCompressed* compressed = NULL;
    state_sync_delta_compress_snapshot(state_sync, snapshot, &compressed);

    // Send to server
    Packet pkt = {
        .data = compressed->data,
        .size = compressed->size,
        .type = PACKET_STATE_UPDATE,
        .delivery_type = PACKET_DELIVERY_UNRELIABLE,
    };

    connection_manager_send_packet(connection_manager, server_connection, &pkt);

    // Cleanup
    state_sync_free_snapshot(snapshot);
    state_sync_free_delta_compressed(compressed);
}

// 4. Receive and render remote entities
void receive_and_render() {
    connection_manager_update(connection_manager, delta_time);

    Packet* packets = NULL;
    uint32_t count = 0;

    connection_manager_receive_packets(
        connection_manager,
        server_connection,
        &packets,
        &count
    );

    for (uint32_t i = 0; i < count; i++) {
        if (packets[i].type == PACKET_STATE_UPDATE) {
            // Decompress
            DeltaCompressed comp = {
                .data = packets[i].data,
                .size = packets[i].size,
            };

            StateSnapshot* snapshot = NULL;
            state_sync_delta_decompress(state_sync, &comp, &snapshot);

            // Apply state
            state_sync_apply_snapshot(state_sync, snapshot);

            // Get interpolated state for rendering
            InterpolatedState interp = {0};
            state_sync_get_interpolated_state(
                state_sync,
                remote_player_id,
                0.5f,
                &interp
            );

            // Render remote player at interp.position
            render_player(remote_player_id, &interp.position);

            state_sync_free_snapshot(snapshot);
        }
    }

    connection_manager_free_packets(packets, count);
}
```

---

## Compilation Integration

### CMakeLists.txt Addition

```cmake
# Asset Export
add_library(ModelExporter
    src/engine/assets/io/export/model_exporter.c
)
target_include_directories(ModelExporter PUBLIC src)

# Compression (with optional LZ4/ZSTD)
add_library(CompressionWrapper
    src/engine/assets/io/compression/compression_wrapper.c
)
target_include_directories(CompressionWrapper PUBLIC src)

if(ENABLE_LZ4)
    target_link_libraries(CompressionWrapper PUBLIC ${LZ4_LIBRARY})
    target_compile_definitions(CompressionWrapper PRIVATE ENABLE_LZ4)
endif()

if(ENABLE_ZSTD)
    target_link_libraries(CompressionWrapper PUBLIC ${ZSTD_LIBRARY})
    target_compile_definitions(CompressionWrapper PRIVATE ENABLE_ZSTD)
endif()

# Networking
add_library(NetworkingSystem
    src/engine/network/connection_manager.c
    src/engine/network/state_sync.c
)
target_include_directories(NetworkingSystem PUBLIC src)
target_link_libraries(NetworkingSystem PUBLIC pthread)

# Link to main engine
target_link_libraries(Engine
    PUBLIC
    ModelExporter
    CompressionWrapper
    NetworkingSystem
)
```

---

## Testing Integration

### Unit Test Example

```c
#include <assert.h>
#include "assets/io/export/model_exporter.h"

void test_model_export() {
    // Create test mesh
    MeshData mesh = {
        .name = "TestMesh",
        .vertex_count = 3,
        .index_count = 3,
    };

    mesh.vertices = malloc(sizeof(VertexData) * 3);
    mesh.indices = malloc(sizeof(uint32_t) * 3);

    // Fill with triangle data...

    // Export
    ModelExporter* exporter = model_exporter_create();

    ExportOptions opts = {
        .format = MODEL_EXPORT_FORMAT_GLTF,
    };

    int result = model_exporter_export_mesh(exporter, &mesh, "/tmp/test.glb", &opts);
    assert(result == 0);

    // Verify file exists
    FILE* f = fopen("/tmp/test.glb", "rb");
    assert(f != NULL);
    fclose(f);

    model_exporter_destroy(exporter);
    free(mesh.vertices);
    free(mesh.indices);
}
```

---

## Performance Optimization Tips

### 1. Asset Export
- Use compression for large models
- Consider LOD generation for distant objects
- Pre-export assets offline

### 2. Compression
- LZ4 for network packets (speed priority)
- ZSTD for stored assets (size priority)
- Adjust compression level based on CPU budget

### 3. Networking
- Use UNRELIABLE delivery for position updates
- Use RELIABLE delivery for critical game state
- Batch state updates into single snapshots
- Enable interpolation for smooth remote movement

---

## Troubleshooting

### Export Issues
- **Empty export**: Validate mesh with `model_exporter_validate_mesh()`
- **Missing materials**: Ensure material data is populated
- **Format not supported**: Check `model_exporter_is_algorithm_available()`

### Compression Issues
- **Decompression fails**: Data may be corrupted, check source
- **Poor compression ratio**: Try different algorithm or compression level
- **High CPU usage**: Use LZ4 instead of ZSTD, or reduce compression level

### Networking Issues
- **Timeouts**: Increase timeout value or check network connectivity
- **Packets lost**: Use RELIABLE delivery mode for critical data
- **Jittery movement**: Enable interpolation or increase snapshot rate
- **High latency**: Add prediction or reduce update frequency

---

## License
All Phase 2 modules: MIT License (2024 Game Engine Project)
