import SwiftUI
import simd

struct TransformGizmo: View {
    @ObservedObject var controller: TransformController
    let selectionManager: SelectionManager
    let viewportSize: CGSize
    
    @State private var gizmoPosition: CGPoint = .zero
    @State private var gizmoScale: CGFloat = 1.0
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Only render gizmo if there's a selection
                if !selectionManager.selectedEntities.isEmpty {
                    switch controller.mode {
                    case .idle, .translating:
                        TranslationGizmo(
                            controller: controller,
                            position: gizmoPosition,
                            scale: gizmoScale
                        )
                    case .rotating:
                        RotationGizmo(
                            controller: controller,
                            position: gizmoPosition,
                            scale: gizmoScale
                        )
                    case .scaling:
                        ScaleGizmo(
                            controller: controller,
                            position: gizmoPosition,
                            scale: gizmoScale
                        )
                    }
                }
                
                // Transform HUD
                if controller.mode.isActive {
                    VStack {
                        Spacer()
                        TransformHUD(controller: controller)
                            .padding(.bottom, 20)
                    }
                }
            }
            .onAppear {
                updateGizmoPosition(in: geometry.size)
            }
            .onChange(of: selectionManager.selectedEntities) { _ in
                updateGizmoPosition(in: geometry.size)
            }
        }
    }
    
    private func updateGizmoPosition(in size: CGSize) {
        // Project 3D pivot point to screen space
        guard !selectionManager.selectedEntities.isEmpty else {
            // No selection, center of viewport
            gizmoPosition = CGPoint(x: size.width / 2, y: size.height / 2)
            gizmoScale = 1.0
            return
        }
        
        // Get the pivot point of the first selected entity
        let firstEntityID = selectionManager.selectedEntities.first!
        let pivotPoint = EngineBridge.shared.getEntityPivotPoint(firstEntityID)
        
        // Get camera matrices for projection
        let viewMatrix = EngineBridge.shared.getCameraViewMatrix()
        let projectionMatrix = EngineBridge.shared.getCameraProjectionMatrix()
        let viewportSize = SIMD2<Float>(Float(size.width), Float(size.height))
        
        // Project 3D world position to 2D screen space
        let screenPosition = projectWorldToScreen(
            worldPosition: pivotPoint,
            viewMatrix: viewMatrix,
            projectionMatrix: projectionMatrix,
            viewportSize: viewportSize
        )
        
        // Check if the point is in front of the camera
        if screenPosition.z > 0 && screenPosition.z < 1.0 {
            gizmoPosition = CGPoint(x: CGFloat(screenPosition.x), y: CGFloat(screenPosition.y))
            
            // Calculate scale based on distance from camera
            let distance = screenPosition.z
            let baseScale: Float = 80.0 // Base gizmo size in pixels
            let scaleFactor = baseScale / (distance * 0.5 + 1.0) // Perspective scaling
            gizmoScale = CGFloat(scaleFactor)
        } else {
            // Behind camera, hide gizmo or show at edge
            gizmoPosition = CGPoint(x: -1000, y: -1000) // Off screen
            gizmoScale = 1.0
        }
    }
    
    // Helper function to project 3D world coordinates to 2D screen space
    private func projectWorldToScreen(
        worldPosition: SIMD3<Float>,
        viewMatrix: simd_float4x4,
        projectionMatrix: simd_float4x4,
        viewportSize: SIMD2<Float>
    ) -> SIMD3<Float> {
        // Transform world position to view space
        let viewPosition = viewMatrix * SIMD4<Float>(worldPosition.x, worldPosition.y, worldPosition.z, 1.0)
        
        // Transform view position to clip space
        let clipPosition = projectionMatrix * viewPosition
        
        // Perspective divide to get normalized device coordinates
        guard clipPosition.w != 0 else {
            return SIMD3<Float>(0, 0, -1) // Behind camera
        }
        
        let ndc = SIMD3<Float>(
            clipPosition.x / clipPosition.w,
            clipPosition.y / clipPosition.w,
            clipPosition.z / clipPosition.w
        )
        
        // Convert NDC to screen coordinates
        let screenPosition = SIMD3<Float>(
            (ndc.x + 1.0) * 0.5 * viewportSize.x,
            (1.0 - ndc.y) * 0.5 * viewportSize.y, // Flip Y axis
            ndc.z // Depth value for distance checking
        )
        
        return screenPosition
    }
}

// MARK: - Translation Gizmo

struct TranslationGizmo: View {
    @ObservedObject var controller: TransformController
    let position: CGPoint
    let scale: CGFloat
    
    private let arrowLength: CGFloat = 80
    private let arrowWidth: CGFloat = 3
    
    var body: some View {
        ZStack {
            // X Axis (Red)
            GizmoArrow(
                start: position,
                end: CGPoint(x: position.x + arrowLength * scale, y: position.y),
                color: .red,
                isHovered: controller.hoveredGizmoComponent == .translateX
            )
            
            // Y Axis (Green)
            GizmoArrow(
                start: position,
                end: CGPoint(x: position.x, y: position.y - arrowLength * scale),
                color: .green,
                isHovered: controller.hoveredGizmoComponent == .translateY
            )
            
            // Z Axis (Blue)
            GizmoArrow(
                start: position,
                end: CGPoint(x: position.x - arrowLength * 0.5 * scale, y: position.y + arrowLength * 0.5 * scale),
                color: .blue,
                isHovered: controller.hoveredGizmoComponent == .translateZ
            )
            
            // Center sphere
            Circle()
                .fill(Color.white)
                .frame(width: 12 * scale, height: 12 * scale)
                .position(position)
        }
    }
}

// MARK: - Rotation Gizmo

struct RotationGizmo: View {
    @ObservedObject var controller: TransformController
    let position: CGPoint
    let scale: CGFloat
    
    private let radius: CGFloat = 60
    
    var body: some View {
        ZStack {
            // X Axis Circle (Red)
            Circle()
                .stroke(Color.red, lineWidth: 2)
                .frame(width: radius * 2 * scale, height: radius * 2 * scale)
                .opacity(controller.hoveredGizmoComponent == .rotateX ? 1.0 : 0.6)
                .position(position)
            
            // Y Axis Circle (Green)
            Circle()
                .stroke(Color.green, lineWidth: 2)
                .frame(width: radius * 2 * scale, height: radius * 2 * scale)
                .opacity(controller.hoveredGizmoComponent == .rotateY ? 1.0 : 0.6)
                .position(position)
            
            // Z Axis Circle (Blue)
            Circle()
                .stroke(Color.blue, lineWidth: 2)
                .frame(width: radius * 2 * scale, height: radius * 2 * scale)
                .opacity(controller.hoveredGizmoComponent == .rotateZ ? 1.0 : 0.6)
                .position(position)
        }
    }
}

// MARK: - Scale Gizmo

struct ScaleGizmo: View {
    @ObservedObject var controller: TransformController
    let position: CGPoint
    let scale: CGFloat
    
    private let cubeSize: CGFloat = 10
    private let axisLength: CGFloat = 70
    
    var body: some View {
        ZStack {
            // X Axis
            GizmoLine(
                start: position,
                end: CGPoint(x: position.x + axisLength * scale, y: position.y),
                color: .red
            )
            GizmoCube(
                position: CGPoint(x: position.x + axisLength * scale, y: position.y),
                size: cubeSize * scale,
                color: .red,
                isHovered: controller.hoveredGizmoComponent == .scaleX
            )
            
            // Y Axis
            GizmoLine(
                start: position,
                end: CGPoint(x: position.x, y: position.y - axisLength * scale),
                color: .green
            )
            GizmoCube(
                position: CGPoint(x: position.x, y: position.y - axisLength * scale),
                size: cubeSize * scale,
                color: .green,
                isHovered: controller.hoveredGizmoComponent == .scaleY
            )
            
            // Z Axis
            GizmoLine(
                start: position,
                end: CGPoint(x: position.x - axisLength * 0.5 * scale, y: position.y + axisLength * 0.5 * scale),
                color: .blue
            )
            GizmoCube(
                position: CGPoint(x: position.x - axisLength * 0.5 * scale, y: position.y + axisLength * 0.5 * scale),
                size: cubeSize * scale,
                color: .blue,
                isHovered: controller.hoveredGizmoComponent == .scaleZ
            )
            
            // Center cube (uniform scale)
            GizmoCube(
                position: position,
                size: cubeSize * 1.5 * scale,
                color: .white,
                isHovered: controller.hoveredGizmoComponent == .scaleUniform
            )
        }
    }
}

// MARK: - Gizmo Primitives

struct GizmoArrow: View {
    let start: CGPoint
    let end: CGPoint
    let color: Color
    let isHovered: Bool
    
    var body: some View {
        Path { path in
            path.move(to: start)
            path.addLine(to: end)
        }
        .stroke(color, lineWidth: isHovered ? 4 : 2)
        
        // Arrow head
        Path { path in
            let angle = atan2(end.y - start.y, end.x - start.x)
            let arrowSize: CGFloat = 12
            
            let point1 = CGPoint(
                x: end.x - arrowSize * cos(angle - .pi / 6),
                y: end.y - arrowSize * sin(angle - .pi / 6)
            )
            let point2 = CGPoint(
                x: end.x - arrowSize * cos(angle + .pi / 6),
                y: end.y - arrowSize * sin(angle + .pi / 6)
            )
            
            path.move(to: end)
            path.addLine(to: point1)
            path.move(to: end)
            path.addLine(to: point2)
        }
        .stroke(color, lineWidth: isHovered ? 4 : 2)
    }
}

struct GizmoLine: View {
    let start: CGPoint
    let end: CGPoint
    let color: Color
    
    var body: some View {
        Path { path in
            path.move(to: start)
            path.addLine(to: end)
        }
        .stroke(color, lineWidth: 2)
    }
}

struct GizmoCube: View {
    let position: CGPoint
    let size: CGFloat
    let color: Color
    let isHovered: Bool
    
    var body: some View {
        Rectangle()
            .fill(color)
            .frame(width: size, height: size)
            .overlay(
                Rectangle()
                    .stroke(Color.white, lineWidth: isHovered ? 2 : 1)
            )
            .position(position)
            .opacity(isHovered ? 1.0 : 0.8)
    }
}

// MARK: - Transform HUD

struct TransformHUD: View {
    @ObservedObject var controller: TransformController
    
    var body: some View {
        VStack(spacing: 4) {
            Text(controller.transformDisplayText)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            if controller.snapMode != .none {
                HStack(spacing: 4) {
                    Image(systemName: controller.snapMode.icon)
                        .font(.system(size: 12))
                    Text("Snap: \(String(format: "%.2f", controller.snapIncrement))")
                        .font(DesignSystem.Typography.small)
                }
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            
            Text("Enter to confirm  Esc to cancel")
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(12)
        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.95))
        .cornerRadius(8)
        .shadow(radius: 4)
    }
}
