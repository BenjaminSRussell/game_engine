import Foundation
import simd

// MARK: - Transform Mode

enum TransformMode: Equatable {
    case idle
    case translating(constraint: AxisConstraint?)
    case rotating(constraint: AxisConstraint?)
    case scaling(constraint: AxisConstraint?)
    
    var isActive: Bool {
        switch self {
        case .idle: return false
        default: return true
        }
    }
    
    var displayName: String {
        switch self {
        case .idle: return "Idle"
        case .translating: return "Translating"
        case .rotating: return "Rotating"
        case .scaling: return "Scaling"
        }
    }
}

// MARK: - Axis Constraint

enum AxisConstraint: Equatable {
    case x
    case y
    case z
    case xy  // Plane constraint
    case xz
    case yz
    case view  // Screen-space
    
    var displayName: String {
        switch self {
        case .x: return "X"
        case .y: return "Y"
        case .z: return "Z"
        case .xy: return "XY"
        case .xz: return "XZ"
        case .yz: return "YZ"
        case .view: return "View"
        }
    }
    
    var color: (r: Float, g: Float, b: Float) {
        switch self {
        case .x: return (1.0, 0.0, 0.0)  // Red
        case .y: return (0.0, 1.0, 0.0)  // Green
        case .z: return (0.0, 0.0, 1.0)  // Blue
        default: return (1.0, 1.0, 0.0)  // Yellow for planes
        }
    }
}

// MARK: - Pivot Point

enum PivotPoint: String, CaseIterable {
    case individualOrigins = "Individual Origins"
    case median = "Median Point"
    case cursor3D = "3D Cursor"
    case activeElement = "Active Element"
    case boundingBoxCenter = "Bounding Box Center"
    
    var icon: String {
        switch self {
        case .individualOrigins: return "square.grid.3x3"
        case .median: return "circle.grid.cross"
        case .cursor3D: return "scope"
        case .activeElement: return "target"
        case .boundingBoxCenter: return "cube"
        }
    }
}

// MARK: - Transform Space

enum TransformSpace: String, CaseIterable {
    case world = "World"
    case local = "Local"
    case view = "View"
    
    var icon: String {
        switch self {
        case .world: return "globe"
        case .local: return "cube"
        case .view: return "eye"
        }
    }
}

// MARK: - Snap Mode

enum SnapMode: String, CaseIterable {
    case none = "None"
    case increment = "Increment"
    case vertex = "Vertex"
    case edge = "Edge"
    case face = "Face"
    case grid = "Grid"
    
    var icon: String {
        switch self {
        case .none: return "xmark"
        case .increment: return "ruler"
        case .vertex: return "circle.fill"
        case .edge: return "line.diagonal"
        case .face: return "square.fill"
        case .grid: return "square.grid.2x2"
        }
    }
}

// MARK: - Transform Data

struct TransformSnapshot {
    let entityID: UUID
    let position: SIMD3<Float>
    let rotation: SIMD3<Float>
    let scale: SIMD3<Float>
}

// MARK: - Gizmo Component

enum GizmoComponent: Equatable {
    case none
    case translateX
    case translateY
    case translateZ
    case translateXY
    case translateXZ
    case translateYZ
    case rotateX
    case rotateY
    case rotateZ
    case rotateView
    case scaleX
    case scaleY
    case scaleZ
    case scaleUniform
    
    var axis: AxisConstraint? {
        switch self {
        case .translateX, .rotateX, .scaleX: return .x
        case .translateY, .rotateY, .scaleY: return .y
        case .translateZ, .rotateZ, .scaleZ: return .z
        case .translateXY: return .xy
        case .translateXZ: return .xz
        case .translateYZ: return .yz
        case .rotateView: return .view
        default: return nil
        }
    }
    
    var color: SIMD4<Float> {
        switch self {
        case .translateX, .rotateX, .scaleX:
            return SIMD4<Float>(1.0, 0.0, 0.0, 1.0)  // Red
        case .translateY, .rotateY, .scaleY:
            return SIMD4<Float>(0.0, 1.0, 0.0, 1.0)  // Green
        case .translateZ, .rotateZ, .scaleZ:
            return SIMD4<Float>(0.0, 0.0, 1.0, 1.0)  // Blue
        case .scaleUniform:
            return SIMD4<Float>(1.0, 1.0, 1.0, 1.0)  // White
        default:
            return SIMD4<Float>(1.0, 1.0, 0.0, 0.5)  // Yellow semi-transparent
        }
    }
}
