import SwiftUI
import simd

// MARK: - Selection Manager

class SelectionManager: ObservableObject {
    @Published var selectedEntities: Set<UUID> = []
    @Published var gizmoMode: GizmoMode = .translate
    @Published var gizmoSpace: GizmoSpace = .world
    @Published var isSnappingEnabled: Bool = false
    @Published var snapValue: Float = 1.0
    
    enum GizmoMode {
        case translate, rotate, scale
        
        var icon: String {
            switch self {
            case .translate: return "arrow.up.and.down.and.arrow.left.and.right"
            case .rotate: return "arrow.triangle.2.circlepath"
            case .scale: return "arrow.up.left.and.arrow.down.right"
            }
        }
        
        var hotkey: String {
            switch self {
            case .translate: return "W"
            case .rotate: return "E"
            case .scale: return "R"
            }
        }
    }
    
    enum GizmoSpace {
        case world, local
        
        var displayName: String {
            switch self {
            case .world: return "World"
            case .local: return "Local"
            }
        }
    }
    
    enum Axis {
        case x, y, z, xy, xz, yz, xyz
        
        var color: Color {
            switch self {
            case .x: return .red
            case .y: return .green
            case .z: return .blue
            case .xy, .xz, .yz, .xyz: return .yellow
            }
        }
    }
    
    // MARK: - Selection
    
    func selectAtScreenPoint(_ point: CGPoint, viewportSize: CGSize, camera: CameraController, entities: [EntityNode]) -> UUID? {
        let ray = screenPointToSelectionRay(point, viewportSize: viewportSize, camera: camera)
        
        var closestEntity: UUID?
        var closestDistance: Float = .infinity
        
        for entity in entities {
            if let distance = rayIntersectsEntity(ray, entity: entity) {
                if distance < closestDistance {
                    closestDistance = distance
                    closestEntity = entity.id
                }
            }
        }
        
        return closestEntity
    }
    
    func boxSelect(start: CGPoint, end: CGPoint, viewportSize: CGSize, camera: CameraController, entities: [EntityNode]) -> Set<UUID> {
        var selected = Set<UUID>()
        
        let minX = min(start.x, end.x)
        let maxX = max(start.x, end.x)
        let minY = min(start.y, end.y)
        let maxY = max(start.y, end.y)
        
        for entity in entities {
            let screenPos = worldToScreen(entity.transform.position, viewportSize: viewportSize, camera: camera)
            
            if screenPos.x >= minX && screenPos.x <= maxX &&
               screenPos.y >= minY && screenPos.y <= maxY {
                selected.insert(entity.id)
            }
        }
        
        return selected
    }
    
    func selectEntity(_ id: UUID, additive: Bool = false) {
        if additive {
            if selectedEntities.contains(id) {
                selectedEntities.remove(id)
            } else {
                selectedEntities.insert(id)
            }
        } else {
            selectedEntities = [id]
        }
    }
    
    func deselectAll() {
        selectedEntities.removeAll()
    }
    
    // MARK: - Gizmo Manipulation
    
    func handleGizmoDrag(axis: Axis, delta: CGPoint, camera: CameraController, entities: [EntityNode]) {
        guard !selectedEntities.isEmpty else { return }
        
        switch gizmoMode {
        case .translate:
            handleTranslate(axis: axis, delta: delta, camera: camera, entities: entities)
        case .rotate:
            handleRotate(axis: axis, delta: delta, entities: entities)
        case .scale:
            handleScale(axis: axis, delta: delta, entities: entities)
        }
    }
    
    private func handleTranslate(axis: Axis, delta: CGPoint, camera: CameraController, entities: [EntityNode]) {
        let sensitivity: Float = 0.01
        var movement = SIMD3<Float>(0, 0, 0)
        
        switch axis {
        case .x:
            movement.x = Float(delta.x) * sensitivity
        case .y:
            movement.y = -Float(delta.y) * sensitivity
        case .z:
            movement.z = Float(delta.x) * sensitivity
        case .xy:
            movement.x = Float(delta.x) * sensitivity
            movement.y = -Float(delta.y) * sensitivity
        case .xz:
            movement.x = Float(delta.x) * sensitivity
            movement.z = Float(delta.y) * sensitivity
        case .yz:
            movement.y = -Float(delta.y) * sensitivity
            movement.z = Float(delta.x) * sensitivity
        case .xyz:
            // Free movement
            let right = camera.right()
            let up = camera.up()
            movement = right * Float(delta.x) * sensitivity + up * Float(-delta.y) * sensitivity
        }
        
        // Apply snapping
        if isSnappingEnabled {
            movement = snap(movement, to: snapValue)
        }
        
        // Apply to selected entities
        for entity in entities where selectedEntities.contains(entity.id) {
            entity.transform.position += movement
        }
    }
    
    private func handleRotate(axis: Axis, delta: CGPoint, entities: [EntityNode]) {
        let sensitivity: Float = 0.5
        var rotation = SIMD3<Float>(0, 0, 0)
        
        switch axis {
        case .x:
            rotation.x = Float(delta.x) * sensitivity
        case .y:
            rotation.y = Float(delta.x) * sensitivity
        case .z:
            rotation.z = Float(delta.x) * sensitivity
        default:
            rotation.y = Float(delta.x) * sensitivity
        }
        
        // Apply snapping (15-degree increments)
        if isSnappingEnabled {
            rotation = snap(rotation, to: 15.0)
        }
        
        // Apply to selected entities
        for entity in entities where selectedEntities.contains(entity.id) {
            entity.transform.rotation += rotation
        }
    }
    
    private func handleScale(axis: Axis, delta: CGPoint, entities: [EntityNode]) {
        let sensitivity: Float = 0.01
        var scale = SIMD3<Float>(1, 1, 1)
        
        let scaleDelta = Float(delta.x) * sensitivity
        
        switch axis {
        case .x:
            scale.x += scaleDelta
        case .y:
            scale.y += scaleDelta
        case .z:
            scale.z += scaleDelta
        case .xyz:
            // Uniform scale
            scale += SIMD3(scaleDelta, scaleDelta, scaleDelta)
        default:
            scale += SIMD3(scaleDelta, scaleDelta, scaleDelta)
        }
        
        // Apply to selected entities
        for entity in entities where selectedEntities.contains(entity.id) {
            entity.transform.scale *= scale
        }
    }
    
    // MARK: - Ray Casting
    
    private func screenPointToSelectionRay(_ point: CGPoint, viewportSize: CGSize, camera: CameraController) -> SelectionRay {
        // Convert screen coordinates to NDC (-1 to 1)
        let x = (2.0 * Float(point.x)) / Float(viewportSize.width) - 1.0
        let y = 1.0 - (2.0 * Float(point.y)) / Float(viewportSize.height)
        
        // Create ray in clip space
        let rayClip = SIMD4<Float>(x, y, -1.0, 1.0)
        
        // Transform to view space
        let aspectRatio = Float(viewportSize.width / viewportSize.height)
        let projectionMatrix = camera.projectionMatrix(aspectRatio: aspectRatio)
        let invProjection = projectionMatrix.inverse
        var rayView = invProjection * rayClip
        rayView = SIMD4(rayView.x, rayView.y, -1.0, 0.0)
        
        // Transform to world space
        let viewMatrix = camera.viewMatrix()
        let invView = viewMatrix.inverse
        let rayWorld = invView * rayView
        
        let direction = normalize(SIMD3(rayWorld.x, rayWorld.y, rayWorld.z))
        
        return SelectionRay(origin: camera.position, direction: direction)
    }
    
    private func rayIntersectsEntity(_ ray: SelectionRay, entity: EntityNode) -> Float? {
        // Get entity bounds from transform scale
        let transform = EngineBridge.shared.getTransform(entity.id)
        let radius: Float = max(transform.scale.x, max(transform.scale.y, transform.scale.z)) * 0.5
        let center = transform.position
        
        let oc = ray.origin - center
        let a = dot(ray.direction, ray.direction)
        let b = 2.0 * dot(oc, ray.direction)
        let c = dot(oc, oc) - radius * radius
        let discriminant = b * b - 4 * a * c
        
        if discriminant < 0 {
            return nil
        }
        
        let t = (-b - sqrt(discriminant)) / (2.0 * a)
        return t > 0 ? t : nil
    }
    
    private func worldToScreen(_ worldPos: SIMD3<Float>, viewportSize: CGSize, camera: CameraController) -> CGPoint {
        let aspectRatio = Float(viewportSize.width / viewportSize.height)
        let viewMatrix = camera.viewMatrix()
        let projectionMatrix = camera.projectionMatrix(aspectRatio: aspectRatio)
        
        let viewPos = viewMatrix * SIMD4(worldPos.x, worldPos.y, worldPos.z, 1.0)
        let clipPos = projectionMatrix * viewPos
        
        let ndc = SIMD3(clipPos.x, clipPos.y, clipPos.z) / clipPos.w
        
        let screenX = (ndc.x + 1.0) * 0.5 * Float(viewportSize.width)
        let screenY = (1.0 - ndc.y) * 0.5 * Float(viewportSize.height)
        
        return CGPoint(x: CGFloat(screenX), y: CGFloat(screenY))
    }
    
    // MARK: - Utility
    
    private func snap(_ value: SIMD3<Float>, to gridSize: Float) -> SIMD3<Float> {
        return SIMD3(
            round(value.x / gridSize) * gridSize,
            round(value.y / gridSize) * gridSize,
            round(value.z / gridSize) * gridSize
        )
    }
}

// MARK: - Supporting Types

struct SelectionRay {
    let origin: SIMD3<Float>
    let direction: SIMD3<Float>
}

extension EntityNode {
    var transform: Transform {
        get {
            // Get from actual transform component via bridge
            let transformData = EngineBridge.shared.getTransform(id)
            return Transform(
                position: transformData.position,
                rotation: transformData.rotation,
                scale: transformData.scale
            )
        }
        set {
            // Apply snapping before setting transform
            let snappedPosition = SnapManager.shared.snapPosition(newValue.position)
            let snappedRotation = SnapManager.shared.snapRotation(newValue.rotation)
            let snappedScale = SnapManager.shared.snapScale(newValue.scale)
            
            let snappedTransform = TransformData(
                position: snappedPosition,
                rotation: snappedRotation,
                scale: snappedScale
            )
            
            // Set to actual transform component via bridge
            EngineBridge.shared.setTransform(id, transform: snappedTransform)
        }
    }
}

struct Transform {
    var position: SIMD3<Float>
    var rotation: SIMD3<Float>
    var scale: SIMD3<Float>
}

// MARK: - Gizmo Controls View

struct GizmoControlsView: View {
    @ObservedObject var selection: SelectionManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Gizmo mode buttons
            ForEach([SelectionManager.GizmoMode.translate, .rotate, .scale], id: \.self) { mode in
                Button(action: {
                    selection.gizmoMode = mode
                }) {
                    VStack(spacing: 2) {
                        Image(systemName: mode.icon)
                            .font(.system(size: 16))
                        Text(mode.hotkey)
                            .font(DesignSystem.Typography.small)
                    }
                    .foregroundColor(selection.gizmoMode == mode ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                    .frame(width: 40, height: 40)
                    .background(selection.gizmoMode == mode ? DesignSystem.Colors.selection : Color.clear)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                .help("\(mode.hotkey) - \(String(describing: mode).capitalized)")
            }
            
            EditorDivider()
                .frame(height: 30)
            
            // Space toggle
            Button(action: {
                selection.gizmoSpace = selection.gizmoSpace == .world ? .local : .world
            }) {
                Text(selection.gizmoSpace.displayName)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            }
            .buttonStyle(.plain)
            
            EditorDivider()
                .frame(height: 30)
            
            // Snap toggle
            Button(action: {
                selection.isSnappingEnabled.toggle()
            }) {
                HStack(spacing: 4) {
                    Image(systemName: selection.isSnappingEnabled ? "grid.circle.fill" : "grid.circle")
                        .foregroundColor(selection.isSnappingEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    Text(String(format: "%.1f", selection.snapValue))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(selection.isSnappingEnabled ? DesignSystem.Colors.selection : Color.clear)
                .cornerRadius(4)
            }
            .buttonStyle(.plain)
            .help("Toggle Snapping (Ctrl)")
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
        .cornerRadius(4)
    }
}

// MARK: - Preview

#Preview {
    VStack {
        GizmoControlsView(selection: SelectionManager())
    }
    .padding()
    .background(Color.black)
}
