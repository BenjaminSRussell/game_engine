import SwiftUI
import simd

// MARK: - Gizmo System (Phase 10)

enum GizmoType {
    case translate
    case rotate
    case scale
}

enum GizmoAxis {
    case x, y, z
    case xy, yz, xz
    case xyz
}

struct GizmoInteraction {
    let gizmoType: GizmoType
    let axis: GizmoAxis
    let startPosition: SIMD3<Float>
    let startValue: SIMD3<Float>
}

class GizmoManager: ObservableObject {
    @Published var gizmoType: GizmoType = .translate
    @Published var currentInteraction: GizmoInteraction?
    @Published var hoveredAxis: GizmoAxis?
    
    func startInteraction(type: GizmoType, axis: GizmoAxis, position: SIMD3<Float>, value: SIMD3<Float>) {
        currentInteraction = GizmoInteraction(
            gizmoType: type,
            axis: axis,
            startPosition: position,
            startValue: value
        )
    }
    
    func updateInteraction(position: SIMD3<Float>) -> SIMD3<Float>? {
        guard let interaction = currentInteraction else { return nil }
        
        let delta = position - interaction.startPosition
        
        switch interaction.gizmoType {
        case .translate:
            return calculateTranslationDelta(delta: delta, axis: interaction.axis, startValue: interaction.startValue)
        case .rotate:
            return calculateRotationDelta(delta: delta, axis: interaction.axis, startValue: interaction.startValue)
        case .scale:
            return calculateScaleDelta(delta: delta, axis: interaction.axis, startValue: interaction.startValue)
        }
    }
    
    func endInteraction() {
        currentInteraction = nil
    }
    
    private func calculateTranslationDelta(delta: SIMD3<Float>, axis: GizmoAxis, startValue: SIMD3<Float>) -> SIMD3<Float> {
        switch axis {
        case .x:
            return startValue + SIMD3<Float>(delta.x, 0, 0)
        case .y:
            return startValue + SIMD3<Float>(0, delta.y, 0)
        case .z:
            return startValue + SIMD3<Float>(0, 0, delta.z)
        case .xy:
            return startValue + SIMD3<Float>(delta.x, delta.y, 0)
        case .yz:
            return startValue + SIMD3<Float>(0, delta.y, delta.z)
        case .xz:
            return startValue + SIMD3<Float>(delta.x, 0, delta.z)
        case .xyz:
            return startValue + delta
        }
    }
    
    private func calculateRotationDelta(delta: SIMD3<Float>, axis: GizmoAxis, startValue: SIMD3<Float>) -> SIMD3<Float> {
        let rotationSpeed: Float = 0.5
        let rotationDelta = length(delta) * rotationSpeed
        
        switch axis {
        case .x:
            return startValue + SIMD3<Float>(rotationDelta, 0, 0)
        case .y:
            return startValue + SIMD3<Float>(0, rotationDelta, 0)
        case .z:
            return startValue + SIMD3<Float>(0, 0, rotationDelta)
        default:
            return startValue
        }
    }
    
    private func calculateScaleDelta(delta: SIMD3<Float>, axis: GizmoAxis, startValue: SIMD3<Float>) -> SIMD3<Float> {
        let scaleSpeed: Float = 0.01
        let scaleDelta = 1.0 + (length(delta) * scaleSpeed)
        
        switch axis {
        case .x:
            return startValue * SIMD3<Float>(scaleDelta, 1, 1)
        case .y:
            return startValue * SIMD3<Float>(1, scaleDelta, 1)
        case .z:
            return startValue * SIMD3<Float>(1, 1, scaleDelta)
        case .xyz:
            return startValue * SIMD3<Float>(scaleDelta, scaleDelta, scaleDelta)
        default:
            return startValue
        }
    }
}

// MARK: - Camera Enhancement (Phase 11)

extension CameraController {
    // MARK: - Frame Selection
    
    func frameSelection(position: SIMD3<Float>, radius: Float = 5.0) {
        let direction = normalize(position - self.position)
        let distance = radius * 2.5
        
        // Smooth transition to new position
        let targetPosition = position - (direction * distance)
        
        animateToPosition(targetPosition, duration: 0.3)
    }
    
    private func animateToPosition(_ target: SIMD3<Float>, duration: TimeInterval) {
        // Simple animation - in production would use proper animation system
        let steps = 30
        let start = self.position
        
        for i in 0...steps {
            DispatchQueue.main.asyncAfter(deadline: .now() + (duration / Double(steps)) * Double(i)) {
                let t = Float(i) / Float(steps)
                let eased = self.easeInOutCubic(t)
                self.position = mix(start, target, t: eased)
            }
        }
    }
    
    private func easeInOutCubic(_ t: Float) -> Float {
        return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2
    }
    
    // MARK: - Camera Modes
    
    func setOrbitMode(target: SIMD3<Float>) {
        // Store orbit target for mouse orbit calculations
        // In production this would be a property
    }
    
    func setFlyMode() {
        // Enable WASD movement
        // Already implemented in base CameraController
    }
}

// MARK: - Object Picking (Phase 12)

struct PickResult {
    let entityID: UUID
    let hitPosition: SIMD3<Float>
    let distance: Float
}

class ObjectPicker {
    func pick(ray: GizmoRay, entities: [(UUID, SIMD3<Float>, Float)]) -> PickResult? {
        var closestHit: PickResult?
        var closestDistance: Float = .infinity
        
        for (id, position, radius) in entities {
            if let hit = raySphereIntersect(ray: ray, center: position, radius: radius) {
                let distance = length(hit - ray.origin)
                if distance < closestDistance {
                    closestDistance = distance
                    closestHit = PickResult(entityID: id, hitPosition: hit, distance: distance)
                }
            }
        }
        
        return closestHit
    }
    
    private func raySphereIntersect(ray: GizmoRay, center: SIMD3<Float>, radius: Float) -> SIMD3<Float>? {
        let oc = ray.origin - center
        let a = dot(ray.direction, ray.direction)
        let b = 2.0 * dot(oc, ray.direction)
        let c = dot(oc, oc) - radius * radius
        let discriminant = b * b - 4 * a * c
        
        if discriminant < 0 {
            return nil
        }
        
        let t = (-b - sqrt(discriminant)) / (2.0 * a)
        if t < 0 { return nil }
        
        return ray.origin + ray.direction * t
    }
}

struct GizmoRay {
    let origin: SIMD3<Float>
    let direction: SIMD3<Float>
}

// MARK: - Bounding Box Selection (Phase 12)

struct SelectionBox {
    let min: SIMD2<Float>
    let max: SIMD2<Float>
    
    func contains(_ point: SIMD2<Float>) -> Bool {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y
    }
}

class BoundingBoxSelector {
    func getEntitiesInBox(box: SelectionBox, entities: [(UUID, SIMD2<Float>)]) -> [UUID] {
        return entities.filter { box.contains($0.1) }.map { $0.0 }
    }
}
