# Minecraft V2 - Game Engine

A modern, performance-focused 3D game engine built in C, designed for voxel-based games with advanced physics simulation and rendering capabilities.

## Features

### Implemented 
- **Rigid Body Physics**: Collision detection and resolution (Box-Box, Sphere-Sphere, Sphere-Box)
- **Demos**:
  - Physics Sandbox: Interactive physics simulation with falling objects
  - Simple Terrain: Procedural terrain generation with Perlin noise
- **CMake Build System**: Cross-platform build configuration with Debug/Release presets
- **Modular Architecture**: Clean separation between core engine and game code

### In Development 
- Advanced rendering pipeline
- Procedural terrain generation
- Networking and multiplayer

## Quick Start

### Requirements
- **Platform**: macOS (Apple Silicon or Intel), Linux, Windows
- **Compiler**: GCC 9+ or Clang 11+
- **CMake**: 3.20 or higher
- **Dependencies**: GLFW 3.3+

### macOS Installation (Homebrew)
```bash
# Install dependencies
brew install cmake glfw

# Clone repository
git clone <repository-url>
cd "Minecraft v2"

# Build with CMake
mkdir -p build && cd build
cmake ..
cmake --build .

# Run demos
./demo_sandbox    # Physics sandbox
./demo_terrain    # Terrain demo
```

### Linux Installation
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install cmake libglfw3-dev

# Build
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Windows Installation
```powershell
# Install via vcpkg
vcpkg install glfw3

# Build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Project Structure

```
Minecraft v2/
├── src/
│   └── engine/                    # Core engine code
│       ├── core/                  # Window, logging, memory management
│       ├── physics/               # Physics simulation
│       ├── rendering/             # Rendering systems
│       ├── animation/             # Animation systems
│       ├── audio/                 # Audio systems
│       ├── ai/                    # AI and NPC systems
│       ├── assets/                # Asset management
│       ├── network/               # Networking systems
│       ├── gameplay/              # Gameplay systems
│       ├── character/             # Character systems
│       ├── geometry/              # Geometry and mesh processing
│       ├── materials/             # Materials and shaders
│       ├── lighting/              # Lighting and global illumination
│       ├── effects/               # Visual effects and particles
│       ├── editor/                # Editor tools
│       ├── debug/                 # Debug systems
│       ├── include/               # Public API headers
│       └── backend/               # Rendering backends (Metal/Vulkan)
│   └── game/                      # Game-specific code + demos
├── tests/                         # Test suites
│   ├── unit/                      # Unit tests
│   ├── integration/               # Integration tests
│   ├── performance/               # Performance tests
│   └── fixtures/                  # Test fixtures and data
├── build/                         # CMake build directory
├── CMakeLists.txt                 # Build configuration
├── README.md                       # This file
├── master/                        # Master folder
│   ├── todo.csv                   # Task tracking
│   ├── todo.md                    # Task notes
│   ├── master.md                  # Workflow rules
│   └── expectations.md            # Quality standards
└── docs/                          # Documentation
```

## Documentation

- **[Quickstart Guide](docs/QUICKSTART.md)**: Build instructions and first steps
- **[Physics API](docs/PHYSICS_API.md)**: Physics system documentation
- **[Architecture](docs/ARCHITECTURE.md)**: Engine design overview *(coming soon)*

## Demo Controls

### Physics Sandbox (`demo_sandbox`)
- **SPACE**: Spawn falling sphere
- **ESC**: Quit

### Terrain Demo (`demo_terrain`)
- **WASD**: Move camera
- **Mouse**: Look around
- **SPACE**: Fly up
- **SHIFT**: Fly down
- **ESC**: Quit

## Testing

### Test Organization
Tests are organized by type in the `tests/` directory:

- **`tests/unit/`** - Unit tests for individual components
  - Physics unit tests
  - Material system tests
  - Crafting system tests
  - Framework tests
  - Particle system tests

- **`tests/integration/`** - Integration tests for system interactions
  - Physics integration tests
  - Gameplay system tests
  - Frame graph tests
  - Networking tests
  - Player/food system tests

- **`tests/performance/`** - Performance and stress tests
  - Instance data tests
  - GBuffer resize tests
  - Framebuffer tests

- **`tests/fixtures/`** - Test data and fixtures

### Running Tests
```bash
# Build all tests
cmake --build build --target all

# Run specific test
./build/unit_tests
./build/integration_tests
./build/performance_tests

# Run with coverage
cmake --build build --target coverage
```

## Development

### Build Targets
```bash
# Build everything
cmake --build build

# Build specific demo
cmake --build build --target demo_sandbox
cmake --build build --target demo_terrain

# Clean build
rm -rf build && mkdir build && cd build && cmake ..
```

### Build Types
```bash
# Debug build (default)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

## Contributing

This is an educational project focused on learning game engine architecture. Contributions are welcome but please note this is primarily a learning exercise.

## License

*License information pending*

## Roadmap

### Phase 1: Foundation 
- [x] Clean architecture
- [x] Rigid body physics
- [x] Demo applications

### Phase 2: Professional Polish (Current)
- [x] CMake build system
- [x] Core documentation
- [ ] Editor workflow

### Phase 3: Differentiation
- [ ] Advanced terrain features
- [ ] Performance benchmarks
- [ ] Community alpha release

## Credits

Built as a learning project to understand game engine architecture, physics simulation, and graphics programming.
