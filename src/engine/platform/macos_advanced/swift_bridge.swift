/**
 * =================================================================================================
 *                          SWIFT BRIDGE LAYER
 * =================================================================================================
 *
 * Bridging header and Swift extensions for interacting with the C engine core.
 * Allows modern macOS UI (SwiftUI) to control the engine.
 *
 * =================================================================================================
 */

import Foundation
import Metal
import MetalKit
import SwiftUI

// MARK: - C-Interop Structs

/// A 2-component floating-point vector.
/// Mirrors `Vec2` in `src/engine/include/math/vec2.h`.
@frozen
public struct Vec2 {
    public var x: Float
    public var y: Float

    public init(x: Float, y: Float) {
        self.x = x
        self.y = y
    }

    public static let zero = Vec2(x: 0, y: 0)
}

/// A 3-component floating-point vector.
/// Mirrors `Vec3` in `src/engine/include/math/vec3.h`.
@frozen
public struct Vec3 {
    public var x: Float
    public var y: Float
    public var z: Float

    public init(x: Float, y: Float, z: Float) {
        self.x = x
        self.y = y
        self.z = z
    }

    public static let zero = Vec3(x: 0, y: 0, z: 0)
    public static let one = Vec3(x: 1, y: 1, z: 1)
}

/// A 4-component floating-point vector.
/// Mirrors `Vec4` in `src/engine/include/math/vec4.h`.
@frozen
public struct Vec4 {
    public var x: Float
    public var y: Float
    public var z: Float
    public var w: Float

    public init(x: Float, y: Float, z: Float, w: Float) {
        self.x = x
        self.y = y
        self.z = z
        self.w = w
    }
}

/// A 4x4 floating-point matrix.
/// Mirrors `Mat4` in `src/engine/include/math/mat4.h`.
/// Stored in column-major order.
@frozen
public struct Mat4 {
    // 1D array to match C union layout
    public var m: (
        Float, Float, Float, Float,
        Float, Float, Float, Float,
        Float, Float, Float, Float,
        Float, Float, Float, Float
    )

    public init(m: (
        Float, Float, Float, Float,
        Float, Float, Float, Float,
        Float, Float, Float, Float,
        Float, Float, Float, Float
    )) {
        self.m = m
    }

    public static let identity = Mat4(m: (
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    ))
}

/// A quaternion for 3D rotations.
/// Mirrors `Quat` in `src/engine/include/math/quat.h`.
@frozen
public struct Quat {
    public var w: Float
    public var x: Float
    public var y: Float
    public var z: Float

    public init(w: Float, x: Float, y: Float, z: Float) {
        self.w = w
        self.x = x
        self.y = y
        self.z = z
    }

    public static let identity = Quat(w: 1, x: 0, y: 0, z: 0)
}

// MARK: - Engine Callbacks

// Forward declarations for C callbacks.
// Note: These must match the signatures in `src/engine/platform/engine_bridge.h`
// We use `@convention(c)` to ensure C compatibility.

public typealias EntityCreatedCallback = @convention(c) (UInt64) -> Void
public typealias EntityDeletedCallback = @convention(c) (UInt64) -> Void
public typealias EntityModifiedCallback = @convention(c) (UInt64) -> Void
public typealias LogMessageCallback = @convention(c) (UnsafePointer<CChar>?, Int32, UnsafePointer<CChar>?) -> Void
public typealias SceneLoadedCallback = @convention(c) (UnsafePointer<CChar>?) -> Void

public struct EngineCallbacks {
    var on_entity_created: EntityCreatedCallback?
    var on_entity_deleted: EntityDeletedCallback?
    var on_entity_modified: EntityModifiedCallback?
    var on_log_message: LogMessageCallback?
    var on_scene_loaded: SceneLoadedCallback?
}

// MARK: - Engine Wrapper

/// Wrapper for the C engine core.
/// This class handles the initialization, update, and shutdown of the engine.
public class Engine {

    public static let shared = Engine()

    private var isInitialized = false

    // Callbacks
    public var onEntityCreated: ((UInt64) -> Void)?
    public var onEntityDeleted: ((UInt64) -> Void)?
    public var onEntityModified: ((UInt64) -> Void)?
    public var onLogMessage: ((String, Int, String) -> Void)?
    public var onSceneLoaded: ((String) -> Void)?

    private init() {
    }

    /// Initializes the engine with the given configuration.
    public func initialize(width: Int, height: Int) {
        guard !isInitialized else {
             print("Engine already initialized")
             return
        }

        // Setup C callbacks
        // Note: In a real scenario, we'd need a way to route these static C callbacks back to the instance.
        // For simplicity, we'll assume a global trampoline or similar mechanism is handled by the bridge.
        // Since we can't easily capture 'self' in a C callback without a context pointer, we might use a global var in C or Swift.

        let callbacks = EngineCallbacks(
            on_entity_created: { entityId in
                Engine.shared.onEntityCreated?(entityId)
            },
            on_entity_deleted: { entityId in
                Engine.shared.onEntityDeleted?(entityId)
            },
            on_entity_modified: { entityId in
                Engine.shared.onEntityModified?(entityId)
            },
            on_log_message: { message, level, source in
                let msgStr = message != nil ? String(cString: message!) : ""
                let srcStr = source != nil ? String(cString: source!) : ""
                Engine.shared.onLogMessage?(msgStr, Int(level), srcStr)
            },
            on_scene_loaded: { scenePath in
                let pathStr = scenePath != nil ? String(cString: scenePath!) : ""
                Engine.shared.onSceneLoaded?(pathStr)
            }
        )

        // TODO: Call engine_init(callbacks)

        print("Engine initialized with size: \(width)x\(height)")
        isInitialized = true
    }

    /// Updates the engine state.
    /// Should be called every frame.
    public func update(deltaTime: Float) {
        guard isInitialized else { return }

        // TODO: Call engine_update(deltaTime)
        // print("Engine update: \(deltaTime)")
    }

    /// Renders the current frame.
    public func render() {
        guard isInitialized else { return }

        // TODO: Call engine_render(enginePtr) - Note: engine_render is not in engine_bridge.h, might be implicit or different
    }

    /// Shuts down the engine and releases resources.
    public func shutdown() {
        guard isInitialized else { return }

        // TODO: Call engine_shutdown()
        isInitialized = false
        print("Engine shutdown")
    }
}

// MARK: - SwiftUI View

#if os(macOS)
public struct EngineView: NSViewRepresentable {

    public init() {}

    public class Coordinator: NSObject, MTKViewDelegate {
        var parent: EngineView

        init(_ parent: EngineView) {
            self.parent = parent
        }

        public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            // Forward resize to engine
            // TODO: engine_set_viewport_size(UInt32(size.width), UInt32(size.height))
            print("Viewport resized: \(size)")
        }

        public func draw(in view: MTKView) {
            // Calculate delta time
            // For now, assume 60 FPS or use a timer
            let deltaTime: Float = 1.0 / 60.0

            Engine.shared.update(deltaTime: deltaTime)
            Engine.shared.render()
        }
    }

    public func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }

    public func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.delegate = context.coordinator
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.framebufferOnly = false
        mtkView.clearColor = MTLClearColor(red: 0.1, green: 0.1, blue: 0.1, alpha: 1.0)
        mtkView.drawableSize = mtkView.frame.size

        // Initialize engine with initial size
        Engine.shared.initialize(width: Int(mtkView.drawableSize.width), height: Int(mtkView.drawableSize.height))

        return mtkView
    }

    public func updateNSView(_ nsView: MTKView, context: Context) {
        // Handle updates
    }
}
#endif

// TODO(AGENT_MACOS_2): Implement Asset loader bridge
//   - Load assets via Swift Bundle APIs
//   - Pass data pointers to C engine
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create Error handling bridge
//   - Convert C error codes on Swift Error implementation
//   - Difficulty: 2
