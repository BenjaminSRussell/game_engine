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
                .headerSearchPath("../../../engine/include"),
                .headerSearchPath("../../../engine/core"),
                .headerSearchPath("../../../engine/renderer/core"),
                .headerSearchPath("../../../engine/ecs"),
                .headerSearchPath("../../../engine/ai"),
            ]
        ),
        .executableTarget(
            name: "VoxelForgeEditor",
            dependencies: ["CEngineCore"],
            path: ".",
            sources: ["Core/", "UI/", "Platform/", "Audio/"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "../engine/include"])
            ],
            linkerSettings: [
                .linkedLibrary("Engine"),
                .linkedFramework("Metal"),
                .linkedFramework("MetalKit"),
                .linkedFramework("SwiftUI"),
                .linkedFramework("Cocoa"),
                .linkedFramework("QuartzCore"),
                .unsafeFlags([
                    "-L", "../../build/",
                    "-Xlinker", "-rpath", "-Xlinker", "@executable_path/../../build/"
                ])
            ]
        )
    ]
)
