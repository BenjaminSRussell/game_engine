import SwiftUI
import CEngineCore
import MetalKit

// MARK: - Premium Main Content View

@available(macOS 14.0, *)
struct ContentView: View {
    @State private var selectedTab: String = "engine"
    @State private var isShowingSearch = false
    
    var rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    init(rendererBridge: UnsafeMutablePointer<RendererBridge>? = nil) {
        self.rendererBridge = rendererBridge
    }
    
    var body: some View {
        NavigationSplitView {
            // Premium Sidebar
            PremiumSidebar(selectedTab: $selectedTab)
        } detail: {
            switch selectedTab {
            case "engine":
                EditorWorkspace(rendererBridge: rendererBridge)
            case "material":
                MaterialEditorPanel()
            case "lighting":
                LightingEditorView()
            case "audio":
                AudioEditorView()
            case "particle":
                ParticleEditorPanel()
            case "vfx_control":
                VFXControlPanel()
            case "physics":
                PhysicsEditorView()
            case "profiler":
                ProfilerPanel()
            case "vfx_optimization":
                VFXOptimizationPanel()
            case "optimization":
                OptimizationPanel()
            case "console":
                ConsolePanel()
            case "gameview":
                GameViewPanel()
            case "post_processing":
                PostProcessingPanel()
            case "cinematics":
                CinematicToolsPanel()
            case "gridsettings":
                GridSettingsPanel()
            case "animation":
                AnimationTimelineView()
            case "visualscript":
                if #available(macOS 14.0, *) {
                    NodeGraphView()
                } else {
                    UnavailableFeatureView(title: "Visual Scripting", requirement: "macOS 14.0+")
                }
            case "terrain":
                TerrainEditorView()
            case "weather":
                WeatherPanel()
            case "city":
                CityGenerationPanel()
            case "ecosystem":
                EcosystemPanel()
            case "world_building":
                WorldBuildingPanel()
            case "skeleton":
                SkeletonEditorView()
            case "uieditor":
                UIEditorView()
            case "prefab":
                PrefabEditorView()
            case "rendering":
                RenderingEditorView()
            case "collab":
                CollaborationPanel()
            case "build":
                BuildSettingsView()
            case "plugins":
                PluginManagerView()
            case "project":
                ProjectExplorerView()
            case "docs":
                DocumentationView()
            case "assets":
                AssetLibraryView()
            case "settings":
                SettingsView()
            case "music_timeline":
                MusicTimelinePanel()
            case "audio_daw":
                AudioDAWView()
            case "boat_editor":
                BoatInteriorEditorView()
            case "audio_animation":
                AudioAnimationPanel()
            case "tasks":
                TaskPanel()
            default:
                EmptyEditorPlaceholder()
            }
        }
        .frame(minWidth: 1200, minHeight: 800)
        .preferredColorScheme(.dark)
        .background(DesignSystem.Colors.backgroundPrimary)
        .overlay(NotificationOverlay())
        .sheet(isPresented: $isShowingSearch) {
            SearchCommandPalette()
        }
        .background(
            Button("") {
                isShowingSearch.toggle()
            }
            .keyboardShortcut("p", modifiers: .command)
            .opacity(0)
        )
    }
}

// MARK: - Premium Sidebar

private struct PremiumSidebar: View {
    @Binding var selectedTab: String
    
    var body: some View {
        VStack(spacing: 0) {
            // Premium Logo Header
            HStack(spacing: DesignSystem.Spacing.sm) {
                // Animated Logo Icon
                ZStack {
                    // Glow layer
                    Image(systemName: "cube.transparent.fill")
                        .font(.system(size: 28, weight: .semibold))
                        .foregroundStyle(
                            LinearGradient(
                                colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentSecondary],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                        .blur(radius: 8)
                        .opacity(0.5)
                    
                    // Main icon
                    Image(systemName: "cube.transparent.fill")
                        .font(.system(size: 28, weight: .semibold))
                        .foregroundStyle(
                            LinearGradient(
                                colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.gradientEnd],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                }
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("VoxelForge")
                        .font(.system(size: 15, weight: .semibold, design: .rounded))
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    HStack(spacing: 4) {
                        Text("Studio")
                            .font(.system(size: 11, weight: .medium))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Text("Pro")
                            .font(.system(size: 9, weight: .bold))
                            .foregroundColor(.white)
                            .padding(.horizontal, 5)
                            .padding(.vertical, 2)
                            .background(
                                Capsule()
                                    .fill(
                                        LinearGradient(
                                            colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentSecondary],
                                            startPoint: .leading,
                                            endPoint: .trailing
                                        )
                                    )
                            )
                    }
                }
                
                Spacer()
            }
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(
                DesignSystem.Colors.backgroundSecondary
                    .overlay(
                        LinearGradient(
                            colors: [Color.white.opacity(0.02), Color.clear],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
            )
            
            // Navigation List
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.xs) {
                    SidebarSection(title: "EDITOR") {
                        SidebarItem(icon: "cube.3d", title: "Scene", value: "engine", selectedTab: $selectedTab)
                    }
                    
                    SidebarSection(title: "TOOLS") {
                        SidebarItem(icon: "paintpalette.fill", title: "Materials", value: "material", selectedTab: $selectedTab)
                        SidebarItem(icon: "sun.max.fill", title: "Lighting", value: "lighting", selectedTab: $selectedTab)
                        SidebarItem(icon: "speaker.wave.3.fill", title: "Audio", value: "audio", selectedTab: $selectedTab)
                        SidebarItem(icon: "waveform.path.ecg", title: "Audio Animation", value: "audio_animation", selectedTab: $selectedTab)
                        SidebarItem(icon: "sparkles", title: "Particles", value: "particle", selectedTab: $selectedTab)
                        SidebarItem(icon: "sparkles.rectangle.stack.fill", title: "VFX Master Control", value: "vfx_control", selectedTab: $selectedTab)
                        SidebarItem(icon: "camera.filters", title: "Post-Processing", value: "post_processing", selectedTab: $selectedTab)
                        SidebarItem(icon: "video.fill", title: "Cinematics", value: "cinematics", selectedTab: $selectedTab)
                        SidebarItem(icon: "figure.walk", title: "Animation", value: "animation", selectedTab: $selectedTab)
                        SidebarItem(icon: "figure.stand", title: "Skeleton", value: "skeleton", selectedTab: $selectedTab)
                        SidebarItem(icon: "atom", title: "Physics", value: "physics", selectedTab: $selectedTab)
                        SidebarItem(icon: "point.3.connected.trianglepath.dotted", title: "Visual Scripting", value: "visualscript", selectedTab: $selectedTab)
                    }
                    
                     SidebarSection(title: "SYSTEMS") {
                        SidebarItem(icon: "cloud.sun.fill", title: "Weather", value: "weather", selectedTab: $selectedTab)
                        SidebarItem(icon: "building.2.fill", title: "City Generator", value: "city", selectedTab: $selectedTab)
                        SidebarItem(icon: "leaf.fill", title: "Ecosystem", value: "ecosystem", selectedTab: $selectedTab)
                        SidebarItem(icon: "globe.americas.fill", title: "World Building", value: "world_building", selectedTab: $selectedTab)
                        SidebarItem(icon: "mountain.2.fill", title: "Terrain", value: "terrain", selectedTab: $selectedTab)
                        SidebarItem(icon: "rectangle.3.group.fill", title: "UI Editor", value: "uieditor", selectedTab: $selectedTab)
                        SidebarItem(icon: "cube.transparent", title: "Prefabs", value: "prefab", selectedTab: $selectedTab)
                        SidebarItem(icon: "camera.filters", title: "Rendering", value: "rendering", selectedTab: $selectedTab)
                        SidebarItem(icon: "person.2.fill", title: "Collaboration", value: "collab", selectedTab: $selectedTab)
                    }
                    
                    SidebarSection(title: "PROJECT") {
                        SidebarItem(icon: "list.bullet.rectangle.portrait.fill", title: "Project Tasks", value: "tasks", selectedTab: $selectedTab)
                        SidebarItem(icon: "folder.fill", title: "Files", value: "project", selectedTab: $selectedTab)
                        SidebarItem(icon: "square.grid.2x2.fill", title: "Asset Library", value: "assets", selectedTab: $selectedTab)
                        SidebarItem(icon: "hammer.fill", title: "Build Settings", value: "build", selectedTab: $selectedTab)
                        SidebarItem(icon: "puzzlepiece.extension.fill", title: "Plugins", value: "plugins", selectedTab: $selectedTab)
                    }
                    
                    SidebarSection(title: "UTILITIES") {
                        SidebarItem(icon: "chart.xyaxis.line", title: "Profiler", value: "profiler", selectedTab: $selectedTab)
                        SidebarItem(icon: "speedometer", title: "VFX Performance", value: "vfx_optimization", selectedTab: $selectedTab)
                        SidebarItem(icon: "cpu.fill", title: "Optimization", value: "optimization", selectedTab: $selectedTab)
                        SidebarItem(icon: "terminal.fill", title: "Console", value: "console", selectedTab: $selectedTab)
                        SidebarItem(icon: "play.rectangle.fill", title: "Game View", value: "gameview", selectedTab: $selectedTab)
                    }
                    
                    SidebarSection(title: "HELP") {
                        SidebarItem(icon: "book.fill", title: "Documentation", value: "docs", selectedTab: $selectedTab)
                        SidebarItem(icon: "gearshape.fill", title: "Settings", value: "settings", selectedTab: $selectedTab)
                    }
                }
                .padding(.vertical, DesignSystem.Spacing.sm)
            }
        }
        .frame(minWidth: 220, maxWidth: 260)
        .background(.regularMaterial)
    }
}

// MARK: - Sidebar Components

private struct SidebarSection<Content: View>: View {
    let title: String
    @ViewBuilder let content: Content
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xxs) {
            Text(title)
                .font(DesignSystem.Typography.caption)
                .fontWeight(.semibold)
                .foregroundColor(DesignSystem.Colors.textTertiary)
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.top, DesignSystem.Spacing.sm)
            
            content
        }
    }
}

private struct SidebarItem: View {
    let icon: String
    let title: String
    let value: String
    @Binding var selectedTab: String
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    private var isSelected: Bool {
        selectedTab == value
    }
    
    var body: some View {
        Button(action: {
            withAnimation(.spring(response: 0.3, dampingFraction: 0.7)) {
                selectedTab = value
            }
        }) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                // Premium Selection Indicator (Animated pill)
                Capsule()
                    .fill(
                        isSelected 
                            ? LinearGradient(
                                colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentPrimary.opacity(0.8)],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                            : LinearGradient(colors: [Color.clear], startPoint: .top, endPoint: .bottom)
                    )
                    .frame(width: isSelected ? 4 : 2, height: isSelected ? 24 : 16)
                    .shadow(color: isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.4) : .clear, radius: 4, x: 0, y: 0)
                    .animation(.spring(response: 0.25, dampingFraction: 0.8), value: isSelected)
                
                // Icon with subtle glow when selected
                ZStack {
                    if isSelected {
                        Image(systemName: icon)
                            .font(.system(size: 16, weight: .semibold))
                            .foregroundColor(DesignSystem.Colors.accentPrimary.opacity(0.3))
                            .blur(radius: 6)
                    }
                    Image(systemName: icon)
                        .font(.system(size: 16, weight: isSelected ? .semibold : .medium))
                        .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : (isHovered ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary))
                        .scaleEffect(isPressed ? 0.9 : 1.0)
                }
                .frame(width: 22)
                .animation(.easeOut(duration: 0.15), value: isPressed)
                
                // Title with weight change on selection
                Text(title)
                    .font(.system(size: 13, weight: isSelected ? .medium : .regular))
                    .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : (isHovered ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary))
                
                Spacer()
                
                // Subtle chevron on hover (hidden when selected)
                if isHovered && !isSelected {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .transition(.opacity.combined(with: .scale(scale: 0.8)))
                }
            }
            .padding(.leading, 4)
            .padding(.trailing, DesignSystem.Spacing.md)
            .padding(.vertical, 10)
            .background(
                ZStack {
                    // Background fill
                    RoundedRectangle(cornerRadius: 8)
                        .fill(backgroundColor)
                    
                    // Subtle inner highlight on selection
                    if isSelected {
                        RoundedRectangle(cornerRadius: 8)
                            .strokeBorder(
                                LinearGradient(
                                    colors: [Color.white.opacity(0.1), Color.clear],
                                    startPoint: .top,
                                    endPoint: .bottom
                                ),
                                lineWidth: 1
                            )
                    }
                }
            )
            .shadow(
                color: isSelected ? Color.black.opacity(0.08) : .clear,
                radius: 2,
                x: 0,
                y: 1
            )
            .contentShape(Rectangle())
            .scaleEffect(isPressed ? 0.98 : 1.0)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(.easeOut(duration: 0.15)) {
                isHovered = hovering
            }
        }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0)
                .onChanged { _ in
                    withAnimation(.easeOut(duration: 0.1)) { isPressed = true }
                }
                .onEnded { _ in
                    withAnimation(.easeOut(duration: 0.1)) { isPressed = false }
                }
        )
    }
    
    private var backgroundColor: Color {
        if isSelected {
            return DesignSystem.Colors.selectionStrong
        } else if isHovered {
            return DesignSystem.Colors.hoverStrong
        }
        return Color.clear
    }
}

// MARK: - Helper Views

private struct UnavailableFeatureView: View {
    let title: String
    let requirement: String
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Image(systemName: "exclamationmark.triangle.fill")
                .font(.system(size: 48))
                .foregroundColor(DesignSystem.Colors.accentWarning)
            
            Text("\(title) Unavailable")
                .font(DesignSystem.Typography.title2)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text("Requires \(requirement)")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

private struct EmptyEditorPlaceholder: View {
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Image(systemName: "cube.transparent")
                .font(.system(size: 64, weight: .thin))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Text("Select an item from the sidebar")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Editor Workspace (Main 3-Panel Layout)

struct EditorWorkspace: View {
    @State private var hierarchyWidth: CGFloat = 280
    @State private var inspectorWidth: CGFloat = 320
    @State private var contentBrowserHeight: CGFloat = 250
    @StateObject private var selectionManager = SelectionManager()
    
    var rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    var body: some View {
        GeometryReader { geometry in
            VStack(spacing: 0) {
                // Top toolbar
                EditorMainToolbar()
                
                EditorDivider()
                
                // Main 3-panel layout
                HStack(spacing: 0) {
                    // Left: Scene Hierarchy
                    SceneHierarchyPanel()
                        .frame(width: hierarchyWidth)
                        .environmentObject(selectionManager)
                    
                    EditorDivider()
                    
                    // Center: Viewport + Bottom Content Browser
                    VStack(spacing: 0) {
                        // Viewport
                        ViewportPanel(rendererBridge: rendererBridge)
                            .frame(maxHeight: .infinity)
                            .environmentObject(selectionManager)
                        
                        EditorDivider()
                        
                        // Bottom: Content Browser
                        ContentBrowserPanel()
                            .frame(height: contentBrowserHeight)
                    }
                    .frame(maxWidth: .infinity)
                    
                    EditorDivider()
                    
                    // Right: Inspector
                    InspectorPanel(selectionManager: selectionManager)
                        .frame(width: inspectorWidth)
                }
                .frame(maxHeight: .infinity)
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Premium Main Toolbar

struct EditorMainToolbar: View {
    @State private var isPlaying = false
    @State private var showHistory = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // History
            EditorIconButton(icon: "clock.arrow.circlepath", tooltip: "Undo History") {
                showHistory.toggle()
            }
            .popover(isPresented: $showHistory) {
                CommandHistoryPanel()
                    .frame(width: 300, height: 400)
            }
            
            Divider()
                .frame(height: 20)
                .background(DesignSystem.Colors.border)
            
            // File operations
            HStack(spacing: DesignSystem.Spacing.xs) {
                EditorIconButton(icon: "doc.badge.plus", tooltip: "New Scene") {
                    EngineBridge.shared.newScene()
                }
                
                EditorIconButton(icon: "folder", tooltip: "Open Scene") {
                    print("[Swift] Open scene")
                }
                
                EditorIconButton(icon: "square.and.arrow.down", tooltip: "Save Scene") {
                    EngineBridge.shared.saveScene(path: "/tmp/current_scene.voxel")
                }
            }
            
            Divider()
                .frame(height: 20)
                .background(DesignSystem.Colors.border)
            
            // Play controls
            HStack(spacing: DesignSystem.Spacing.xs) {
                EditorIconButton(
                    icon: isPlaying ? "stop.fill" : "play.fill",
                    tooltip: isPlaying ? "Stop" : "Play"
                ) {
                    isPlaying.toggle()
                }
                
                EditorIconButton(icon: "forward.frame", tooltip: "Step Frame") {
                    print("[Swift] Stepping one frame")
                }
            }
            
            Spacer()
            
            // Build
            EditorButton("Build", icon: "hammer", style: .primary) {
                print("[Swift] Building project...")
            }
        }
        .padding(.horizontal, DesignSystem.Spacing.md)
        .padding(.vertical, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Viewport Panel

struct ViewportPanel: View {
    @ObservedObject var profiler = Profiler.shared
    @State private var gizmoMode: GizmoMode = .translate
    @State private var showGrid = true
    @State private var showStats = true
    
    var rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    enum GizmoMode {
        case translate, rotate, scale
        
        var icon: String {
            switch self {
            case .translate: return "arrow.up.and.down.and.arrow.left.and.right"
            case .rotate: return "arrow.triangle.2.circlepath"
            case .scale: return "arrow.up.left.and.arrow.down.right"
            }
        }
    }
    
    var body: some View {
        ZStack {
            // Metal viewport placeholder
            if let bridge = rendererBridge {
                EngineViewport(rendererBridge: bridge)
            } else {
                DesignSystem.Colors.backgroundPrimary
                    .overlay(Text("No Engine Connection").foregroundStyle(.white))
            }
            
            // Viewport content
            VStack {
                // Viewport toolbar
                HStack(spacing: DesignSystem.Spacing.xs) {
                    // Camera mode
                    Menu {
                        Button("Perspective") { }
                        Button("Orthographic") { }
                        Divider()
                        Button("Top") { }
                        Button("Front") { }
                        Button("Right") { }
                    } label: {
                        HStack(spacing: 4) {
                            Image(systemName: "camera")
                            Text("Perspective")
                        }
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .padding(.horizontal, DesignSystem.Spacing.sm)
                        .padding(.vertical, DesignSystem.Spacing.xs)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(DesignSystem.CornerRadius.tight)
                    }
                    .menuStyle(.borderlessButton)
                    
                    Spacer()
                    
                    // Gizmo modes
                    HStack(spacing: 4) {
                        ForEach([GizmoMode.translate, .rotate, .scale], id: \.icon) { mode in
                            Button(action: { gizmoMode = mode }) {
                                Image(systemName: mode.icon)
                                    .font(.system(size: 14, weight: .medium))
                                    .foregroundColor(gizmoMode == mode ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                                    .frame(width: 28, height: 28)
                                    .background(gizmoMode == mode ? DesignSystem.Colors.selection : Color.clear)
                                    .cornerRadius(4)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                    .padding(4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.regular)
                    
                    // Grid toggle
                    EditorIconButton(icon: showGrid ? "square.grid.3x3" : "square.grid.3x3.fill", tooltip: "Toggle Grid") {
                        showGrid.toggle()
                    }
                    
                    // Stats toggle
                    EditorIconButton(icon: "chart.bar", tooltip: "Toggle Stats") {
                        showStats.toggle()
                    }
                }
                .padding(DesignSystem.Spacing.sm)
                
                Spacer()
                
                // Stats overlay
                if showStats {
                    HStack {
                        VStack(alignment: .leading, spacing: 4) {
                            Text("FPS: \(Int(profiler.stats.fps))")
                            Text("Draw Calls: \(profiler.stats.drawCalls)")
                            Text("Triangles: \(profiler.stats.triangles)")
                        }
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .padding(DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.8))
                        .cornerRadius(DesignSystem.CornerRadius.tight)
                        
                        Spacer()
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
            }
        }
    }
}

