# SwiftUI Editor - Xcode Project Configuration
# Package.swift for editor dependencies

// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "EngineEditor",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(
            name: "EngineEditor",
            targets: ["EngineEditor"]
        ),
    ],
    dependencies: [
        // Add any third-party SwiftUI libraries here
    ],
    targets: [
        .executableTarget(
            name: "EngineEditor",
            dependencies: [],
            path: "src/editor/SwiftUI",
            resources: [
                .process("Resources")
            ]
        ),
    ]
)
