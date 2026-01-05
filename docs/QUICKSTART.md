# Quickstart Guide

Get up and running with Minecraft V2 engine in under 15 minutes.

## Prerequisites

### macOS
```bash
brew install cmake glfw
```

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install cmake libglfw3-dev build-essential
```

### Windows
```powershell
# Install vcpkg first: https://vcpkg.io/
vcpkg install glfw3
```

## Building

### Step 1: Clone Repository
```bash
cd ~/Desktop  # or your preferred location
# git clone <repo-url>  # When available
cd "Minecraft v2"
```

### Step 2: Configure with CMake
```bash
mkdir -p build
cd build
cmake ..
```

**Expected output:**
```
-- GLFW3 found and will be linked
-- Demo applications configured: demo_sandbox, demo_terrain
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

### Step 3: Build
```bash
cmake --build .
```

**Expected output:**
```
[100%] Built target demo_sandbox
[100%] Built target demo_terrain
```

### Step 4: Run Demos
```bash
# Physics sandbox with falling boxes and spheres
./demo_sandbox

# Procedural terrain with flythrough camera
./demo_terrain
```

## Troubleshooting

### GLFW Not Found
**macOS:**
```bash
# Ensure Homebrew is updated
brew update
brew install glfw

# If still issues, try explicit path
cmake .. -DGLFW_LIBRARY=/opt/homebrew/lib/libglfw.dylib
```

**Linux:**
```bash
# Check package installation
dpkg -l | grep glfw
sudo apt install libglfw3-dev
```

### Compilation Errors
```bash
# Clean rebuild
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --verbose
```

### Runtime Issues

**Black screen / No window:**
- Check GPU drivers are up to date
- Verify OpenGL 3.3+ support: `glxinfo | grep "OpenGL version"` (Linux)

**Demo crashes on macOS:**
```bash
# Run with error logging
./demo_sandbox 2>&1 | tee demo.log
```

## Next Steps

1. **Explore Physics API**: See [docs/PHYSICS_API.md](PHYSICS_API.md)
2. **Modify Demos**: Edit `src/game/demo_*.c` files
3. **Add Custom Code**: Create new files in `src/game/`

## Build Options

### Debug vs Release
```bash
# Debug (default) - includes symbols, slower
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release - optimized, ~3x faster
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Disable Demos
```bash
cmake .. -DBUILD_DEMOS=OFF
```

### Enable Tests
```bash
cmake .. -DBUILD_TESTS=ON
```

## IDE Integration

### VS Code
1. Install "C/C++" extension
2. CMake will generate `compile_commands.json` automatically
3. Open folder in VS Code

### CLion
1. Open folder
2. CLion detects CMakeLists.txt automatically
3. Select build target from dropdown

### Xcode (macOS)
```bash
cmake .. -G Xcode
open MinecraftV2.xcodeproj
```

## Common Tasks

### Rebuild After Code Changes
```bash
cd build
cmake --build .  # Rebuilds only changed files
```

### Clean Build
```bash
rm -rf build
mkdir build && cd build && cmake .. && cmake --build .
```

### Run Specific Demo
```bash
cmake --build . --target demo_terrain
./demo_terrain
```

### Update Dependencies
```bash
# macOS
brew upgrade cmake glfw

# Linux
sudo apt update && sudo apt upgrade
```

## Performance Tips

1. **Use Release builds for testing physics**: `cmake .. -DCMAKE_BUILD_TYPE=Release`
2. **Check FPS**: Look for frame time logs in terminal
3. **Profile bottlenecks**: Use `Instruments` on macOS or `perf` on Linux

## Getting Help

- Check existing issues in repository
- Review [Physics API docs](PHYSICS_API.md)
- Examine demo source code in `src/game/`

**Estimated time to first demo run: 5-10 minutes** ✅
