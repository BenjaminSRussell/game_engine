import SwiftUI
import simd

// MARK: - Viewport Layout Manager
// Manages multi-viewport configurations and layouts

class ViewportLayoutManager: ObservableObject {
    @Published var currentLayout: ViewportLayout = .single
    @Published var viewports: [ViewportConfiguration] = [ViewportConfiguration.default]
    @Published var syncedCameras = false
    @Published var activeViewportIndex = 0
    
    // Layout presets
    static let layoutPresets: [String: ViewportLayout] = [
        "Single": .single,
        "Horizontal Split": .horizontalSplit,
        "Vertical Split": .verticalSplit,
        "Quad (2x2)": .quad,
        "Three Vertical": .threeVertical,
        "Three Horizontal": .threeHorizontal,
        "Picture-in-Picture": .pip
    ]
    
    // MARK: - Viewport Layout
    enum ViewportLayout: String, CaseIterable, Identifiable {
        // TODO: Implement single viewport
        case single = "Single"
        // TODO: Implement horizontal split (2 viewports)
        case horizontalSplit = "Horizontal Split"
        // TODO: Implement vertical split (2 viewports)
        case verticalSplit = "Vertical Split"
        // TODO: Implement quad layout (2x2, 4 viewports)
        case quad = "Quad (2x2)"
        // TODO: Implement three vertical layout
        case threeVertical = "Three Vertical"
        // TODO: Implement three horizontal layout
        case threeHorizontal = "Three Horizontal"
        // TODO: Implement picture-in-picture
        case pip = "Picture in Picture"
        // TODO: Implement custom layout
        case custom = "Custom"
        
        var id: String { rawValue }
        
        var viewportCount: Int {
            switch self {
            case .single: return 1
            case .horizontalSplit, .verticalSplit: return 2
            case .threeVertical, .threeHorizontal: return 3
            case .quad: return 4
            case .pip: return 2
            case .custom: return 1
            }
        }
        
        var icon: String {
            switch self {
            case .single: return "rectangle"
            case .horizontalSplit: return "rectangle.split.2x1"
            case .verticalSplit: return "rectangle.split.1x2"
            case .quad: return "rectangle.split.2x2"
            case .threeVertical: return "rectangle.split.3x1"
            case .threeHorizontal: return "rectangle.split.1x3"
            case .pip: return "pip"
            case .custom: return "rectangle.grid.1x2"
            }
        }
    }
    
    // MARK: - Viewport Configuration
    struct ViewportConfiguration: Identifiable {
        let id = UUID()
        var name: String
        var cameraPreset: CameraPreset
        var renderingMode: RenderingModeManager.RenderingMode
        var overlays: Set<RenderingModeManager.RenderingOverlay>
        var showGrid: Bool
        var showGizmos: Bool
        var showStats: Bool
        
        static var `default`: ViewportConfiguration {
            ViewportConfiguration(
                name: "Perspective",
                cameraPreset: .perspective,
                renderingMode: .lit,
                overlays: [],
                showGrid: true,
                showGizmos: true,
                showStats: false
            )
        }
        
        // TODO: Save viewport configuration to file
        // TODO: Load viewport configuration from file
    }
    
    // MARK: - Camera Presets
    enum CameraPreset: String, CaseIterable {
        case perspective = "Perspective"
        case top = "Top"
        case bottom = "Bottom"
        case front = "Front"
        case back = "Back"
        case left = "Left"
        case right = "Right"
        case isometric = "Isometric"
        
        var icon: String {
            switch self {
            case .perspective: return "cube.transparent"
            case .top: return "arrow.down"
            case .bottom: return "arrow.up"
            case .front: return "arrow.forward"
            case .back: return "arrow.backward"
            case .left: return "arrow.left"
            case .right: return "arrow.right"
            case .isometric: return "cube"
            }
        }
        
        // TODO: Calculate camera position and rotation for each preset
        func getCameraTransform() -> (position: SIMD3<Float>, rotation: SIMD3<Float>) {
            switch self {
            case .perspective:
                return (SIMD3(5, 5, 5), SIMD3(-30, 45, 0))
            case .top:
                return (SIMD3(0, 10, 0), SIMD3(-90, 0, 0))
            case .bottom:
                return (SIMD3(0, -10, 0), SIMD3(90, 0, 0))
            case .front:
                return (SIMD3(0, 0, 10), SIMD3(0, 0, 0))
            case .back:
                return (SIMD3(0, 0, -10), SIMD3(0, 180, 0))
            case .left:
                return (SIMD3(-10, 0, 0), SIMD3(0, -90, 0))
            case .right:
                return (SIMD3(10, 0, 0), SIMD3(0, 90, 0))
            case .isometric:
                return (SIMD3(5, 5, 5), SIMD3(-35.264, 45, 0))
            }
        }
    }
    
    // MARK: - Functions
    
    func setLayout(_ layout: ViewportLayout) {
        currentLayout = layout
        regenerateViewports()
        print("[ViewportLayout] Set layout to: \(layout.rawValue)")
    }
    
    // TODO: Regenerate viewports based on layout
    private func regenerateViewports() {
        let count = currentLayout.viewportCount
        
        // Keep existing viewports if possible, create new ones as needed
        if viewports.count < count {
            while viewports.count < count {
                viewports.append(ViewportConfiguration.default)
            }
        } else if viewports.count > count {
            viewports = Array(viewports.prefix(count))
        }
        
        // Configure default presets for standard layouts
        switch currentLayout {
        case .single:
            break // Keep existing
        case .horizontalSplit, .verticalSplit:
            if viewports.count >= 2 {
                viewports[0].cameraPreset = .perspective
                viewports[1].cameraPreset = .top
            }
        case .quad:
            if viewports.count >= 4 {
                viewports[0].cameraPreset = .perspective
                viewports[1].cameraPreset = .top
                viewports[2].cameraPreset = .front
                viewports[3].cameraPreset = .right
            }
        case .threeVertical, .threeHorizontal:
            if viewports.count >= 3 {
                viewports[0].cameraPreset = .perspective
                viewports[1].cameraPreset = .top
                viewports[2].cameraPreset = .front
            }
        case .pip:
            if viewports.count >= 2 {
                viewports[0].cameraPreset = .perspective
                viewports[1].cameraPreset = .top
            }
        case .custom:
            break
        }
    }
    
    // TODO: Implement synchronized camera movement across viewports
    func setSyncedCameras(_ synced: Bool) {
        syncedCameras = synced
        print("[ViewportLayout] Camera sync: \(synced ? "enabled" : "disabled")")
    }
    
    // TODO: Implement viewport cloning
    func cloneViewport(_ index: Int) {
        guard index < viewports.count else { return }
        let clone = viewports[index]
        viewports.append(clone)
        print("[ViewportLayout] Cloned viewport \(index)")
    }
    
    // TODO: Implement viewport layout saving/loading
    func saveLayout(name: String) {
        print("[ViewportLayout] Saved layout: \(name)")
        // TODO: Persist to UserDefaults or file
    }
    
    func loadLayout(name: String) {
        print("[ViewportLayout] Loaded layout: \(name)")
        // TODO: Load from UserDefaults or file
    }
    
    func setActiveViewport(_ index: Int) {
        guard index < viewports.count else { return }
        activeViewportIndex = index
    }
    
    func updateViewport(_ index: Int, configuration: ViewportConfiguration) {
        guard index < viewports.count else { return }
        viewports[index] = configuration
    }
}

// MARK: - Viewport Layout Selector UI
struct ViewportLayoutSelector: View {
    @ObservedObject var manager: ViewportLayoutManager
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Viewport Layout")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
            }
            .padding(8)
            
            EditorDivider()
            
            // Layout options
            LazyVGrid(columns: [
                GridItem(.adaptive(minimum: 80), spacing: 8)
            ], spacing: 8) {
                ForEach(ViewportLayoutManager.ViewportLayout.allCases) { layout in
                    LayoutButton(
                        layout: layout,
                        isSelected: manager.currentLayout == layout
                    ) {
                        manager.setLayout(layout)
                    }
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Sync cameras toggle
            Toggle("Sync Cameras", isOn: $manager.syncedCameras)
                .font(DesignSystem.Typography.body)
                .padding(8)
            
            EditorDivider()
            
            // Viewport configurations
            VStack(alignment: .leading, spacing: 8) {
                Text("Viewport Settings")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                ForEach(Array(manager.viewports.enumerated()), id: \.offset) { index, config in
                    ViewportConfigRow(
                        index: index,
                        configuration: config,
                        isActive: index == manager.activeViewportIndex,
                        onSelect: {
                            manager.setActiveViewport(index)
                        },
                        onChange: { newConfig in
                            manager.updateViewport(index, configuration: newConfig)
                        }
                    )
                }
            }
            .padding(8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Layout Button
private struct LayoutButton: View {
    let layout: ViewportLayoutManager.ViewportLayout
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Image(systemName: layout.icon)
                    .font(.system(size: 24))
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                
                Text(layout.rawValue)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .multilineTextAlignment(.center)
                    .lineLimit(2)
            }
            .frame(maxWidth: .infinity)
            .padding(8)
            .background(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Viewport Config Row
private struct ViewportConfigRow: View {
    let index: Int
    @State var configuration: ViewportLayoutManager.ViewportConfiguration
    let isActive: Bool
    let onSelect: () -> Void
    let onChange: (ViewportLayoutManager.ViewportConfiguration) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Button(action: onSelect) {
                HStack {
                    Circle()
                        .fill(isActive ? DesignSystem.Colors.accentPrimary : Color.clear)
                        .frame(width: 8, height: 8)
                    
                    Text("Viewport \(index + 1)")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Text(configuration.cameraPreset.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .buttonStyle(.plain)
            
            if isActive {
                VStack(spacing: 4) {
                    Menu {
                        ForEach(ViewportLayoutManager.CameraPreset.allCases, id: \.self) { preset in
                            Button(action: {
                                configuration.cameraPreset = preset
                                onChange(configuration)
                            }) {
                                HStack {
                                    Image(systemName: preset.icon)
                                    Text(preset.rawValue)
                                }
                            }
                        }
                    } label: {
                        HStack {
                            Text("Camera:")
                                .font(DesignSystem.Typography.small)
                            Text(configuration.cameraPreset.rawValue)
                                .font(DesignSystem.Typography.small)
                            Image(systemName: "chevron.down")
                                .font(.system(size: 8))
                        }
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    HStack(spacing: 8) {
                        Toggle("Grid", isOn: $configuration.showGrid)
                            .font(DesignSystem.Typography.small)
                            .toggleStyle(.checkbox)
                            .onChange(of: configuration.showGrid) { _ in
                                onChange(configuration)
                            }
                        
                        Toggle("Gizmos", isOn: $configuration.showGizmos)
                            .font(DesignSystem.Typography.small)
                            .toggleStyle(.checkbox)
                            .onChange(of: configuration.showGizmos) { _ in
                                onChange(configuration)
                            }
                        
                        Toggle("Stats", isOn: $configuration.showStats)
                            .font(DesignSystem.Typography.small)
                            .toggleStyle(.checkbox)
                            .onChange(of: configuration.showStats) { _ in
                                onChange(configuration)
                            }
                    }
                }
                .padding(.leading, 16)
            }
        }
        .padding(4)
        .background(isActive ? DesignSystem.Colors.selection.opacity(0.3) : Color.clear)
        .cornerRadius(4)
    }
}
