// swift-tools-version: 6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "VoxelForge",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .library(name: "VoxelForge", targets: ["VoxelForge"])
    ],
    targets: [
        .target(
            name: "VoxelForge",
            resources: [
                .process("Shaders.metal"),
                .process("Culling.metal"),
                .process("Deferred.metal"),
                .process("HZBCulling.metal")
            ]
        ),
    ]
)
