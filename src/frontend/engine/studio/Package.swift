// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "VoxelForgeStudio",
    platforms: [.macOS(.v13)],
    products: [
        .executable(name: "VoxelForgeStudio", targets: ["VoxelForgeStudio"])
    ],
    dependencies: [],
    targets: [
        .target(
            name: "CVoxelForge",
            path: "CVoxelForge",
            sources: [
                "swift_bridge.c",
                "occlusion_vfx_stubs.c",
                "world_editor_stubs.c",
                "graphics_debug_stubs.c",
                "rendering_tools_stubs.c",
                "visualization_stubs.c"
            ],
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include")
            ]
        ),
        .executableTarget(
            name: "VoxelForgeStudio",
            dependencies: ["CVoxelForge"],
            path: ".",
            exclude: ["Tests", ".build", "Resources", "Editors", "build_output.txt", "CVoxelForge"],
            sources: ["App.swift", "ContentView.swift", "ProjectExplorerView.swift", "DocumentationView.swift", "Sources/"],
            linkerSettings: [
                .unsafeFlags([
                    "-LCVoxelForge", 
                    "-L../../../../build/lib", // Path to CMake build libs
                    "-L/opt/homebrew/lib",
                    "-L/usr/local/lib",
                    "-lEngine", 
                    "-lCore",
                    "-lRenderer",
                    "-lPhysics",
                    "-lAudio",
                    "-lScripting",
                    "-lglfw", 
                    "-lz",
                    "-framework", "Cocoa",
                    "-framework", "IOKit",
                    "-framework", "CoreVideo",
                    "-framework", "OpenGL"
                ])
            ]
        ),
        .testTarget(name: "VoxelForgeStudioTests", dependencies: ["VoxelForgeStudio"], path: "Tests", sources: ["CoreSystemsTests.swift"])
    ]
)
