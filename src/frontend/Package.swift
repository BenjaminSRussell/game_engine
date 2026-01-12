// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "VoxelForgeStudio",
    platforms: [.macOS(.v14)],
    products: [
        .executable(name: "VoxelForgeStudio", targets: ["VoxelForgeStudio"])
    ],
    dependencies: [],
    targets: [
        // Bridge for Editor Core components
        .target(
            name: "CEngineCore",
            path: "Core/Bridges",
            sources: ["engine_bridge.c"],
            publicHeadersPath: "include",
            cSettings: [
                .unsafeFlags([
                    "-I", "../../src/engine/include",
                    "-I", "../../src/engine/core",
                    "-I", "../../src/engine/renderer/core",
                    "-I", "../../src/engine/ecs",
                    "-I", "../../src/engine/ai"
                ])
            ]
        ),
        // Bridge for VoxelForge Engine components
        .target(
            name: "CVoxelForge",
            path: "CVoxelForge",
            sources: [
                "swift_bridge_clean.c",
                "occlusion_vfx_stubs.c",
                "world_editor_stubs.c",
                "graphics_debug_stubs.c",
                "rendering_tools_stubs.c",
                "visualization_stubs.c"
            ],
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("include"),
                .unsafeFlags([
                    "-I", "../../src/engine/include",
                    "-I", "../../src/engine",
                    "-I", "../../src/game/blockgame/include",
                    "-I", "../../src/engine/core",
                    "-I", "../../src/engine/renderer/core",
                    "-I", "../../src/engine/ecs",
                    "-I", "../../src/engine/ai"
                ])
            ]
        ),
        // Main Studio Executable
        .executableTarget(
            name: "VoxelForgeStudio",
            dependencies: ["CEngineCore", "CVoxelForge"],
            path: ".",
            exclude: [
                "Tests", 
                ".build", 
                "Backups",
                "CVoxelForge", 
                "Core/Bridges", 
                "Core/Tests",
                "Platform/Tests",
                "Editors",
                "build_output.txt",
                "Core/CommandManager.swift",
                "Core/DesignSystem.swift",
                "Core/Profiler.swift",
                "Core/SelectionManager.swift",
                "verify_connections.py",
                "build_frontend.sh",
                "Sources/UIComponents.swift"
            ],
            sources: [
                "App.swift", 
                "ContentView.swift", 
                "ProjectExplorerView.swift", 
                "DocumentationView.swift", 
                "Sources/",
                "Core/", 
                "UI/", 
                "Platform/"
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags([
                    "-I", "../../src/engine/include",
                    "-I", "../../src/engine",
                    "-I", "../../src/game/blockgame/include"
                ])
            ],
            linkerSettings: [
                .unsafeFlags([
                    "-LCVoxelForge", 
                    "-L../../build/lib", 
                    "-L../../build/",
                    "-L/opt/homebrew/lib",
                    "-L/usr/local/lib",
                    "-lEngine", 
                    "-lGame",
                    "-lglfw", 
                    "-lz",
                    "-framework", "Cocoa",
                    "-framework", "IOKit",
                    "-framework", "CoreVideo",
                    "-framework", "OpenGL",
                    "-framework", "Metal",
                    "-framework", "MetalKit",
                    "-framework", "SwiftUI",
                    "-framework", "QuartzCore",
                    "-framework", "QuickLookThumbnailing",
                    "-framework", "AVFoundation"
                ])
            ]
        ),
        .testTarget(
            name: "VoxelForgeStudioTests", 
            dependencies: ["VoxelForgeStudio"], 
            path: "Tests", 
            sources: [
                "CoreSystemsTests.swift",
                "EngineBridgeTests.swift",
                "FrontendIntegrationTests.swift",
                "EngineConnectionTests.swift"
            ]
        )
    ]
)
