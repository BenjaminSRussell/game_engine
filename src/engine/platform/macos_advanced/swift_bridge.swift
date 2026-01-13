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

        // Initialize engine with C callbacks
        var c_callbacks = EngineCallbacks()
        
        // Convert Swift closures to C function pointers
        // Note: This requires a global context or trampoline in the C layer
        
        // Call C engine initialization
        swift_engine_init(
            nil, // on_entity_created - needs trampoline
            nil, // on_entity_deleted - needs trampoline
            nil, // on_entity_modified - needs trampoline
            nil, // on_log_message - needs trampoline
            nil  // on_scene_loaded - needs trampoline
        )

        print("Engine initialized with size: \(width)x\(height)")
        isInitialized = true
    }

    /// Updates the engine state.
    /// Should be called every frame.
    public func update(deltaTime: Float) {
        guard isInitialized else { return }

        // Call C engine update
        swift_engine_update(deltaTime)
        // print("Engine update: \(deltaTime)")
    }

    /// Renders the current frame.
    public func render() {
        guard isInitialized else { return }

        // Call C engine render (if available)
        // Note: engine_render might be implicit or handled differently
        // swift_engine_render() // if available
    }

    /// Shuts down the engine and releases resources.
    public func shutdown() {
        guard isInitialized else { return }

        // Call C engine shutdown
        swift_engine_shutdown()
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
            // Forward resize to C engine
            swift_engine_set_viewport_size(UInt32(size.width), UInt32(size.height))
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

// MARK: - Asset Loader Bridge

/// Bridge for loading assets from Swift Bundle APIs to C engine
public class AssetLoader {
    
    /// Load asset data and pass to C engine
    public static func loadAsset(named name: String, extension: String) -> Data? {
        guard let url = Bundle.main.url(forResource: name, withExtension: `extension`) else {
            print("Failed to find asset: \(name).\(`extension`)")
            return nil
        }
        
        do {
            let data = try Data(contentsOf: url)
            print("Loaded asset: \(name).\(`extension`) (\(data.count) bytes)")
            return data
        } catch {
            print("Failed to load asset: \(error)")
            return nil
        }
    }
    
    /// Load texture asset and pass to C engine
    public static func loadTexture(named name: String, extension: String = "png") -> Bool {
        guard let data = loadAsset(named: name, extension: `extension`) else {
            return false
        }
        
        // Pass data pointer to C engine
        // Note: This requires a C function like swift_engine_load_texture()
        // swift_engine_load_texture(name, data.withUnsafeBytes { $0.baseAddress }, data.count)
        
        return true
    }
    
    /// Load model asset and pass to C engine
    public static func loadModel(named name: String, extension: String = "obj") -> Bool {
        guard let data = loadAsset(named: name, extension: `extension`) else {
            return false
        }
        
        // Pass data pointer to C engine
        // Note: This requires a C function like swift_engine_load_model()
        // swift_engine_load_model(name, data.withUnsafeBytes { $0.baseAddress }, data.count)
        
        return true
    }
}

// MARK: - Error Handling Bridge

/// Swift Error implementation for C engine error codes
public enum EngineError: Error, LocalizedError {
    case initializationFailed
    case invalidParameter(String)
    case outOfMemory
    case fileNotFound(String)
    case invalidFormat(String)
    case unknown(Int32)
    
    public var errorDescription: String? {
        switch self {
        case .initializationFailed:
            return "Engine initialization failed"
        case .invalidParameter(let param):
            return "Invalid parameter: \(param)"
        case .outOfMemory:
            return "Out of memory"
        case .fileNotFound(let file):
            return "File not found: \(file)"
        case .invalidFormat(let format):
            return "Invalid format: \(format)"
        case .unknown(let code):
            return "Unknown error code: \(code)"
        }
    }
    
    /// Convert C error code to Swift Error
    public static func fromErrorCode(_ code: Int32) -> EngineError {
        switch code {
        case 1:
            return .initializationFailed
        case 2:
            return .invalidParameter("Unknown")
        case 3:
            return .outOfMemory
        case 4:
            return .fileNotFound("Unknown")
        case 5:
            return .invalidFormat("Unknown")
        default:
            return .unknown(code)
        }
    }
}

/// Error handling utilities for C engine integration
public class ErrorHandler {
    
    /// Wrap C function call with error handling
    public static func wrapCFunction<T>(_ closure: () throws -> T) -> Result<T, EngineError> {
        do {
            let result = try closure()
            return .success(result)
        } catch {
            if let engineError = error as? EngineError {
                return .failure(engineError)
            } else {
                return .failure(.unknown(-1))
            }
        }
    }
    
    /// Log error with context
    public static func logError(_ error: EngineError, context: String = "") {
        let message = context.isEmpty ? error.localizedDescription : "\(context): \(error.localizedDescription)"
        print("Engine Error: \(message)")
        
        // Forward to C engine logging
        // swift_engine_log(1, "SwiftBridge", message)
    }
}
