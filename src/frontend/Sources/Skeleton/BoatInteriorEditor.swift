import SwiftUI
import simd

// MARK: - First-Person Interior Editor
/// Game-like boat interior editor with FPS controls

@available(macOS 14.0, *)
struct BoatInteriorEditorView: View {
    @StateObject private var builder = BoatBuilderSystem.shared
    @StateObject private var assetLibrary = InteriorAssetLibrary.shared
    
    // Camera state
    @State private var cameraPosition: SIMD3<Float> = SIMD3<Float>(0, 1.6, 0)  // Eye height
    @State private var cameraYaw: Float = 0  // Horizontal rotation (radians)
    @State private var cameraPitch: Float = 0  // Vertical rotation (radians)
    @State private var cameraVelocity: SIMD3<Float> = .zero
    
    // Editor state
    @State private var movementSpeed: Float = 2.0  // m/s
    @State private var mouseSensitivity: Float = 0.002
    @State private var placementMode: Bool = false
    @State private var selectedAsset: InteriorAsset?
    @State private var ghostPosition: SIMD3<Float>?
    @State private var ghostRotation: Float = 0
    @State private var showAssetBrowser: Bool = true
    @State private var selectedCategory: InteriorAsset.AssetCategory = .seating
    
    // Performance tracking
    @State private var fps: Int = 60
    @State private var lastFrameTime: Date = Date()
    
    var body: some View {
        ZStack {
            // 3D Viewport (would integrate with Metal/Vulkan)
            Color.black
                .overlay(
                    Text("3D Viewport\nCamera: \(formatVector(cameraPosition))\nYaw: \(Int(cameraYaw * 180 / .pi)) Pitch: \(Int(cameraPitch * 180 / .pi))")
                        .foregroundColor(.white)
                        .font(.system(.caption, design: .monospaced))
                )
                .onAppear {
                    startGameLoop()
                }
            
            // Crosshair
            Circle()
                .stroke(placementMode ? Color.green : Color.white, lineWidth: 2)
                .frame(width: 20, height: 20)
            
            // HUD Overlay
            VStack(alignment: .leading, spacing: 0) {
                // Top HUD
                HStack {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("FPS: \(fps)")
                            .font(.system(.caption, design: .monospaced))
                        Text("Pos: \(formatVector(cameraPosition))")
                            .font(.system(.caption2, design: .monospaced))
                        Text("Placed: \(builder.placedComponents.count) items")
                            .font(.system(.caption2, design: .monospaced))
                    }
                    .padding(8)
                    .background(Color.black.opacity(0.7))
                    .cornerRadius(8)
                    
                    Spacer()
                    
                    if placementMode, let asset = selectedAsset {
                        VStack {
                            Text("Placing: \(asset.name)")
                                .font(.headline)
                            Text("Left Click: Place | R: Rotate | ESC: Cancel")
                                .font(.caption)
                        }
                        .padding()
                        .background(Color.green.opacity(0.8))
                        .cornerRadius(8)
                    }
                    
                    Spacer()
                    
                    // Controls hint
                    VStack(alignment: .trailing, spacing: 2) {
                        Text("WASD: Move")
                        Text("Space/Shift: Up/Down")
                        Text("Mouse: Look")
                        Text("Tab: Asset Browser")
                    }
                    .font(.caption2)
                    .padding(8)
                    .background(Color.black.opacity(0.7))
                    .cornerRadius(8)
                }
                .padding()
                
                Spacer()
                
                // Asset Browser (bottom)
                if showAssetBrowser && !placementMode {
                    AssetBrowserPanel(
                        selectedCategory: $selectedCategory,
                        selectedAsset: $selectedAsset,
                        onSelectAsset: { asset in
                            enterPlacementMode(asset: asset)
                        }
                    )
                    .frame(height: 200)
                }
            }
            
            // Snap point indicators (visible when near)
            ForEach(builder.snapPoints) { snapPoint in
                if distance(snapPoint.localPosition, cameraPosition) < 5.0 {
                    SnapPointIndicator(snapPoint: snapPoint)
                }
            }
        }
        .background(Color.black)
        .onKeyPress(KeyEquivalent("w")) { moveForward(); return .handled }
        .onKeyPress(KeyEquivalent("s")) { moveBackward(); return .handled }
        .onKeyPress(KeyEquivalent("a")) { moveLeft(); return .handled }
        .onKeyPress(KeyEquivalent("d")) { moveRight(); return .handled }
        .onKeyPress(.space) { moveUp(); return .handled }
        // .onKeyPress(.leftShift) { moveDown(); return .handled } // SwiftUI doesn't support shift as KeyEquivalent
        .onKeyPress(KeyEquivalent("r")) { rotateGhost(); return .handled }
        .onKeyPress(.escape) { exitPlacementMode(); return .handled }
        .onKeyPress(.tab) { showAssetBrowser.toggle(); return .handled }
    }
    
    // MARK: - Movement Functions
    
    private func moveForward() {
        let forward = getCameraForward()
        cameraVelocity.x = forward.x * movementSpeed
        cameraVelocity.z = forward.z * movementSpeed
    }
    
    private func moveBackward() {
        let forward = getCameraForward()
        cameraVelocity.x = -forward.x * movementSpeed
        cameraVelocity.z = -forward.z * movementSpeed
    }
    
    private func moveLeft() {
        let right = getCameraRight()
        cameraVelocity.x = -right.x * movementSpeed
        cameraVelocity.z = -right.z * movementSpeed
    }
    
    private func moveRight() {
        let right = getCameraRight()
        cameraVelocity.x = right.x * movementSpeed
        cameraVelocity.z = right.z * movementSpeed
    }
    
    private func moveUp() {
        cameraVelocity.y = movementSpeed
    }
    
    private func moveDown() {
        cameraVelocity.y = -movementSpeed
    }
    
    private func getCameraForward() -> SIMD3<Float> {
        return SIMD3<Float>(
            sin(cameraYaw),
            0,  // No vertical component for horizontal movement
            cos(cameraYaw)
        )
    }
    
    private func getCameraRight() -> SIMD3<Float> {
        return SIMD3<Float>(
            cos(cameraYaw),
            0,
            -sin(cameraYaw)
        )
    }
    
    private func getCameraDirection() -> SIMD3<Float> {
        return SIMD3<Float>(
            cos(cameraPitch) * sin(cameraYaw),
            sin(cameraPitch),
            cos(cameraPitch) * cos(cameraYaw)
        )
    }
    
    // MARK: - Placement Functions
    
    private func enterPlacementMode(asset: InteriorAsset) {
        selectedAsset = asset
        placementMode = true
        updateGhostPosition()
    }
    
    private func exitPlacementMode() {
        placementMode = false
        selectedAsset = nil
        ghostPosition = nil
    }
    
    private func rotateGhost() {
        ghostRotation += .pi / 4  // 45 degree increments
        if ghostRotation >= 2 * .pi {
            ghostRotation = 0
        }
    }
    
    private func updateGhostPosition() {
        guard placementMode else { return }
        
        // Raycast from camera
        let ray = BoatRay(
            origin: cameraPosition,
            direction: getCameraDirection()
        )
        
        // Find intersection with boat floor/surfaces
        if let hitPoint = raycastToBoat(ray) {
            if selectedAsset?.snapToGrid == true {
                ghostPosition = snapToGrid(hitPoint, gridSize: selectedAsset?.gridSize ?? 0.25)
            } else {
                ghostPosition = hitPoint
            }
        }
    }
    
    private func placeAsset() {
        guard let asset = selectedAsset,
              let position = ghostPosition else { return }
        
        // Try to snap to nearest snap point
        if let snapPoint = builder.findNearestSnapPoint(position: position, type: asset.snapType) {
            var component: any BoatComponent = asset
            builder.snapComponent(&component, to: snapPoint)
            // No need to update asset back since it's a value type in this context
        } else {
            // Place without snap point (freeform placement)
            builder.placedComponents[asset.id] = asset
        }
        
        // Stay in placement mode for rapid placement
        if let originalAsset = selectedAsset {
            selectedAsset = originalAsset  // Create new instance
        }
    }
    
    // MARK: - Utility Functions
    
    private func startGameLoop() {
        Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { _ in
            update(deltaTime: 1.0/60.0)
        }
    }
    
    private func update(deltaTime: Float) {
        // Update camera position
        cameraPosition += cameraVelocity * deltaTime
        
        // Apply damping to velocity
        cameraVelocity *= 0.8
        
        // Update ghost position if in placement mode
        if placementMode {
            updateGhostPosition()
        }
        
        // Update FPS
        let now = Date()
        fps = Int(1.0 / now.timeIntervalSince(lastFrameTime))
        lastFrameTime = now
    }
    
    private func raycastToBoat(_ ray: BoatRay) -> SIMD3<Float>? {
        // Simplified raycast - intersect with ground plane
        // In full implementation, this would test against boat geometry
        let groundY: Float = 0.0
        
        if ray.direction.y != 0 {
            let t = (groundY - ray.origin.y) / ray.direction.y
            if t > 0 {
                return ray.origin + ray.direction * t
            }
        }
        
        return nil
    }
    
    private func snapToGrid(_ position: SIMD3<Float>, gridSize: Float) -> SIMD3<Float> {
        return SIMD3<Float>(
            round(position.x / gridSize) * gridSize,
            position.y,  // Don't snap Y (height)
            round(position.z / gridSize) * gridSize
        )
    }
    
    private func formatVector(_ vec: SIMD3<Float>) -> String {
        String(format: "(%.1f, %.1f, %.1f)", vec.x, vec.y, vec.z)
    }
}

// MARK: - Ray Helper

struct BoatRay {
    var origin: SIMD3<Float>
    var direction: SIMD3<Float>
}

// MARK: - Asset Browser Panel

struct AssetBrowserPanel: View {
    @Binding var selectedCategory: InteriorAsset.AssetCategory
    @Binding var selectedAsset: InteriorAsset?
    let onSelectAsset: (InteriorAsset) -> Void
    
    @StateObject private var library = InteriorAssetLibrary.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Category tabs
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(InteriorAsset.AssetCategory.allCases, id: \.self) { category in
                        Button {
                            selectedCategory = category
                        } label: {
                            Text(category.rawValue)
                                .padding(.horizontal, 12)
                                .padding(.vertical, 6)
                                .background(selectedCategory == category ? Color.blue : Color.gray.opacity(0.3))
                                .foregroundColor(.white)
                                .cornerRadius(8)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal)
            }
            .padding(.vertical, 8)
            .background(Color.black.opacity(0.7))
            
            // Asset grid
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 12) {
                    ForEach(library.getAssets(for: selectedCategory)) { asset in
                        AssetCard(asset: asset) {
                            selectedAsset = asset
                            onSelectAsset(asset)
                        }
                    }
                }
                .padding()
            }
            .background(Color.black.opacity(0.9))
        }
    }
}

struct AssetCard: View {
    let asset: InteriorAsset
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 4) {
                // Placeholder icon
                Image(systemName: asset.snapType.icon)
                    .font(.system(size: 40))
                    .foregroundColor(.white)
                    .frame(width: 80, height: 80)
                    .background(asset.snapType.color.opacity(0.3))
                    .cornerRadius(8)
                
                Text(asset.name)
                    .font(.caption)
                    .foregroundColor(.white)
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
                    .frame(width: 80)
                
                Text("\(Int(asset.mass))kg")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }
            .padding(8)
            .background(Color.gray.opacity(0.2))
            .cornerRadius(12)
        }
        .buttonStyle(.plain)
    }
}

struct SnapPointIndicator: View {
    let snapPoint: SnapPoint
    
    var body: some View {
        Circle()
            .fill(snapPoint.indicatorColor.opacity(0.8))
            .frame(width: 30, height: 30)
            .overlay(
                Image(systemName: snapPoint.type.icon)
                    .foregroundColor(.white)
                    .font(.system(size: 14))
            )
            // Position would be calculated based on 3D->2D projection
    }
}
