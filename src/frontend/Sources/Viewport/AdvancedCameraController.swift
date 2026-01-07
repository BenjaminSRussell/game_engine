import SwiftUI
import simd

// MARK: - Advanced Camera Controller
// Provides professional-grade camera controls for viewport navigation

class AdvancedCameraController: ObservableObject {
    @Published var cameraMode: CameraMode = .orbit
    @Published var cameraSpeed: Float = 1.0
    @Published var fieldOfView: Float = 60.0
    @Published var nearPlane: Float = 0.1
    @Published var farPlane: Float = 1000.0
    
    // Camera bookmarks
    @Published var bookmarks: [CameraBookmark] = []
    
    // Camera path recording
    @Published var isRecordingPath = false
    @Published var recordedPath: [CameraKeyframe] = []
    
    // Camera settings
    @Published var enableInertia = true
    @Published var inertiaDamping: Float = 0.9
    @Published var enableAcceleration = true
    @Published var maxSpeed: Float = 10.0
    
    // Follow target
    @Published var followTarget: UUID?
    @Published var followDistance: Float = 5.0
    @Published var followHeight: Float = 2.0
    
    // MARK: - Camera Modes
    enum CameraMode: String, CaseIterable, Identifiable {
        // TODO: Implement orbital camera with zoom limits
        case orbit = "Orbit"
        // TODO: Implement free-fly camera with WASD + mouse
        case freeFly = "Free Fly"
        // TODO: Implement lock camera to selected object
        case locked = "Locked to Object"
        // TODO: Implement follow camera mode
        case follow = "Follow"
        // TODO: Implement first-person camera
        case firstPerson = "First Person"
        // TODO: Implement third-person camera
        case thirdPerson = "Third Person"
        // TODO: Implement top-down orthographic
        case topDown = "Top Down"
        // TODO: Implement side view orthographic
        case sideView = "Side View"
        // TODO: Implement front view orthographic
        case frontView = "Front View"
        // TODO: Implement isometric view
        case isometric = "Isometric"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .orbit: return "arrow.triangle2.circlepath.circle"
            case .freeFly: return "paperplane"
            case .locked: return "lock.circle"
            case .follow: return "person.fill.viewfinder"
            case .firstPerson: return "eye"
            case .thirdPerson: return "figure.walk"
            case .topDown: return "arrow.down.to.line"
            case .sideView: return "arrow.right.to.line"
            case .frontView: return "arrow.forward.to.line"
            case .isometric: return "cube.transparent"
            }
        }
    }
    
    // MARK: - Camera Bookmark
    struct CameraBookmark: Identifiable {
        let id = UUID()
        var name: String
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var fov: Float
        var timestamp: Date = Date()
        
        // TODO: Save bookmark to persistent storage
        // TODO: Load bookmark from persistent storage
    }
    
    // MARK: - Camera Keyframe
    struct CameraKeyframe {
        var timestamp: TimeInterval
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var fov: Float
        
        // TODO: Implement interpolation between keyframes
        // TODO: Support bezier curve interpolation
        // TODO: Support catmull-rom spline interpolation
    }
    
    // MARK: - Camera Speed Presets
    enum SpeedPreset: String, CaseIterable {
        case slow = "Slow (0.25x)"
        case normal = "Normal (1x)"
        case fast = "Fast (2x)"
        case veryFast = "Very Fast (5x)"
        case ultraFast = "Ultra Fast (10x)"
        
        var multiplier: Float {
            switch self {
            case .slow: return 0.25
            case .normal: return 1.0
            case .fast: return 2.0
            case .veryFast: return 5.0
            case .ultraFast: return 10.0
            }
        }
    }
    
    // MARK: - Functions
    
    // TODO: Implement orbital camera with zoom limits
    func updateOrbitCamera(distance: Float, angle: SIMD2<Float>) {
        print("[Camera] Orbit mode: distance=\(distance), angle=\(angle)")
    }
    
    // TODO: Implement free-fly camera with WASD + mouse
    func updateFreeFlyCameraMovement(forward: Float, right: Float, up: Float) {
        print("[Camera] Free fly movement")
    }
    
    // TODO: Implement camera bookmark system
    func saveBookmark(name: String, position: SIMD3<Float>, rotation: SIMD3<Float>) {
        let bookmark = CameraBookmark(name: name, position: position, rotation: rotation, fov: fieldOfView)
        bookmarks.append(bookmark)
        print("[Camera] Saved bookmark: \(name)")
    }
    
    func loadBookmark(_ bookmark: CameraBookmark) {
        // TODO: Animate camera to bookmark position
        print("[Camera] Loading bookmark: \(bookmark.name)")
    }
    
    func deleteBookmark(_ id: UUID) {
        bookmarks.removeAll { $0.id == id }
    }
    
    // TODO: Implement camera path recording
    func startPathRecording() {
        isRecordingPath = true
        recordedPath.removeAll()
        print("[Camera] Started path recording")
    }
    
    func stopPathRecording() {
        isRecordingPath = false
        print("[Camera] Stopped path recording (\(recordedPath.count) keyframes)")
    }
    
    func recordKeyframe(position: SIMD3<Float>, rotation: SIMD3<Float>, timestamp: TimeInterval) {
        guard isRecordingPath else { return }
        let keyframe = CameraKeyframe(timestamp: timestamp, position: position, rotation: rotation, fov: fieldOfView)
        recordedPath.append(keyframe)
    }
    
    // TODO: Implement camera path playback
    func playbackPath(speed: Float = 1.0) {
        print("[Camera] Playing back recorded path")
    }
    
    // TODO: Implement camera orbit around pivot
    func orbitAroundPoint(_ point: SIMD3<Float>, angularVelocity: Float) {
        print("[Camera] Orbiting around point: \(point)")
    }
    
    // TODO: Implement camera dolly zoom effect
    func dollyZoom(targetFov: Float, duration: TimeInterval) {
        print("[Camera] Dolly zoom to FOV: \(targetFov)")
    }
    
    // TODO: Implement camera roll control
    func setCameraRoll(_ roll: Float) {
        print("[Camera] Setting camera roll: \(roll)")
    }
    
    // TODO: Implement camera shake simulation
    func applyCameraShake(intensity: Float, frequency: Float, duration: TimeInterval) {
        print("[Camera] Applying camera shake")
    }
    
    // TODO: Implement handheld camera simulation
    func enableHandheldSimulation(intensity: Float) {
        print("[Camera] Enabling handheld simulation")
    }
    
    // TODO: Implement camera look-at target
    func lookAt(_ target: SIMD3<Float>, up: SIMD3<Float> = SIMD3(0, 1, 0)) {
        print("[Camera] Looking at target: \(target)")
    }
    
    // TODO: Implement frustumvisualization
    func getFrustumCorners() -> [SIMD3<Float>] {
        // Calculate 8 corners of camera frustum
        return []
    }
    
    func setSpeedPreset(_ preset: SpeedPreset) {
        cameraSpeed = preset.multiplier
    }
    
    func resetCamera() {
        cameraMode = .orbit
        cameraSpeed = 1.0
        fieldOfView = 60.0
        nearPlane = 0.1
        farPlane = 1000.0
        followTarget = nil
        print("[Camera] Reset to defaults")
    }
}

// MARK: - Camera Controls UI Panel
struct CameraControlsPanel: View {
    @ObservedObject var controller: AdvancedCameraController
    @State private var showingBookmarks = false
    @State private var showingPathRecorder = false
    @State private var newBookmarkName = ""
    
    var body: some View {
        VStack(spacing: 0) {
            // Camera mode selector
            HStack {
                Text("Camera Mode")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Menu {
                    ForEach(AdvancedCameraController.CameraMode.allCases) { mode in
                        Button(action: {
                            controller.cameraMode = mode
                        }) {
                            HStack {
                                Image(systemName: mode.icon)
                                Text(mode.rawValue)
                            }
                        }
                    }
                } label: {
                    HStack {
                        Image(systemName: controller.cameraMode.icon)
                        Text(controller.cameraMode.rawValue)
                        Image(systemName: "chevron.down")
                            .font(.system(size: 10))
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Camera settings
            VStack(spacing: 8) {
                // Speed
                HStack {
                    Text("Speed")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $controller.cameraSpeed, in: 0.1...10.0)
                    
                    Text(String(format: "%.1fx", controller.cameraSpeed))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
                
                // Field of View
                HStack {
                    Text("FOV")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $controller.fieldOfView, in: 30...120)
                    
                    Text("\(Int(controller.fieldOfView))°")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
                
              // Inertia toggle
                Toggle("Enable Inertia", isOn: $controller.enableInertia)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
                
                Toggle("Enable Acceleration", isOn: $controller.enableAcceleration)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
            }
            .padding(8)
            
            EditorDivider()
            
            // Bookmarks
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Bookmarks (\(controller.bookmarks.count))")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Button(action: {
                        showingBookmarks.toggle()
                    }) {
                        Image(systemName: showingBookmarks ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                    .buttonStyle(.plain)
                }
                
                if showingBookmarks {
                    ForEach(controller.bookmarks) { bookmark in
                        BookmarkRow(
                            bookmark: bookmark,
                            onLoad: {
                                controller.loadBookmark(bookmark)
                            },
                            onDelete: {
                                controller.deleteBookmark(bookmark.id)
                            }
                        )
                    }
                    
                    EditorButton("Save Current Position", icon: "plus", style: .primary) {
                        // TODO: Get current camera position from viewport
                        let position = SIMD3<Float>(0, 0, 0)
                        let rotation = SIMD3<Float>(0, 0, 0)
                        controller.saveBookmark(name: "Bookmark \(controller.bookmarks.count + 1)", position: position, rotation: rotation)
                    }
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Path Recording
            HStack {
                Text("Path Recording")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if controller.isRecordingPath {
                    Button(action: {
                        controller.stopPathRecording()
                    }) {
                        HStack {
                            Image(systemName: "stop.circle.fill")
                            Text("Stop")
                        }
                        .foregroundColor(.red)
                    }
                    .buttonStyle(.plain)
                } else {
                    Button(action: {
                        controller.startPathRecording()
                    }) {
                        HStack {
                            Image(systemName: "record.circle")
                            Text("Record")
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Bookmark Row
private struct BookmarkRow: View {
    let bookmark: AdvancedCameraController.CameraBookmark
    let onLoad: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(bookmark.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(bookmark.timestamp, style: .relative)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            Button(action: onLoad) {
                Image(systemName: "arrow.uturn.backward")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .foregroundColor(.red)
            }
            .buttonStyle(.plain)
        }
        .padding(4)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}
