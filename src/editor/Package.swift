// swift-tools-version:6.0
import PackageDescription

let package = Package(
    name: "VoxelForgeEditor",
    platforms: [.macOS(.v14)],
    products: [
        .executable(name: "VoxelForgeEditor", targets: ["VoxelForgeEditor"])
    ],
    dependencies: [],
    targets: [
        .target(
            name: "CEngineCore",
            path: "Core/Bridges",
            sources: ["engine_bridge.c"],
            publicHeadersPath: "include",
            cSettings: [
                .unsafeFlags([
                    "-I", "../engine/include",
                    "-I", "../engine/core",
                    "-I", "../engine/renderer/core",
                    "-I", "../engine/ecs",
                    "-I", "../engine/ai"
                ])
            ]
        ),
        .executableTarget(
            name: "VoxelForgeEditor",
            dependencies: ["CEngineCore"],
            path: ".",
            exclude: ["Core/Tests/", "Platform/Tests/", "Core/Bridges/"], // Exclude test files and C bridge from main target
            sources: ["Core/", "UI/", "Platform/", "Audio/"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "../engine/include"])
            ],
            linkerSettings: [
                .linkedLibrary("GameEngine"),
                .linkedFramework("Metal"),
                .linkedFramework("MetalKit"),
                .linkedFramework("SwiftUI"),
                .linkedFramework("Cocoa"),
                .linkedFramework("QuartzCore"),
                .linkedFramework("QuickLookThumbnailing"),
                .linkedFramework("AVFoundation"),
                .unsafeFlags([
                    "-L", "../../build/",
                    "-Xlinker", "-rpath", "-Xlinker", "@executable_path/../../build/"
                ])
            ]
        ),
        .testTarget(
            name: "VoxelForgeEditorTests",
            dependencies: ["VoxelForgeEditor"],
            path: "Core/Tests",
            sources: ["InteropTests.swift"]
        ),
        .testTarget(
            name: "PlatformTests",
            dependencies: ["VoxelForgeEditor"],
            path: "Platform/Tests",
            sources: ["UnifiedMemoryBenchmark.swift"]
        )
    ]
)
