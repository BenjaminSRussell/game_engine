import SwiftUI
import simd

// MARK: - Camera Controller

class CameraController: ObservableObject {
    // Camera state
    @Published var position = SIMD3<Float>(0, 5, 10)
    @Published var rotation = SIMD3<Float>(-20, 0, 0) // pitch, yaw, roll
    @Published var fov: Float = 60.0
    @Published var mode: CameraMode = .perspective
    
    // Movement settings
    private let moveSpeed: Float = 5.0
    private let sprintMultiplier: Float = 2.0
    private let rotationSpeed: Float = 0.3
    private let zoomSpeed: Float = 1.0
    private let smoothingFactor: Float = 0.15
    
    // Input state
    private var keyStates: Set<KeyCode> = []
    private var isOrbiting = false
    private var isPanning = false
    
    enum CameraMode {
        case perspective
        case orthographic
        case top, front, right, left, bottom, back
        
        var displayName: String {
            switch self {
            case .perspective: return "Perspective"
            case .orthographic: return "Orthographic"
            case .top: return "Top"
            case .front: return "Front"
            case .right: return "Right"
            case .left: return "Left"
            case .bottom: return "Bottom"
            case .back: return "Back"
            }
        }
    }
    
    enum KeyCode: String {
        case w, a, s, d, q, e
        case shift, space
    }
    
    // MARK: - Update Loop
    
    func update(deltaTime: Float) {
        guard mode == .perspective else { return }
        
        var movement = SIMD3<Float>(0, 0, 0)
        let speed = keyStates.contains(.shift) ? moveSpeed * sprintMultiplier : moveSpeed
        
        // WASD movement
        if keyStates.contains(.w) {
            movement += forward() * speed * deltaTime
        }
        if keyStates.contains(.s) {
            movement -= forward() * speed * deltaTime
        }
        if keyStates.contains(.a) {
            movement -= right() * speed * deltaTime
        }
        if keyStates.contains(.d) {
            movement += right() * speed * deltaTime
        }
        
        // Q/E for up/down
        if keyStates.contains(.q) {
            movement.y -= speed * deltaTime
        }
        if keyStates.contains(.e) {
            movement.y += speed * deltaTime
        }
        
        // Apply movement with smoothing
        if length(movement) > 0 {
            position += movement
        }
    }
    
    // MARK: - Input Handling
    
    func handleKeyDown(_ key: KeyCode) {
        keyStates.insert(key)
    }
    
    func handleKeyUp(_ key: KeyCode) {
        keyStates.remove(key)
    }
    
    func handleMouseDrag(_ delta: CGPoint, isRightButton: Bool, isMiddleButton: Bool) {
        if isRightButton {
            // Orbit camera
            rotation.y += Float(delta.x) * rotationSpeed
            rotation.x += Float(delta.y) * rotationSpeed
            
            // Clamp pitch to avoid gimbal lock
            rotation.x = max(-89, min(89, rotation.x))
        } else if isMiddleButton {
            // Pan camera
            let panSpeed: Float = 0.01
            let rightVec = right()
            let upVec = up()
            
            position -= rightVec * Float(delta.x) * panSpeed
            position += upVec * Float(delta.y) * panSpeed
        }
    }
    
    func handleScroll(_ delta: Float) {
        guard mode == .perspective else { return }
        
        // Zoom by moving forward/backward
        let zoomAmount = delta * zoomSpeed
        position += forward() * zoomAmount
    }
    
    func focusOnEntity(at targetPosition: SIMD3<Float>, radius: Float = 5.0) {
        // Smooth transition to focus on entity
        let direction = normalize(position - targetPosition)
        let targetCameraPosition = targetPosition + direction * radius
        
        withAnimation(.easeInOut(duration: 0.3)) {
            position = targetCameraPosition
            
            // Look at target
            let lookDirection = normalize(targetPosition - targetCameraPosition)
            rotation.y = atan2(lookDirection.x, lookDirection.z) * 180 / .pi
            rotation.x = asin(-lookDirection.y) * 180 / .pi
        }
    }
    
    func setOrthographicView(_ view: CameraMode) {
        mode = view
        
        withAnimation(.easeInOut(duration: 0.3)) {
            switch view {
            case .top:
                rotation = SIMD3(-90, 0, 0)
                position = SIMD3(0, 20, 0)
            case .front:
                rotation = SIMD3(0, 0, 0)
                position = SIMD3(0, 0, 20)
            case .right:
                rotation = SIMD3(0, 90, 0)
                position = SIMD3(20, 0, 0)
            case .left:
                rotation = SIMD3(0, -90, 0)
                position = SIMD3(-20, 0, 0)
            case .bottom:
                rotation = SIMD3(90, 0, 0)
                position = SIMD3(0, -20, 0)
            case .back:
                rotation = SIMD3(0, 180, 0)
                position = SIMD3(0, 0, -20)
            default:
                mode = .perspective
            }
        }
    }
    
    // MARK: - Camera Vectors
    
    func forward() -> SIMD3<Float> {
        let pitch = rotation.x * .pi / 180
        let yaw = rotation.y * .pi / 180
        
        return SIMD3(
            sin(yaw) * cos(pitch),
            -sin(pitch),
            cos(yaw) * cos(pitch)
        )
    }
    
    func right() -> SIMD3<Float> {
        let yaw = rotation.y * .pi / 180
        return SIMD3(
            cos(yaw),
            0,
            -sin(yaw)
        )
    }
    
    func up() -> SIMD3<Float> {
        return cross(right(), forward())
    }
    
    // MARK: - Matrices
    
    func viewMatrix() -> simd_float4x4 {
        let target = position + forward()
        return lookAt(eye: position, center: target, up: SIMD3(0, 1, 0))
    }
    
    func projectionMatrix(aspectRatio: Float) -> simd_float4x4 {
        switch mode {
        case .perspective:
            return perspectiveProjection(fov: fov * .pi / 180, aspectRatio: aspectRatio, near: 0.1, far: 1000)
        default:
            let height: Float = 10.0
            let width = height * aspectRatio
            return orthographicProjection(left: -width/2, right: width/2, bottom: -height/2, top: height/2, near: 0.1, far: 1000)
        }
    }
    
    // MARK: - Helper Functions
    
    private func lookAt(eye: SIMD3<Float>, center: SIMD3<Float>, up: SIMD3<Float>) -> simd_float4x4 {
        let z = normalize(eye - center)
        let x = normalize(cross(up, z))
        let y = cross(z, x)
        
        return simd_float4x4(
            SIMD4(x.x, y.x, z.x, 0),
            SIMD4(x.y, y.y, z.y, 0),
            SIMD4(x.z, y.z, z.z, 0),
            SIMD4(-dot(x, eye), -dot(y, eye), -dot(z, eye), 1)
        )
    }
    
    private func perspectiveProjection(fov: Float, aspectRatio: Float, near: Float, far: Float) -> simd_float4x4 {
        let tanHalfFov = tan(fov / 2)
        
        return simd_float4x4(
            SIMD4(1 / (aspectRatio * tanHalfFov), 0, 0, 0),
            SIMD4(0, 1 / tanHalfFov, 0, 0),
            SIMD4(0, 0, -(far + near) / (far - near), -1),
            SIMD4(0, 0, -(2 * far * near) / (far - near), 0)
        )
    }
    
    private func orthographicProjection(left: Float, right: Float, bottom: Float, top: Float, near: Float, far: Float) -> simd_float4x4 {
        return simd_float4x4(
            SIMD4(2 / (right - left), 0, 0, 0),
            SIMD4(0, 2 / (top - bottom), 0, 0),
            SIMD4(0, 0, -2 / (far - near), 0),
            SIMD4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1)
        )
    }
}

// MARK: - Camera Controls View

struct CameraControlsOverlay: View {
    @ObservedObject var camera: CameraController
    
    var body: some View {
        VStack {
            HStack {
                // Camera mode dropdown
                Menu {
                    Button("Perspective") {
                        camera.mode = .perspective
                    }
                    Button("Orthographic") {
                        camera.mode = .orthographic
                    }
                    
                    Divider()
                    
                    Button("Top (Numpad 7)") {
                        camera.setOrthographicView(.top)
                    }
                    Button("Front (Numpad 1)") {
                        camera.setOrthographicView(.front)
                    }
                    Button("Right (Numpad 3)") {
                        camera.setOrthographicView(.right)
                    }
                } label: {
                    HStack {
                        Image(systemName: "camera")
                        Text(camera.mode.displayName)
                        Image(systemName: "chevron.down")
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
                    .cornerRadius(4)
                }
                
                Spacer()
                
                // Camera info
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Pos: \(formatVector(camera.position))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("Rot: \(formatVector(camera.rotation))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
                .cornerRadius(4)
            }
            .padding(DesignSystem.Spacing.sm)
            
            Spacer()
            
            // Controls hint
            VStack(alignment: .leading, spacing: 4) {
                Text("Camera Controls")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text("WASD - Move")
                Text("Q/E - Down/Up")
                Text("Right Click + Drag - Orbit")
                Text("Middle Click + Drag - Pan")
                Text("Scroll - Zoom")
                Text("F - Focus on Selection")
            }
            .font(DesignSystem.Typography.small)
            .foregroundColor(DesignSystem.Colors.textSecondary)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
            .cornerRadius(8)
            .padding(DesignSystem.Spacing.md)
        }
    }
    
    private func formatVector(_ vec: SIMD3<Float>) -> String {
        String(format: "(%.1f, %.1f, %.1f)", vec.x, vec.y, vec.z)
    }
}

// MARK: - Preview

#Preview {
    ZStack {
        Color.black
        
        CameraControlsOverlay(camera: CameraController())
    }
}
