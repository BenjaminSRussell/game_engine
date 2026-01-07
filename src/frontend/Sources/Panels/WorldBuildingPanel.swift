import SwiftUI

// MARK: - World Building Panel

struct WorldBuildingPanel: View {
    @StateObject private var viewModel = WorldBuildingViewModel()
    @State private var selectedTab: WorldTab = .biome
    
    enum WorldTab: String, CaseIterable {
        case biome = "Biomes & Terrain"
        case chunks = "Chunk Streaming"
        case generation = "generation"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(WorldTab.allCases, id: \.self) { tab in
                    if tab == .generation {
                         // Skip for now or rename
                    } else {
                        Button(action: { selectedTab = tab }) {
                            VStack(spacing: 8) {
                                Text(tab.rawValue)
                                    .font(DesignSystem.Typography.bodyBold)
                                    .foregroundColor(selectedTab == tab ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                                
                                Rectangle()
                                    .fill(selectedTab == tab ? DesignSystem.Colors.accentPrimary : Color.clear)
                                    .frame(height: 2)
                            }
                            .padding(.horizontal, DesignSystem.Spacing.md)
                            .padding(.top, DesignSystem.Spacing.sm)
                        }
                        .buttonStyle(.plain)
                        .frame(maxWidth: .infinity)
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Content
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    switch selectedTab {
                    case .biome:
                        BiomeControlView(viewModel: viewModel)
                    case .chunks:
                        ChunkControlView(viewModel: viewModel)
                    default:
                        EmptyView()
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Sub-Views

private struct BiomeControlView: View {
    @ObservedObject var viewModel: WorldBuildingViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            // Biome Parameters
            EditorCollapsibleSection("Global Biome Parameters", isExpanded: true) {
                // Biome Grid Preview (Mock)
                ZStack {
                    RoundedRectangle(cornerRadius: 8)
                        .fill(
                            LinearGradient(
                                colors: [.blue, .green, .yellow, .red],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            )
                        )
                        .frame(height: 150)
                        .opacity(0.8)
                    
                    Text("Biome Heatmap Preview")
                        .font(DesignSystem.Typography.smallBold)
                        .foregroundColor(.white)
                        .shadow(radius: 2)
                }
                
                // Controls
                VStack(spacing: 12) {
                    HStack {
                        VStack(alignment: .leading) {
                            Text("Temperature Bias")
                            Slider(value: $viewModel.temperatureBias, in: -1...1)
                                .accentColor(viewModel.temperatureColor)
                            Text(viewModel.temperatureLabel)
                                .font(DesignSystem.Typography.caption)
                        }
                        
                        VStack(alignment: .leading) {
                            Text("Humidity Bias")
                            Slider(value: $viewModel.humidityBias, in: 0...1)
                                .accentColor(DesignSystem.Colors.accentSecondary)
                            Text(viewModel.humidityLabel)
                                .font(DesignSystem.Typography.caption)
                        }
                    }
                }
            }
            
            // Vegetation
            EditorCollapsibleSection("Vegetation & Scattering", isExpanded: true) {
                EditorNumericField(label: "Vegetation Density", value: $viewModel.vegetationDensity, range: 0...1, step: 0.05)
                EditorNumericField(label: "Blend Distance", value: $viewModel.blendDistance, range: 0...100, step: 1)
            }
            
            // Current Biome Info
            EditorCollapsibleSection("Inspector", isExpanded: true) {
                HStack {
                    Text("Current Biome:")
                    Spacer()
                    Text(viewModel.currentBiomeName)
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
}

private struct ChunkControlView: View {
    @ObservedObject var viewModel: WorldBuildingViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            // Streaming Settings
            EditorCollapsibleSection("Streaming Settings", isExpanded: true) {
                Toggle("Async Loading", isOn: $viewModel.asyncLoading)
                
                EditorNumericField(label: "Load Distance (Chunks)", value: $viewModel.loadDistance, range: 2...32, step: 1)
                EditorNumericField(label: "Unload Distance", value: $viewModel.unloadDistance, range: 4...40, step: 1)
                
                Text("Higher distances increase memory usage significantly.")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Stats
            EditorCollapsibleSection("Statistics", isExpanded: true) {
                StatRow(label: "Loaded Chunks", value: "\(viewModel.loadedChunks)")
                StatRow(label: "Memory Usage", value: viewModel.memoryUsageString)
                
                ProgressView(value: Float(viewModel.loadedChunks), total: 1000)
                    .tint(DesignSystem.Colors.accentPrimary)
            }
            
            // Manual Controls
            EditorCollapsibleSection("Manual Override", isExpanded: true) {
                HStack {
                    TextField("X", value: $viewModel.manualChunkX, formatter: NumberFormatter())
                        .textFieldStyle(.roundedBorder)
                    TextField("Z", value: $viewModel.manualChunkZ, formatter: NumberFormatter())
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    EditorButton("Force Load") { viewModel.forceLoadChunk() }
                    EditorButton("Force Unload") { viewModel.forceUnloadChunk() }
                }
            }
        }
    }
}

// MARK: - View Model

class WorldBuildingViewModel: ObservableObject {
    // Biome
    @Published var temperatureBias: Float = 0.0
    @Published var humidityBias: Float = 0.5
    @Published var vegetationDensity: Float = 0.8
    @Published var blendDistance: Float = 32.0
    
    // Chunk
    @Published var asyncLoading: Bool = true {
        didSet { ChunkAPI.shared.asyncLoadingEnabled = asyncLoading }
    }
    @Published var loadDistance: Float = 8.0 {
        didSet { ChunkAPI.shared.loadDistance = loadDistance }
    }
    @Published var unloadDistance: Float = 12.0 {
        didSet { ChunkAPI.shared.unloadDistance = unloadDistance }
    }
    
    // Stats / Info
    @Published var currentBiomeName: String = "Temperate Forest"
    @Published var loadedChunks: Int = 144
    @Published var manualChunkX: Int = 0
    @Published var manualChunkZ: Int = 0
    
    private var timer: Timer?
    
    // UI Helpers
    var temperatureLabel: String {
        if temperatureBias < -0.6 { return "Arctic" }
        if temperatureBias < -0.2 { return "Cold" }
        if temperatureBias < 0.2 { return "Temperate" }
        if temperatureBias < 0.6 { return "Warm" }
        return "Scorching"
    }
    
    var temperatureColor: Color {
        if temperatureBias < 0 { return .cyan }
        return .orange
    }
    
    var humidityLabel: String {
        if humidityBias < 0.3 { return "Arid" }
        if humidityBias < 0.7 { return "Temperate" }
        return "Tropical"
    }
    
    var memoryUsageString: String {
        let mb = Float(loadedChunks * 64 * 1024) / 1024 / 1024 // approx 64kb per chunk
        return String(format: "%.1f MB", mb)
    }
    
    init() {
        // Sync
        // temperatureBias = BiomeAPI.shared.temperature
        // humidityBias = BiomeAPI.shared.humidity
        asyncLoading = ChunkAPI.shared.asyncLoadingEnabled
        loadDistance = ChunkAPI.shared.loadDistance
        unloadDistance = ChunkAPI.shared.unloadDistance
        
        startMonitoring()
    }
    
    func startMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            self.loadedChunks = Int(ChunkAPI.shared.loadedCount)
            // Mock biome reading at 0,0,0 for now
            self.currentBiomeName = "\(BiomeAPI.shared.sampleBiomeAt(position: (0,0,0)))"
        }
    }
    
    func forceLoadChunk() {
        ChunkAPI.shared.forceLoad(x: Int32(manualChunkX), z: Int32(manualChunkZ))
    }
    
    func forceUnloadChunk() {
        ChunkAPI.shared.forceUnload(x: Int32(manualChunkX), z: Int32(manualChunkZ))
    }
}
