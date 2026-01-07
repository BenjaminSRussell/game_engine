import SwiftUI

// MARK: - Build Pipeline Manager (TODO-6601 to TODO-6860)

class BuildManager: ObservableObject {
    static let shared = BuildManager()
    
    @Published var selectedPlatform: BuildPlatform = .macOS
    @Published var buildConfiguration: BuildConfiguration = .development
    @Published var isBuilding: Bool = false
    @Published var buildProgress: Float = 0.0
    @Published var buildLog: [BuildLogEntry] = []
    @Published var scenes: [SceneEntry] = []
    
    enum BuildPlatform: String, CaseIterable {
        case macOS, windows, linux, iOS, android, webGL
        
        var icon: String {
            switch self {
            case .macOS: return "desktopcomputer"
            case .windows: return "pc"
            case .linux: return "terminal"
            case .iOS: return "iphone"
            case .android: return "apps.iphone"
            case .webGL: return "globe"
            }
        }
    }
    
    enum BuildConfiguration: String, CaseIterable {
        case development, release, distribution
    }
    
    init() {
        scenes = [
            SceneEntry(name: "MainMenu", path: "Scenes/MainMenu.scene", isIncluded: true),
            SceneEntry(name: "Level01", path: "Scenes/Level01.scene", isIncluded: true),
            SceneEntry(name: "Level02", path: "Scenes/Level02.scene", isIncluded: true),
            SceneEntry(name: "GameOver", path: "Scenes/GameOver.scene", isIncluded: true)
        ]
    }
    
    func startBuild() {
        isBuilding = true
        buildProgress = 0.0
        buildLog.append(BuildLogEntry(message: "Starting build for \(selectedPlatform.rawValue)...", type: .info))
        
        // Simulate build progress
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] timer in
            guard let self = self else { timer.invalidate(); return }
            self.buildProgress += 0.02
            
            if self.buildProgress >= 1.0 {
                timer.invalidate()
                self.isBuilding = false
                self.buildLog.append(BuildLogEntry(message: "Build completed successfully!", type: .success))
                NotificationManager.shared.notify("Build completed!", type: .success)
            }
        }
    }
}

struct SceneEntry: Identifiable {
    let id = UUID()
    var name: String
    var path: String
    var isIncluded: Bool
}

struct BuildLogEntry: Identifiable {
    let id = UUID()
    let timestamp = Date()
    let message: String
    let type: LogType
    
    enum LogType {
        case info, warning, error, success
        
        var color: Color {
            switch self {
            case .info: return .primary
            case .warning: return .orange
            case .error: return .red
            case .success: return .green
            }
        }
    }
}

// MARK: - Build Settings View
struct BuildSettingsView: View {
    @ObservedObject var manager = BuildManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Build Settings")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button(action: { manager.startBuild() }) {
                    Label(manager.isBuilding ? "Building..." : "Build", systemImage: "hammer")
                }
                .disabled(manager.isBuilding)
                
                Button(action: {}) {
                    Label("Build and Run", systemImage: "play")
                }
                .disabled(manager.isBuilding)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Scene List
                VStack(alignment: .leading, spacing: 0) {
                    HStack {
                        Text("Scenes in Build")
                            .font(DesignSystem.Typography.bodyBold)
                        Spacer()
                        Button(action: {}) {
                            Image(systemName: "plus")
                        }
                        .buttonStyle(.plain)
                    }
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    
                    EditorDivider()
                    
                    List {
                        ForEach($manager.scenes) { $scene in
                            HStack {
                                Toggle("", isOn: $scene.isIncluded)
                                    .labelsHidden()
                                VStack(alignment: .leading) {
                                    Text(scene.name)
                                        .font(DesignSystem.Typography.body)
                                    Text(scene.path)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(DesignSystem.Colors.textSecondary)
                                }
                            }
                        }
                        .onMove { from, to in
                            manager.scenes.move(fromOffsets: from, toOffset: to)
                        }
                    }
                    .listStyle(.plain)
                }
                .frame(minWidth: 250, maxWidth: 350)
                
                // Platform/Settings
                ScrollView {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                        // Platform selector
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Target Platform")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            LazyVGrid(columns: [GridItem(.adaptive(minimum: 80))], spacing: 8) {
                                ForEach(BuildManager.BuildPlatform.allCases, id: \.self) { platform in
                                    PlatformButton(platform: platform, isSelected: manager.selectedPlatform == platform) {
                                        manager.selectedPlatform = platform
                                    }
                                }
                            }
                        }
                        .padding(DesignSystem.Spacing.md)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(8)
                        
                        // Configuration
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Configuration")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            Picker("", selection: $manager.buildConfiguration) {
                                ForEach(BuildManager.BuildConfiguration.allCases, id: \.self) { config in
                                    Text(config.rawValue.capitalized).tag(config)
                                }
                            }
                            .pickerStyle(.segmented)
                        }
                        .padding(DesignSystem.Spacing.md)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(8)
                        
                        // Build progress
                        if manager.isBuilding {
                            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                Text("Building...")
                                    .font(DesignSystem.Typography.bodyBold)
                                
                                ProgressView(value: Double(manager.buildProgress))
                                    .progressViewStyle(.linear)
                                
                                Text("\(Int(manager.buildProgress * 100))%")
                                    .font(DesignSystem.Typography.mono)
                            }
                            .padding(DesignSystem.Spacing.md)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(8)
                        }
                        
                        // Build log
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Build Log")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            ScrollView {
                                LazyVStack(alignment: .leading, spacing: 2) {
                                    ForEach(manager.buildLog) { entry in
                                        Text("[\(entry.timestamp, style: .time)] \(entry.message)")
                                            .font(DesignSystem.Typography.mono)
                                            .foregroundColor(entry.type.color)
                                    }
                                }
                            }
                            .frame(height: 150)
                            .padding(8)
                            .background(Color.black)
                            .cornerRadius(4)
                        }
                        .padding(DesignSystem.Spacing.md)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(8)
                    }
                    .padding(DesignSystem.Spacing.md)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct PlatformButton: View {
    let platform: BuildManager.BuildPlatform
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Image(systemName: platform.icon)
                    .font(.system(size: 24))
                Text(platform.rawValue)
                    .font(.system(size: 10))
            }
            .frame(width: 70, height: 60)
            .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
            .background(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(8)
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Plugin Manager (TODO-7801 to TODO-7960)

class PluginManager: ObservableObject {
    static let shared = PluginManager()
    
    @Published var installedPlugins: [Plugin] = []
    @Published var availablePlugins: [Plugin] = []
    
    init() {
        installedPlugins = [
            Plugin(name: "Git Integration", version: "1.2.0", author: "VoxelForge", isEnabled: true, description: "Version control integration"),
            Plugin(name: "Shader Graph", version: "2.0.1", author: "VoxelForge", isEnabled: true, description: "Visual shader editor"),
            Plugin(name: "Terrain Tools Pro", version: "1.0.0", author: "Community", isEnabled: false, description: "Advanced terrain brushes")
        ]
        
        availablePlugins = [
            Plugin(name: "Dialogue System", version: "1.5.0", author: "Community", isEnabled: false, description: "NPC dialogue editor"),
            Plugin(name: "Quest Editor", version: "1.0.0", author: "Community", isEnabled: false, description: "Quest and objective editor"),
            Plugin(name: "Localization", version: "2.1.0", author: "VoxelForge", isEnabled: false, description: "Multi-language support")
        ]
    }
}

struct Plugin: Identifiable {
    let id = UUID()
    var name: String
    var version: String
    var author: String
    var isEnabled: Bool
    var description: String
}

struct PluginManagerView: View {
    @ObservedObject var manager = PluginManager.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Tabs
            Picker("", selection: $selectedTab) {
                Text("Installed").tag(0)
                Text("Available").tag(1)
            }
            .pickerStyle(.segmented)
            .padding()
            
            EditorDivider()
            
            // Plugin list
            List {
                ForEach(selectedTab == 0 ? $manager.installedPlugins : $manager.availablePlugins) { $plugin in
                    PluginRow(plugin: $plugin, isInstalled: selectedTab == 0)
                }
            }
            .listStyle(.plain)
        }
    }
}

struct PluginRow: View {
    @Binding var plugin: Plugin
    let isInstalled: Bool
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text(plugin.name)
                        .font(DesignSystem.Typography.bodyBold)
                    Text("v\(plugin.version)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                Text(plugin.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                Text("by \(plugin.author)")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textDisabled)
            }
            
            Spacer()
            
            if isInstalled {
                Toggle("", isOn: $plugin.isEnabled)
                    .labelsHidden()
            } else {
                Button("Install") {
                    // Install plugin
                }
            }
        }
        .padding(.vertical, 4)
    }
}
