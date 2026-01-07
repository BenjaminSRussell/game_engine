import SwiftUI

// MARK: - Cinematic Tools Panel

struct CinematicToolsPanel: View {
    @StateObject private var viewModel = CinematicToolsViewModel()
    @State private var selectedPathId: UInt64?
    @State private var timelineScale: CGFloat = 50.0 // Pixels per second
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Camera Director")
                    .font(DesignSystem.Typography.title1)
                
                Spacer()
                
                EditorButton("New Path", icon: "plus") {
                    viewModel.createNewPath()
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Main Content: Split View (Paths List | Timeline)
            HSplitView {
                // Left: Path List
                VStack(spacing: 0) {
                    List(viewModel.paths, selection: $selectedPathId) { path in
                        HStack {
                            Image(systemName: "video.fill")
                                .foregroundColor(selectedPathId == path.id ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            VStack(alignment: .leading) {
                                Text(path.name)
                                    .font(DesignSystem.Typography.bodyBold)
                                Text("\(path.keyframeCount) keyframes")
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            Spacer()
                            if selectedPathId == path.id {
                                EditorIconButton(icon: "play.circle", tooltip: "Play Path") {
                                    viewModel.playPath(path.id)
                                }
                            }
                        }
                        .padding(.vertical, 4)
                        .tag(path.id)
                    }
                    .frame(minWidth: 200, maxWidth: 300)
                }
                
                // Right: Timeline Editor
                if let pathId = selectedPathId, let path = viewModel.getPath(id: pathId) {
                    VStack(spacing: 0) {
                        // Timeline Controls
                        HStack {
                            Text(path.name)
                                .font(DesignSystem.Typography.sectionLabel)
                            Spacer()
                            
                            Toggle("Loop", isOn: Binding(
                                get: { path.loop },
                                set: { viewModel.setLooping(pathId: pathId, enabled: $0) }
                            ))
                            
                            EditorIconButton(icon: "bolt.horizontal.circle", tooltip: "Add Keyframe from Camera") {
                                // Snap to current view
                                viewModel.addKeyframeFromCamera(pathId: pathId)
                            }
                            .help("Add Keyframe at Current View")
                        }
                        .padding(DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        
                        EditorDivider()
                        
                        // Timeline Visualization
                        ScrollView(.horizontal) {
                            ZStack(alignment: .leading) {
                                // Time Ruler
                                VStack(spacing: 0) {
                                    HStack(spacing: 0) {
                                        ForEach(0..<20) { second in
                                            Text(String(second) + "s")
                                                .font(.caption2)
                                                .frame(width: timelineScale, alignment: .leading)
                                        }
                                    }
                                    .frame(height: 20)
                                    .background(DesignSystem.Colors.backgroundSecondary)
                                    
                                    // Track Area
                                    Rectangle()
                                        .fill(DesignSystem.Colors.backgroundPrimary)
                                        .frame(height: 100)
                                        .frame(width: timelineScale * 20)
                                }
                                
                                // Keyframes
                                ForEach(viewModel.keyframes[pathId] ?? []) { keyframe in
                                    CinematicKeyframeMarker(keyframe: keyframe)
                                        .position(x: CGFloat(keyframe.time) * timelineScale, y: 30)
                                }
                                
                                // Playhead
                                Rectangle()
                                    .fill(Color.red)
                                    .frame(width: 1, height: 120)
                                    .offset(x: CGFloat(viewModel.currentTime) * timelineScale)
                            }
                        }
                        
                        EditorDivider()
                        
                        // Selected Keyframe Properties
                        if let keyframe = viewModel.selectedKeyframe {
                            KeyframeInspector(keyframe: keyframe, viewModel: viewModel)
                                .frame(height: 200)
                        } else {
                            Text("Select a keyframe to edit properties")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                                .frame(height: 200)
                        }
                    }
                } else {
                    Text("Select or create a camera path")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
            }
        }
    }
}

// MARK: - Subcomponents

struct CinematicKeyframeMarker: View {
    let keyframe: CameraKeyframeModel
    
    var body: some View {
        VStack(spacing: 0) {
            Image(systemName: "diamond.fill")
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .font(.system(size: 10))
            
            Rectangle()
                .fill(DesignSystem.Colors.accentPrimary.opacity(0.5))
                .frame(width: 1, height: 100)
        }
        .padding(.top, 20) // Offset for ruler
    }
}

struct KeyframeInspector: View {
    let keyframe: CameraKeyframeModel
    @ObservedObject var viewModel: CinematicToolsViewModel
    
    // Local bindings
    @State private var time: Float
    @State private var fov: Float
    
    init(keyframe: CameraKeyframeModel, viewModel: CinematicToolsViewModel) {
        self.keyframe = keyframe
        self.viewModel = viewModel
        _time = State(initialValue: keyframe.time)
        _fov = State(initialValue: keyframe.fov)
    }
    
    var body: some View {
        VStack(alignment: .leading) {
            Text("Keyframe Properties")
                .font(DesignSystem.Typography.sectionLabel)
                .padding(.bottom, 8)
            
            Grid {
                GridRow {
                    Text("Time (s):")
                    TextField("", value: $time, formatter: NumberFormatter())
                        .onSubmit { viewModel.updateKeyframe(keyframe.id, time: time) }
                }
                GridRow {
                    Text("FOV:")
                    EditorNumericField(label: "", value: $fov, range: 30...120)
                    // Note: Ideally would bind directly but keeping simple
                }
            }
        }
        .padding()
        .onChange(of: keyframe) { newItem in
            time = newItem.time
            fov = newItem.fov
        }
    }
}

// MARK: - View Model

struct CameraPathModel: Identifiable, Hashable {
    let id: UInt64
    var name: String
    var keyframeCount: Int
    var loop: Bool
}

struct CameraKeyframeModel: Identifiable, Equatable {
    let id: UInt64
    var time: Float
    var position: SIMD3<Float>
    var rotation: SIMD3<Float>
    var fov: Float
}

class CinematicToolsViewModel: ObservableObject {
    @Published var paths: [CameraPathModel] = []
    @Published var keyframes: [UInt64: [CameraKeyframeModel]] = [:] // PathID -> Keyframes
    @Published var currentTime: Float = 0.0
    @Published var selectedKeyframe: CameraKeyframeModel?
    
    init() {
        refreshPaths()
    }
    
    func refreshPaths() {
        // Mock data for now until connected to real list API
        // Real impl: Loop through CameraDirectorAPI path IDs
        if paths.isEmpty {
            paths = [
                CameraPathModel(id: 1, name: "Intro Sequence", keyframeCount: 3, loop: false),
                CameraPathModel(id: 2, name: "Flyover", keyframeCount: 5, loop: true)
            ]
            
            keyframes[1] = [
                CameraKeyframeModel(id: 101, time: 0.0, position: .zero, rotation: .zero, fov: 90),
                CameraKeyframeModel(id: 102, time: 2.0, position: .zero, rotation: .zero, fov: 80),
                CameraKeyframeModel(id: 103, time: 5.0, position: .zero, rotation: .zero, fov: 90)
            ]
        }
    }
    
    func createNewPath() {
        let newId = CameraDirectorAPI.shared.createPath(name: "New Path")
        refreshPaths()
        // Note: New path ID returned from C is needed here
        let model = CameraPathModel(id: newId, name: "New Path", keyframeCount: 0, loop: false)
        paths.append(model)
        keyframes[newId] = []
    }
    
    func getPath(id: UInt64) -> CameraPathModel? {
        return paths.first { $0.id == id }
    }
    
    func playPath(_ id: UInt64) {
        // CameraDirectorAPI.shared.playPath(pathId: id) // API not available
    }
    
    func setLooping(pathId: UInt64, enabled: Bool) {
        // Find path index
        if let idx = paths.firstIndex(where: { $0.id == pathId }) {
            paths[idx].loop = enabled
            // Call API
        }
    }
    
    func addKeyframeFromCamera(pathId: UInt64) {
        // Get current camera transform (mock)
        let pos = SIMD3<Float>(0, 10, 0)
        let rot = SIMD3<Float>(0, 0, 0)
        let fov: Float = 90.0
        
        // Use last keyframe time + 2s or 0
        let time = (keyframes[pathId]?.last?.time ?? -2.0) + 2.0
        
        CameraDirectorAPI.shared.addKeyframe(
            to: pathId,
            time: time,
            position: (pos.x, pos.y, pos.z),
            lookAt: (0, 0, 0) // Placeholder for LookAt
        )
        
        // Update local model
        let newKeyframe = CameraKeyframeModel(id: UInt64.random(in: 1000...9999), time: time, position: pos, rotation: rot, fov: fov)
        keyframes[pathId, default: []].append(newKeyframe)
        
        // Update path count
        if let idx = paths.firstIndex(where: { $0.id == pathId }) {
            paths[idx].keyframeCount += 1
        }
    }
    
    func updateKeyframe(_ id: UInt64, time: Float) {
        // Update model and API
    }
}
