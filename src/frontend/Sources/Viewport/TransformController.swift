import Foundation
import SwiftUI
import simd

class TransformController: ObservableObject {
    @Published var mode: TransformMode = .idle
    @Published var pivotPoint: PivotPoint = .median
    @Published var transformSpace: TransformSpace = .world
    @Published var snapMode: SnapMode = .none
    @Published var snapIncrement: Float = 1.0
    @Published var hoveredGizmoComponent: GizmoComponent = .none
    
    // Transform state
    private var initialSnapshots: [TransformSnapshot] = []
    private var startMousePosition: CGPoint = .zero
    private var currentMousePosition: CGPoint = .zero
    private var transformDelta: SIMD3<Float> = .zero
    
    // Selection reference
    weak var selectionManager: SelectionManager?
    
    // MARK: - Transform Operations
    
    func beginTranslate(constraint: AxisConstraint? = nil) {
        guard let selection = selectionManager?.selectedEntities, !selection.isEmpty else { return }
        
        mode = .translating(constraint: constraint)
        captureInitialTransforms()
        print("[Transform] Begin translate, constraint: \(constraint?.displayName ?? "none")")
    }
    
    func beginRotate(constraint: AxisConstraint? = nil) {
        guard let selection = selectionManager?.selectedEntities, !selection.isEmpty else { return }
        
        mode = .rotating(constraint: constraint)
        captureInitialTransforms()
        print("[Transform] Begin rotate, constraint: \(constraint?.displayName ?? "none")")
    }
    
    func beginScale(constraint: AxisConstraint? = nil) {
        guard let selection = selectionManager?.selectedEntities, !selection.isEmpty else { return }
        
        mode = .scaling(constraint: constraint)
        captureInitialTransforms()
        print("[Transform] Begin scale, constraint: \(constraint?.displayName ?? "none")")
    }
    
    func setConstraint(_ constraint: AxisConstraint) {
        switch mode {
        case .translating:
            mode = .translating(constraint: constraint)
        case .rotating:
            mode = .rotating(constraint: constraint)
        case .scaling:
            mode = .scaling(constraint: constraint)
        case .idle:
            break
        }
        print("[Transform] Set constraint: \(constraint.displayName)")
    }
    
    func updateTransform(mouseDelta: CGPoint) {
        guard mode.isActive else { return }
        
        currentMousePosition = CGPoint(
            x: startMousePosition.x + mouseDelta.x,
            y: startMousePosition.y + mouseDelta.y
        )
        
        switch mode {
        case .translating(let constraint):
            updateTranslation(mouseDelta: mouseDelta, constraint: constraint)
        case .rotating(let constraint):
            updateRotation(mouseDelta: mouseDelta, constraint: constraint)
        case .scaling(let constraint):
            updateScaling(mouseDelta: mouseDelta, constraint: constraint)
        case .idle:
            break
        }
    }
    
    func commitTransform() {
        guard mode.isActive else { return }
        
        // TODO: Add to undo stack
        print("[Transform] Committed transform")
        
        mode = .idle
        initialSnapshots.removeAll()
        transformDelta = .zero
    }
    
    func cancelTransform() {
        guard mode.isActive else { return }
        
        // Restore initial transforms
        for snapshot in initialSnapshots {
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: snapshot.position,
                rotation: snapshot.rotation,
                scale: snapshot.scale
            )
        }
        
        print("[Transform] Cancelled transform")
        
        mode = .idle
        initialSnapshots.removeAll()
        transformDelta = .zero
    }
    
    // MARK: - Private Helpers
    
    private func captureInitialTransforms() {
        guard let selection = selectionManager?.selectedEntities else { return }
        
        initialSnapshots.removeAll()
        
        for entityID in selection {
            let transform = EngineBridge.shared.getTransform(entityID)
            initialSnapshots.append(TransformSnapshot(
                entityID: entityID,
                position: transform.position,
                rotation: transform.rotation,
                scale: transform.scale
            ))
        }
    }
    
    private func updateTranslation(mouseDelta: CGPoint, constraint: AxisConstraint?) {
        // Convert mouse delta to world space
        let sensitivity: Float = 0.01
        var delta = SIMD3<Float>(
            Float(mouseDelta.x) * sensitivity,
            -Float(mouseDelta.y) * sensitivity,
            0
        )
        
        // Apply axis constraint
        if let constraint = constraint {
            delta = applyAxisConstraint(delta, constraint: constraint)
        }
        
        // Apply snapping
        if snapMode != .none {
            delta = applySnapping(delta)
        }
        
        transformDelta = delta
        
        // Apply to all selected entities
        for snapshot in initialSnapshots {
            let newPosition = snapshot.position + delta
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: newPosition,
                rotation: snapshot.rotation,
                scale: snapshot.scale
            )
        }
    }
    
    private func updateRotation(mouseDelta: CGPoint, constraint: AxisConstraint?) {
        let sensitivity: Float = 0.5
        var delta = SIMD3<Float>(
            -Float(mouseDelta.y) * sensitivity,
            Float(mouseDelta.x) * sensitivity,
            0
        )
        
        // Apply axis constraint
        if let constraint = constraint {
            delta = applyAxisConstraint(delta, constraint: constraint)
        }
        
        // Apply snapping (angle increments)
        if snapMode == .increment {
            delta = SIMD3<Float>(
                snapToIncrement(delta.x, increment: snapIncrement),
                snapToIncrement(delta.y, increment: snapIncrement),
                snapToIncrement(delta.z, increment: snapIncrement)
            )
        }
        
        transformDelta = delta
        
        // Apply to all selected entities
        let pivot = calculatePivotPoint()
        for snapshot in initialSnapshots {
            let newRotation = snapshot.rotation + delta
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: snapshot.position,  // TODO: Rotate around pivot
                rotation: newRotation,
                scale: snapshot.scale
            )
        }
    }
    
    private func updateScaling(mouseDelta: CGPoint, constraint: AxisConstraint?) {
        let sensitivity: Float = 0.01
        var scaleFactor = 1.0 + Float(mouseDelta.x + mouseDelta.y) * sensitivity
        
        var delta = SIMD3<Float>(scaleFactor, scaleFactor, scaleFactor)
        
        // Apply axis constraint
        if let constraint = constraint {
            switch constraint {
            case .x:
                delta = SIMD3<Float>(scaleFactor, 1.0, 1.0)
            case .y:
                delta = SIMD3<Float>(1.0, scaleFactor, 1.0)
            case .z:
                delta = SIMD3<Float>(1.0, 1.0, scaleFactor)
            default:
                break
            }
        }
        
        transformDelta = delta
        
        // Apply to all selected entities
        for snapshot in initialSnapshots {
            let newScale = snapshot.scale * delta
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: snapshot.position,
                rotation: snapshot.rotation,
                scale: newScale
            )
        }
    }
    
    private func applyAxisConstraint(_ value: SIMD3<Float>, constraint: AxisConstraint) -> SIMD3<Float> {
        switch constraint {
        case .x:
            return SIMD3<Float>(value.x, 0, 0)
        case .y:
            return SIMD3<Float>(0, value.y, 0)
        case .z:
            return SIMD3<Float>(0, 0, value.z)
        case .xy:
            return SIMD3<Float>(value.x, value.y, 0)
        case .xz:
            return SIMD3<Float>(value.x, 0, value.z)
        case .yz:
            return SIMD3<Float>(0, value.y, value.z)
        case .view:
            return value
        }
    }
    
    private func applySnapping(_ value: SIMD3<Float>) -> SIMD3<Float> {
        switch snapMode {
        case .increment, .grid:
            return SIMD3<Float>(
                snapToIncrement(value.x, increment: snapIncrement),
                snapToIncrement(value.y, increment: snapIncrement),
                snapToIncrement(value.z, increment: snapIncrement)
            )
        default:
            return value
        }
    }
    
    private func snapToIncrement(_ value: Float, increment: Float) -> Float {
        return round(value / increment) * increment
    }
    
    private func calculatePivotPoint() -> SIMD3<Float> {
        guard !initialSnapshots.isEmpty else { return .zero }
        
        switch pivotPoint {
        case .median, .boundingBoxCenter:
            var sum = SIMD3<Float>.zero
            for snapshot in initialSnapshots {
                sum += snapshot.position
            }
            return sum / Float(initialSnapshots.count)
            
        case .activeElement:
            return initialSnapshots.first?.position ?? .zero
            
        case .individualOrigins:
            // Each object uses its own origin
            return .zero
            
        case .cursor3D:
            // TODO: Implement 3D cursor
            return .zero
        }
    }
    
    // MARK: - Display Info
    
    var transformDisplayText: String {
        switch mode {
        case .idle:
            return ""
        case .translating(let constraint):
            let constraintText = constraint?.displayName ?? "Free"
            return "Translating [\(constraintText)] Δ: (\(String(format: "%.2f", transformDelta.x)), \(String(format: "%.2f", transformDelta.y)), \(String(format: "%.2f", transformDelta.z)))"
        case .rotating(let constraint):
            let constraintText = constraint?.displayName ?? "Free"
            return "Rotating [\(constraintText)] Δ: (\(String(format: "%.1f", transformDelta.x))°, \(String(format: "%.1f", transformDelta.y))°, \(String(format: "%.1f", transformDelta.z))°)"
        case .scaling(let constraint):
            let constraintText = constraint?.displayName ?? "Uniform"
            return "Scaling [\(constraintText)] Factor: (\(String(format: "%.2f", transformDelta.x)), \(String(format: "%.2f", transformDelta.y)), \(String(format: "%.2f", transformDelta.z)))"
        }
    }
}
