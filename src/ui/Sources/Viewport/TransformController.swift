import Foundation
import SwiftUI
import simd

// MARK: - Transform Command for Undo/Redo
struct TransformCommand: Command {
    let initialSnapshots: [TransformSnapshot]
    let finalSnapshots: [TransformSnapshot]
    let timestamp = Date()

    var description: String {
        let count = initialSnapshots.count
        return "Transform \(count) object\(count == 1 ? "" : "s")"
    }

    func execute() {
        // Apply final transforms
        for snapshot in finalSnapshots {
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: snapshot.position,
                rotation: snapshot.rotation,
                scale: snapshot.scale
            )
        }
    }

    func undo() {
        // Restore initial transforms
        for snapshot in initialSnapshots {
            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: snapshot.position,
                rotation: snapshot.rotation,
                scale: snapshot.scale
            )
        }
    }

    func canExecute() -> Bool {
        return !initialSnapshots.isEmpty && !finalSnapshots.isEmpty &&
               initialSnapshots.count == finalSnapshots.count
    }
}

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
    private var newTransformSnapshots: [TransformSnapshot] = []

    // 3D cursor
    @Published var cursor3D: SIMD3<Float> = .zero

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

        // Capture current transforms for undo/redo
        newTransformSnapshots.removeAll()
        guard let selection = selectionManager?.selectedEntities else {
            mode = .idle
            initialSnapshots.removeAll()
            transformDelta = .zero
            return
        }

        for entityID in selection {
            let transform = EngineBridge.shared.getTransform(entityID)
            newTransformSnapshots.append(TransformSnapshot(
                entityID: entityID,
                position: transform.position,
                rotation: transform.rotation,
                scale: transform.scale
            ))
        }

        // Create undo command
        let command = TransformCommand(
            initialSnapshots: initialSnapshots,
            finalSnapshots: newTransformSnapshots
        )

        // Execute command (adds to undo stack)
        CommandManager.shared.execute(command)

        print("[Transform] Committed transform with undo support")

        mode = .idle
        initialSnapshots.removeAll()
        newTransformSnapshots.removeAll()
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

        // Calculate pivot point
        let pivot = calculatePivotPoint()

        // Apply to all selected entities, rotating around pivot
        for snapshot in initialSnapshots {
            let newRotation = snapshot.rotation + delta

            // Rotate position around pivot
            let offsetFromPivot = snapshot.position - pivot
            let newPosition = pivot + rotateVector(offsetFromPivot, by: delta)

            EngineBridge.shared.setTransform(
                snapshot.entityID,
                position: newPosition,
                rotation: newRotation,
                scale: snapshot.scale
            )
        }
    }

    private func rotateVector(_ vector: SIMD3<Float>, by rotation: SIMD3<Float>) -> SIMD3<Float> {
        // Apply rotation using Euler angles (X, Y, Z order)
        var result = vector
        let pitchRad = rotation.x * .pi / 180.0
        let yawRad = rotation.y * .pi / 180.0
        let rollRad = rotation.z * .pi / 180.0

        // Rotate around X axis (pitch)
        var temp = result
        result.y = temp.y * cos(pitchRad) - temp.z * sin(pitchRad)
        result.z = temp.y * sin(pitchRad) + temp.z * cos(pitchRad)

        // Rotate around Y axis (yaw)
        temp = result
        result.x = temp.x * cos(yawRad) + temp.z * sin(yawRad)
        result.z = -temp.x * sin(yawRad) + temp.z * cos(yawRad)

        // Rotate around Z axis (roll)
        temp = result
        result.x = temp.x * cos(rollRad) - temp.y * sin(rollRad)
        result.y = temp.x * sin(rollRad) + temp.y * cos(rollRad)

        return result
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
            // Return the 3D cursor position
            return cursor3D
        }
    }

    // MARK: - 3D Cursor Management
    func setCursor3D(position: SIMD3<Float>) {
        cursor3D = position
        print("[Transform] Set 3D cursor position: \(position)")
    }

    func moveCursor3D(delta: SIMD3<Float>) {
        cursor3D = cursor3D + delta
        print("[Transform] Moved 3D cursor by: \(delta), new position: \(cursor3D)")
    }

    func resetCursor3D() {
        cursor3D = .zero
        print("[Transform] Reset 3D cursor to origin")
    }
    
    // MARK: - Display Info
    
    var transformDisplayText: String {
        switch mode {
        case .idle:
            return ""
        case .translating(let constraint):
            let constraintText = constraint?.displayName ?? "Free"
            return "Translating [\(constraintText)] : (\(String(format: "%.2f", transformDelta.x)), \(String(format: "%.2f", transformDelta.y)), \(String(format: "%.2f", transformDelta.z)))"
        case .rotating(let constraint):
            let constraintText = constraint?.displayName ?? "Free"
            return "Rotating [\(constraintText)] : (\(String(format: "%.1f", transformDelta.x)), \(String(format: "%.1f", transformDelta.y)), \(String(format: "%.1f", transformDelta.z)))"
        case .scaling(let constraint):
            let constraintText = constraint?.displayName ?? "Uniform"
            return "Scaling [\(constraintText)] Factor: (\(String(format: "%.2f", transformDelta.x)), \(String(format: "%.2f", transformDelta.y)), \(String(format: "%.2f", transformDelta.z)))"
        }
    }
}
