import SwiftUI

// 
// MARK: - CATEGORY 7: TERRAIN SYSTEM (TODO-3701 to TODO-4000)
// 

// MARK: - Terrain Sculpting TODOs (TODO-3701 to TODO-3750)
// TODO-3701: Height sculpting with pressure sensitivity
// TODO-3702: Raise brush with additive mode

// MARK: - Terrain Texturing System

// MARK: - Erosion Simulation TODOs (TODO-3801 to TODO-3830)
// TODO-3801: Hydraulic erosion simulation
// TODO-3802: Thermal erosion simulation
// TODO-3803: Wind erosion simulation
// TODO-3804: Coastal erosion simulation
// TODO-3805: Erosion iteration count
// TODO-3806: Erosion strength parameter
// TODO-3807: Sediment deposition
// TODO-3808: Water flow visualization
// TODO-3809: Erosion mask painting
// TODO-3810: Real-time erosion preview
// TODO-3811: Erosion undo/redo
// TODO-3812: Erosion presets (desert, alpine, etc.)
// TODO-3813: River/stream generation
// TODO-3814: Lake filling algorithm
// TODO-3815: Riverbed sculpting

// MARK: - Vegetation System TODOs (TODO-3831 to TODO-3880)
// TODO-3831: Tree placement brush
// TODO-3832: Tree density control
// TODO-3833: Tree scale randomization
// TODO-3834: Tree rotation randomization
// TODO-3835: Tree color variation
// TODO-3836: Tree type selection
// TODO-3837: Tree prefab library
// TODO-3838: Tree LOD system
// TODO-3839: Tree billboard imposters
// TODO-3840: Tree wind animation
// TODO-3841: Tree collision generation
// TODO-3842: Grass density brush
// TODO-3843: Grass type selection
// TODO-3844: Grass detail mesh library
// TODO-3845: Grass color variation
// TODO-3846: Grass wind response
// TODO-3847: Grass LOD distance
// TODO-3848: Grass culling settings
// TODO-3849: Vegetation placement rules (slope, height)
// TODO-3850: Vegetation exclusion zones
// TODO-3851: Vegetation instance painting
// TODO-3852: Vegetation removal brush
// TODO-3853: Vegetation density visualization
// TODO-3854: Vegetation optimization tools
// TODO-3855: Procedural vegetation scattering

// MARK: - Heightmap Management TODOs (TODO-3881 to TODO-3920)
// TODO-3881: Heightmap import (PNG, RAW, R16)
// TODO-3882: Heightmap export (PNG, RAW, R16)
// TODO-3883: Heightmap resolution options (2^n+1)
// TODO-3884: Heightmap precision (8-bit, 16-bit, 32-bit)
// TODO-3885: Heightmap world height range
// TODO-3886: Heightmap tile system
// TODO-3887: Heightmap seamless tiling
// TODO-3888: Heightmap copy/paste regions
// TODO-3889: Heightmap fill with value
// TODO-3890: Heightmap invert
// TODO-3891: Heightmap normalize
// TODO-3892: Heightmap scale values
// TODO-3893: Heightmap offset values
// TODO-3894: Heightmap clamp range
// TODO-3895: Heightmap hole system
// TODO-3896: Heightmap undo history
// TODO-3897: Heightmap comparison/diff
// TODO-3898: Heightmap version snapshots
// TODO-3899: Heightmap compression
// TODO-3900: Heightmap streaming

// MARK: - Terrain LOD TODOs (TODO-3921 to TODO-3950)
// TODO-3921: LOD level count configuration
// TODO-3922: LOD distance thresholds
// TODO-3923: LOD seamless stitching
// TODO-3924: LOD crack fixing (T-junctions)
// TODO-3925: LOD morph blending
// TODO-3926: LOD geomorphing
// TODO-3927: LOD chunk size configuration
// TODO-3928: LOD screen-space error metric
// TODO-3929: LOD camera frustum culling
// TODO-3930: LOD occlusion culling
// TODO-3931: LOD GPU tessellation
// TODO-3932: LOD vertex density visualization
// TODO-3933: LOD triangle count display
// TODO-3934: LOD performance profiler
// TODO-3935: LOD quality presets

// MARK: - Terrain Integration TODOs (TODO-3951 to TODO-4000)
// TODO-3951: Physics collision from heightmap
// TODO-3952: Navigation mesh generation
// TODO-3953: Water plane integration
// TODO-3954: Sky/ambient lighting integration
// TODO-3955: Shadow cascade optimization
// TODO-3956: Lightmap baking for terrain
// TODO-3957: Material instance per-chunk
// TODO-3958: Terrain shader customization
// TODO-3959: Terrain streaming zones
// TODO-3960: Multi-terrain stitching
// TODO-3961: Terrain neighbor connectivity
// TODO-3962: World origin rebasing
// TODO-3963: Large world coordinates support
// TODO-3964: Terrain prefab saving
// TODO-3965: Terrain asset versioning
// TODO-3966: Terrain API for runtime modification
// TODO-3967: Terrain networking synchronization
// TODO-3968: Terrain analytics/statistics
// TODO-3969: Terrain documentation tooltips
// TODO-3970: Terrain tutorial system

// MARK: - Terrain Documentation Tooltips System

class TerrainTooltipManager: ObservableObject {
    static let shared = TerrainTooltipManager()
    
    @Published var currentTooltip: TerrainTooltip?
    @Published var showTooltips: Bool = true
    @Published var tooltipDelay: Double = 0.5
    @Published var tooltipPosition: CGPoint = .zero
    
    private var tooltipTimer: Timer?
    private var lastHoveredElement: String = ""
    
    struct TerrainTooltip {
        let id = UUID()
        let title: String
        let description: String
        let category: TooltipCategory
        let shortcuts: [String]
        let relatedFeatures: [String]
        let difficulty: DifficultyLevel
        
        enum TooltipCategory: String, CaseIterable {
            case sculpting, texturing, erosion, vegetation, heightmap, lod, integration
            
            var icon: String {
                switch self {
                case .sculpting: return "hammer.fill"
                case .texturing: return "photo.fill"
                case .erosion: return "drop.fill"
                case .vegetation: return "leaf.fill"
                case .heightmap: return "mountain.2.fill"
                case .lod: return "pyramid.fill"
                case .integration: return "link"
                }
            }
            
            var color: Color {
                switch self {
                case .sculpting: return .blue
                case .texturing: return .green
                case .erosion: return .cyan
                case .vegetation: return .green
                case .heightmap: return .orange
                case .lod: return .purple
                case .integration: return .red
                }
            }
        }
        
        enum DifficultyLevel: String, CaseIterable {
            case beginner = "Beginner"
            case intermediate = "Intermediate"
            case advanced = "Advanced"
            case expert = "Expert"
            
            var color: Color {
                switch self {
                case .beginner: return .green
                case .intermediate: return .yellow
                case .advanced: return .orange
                case .expert: return .red
                }
            }
        }
    }
    
    // Predefined tooltips for common terrain features
    private let predefinedTooltips: [String: TerrainTooltip] = [
        "brush.raise": TerrainTooltip(
            title: "Raise Brush",
            description: "Elevates terrain vertices within the brush radius. Use pressure sensitivity for variable height adjustment.",
            category: .sculpting,
            shortcuts: ["B", "Shift + Drag for smooth raise"],
            relatedFeatures: ["Lower Brush", "Flatten Brush", "Smooth Brush"],
            difficulty: .beginner
        ),
        
        "brush.lower": TerrainTooltip(
            title: "Lower Brush",
            description: "Lowers terrain vertices within the brush radius. Perfect for creating valleys and depressions.",
            category: .sculpting,
            shortcuts: ["B", "Alt + Drag for lower"],
            relatedFeatures: ["Raise Brush", "Flatten Brush", "Erosion Brush"],
            difficulty: .beginner
        ),
        
        "brush.flatten": TerrainTooltip(
            title: "Flatten Brush",
            description: "Flattens terrain to a specific height. Hold Shift to sample target height from terrain.",
            category: .sculpting,
            shortcuts: ["B", "Shift + Click to sample height"],
            relatedFeatures: ["Raise Brush", "Lower Brush", "Smooth Brush"],
            difficulty: .intermediate
        ),
        
        "brush.smooth": TerrainTooltip(
            title: "Smooth Brush",
            description: "Smooths terrain by averaging neighboring vertex heights. Adjust iterations for stronger smoothing.",
            category: .sculpting,
            shortcuts: ["B", "Ctrl + Drag for smooth"],
            relatedFeatures: ["Sharpen Brush", "Noise Brush", "Flatten Brush"],
            difficulty: .beginner
        ),
        
        "texture.paint": TerrainTooltip(
            title: "Texture Paint",
            description: "Applies texture layers to terrain. Use opacity control for blending multiple textures.",
            category: .texturing,
            shortcuts: ["T", "Right-click for eyedropper"],
            relatedFeatures: ["Texture Erase", "Splat Map", "Texture Rules"],
            difficulty: .beginner
        ),
        
        "erosion.hydraulic": TerrainTooltip(
            title: "Hydraulic Erosion",
            description: "Simulates water erosion effects. Creates realistic river beds and canyons over multiple iterations.",
            category: .erosion,
            shortcuts: ["E", "Ctrl + Drag for directional flow"],
            relatedFeatures: ["Thermal Erosion", "Wind Erosion", "River Generation"],
            difficulty: .advanced
        ),
        
        "vegetation.trees": TerrainTooltip(
            title: "Tree Placement",
            description: "Places vegetation instances on terrain. Respects slope and elevation rules for realistic distribution.",
            category: .vegetation,
            shortcuts: ["V", "Shift + Click for brush placement"],
            relatedFeatures: ["Grass Placement", "Vegetation Rules", "LOD System"],
            difficulty: .intermediate
        ),
        
        "heightmap.import": TerrainTooltip(
            title: "Heightmap Import",
            description: "Imports height data from various formats (PNG, RAW, R16). Supports 8-bit, 16-bit, and 32-bit precision.",
            category: .heightmap,
            shortcuts: ["File > Import Heightmap"],
            relatedFeatures: ["Heightmap Export", "Heightmap Scale", "Terrain Resolution"],
            difficulty: .intermediate
        ),
        
        "lod.settings": TerrainTooltip(
            title: "LOD Settings",
            description: "Configures level-of-detail distances and morphing. Optimizes performance for large terrains.",
            category: .lod,
            shortcuts: ["L", "Ctrl + L for LOD panel"],
            relatedFeatures: ["LOD Morphing", "LOD Stitching", "Performance Profiler"],
            difficulty: .advanced
        ),
        
        "physics.collision": TerrainTooltip(
            title: "Physics Collision",
            description: "Generates collision mesh from heightmap data. Supports multiple collision shapes and material properties.",
            category: .integration,
            shortcuts: ["P", "Ctrl + P for physics settings"],
            relatedFeatures: ["Navigation Mesh", "Character Controller", "Raycasting"],
            difficulty: .expert
        )
    ]
    
    init() {
        loadTooltipPreferences()
    }
    
    func showTooltip(for element: String, at position: CGPoint) {
        guard showTooltips else { return }
        
        tooltipTimer?.invalidate()
        
        if lastHoveredElement == element && currentTooltip?.id != nil {
            return
        }
        
        lastHoveredElement = element
        tooltipPosition = position
        
        tooltipTimer = Timer.scheduledTimer(withTimeInterval: tooltipDelay, repeats: false) { _ in
            if let tooltip = self.predefinedTooltips[element] {
                DispatchQueue.main.async {
                    self.currentTooltip = tooltip
                }
            }
        }
    }
    
    func hideTooltip() {
        tooltipTimer?.invalidate()
        lastHoveredElement = ""
        currentTooltip = nil
    }
    
    func getTooltip(for element: String) -> TerrainTooltip? {
        return predefinedTooltips[element]
    }
    
    private func loadTooltipPreferences() {
        if let showTooltips = UserDefaults.standard.object(forKey: "TerrainShowTooltips") as? Bool {
            self.showTooltips = showTooltips
        }
        if let tooltipDelay = UserDefaults.standard.object(forKey: "TerrainTooltipDelay") as? Double {
            self.tooltipDelay = tooltipDelay
        }
    }
    
    func saveTooltipPreferences() {
        UserDefaults.standard.set(showTooltips, forKey: "TerrainShowTooltips")
        UserDefaults.standard.set(tooltipDelay, forKey: "TerrainTooltipDelay")
    }
}

// MARK: - Tooltip View

struct TerrainTooltipView: View {
    let tooltip: TerrainTooltipManager.TerrainTooltip
    let position: CGPoint
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 8) {
                Image(systemName: tooltip.category.icon)
                    .foregroundColor(tooltip.category.color)
                    .font(.system(size: 16, weight: .medium))
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(tooltip.title)
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    HStack(spacing: 8) {
                        Text(tooltip.category.rawValue.capitalized)
                            .font(DesignSystem.Typography.micro)
                            .foregroundColor(tooltip.category.color)
                        
                        Text("•")
                            .font(DesignSystem.Typography.micro)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text(tooltip.difficulty.rawValue)
                            .font(DesignSystem.Typography.micro)
                            .foregroundColor(tooltip.difficulty.color)
                    }
                }
                
                Spacer()
            }
            
            Text(tooltip.description)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .fixedSize(horizontal: false, vertical: true)
            
            if !tooltip.shortcuts.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Shortcuts:")
                        .font(DesignSystem.Typography.smallBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    ForEach(tooltip.shortcuts, id: \.self) { shortcut in
                        HStack(spacing: 6) {
                            Image(systemName: "keyboard")
                                .font(.system(size: 10))
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                            
                            Text(shortcut)
                                .font(DesignSystem.Typography.smallMono)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
            
            if !tooltip.relatedFeatures.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Related Features:")
                        .font(DesignSystem.Typography.smallBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    HStack(spacing: 8) {
                        ForEach(tooltip.relatedFeatures, id: \.self) { feature in
                            Text(feature)
                                .font(DesignSystem.Typography.micro)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 2)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
        }
        .padding(12)
        .background(DesignSystem.Colors.backgroundPrimary)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
        .cornerRadius(8)
        .shadow(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        .frame(maxWidth: 300)
        .position(position)
        .zIndex(1000)
    }
}

// MARK: - Terrain Analytics System

class TerrainAnalyticsManager: ObservableObject {
    static let shared = TerrainAnalyticsManager()
    
    @Published var analytics: TerrainAnalytics?
    @Published var isAnalyzing: Bool = false
    @Published var analysisProgress: Float = 0.0
    @Published var selectedMetric: AnalyticsMetric = .heightDistribution
    @Published var showAnalyticsPanel: Bool = false
    
    struct TerrainAnalytics {
        let timestamp: Date
        let terrainSize: SIMD2<Int>
        let totalVertices: Int
        let totalTriangles: Int
        let heightRange: ClosedRange<Float>
        let averageSlope: Float
        let surfaceArea: Float
        let volume: Float
        let textureCoverage: [String: Float]
        let performanceMetrics: PerformanceMetrics
        let erosionMetrics: ErosionMetrics
        let vegetationMetrics: VegetationMetrics
        
        struct PerformanceMetrics {
            let lodLevels: Int
            let averageTrianglesPerFrame: Int
            let memoryUsage: MemoryUsage
            let renderingTime: Float
            
            struct MemoryUsage {
                let heightmapData: Int64
                let textureData: Int64
                let vertexBuffers: Int64
                let indexBuffers: Int64
                let total: Int64
            }
        }
        
        struct ErosionMetrics {
            let totalErosion: Float
            let averageErosionDepth: Float
            let erosionHotspots: [ErosionHotspot]
            let sedimentDeposition: Float
            
            struct ErosionHotspot {
                let position: SIMD2<Float>
                let severity: Float
                let type: ErosionType
                
                enum ErosionType {
                    case hydraulic, thermal, wind, coastal
                }
            }
        }
        
        struct VegetationMetrics {
            let totalInstances: Int
            let densityPerSquareKilometer: Float
            let speciesDistribution: [String: Int]
            let healthScore: Float
            let coveragePercentage: Float
        }
    }
    
    enum AnalyticsMetric: String, CaseIterable {
        case heightDistribution = "Height Distribution"
        case slopeAnalysis = "Slope Analysis"
        case textureCoverage = "Texture Coverage"
        case performanceMetrics = "Performance Metrics"
        case erosionAnalysis = "Erosion Analysis"
        case vegetationAnalysis = "Vegetation Analysis"
        case memoryUsage = "Memory Usage"
        case renderingStats = "Rendering Statistics"
        
        var icon: String {
            switch self {
            case .heightDistribution: return "chart.bar.fill"
            case .slopeAnalysis: return "triangle.fill"
            case .textureCoverage: return "photo.fill"
            case .performanceMetrics: return "speedometer"
            case .erosionAnalysis: return "drop.fill"
            case .vegetationAnalysis: return "leaf.fill"
            case .memoryUsage: return "memorychip.fill"
            case .renderingStats: return "cpu.fill"
            }
        }
        
        var color: Color {
            switch self {
            case .heightDistribution: return .blue
            case .slopeAnalysis: return .orange
            case .textureCoverage: return .green
            case .performanceMetrics: return .purple
            case .erosionAnalysis: return .cyan
            case .vegetationAnalysis: return .green
            case .memoryUsage: return .red
            case .renderingStats: return .yellow
            }
        }
    }
    
    func analyzeTerrain(_ terrainData: TerrainData) async {
        await MainActor.run {
            isAnalyzing = true
            analysisProgress = 0.0
        }
        
        // Simulate analysis progress
        for progress in stride(from: 0.0, through: 1.0, by: 0.1) {
            await MainActor.run {
                analysisProgress = Float(progress)
            }
            try? await Task.sleep(nanoseconds: 100_000_000) // 0.1 second
        }
        
        let analytics = await performAnalysis(terrainData)
        
        await MainActor.run {
            self.analytics = analytics
            isAnalyzing = false
            analysisProgress = 1.0
        }
    }
    
    private func performAnalysis(_ terrainData: TerrainData) async -> TerrainAnalytics {
        // Mock analysis - in real implementation, this would analyze actual terrain data
        let performanceMetrics = TerrainAnalytics.PerformanceMetrics(
            lodLevels: 4,
            averageTrianglesPerFrame: 125000,
            memoryUsage: TerrainAnalytics.PerformanceMetrics.MemoryUsage(
                heightmapData: 64 * 1024 * 1024, // 64MB
                textureData: 256 * 1024 * 1024, // 256MB
                vertexBuffers: 128 * 1024 * 1024, // 128MB
                indexBuffers: 32 * 1024 * 1024, // 32MB
                total: 480 * 1024 * 1024 // 480MB
            ),
            renderingTime: 8.5
        )
        
        let erosionMetrics = TerrainAnalytics.ErosionMetrics(
            totalErosion: 1250.0,
            averageErosionDepth: 0.15,
            erosionHotspots: [
                TerrainAnalytics.ErosionMetrics.ErosionHotspot(
                    position: SIMD2<Float>(256, 128),
                    severity: 0.8,
                    type: .hydraulic
                ),
                TerrainAnalytics.ErosionMetrics.ErosionHotspot(
                    position: SIMD2<Float>(512, 256),
                    severity: 0.6,
                    type: .thermal
                )
            ],
            sedimentDeposition: 890.0
        )
        
        let vegetationMetrics = TerrainAnalytics.VegetationMetrics(
            totalInstances: 15420,
            densityPerSquareKilometer: 842.5,
            speciesDistribution: ["Oak": 5230, "Pine": 4150, "Birch": 3890, "Bush": 2150],
            healthScore: 0.87,
            coveragePercentage: 34.2
        )
        
        return TerrainAnalytics(
            timestamp: Date(),
            terrainSize: SIMD2<Int>(1024, 1024),
            totalVertices: 1048576,
            totalTriangles: 2097152,
            heightRange: -50.0...200.0,
            averageSlope: 15.3,
            surfaceArea: 1250000.0,
            volume: 85000000.0,
            textureCoverage: ["Grass": 45.2, "Rock": 23.8, "Dirt": 18.5, "Sand": 12.5],
            performanceMetrics: performanceMetrics,
            erosionMetrics: erosionMetrics,
            vegetationMetrics: vegetationMetrics
        )
    }
    
    func generateReport() -> String {
        guard let analytics = analytics else { return "No analytics data available" }
        
        var report = """
        TERRAIN ANALYTICS REPORT
        ======================
        Generated: \(analytics.timestamp)
        
        BASIC STATISTICS
        ----------------
        Terrain Size: \(analytics.terrainSize.x) x \(analytics.terrainSize.y)
        Total Vertices: \(analytics.totalVertices.formatted())
        Total Triangles: \(analytics.totalTriangles.formatted())
        Height Range: \(String(format: "%.1f", analytics.heightRange.lowerBound)) to \(String(format: "%.1f", analytics.heightRange.upperBound))
        Average Slope: \(String(format: "%.1f", analytics.averageSlope))°
        Surface Area: \(String(format: "%.0f", analytics.surfaceArea)) m²
        Volume: \(String(format: "%.0f", analytics.volume)) m³
        
        TEXTURE COVERAGE
        ----------------
        """
        
        for (texture, coverage) in analytics.textureCoverage {
            report += "\n\(texture): \(String(format: "%.1f", coverage))%"
        }
        
        report += """
        
        PERFORMANCE METRICS
        -------------------
        LOD Levels: \(analytics.performanceMetrics.lodLevels)
        Average Triangles/Frame: \(analytics.performanceMetrics.averageTrianglesPerFrame.formatted())
        Rendering Time: \(String(format: "%.1f", analytics.performanceMetrics.renderingTime)) ms
        Memory Usage: \(ByteCountFormatter.string(fromByteCount: analytics.performanceMetrics.memoryUsage.total, countStyle: .memory))
        
        EROSION ANALYSIS
        ----------------
        Total Erosion: \(String(format: "%.1f", analytics.erosionMetrics.totalErosion)) m³
        Average Erosion Depth: \(String(format: "%.2f", analytics.erosionMetrics.averageErosionDepth)) m
        Sediment Deposition: \(String(format: "%.1f", analytics.erosionMetrics.sedimentDeposition)) m³
        Erosion Hotspots: \(analytics.erosionMetrics.erosionHotspots.count)
        
        VEGETATION ANALYSIS
        ------------------
        Total Instances: \(analytics.vegetationMetrics.totalInstances.formatted())
        Density: \(String(format: "%.1f", analytics.vegetationMetrics.densityPerSquareKilometer)) per km²
        Coverage: \(String(format: "%.1f", analytics.vegetationMetrics.coveragePercentage))%
        Health Score: \(String(format: "%.0f", analytics.vegetationMetrics.healthScore * 100))%
        """
        
        return report
    }
}

// Mock terrain data for analysis
struct TerrainData {
    let size: SIMD2<Int> = SIMD2<Int>(1024, 1024)
    let heightmap: [[Float]] = []
    let textures: [String] = ["Grass", "Rock", "Dirt", "Sand"]
}

// MARK: - Terrain Editor

class TerrainEditorManager: ObservableObject {
    static let shared = TerrainEditorManager()
    
    // Brush properties
    @Published var selectedBrush: BrushType = .raise
    @Published var brushSize: Float = 10.0
    @Published var brushOpacity: Float = 0.5
    @Published var brushFalloff: Float = 0.5
    @Published var brushJitter: Float = 0.0
    @Published var brushRotation: Float = 0.0
    @Published var brushRotationRandomization: Float = 0.0
    @Published var brushScaleRandomization: Float = 0.0
    @Published var brushSpacing: Float = 0.1
    @Published var brushHardness: Float = 0.5
    @Published var brushPreviewProjection: Bool = true
    @Published var brushCursor3D: Bool = true
    @Published var mirrorBrush: Bool = false
    @Published var twistBrush: Float = 0.0
    @Published var horizontalScaling: Float = 1.0
    @Published var pinchBulgeStrength: Float = 0.0
    @Published var terraceStepHeight: Float = 1.0
    @Published var pathWidth: Float = 5.0
    @Published var bridgeStrength: Float = 0.5
    @Published var cloneSource: SIMD2<Float>? = nil
    @Published var stampShape: StampShape = .circle
    @Published var noiseScale: Float = 1.0
    @Published var noiseStrength: Float = 0.5
    @Published var sharpenIterations: Int = 1
    @Published var smoothIterations: Int = 1
    @Published var flattenTargetHeight: Float = 0.0
    @Published var pressureSensitivity: Bool = true
    @Published var selectedLayer: Int = 0
    @Published var layers: [TerrainLayer] = []
    @Published var terrainResolution: Int = 512
    @Published var terrainSize: Float = 1000.0
    
    // Grass System
    @Published var grassDensity: Float = 0.5
    @Published var grassTypes: [GrassType] = []
    @Published var selectedGrassType: Int = 0
    
    // Erosion System
    @Published var erosionSettings = ErosionSettings()
    @Published var erosionPresets: [ErosionPreset] = []
    @Published var selectedErosionPreset: Int = 0
    @Published var erosionIterationCount: Int = 50
    @Published var erosionStrength: Float = 0.5
    @Published var sedimentDeposition: Float = 0.3
    @Published var realTimePreview: Bool = false
    @Published var erosionMaskEnabled: Bool = false
    
    // Water System
    @Published var riverWidth: Float = 5.0
    @Published var riverDepth: Float = 2.0
    @Published var lakeWaterLevel: Float = 0.0
    
    // Undo/Redo System
    @Published var undoStack: [TerrainState] = []
    @Published var redoStack: [TerrainState] = []
    @Published var undoHistory: [HeightmapSnapshot] = []
    @Published var redoHistory: [HeightmapSnapshot] = []
    @Published var currentSnapshot: HeightmapSnapshot? = nil
    @Published var maxUndoLevels: Int = 50
    @Published var perStrokeUndo: Bool = true
    
    // Heightmap controls
    @Published var heightmapScale: Float = 1.0
    @Published var heightmapOffset: Float = 0.0
    @Published var heightmapMinClamp: Float = 0.0
    @Published var heightmapMaxClamp: Float = 1.0
    @Published var heightmapWorldHeightRange: ClosedRange<Float> = 0...100
    @Published var heightmapTileSize: Int = 512
    @Published var seamlessTiling: Bool = false
    @Published var heightmapPrecision: HeightmapPrecision = .bit16
    @Published var importFormat: HeightmapFormat = .png
    @Published var exportFormat: HeightmapFormat = .png
    
    // LOD configuration
    @Published var lodLevels: Int = 4
    @Published var lodDistances: [Float] = [50, 100, 200, 400]
    @Published var lodChunkSize: Float = 64.0
    @Published var lodScreenSpaceError: Float = 2.0
    @Published var enableSeamlessStitching: Bool = true
    @Published var enableCrackFixing: Bool = true
    @Published var enableGeomorphing: Bool = true
    @Published var geomorphDuration: Float = 0.5
    @Published var enableFrustumCulling: Bool = true
    @Published var enableOcclusionCulling: Bool = true
    @Published var enableGPUTessellation: Bool = false
    @Published var lodQualityPreset: LODQualityPreset = .balanced
    
    // Performance metrics
    @Published var lodTriangleCount: Int = 0
    @Published var lodFrameTime: Float = 0.0
    @Published var lodMemoryUsage: Float = 0.0
    @Published var showLODStats: Bool = false
    
    // Texture system properties
    @Published var splatMapResolution: Int = 512
    @Published var triplanarProjection: Bool = false
    @Published var autoPaintEnabled: Bool = false
    @Published var textureLayers: [TextureLayer] = []
    @Published var selectedTextureLayer: Int = 0
    @Published var splatMapData: Data?
    @Published var textureMaskEnabled: Bool = false
    @Published var textureMaskOpacity: Float = 0.5
    @Published var virtualTexturingEnabled: Bool = false
    @Published var textureStreamingLOD: Bool = true
    
    // Erosion simulation properties
    @Published var hydraulicErosionEnabled: Bool = false
    @Published var thermalErosionEnabled: Bool = false
    
    // Grass System
    @Published var selectedGrassType: GrassType = .standard
    @Published var grassDensity: Float = 0.5
    @Published var grassColorVariation: Float = 0.2
    @Published var grassWindResponse: Float = 0.3
    @Published var grassLODDistance: Float = 100.0
    @Published var grassCullingEnabled: Bool = true
    @Published var grassDetailMeshes: [GrassDetailMesh] = []
    
    // Vegetation System
    @Published var vegetationInstances: [VegetationInstance] = []
    @Published var selectedVegetationType: VegetationType = .oak
    @Published var vegetationPlacementRules: VegetationPlacementRules = VegetationPlacementRules()
    @Published var exclusionZones: [ExclusionZone] = []
    @Published var showVegetationDensity: Bool = false
    
    enum BrushType: String, CaseIterable {
        case raise, lower, flatten, smooth, sharpen
        case noise, stamp, erosion, slope
        case paint, paintErase
        case grassDensity, grassType
        case riverCarve, lakeFill
        case erosionMask, erosionPreview
        case hydraulicErosion, thermalErosion
        case autoPaint, textureRules
        case heightmapFill, heightmapInvert, heightmapNormalize
        case vegetationPaint, vegetationRemove
        case pinch, bulge, terrace, path, bridge, clone
        
        var icon: String {
            switch self {
            case .raise: return "arrow.up"
            case .lower: return "arrow.down"
            case .flatten: return "minus"
            case .smooth: return "waveform.path"
            case .sharpen: return "triangle"
            case .noise: return "cloud"
            case .stamp: return "stamp"
            case .erosion: return "drop"
            case .slope: return "angle"
            case .paint: return "paintbrush"
            case .paintErase: return "eraser"
            case .grassDensity: return "leaf"
            case .grassType: return "leaf.arrow.triangle.circlepath"
            case .riverCarve: return "water.waves"
            case .lakeFill: return "drop.circle"
            case .erosionMask: return "mask"
            case .erosionPreview: return "eye"
            case .hydraulicErosion: return "drop.fill"
            case .thermalErosion: return "flame"
            case .autoPaint: return "wand.and.stars"
            case .textureRules: return "gear"
            case .heightmapFill: return "square.fill"
            case .heightmapInvert: return "arrow.up.arrow.down"
            case .heightmapNormalize: return "arrow.up.square"
            case .vegetationPaint: return "tree.fill"
            case .vegetationRemove: return "tree"
            case .pinch: return "arrow.up.and.down.text.horizontal"
            case .bulge: return "arrow.up.and.down.text.horizontal"
            case .terrace: return "rectangle.stack"
            case .path: return "line.diagonal"
            case .bridge: return "bridge"
            case .clone: return "doc.on.doc"
            }
        }
    }
    
    enum StampShape: String, CaseIterable {
        case circle, square, triangle, hexagon
        case star, cross, diamond, heart
        
        var icon: String {
            switch self {
            case .circle: return "circle"
            case .square: return "square"
            case .triangle: return "triangle"
            case .hexagon: return "hexagon"
            case .star: return "star"
            case .cross: return "plus"
            case .diamond: return "diamond"
            case .heart: return "heart"
            }
        }
    }
    
    enum LODQualityPreset: String, CaseIterable {
        case low = "Low"
        case medium = "Medium"
        case balanced = "Balanced"
        case high = "High"
        case ultra = "Ultra"
        
        var lodLevels: Int {
            switch self {
            case .low: return 2
            case .medium: return 3
            case .balanced: return 4
            case .high: return 5
            case .ultra: return 6
            }
        }
        
        var distances: [Float] {
            switch self {
            case .low: return [25, 50]
            case .medium: return [30, 60, 120]
            case .balanced: return [50, 100, 200, 400]
            case .high: return [40, 80, 160, 320, 640]
            case .ultra: return [30, 60, 120, 240, 480, 960]
            }
        }
        
        var screenSpaceError: Float {
            switch self {
            case .low: return 8.0
            case .medium: return 4.0
            case .balanced: return 2.0
            case .high: return 1.0
            case .ultra: return 0.5
            }
        }
    }
    
    init() {
        layers = [
            TerrainLayer(name: "Grass", texture: "grass_diffuse.png", tiling: 10),
            TerrainLayer(name: "Dirt", texture: "dirt_diffuse.png", tiling: 15),
            TerrainLayer(name: "Rock", texture: "rock_diffuse.png", tiling: 5),
            TerrainLayer(name: "Sand", texture: "sand_diffuse.png", tiling: 12)
        ]
        
        // Initialize LOD distances based on preset
        lodDistances = lodQualityPreset.distances
        lodLevels = lodQualityPreset.lodLevels
        lodScreenSpaceError = lodQualityPreset.screenSpaceError
        
        grassDetailMeshes = [
            GrassDetailMesh(name: "Standard Grass", meshPath: "grass_standard.fbx", density: 1.0),
            GrassDetailMesh(name: "Tall Grass", meshPath: "grass_tall.fbx", density: 0.7),
            GrassDetailMesh(name: "Short Grass", meshPath: "grass_short.fbx", density: 1.5),
            GrassDetailMesh(name: "Wild Grass", meshPath: "grass_wild.fbx", density: 0.8)
        ]
        
        // Initialize erosion presets
        initializeErosionPresets()
    }
    
    // MARK: - Brush Operations
    
    func applyBrush(at position: SIMD3<Float>, pressure: Float = 1.0) {
        switch selectedBrush {
        case .raise:
            applyRaiseBrush(at: position, pressure: pressure)
        case .lower:
            applyLowerBrush(at: position, pressure: pressure)
        case .flatten:
            applyFlattenBrush(at: position, pressure: pressure)
        case .smooth:
            applySmoothBrush(at: position, pressure: pressure)
        case .sharpen:
            applySharpenBrush(at: position, pressure: pressure)
        case .noise:
            applyNoiseBrush(at: position, pressure: pressure)
        case .stamp:
            applyStampBrush(at: position, pressure: pressure)
        case .pinch:
            applyPinchBrush(at: position, pressure: pressure)
        case .bulge:
            applyBulgeBrush(at: position, pressure: pressure)
        case .terrace:
            applyTerraceBrush(at: position, pressure: pressure)
        case .path:
            applyPathBrush(at: position, pressure: pressure)
        case .bridge:
            applyBridgeBrush(at: position, pressure: pressure)
        case .clone:
            applyCloneBrush(at: position, pressure: pressure)
        default:
            break
        }
        
        if perStrokeUndo {
            saveToHistory()
        }
    }
    
    private func applyRaiseBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        let effectiveRotation = brushRotation + brushRotationRandomization * Float.random(in: -180...180)
        let effectiveStrength = brushOpacity * pressure * (pressureSensitivity ? pressure : 1.0)
        
        // Apply height increase with falloff and hardness
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let height = effectiveStrength * falloff * brushHardness
                    // Apply to heightmap
                }
            }
        }
    }
    
    private func applyLowerBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveStrength = -brushOpacity * pressure * (pressureSensitivity ? pressure : 1.0)
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let height = effectiveStrength * falloff * brushHardness
                    // Apply to heightmap with subtractive mode
                }
            }
        }
    }
    
    private func applyFlattenBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        let targetHeight = flattenTargetHeight
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let blend = falloff * brushHardness * pressure
                    // Blend current height toward target height
                }
            }
        }
    }
    
    private func applySmoothBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        // Apply smoothing with configurable iterations
        for _ in 0..<smoothIterations {
            var heightSum: Float = 0
            var sampleCount = 0
            
            // Sample surrounding heights
            for dy in -1...1 {
                for dx in -1...1 {
                    if dx == 0 && dy == 0 { continue }
                    let sampleX = centerX + dx
                    let sampleY = centerY + dy
                    if sampleX >= 0 && sampleX < resolution && sampleY >= 0 && sampleY < resolution {
                        // Sample height at this position
                        heightSum += 0 // Get actual heightmap value
                        sampleCount += 1
                    }
                }
            }
            
            if sampleCount > 0 {
                let averageHeight = heightSum / Float(sampleCount)
                let currentHeight = 0 // Get current heightmap value
                let smoothedHeight = currentHeight * (1.0 - brushHardness * pressure) + averageHeight * brushHardness * pressure
                // Apply smoothed height
            }
        }
    }
    
    private func applySharpenBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        // Apply edge enhancement with configurable iterations
        for _ in 0..<sharpenIterations {
            var heightSum: Float = 0
            var edgeSum: Float = 0
            var sampleCount = 0
            
            // Sample surrounding heights for edge detection
            for dy in -1...1 {
                for dx in -1...1 {
                    let sampleX = centerX + dx
                    let sampleY = centerY + dy
                    if sampleX >= 0 && sampleX < resolution && sampleY >= 0 && sampleY < resolution {
                        let height = 0 // Get actual heightmap value
                        heightSum += height
                        
                        // Calculate edge strength using Sobel-like operator
                        if abs(dx) + abs(dy) == 1 {
                            edgeSum += abs(height - 0) // Get center height
                        }
                        sampleCount += 1
                    }
                }
            }
            
            if sampleCount > 0 {
                let averageHeight = heightSum / Float(sampleCount)
                let edgeStrength = edgeSum / 4.0
                let currentHeight = 0 // Get current heightmap value
                let sharpenedHeight = currentHeight + (currentHeight - averageHeight) * edgeStrength * brushHardness * pressure
                // Apply sharpened height with clamping
            }
        }
    }
    
    private func applyNoiseBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let worldPos = SIMD3<Float>(
                        Float(centerX + dx) * terrainSize / Float(resolution),
                        0,
                        Float(centerY + dy) * terrainSize / Float(resolution)
                    )
                    let noiseValue = generateNoise(at: worldPos, scale: noiseScale) * noiseStrength * pressure
                    let height = noiseValue * falloff * brushHardness
                    // Apply noise to heightmap
                }
            }
        }
    }
    
    private func applyStampBrush(at position: SIMD3<Float>, pressure: Float) {
        // Apply custom shape stamp
        switch stampShape {
        case .circle:
            applyCircleStamp(at: position, pressure: pressure)
        case .square:
            applySquareStamp(at: position, pressure: pressure)
        case .triangle:
            applyTriangleStamp(at: position, pressure: pressure)
        case .hexagon:
            applyHexagonStamp(at: position, pressure: pressure)
        case .star:
            applyStarStamp(at: position, pressure: pressure)
        case .cross:
            applyCrossStamp(at: position, pressure: pressure)
        case .diamond:
            applyDiamondStamp(at: position, pressure: pressure)
        case .heart:
            applyHeartStamp(at: position, pressure: pressure)
        }
    }
    
    private func applyPinchBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        let strength = pinchBulgeStrength * pressure
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let direction = SIMD2<Float>(Float(dx), Float(dy))
                    let directionLength = length(direction)
                    
                    if directionLength > 0 {
                        let normalizedDirection = direction / directionLength
                        let pullStrength = strength * falloff * brushHardness * (1.0 - dist / Float(brushRadius))
                        
                        // Apply inward deformation toward center
                        let deformAmount = normalizedDirection * pullStrength
                        // Apply deformation to heightmap gradient
                    }
                }
            }
        }
    }
    
    private func applyBulgeBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        let strength = pinchBulgeStrength * pressure
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let direction = SIMD2<Float>(Float(dx), Float(dy))
                    let directionLength = length(direction)
                    
                    if directionLength > 0 {
                        let normalizedDirection = direction / directionLength
                        let pushStrength = strength * falloff * brushHardness * (1.0 - dist / Float(brushRadius))
                        
                        // Apply outward deformation away from center
                        let deformAmount = normalizedDirection * pushStrength
                        // Apply deformation to heightmap gradient
                    }
                }
            }
        }
    }
    
    private func applyTerraceBrush(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        let stepHeight = terraceStepHeight
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let currentHeight = 0 // Get current heightmap value
                    
                    // Create stepped terrain by quantizing to step heights
                    let steppedHeight = round(currentHeight / stepHeight) * stepHeight
                    let blendStrength = falloff * brushHardness * pressure
                    let newHeight = currentHeight * (1.0 - blendStrength) + steppedHeight * blendStrength
                    
                    // Apply terraced height
                }
            }
        }
    }
    
    private func applyPathBrush(at position: SIMD3<Float>, pressure: Float) {
        let width = pathWidth
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let pathRadius = Int(width * Float(resolution) / terrainSize / 2.0)
        
        // Create path by flattening along a line
        for dy in -pathRadius...pathRadius {
            for dx in -pathRadius...pathRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(pathRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(pathRadius))
                    let currentHeight = 0 // Get current heightmap value
                    let targetHeight = position.y // Use brush Y position as path height
                    
                    let blendStrength = falloff * brushHardness * pressure
                    let newHeight = currentHeight * (1.0 - blendStrength) + targetHeight * blendStrength
                    
                    // Apply path height with smooth edges
                }
            }
        }
        
        // Store path point for continuous path creation
        // pathPoints.append(SIMD2<Float>(position.x, position.z))
    }
    
    private func applyBridgeBrush(at position: SIMD3<Float>, pressure: Float) {
        let strength = bridgeStrength * pressure
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        // Bridge by creating smooth interpolation between terrain points
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let currentHeight = 0 // Get current heightmap value
                    
                    // Calculate bridge height based on surrounding terrain
                    var surroundingHeight: Float = 0
                    var sampleCount = 0
                    
                    for sampleDy in -2...2 {
                        for sampleDx in -2...2 {
                            if abs(sampleDx) > 1 || abs(sampleDy) > 1 {
                                let sampleX = centerX + sampleDx
                                let sampleY = centerY + sampleDy
                                if sampleX >= 0 && sampleX < resolution && sampleY >= 0 && sampleY < resolution {
                                    surroundingHeight += 0 // Get heightmap value
                                    sampleCount += 1
                                }
                            }
                        }
                    }
                    
                    if sampleCount > 0 {
                        let averageHeight = surroundingHeight / Float(sampleCount)
                        let bridgeHeight = currentHeight * (1.0 - strength * falloff) + averageHeight * strength * falloff
                        
                        // Apply bridge height with smooth blending
                    }
                }
            }
        }
    }
    
    private func applyCloneBrush(at position: SIMD3<Float>, pressure: Float) {
        guard let source = cloneSource else { return }
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let destCenterX = Int(position.x * Float(resolution) / terrainSize)
        let destCenterY = Int(position.z * Float(resolution) / terrainSize)
        let sourceCenterX = Int(source.x * Float(resolution) / terrainSize)
        let sourceCenterY = Int(source.y * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    
                    let sourceX = sourceCenterX + dx
                    let sourceY = sourceCenterY + dy
                    let destX = destCenterX + dx
                    let destY = destCenterY + dy
                    
                    if sourceX >= 0 && sourceX < resolution && sourceY >= 0 && sourceY < resolution &&
                       destX >= 0 && destX < resolution && destY >= 0 && destY < resolution {
                        
                        let sourceHeight = 0 // Get heightmap value at source
                        let currentHeight = 0 // Get heightmap value at destination
                        let blendStrength = falloff * brushHardness * pressure
                        let newHeight = currentHeight * (1.0 - blendStrength) + sourceHeight * blendStrength
                        
                        // Apply cloned height
                    }
                }
            }
        }
    }
    
    private func calculateFalloff(distance: Float, radius: Float) -> Float {
        let normalizedDist = distance / radius
        switch brushFalloff {
        case 0.0: // Linear
            return 1.0 - normalizedDist
        case 0.5: // Smooth
            return pow(1.0 - normalizedDist, 2.0)
        case 1.0: // Hard
            return normalizedDist < 0.5 ? 1.0 : 0.0
        default: // Custom curve
            return pow(1.0 - normalizedDist, 1.0 + brushFalloff * 2.0)
        }
    }
    
    private func generateNoise(at position: SIMD3<Float>, scale: Float) -> Float {
        // Generate procedural noise (simplified Perlin-like noise)
        let x = position.x * scale
        let y = position.z * scale
        return sin(x * 0.1) * cos(y * 0.1) + sin(x * 0.2) * cos(y * 0.2) * 0.5
    }
    
    private func applyCircleStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let height = brushOpacity * falloff * brushHardness * pressure
                    // Apply circular stamp height
                }
            }
        }
    }
    
    private func applySquareStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let height = brushOpacity * falloff * brushHardness * pressure
                    // Apply square stamp height
                }
            }
        }
    }
    
    private func applyTriangleStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    // Triangle shape: use Manhattan distance for triangular falloff
                    let triangleDist = abs(Float(dx)) + abs(Float(dy))
                    if triangleDist <= Float(brushRadius) {
                        let falloff = calculateFalloff(distance: triangleDist, radius: Float(brushRadius))
                        let height = brushOpacity * falloff * brushHardness * pressure
                        // Apply triangle stamp height
                    }
                }
            }
        }
    }
    
    private func applyHexagonStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    // Hexagon shape: check if point is within hexagon bounds
                    let hexDist = max(abs(Float(dx)), abs(Float(dy) * 0.866)) // 0.866 ≈ sqrt(3)/2
                    if hexDist <= Float(brushRadius) {
                        let falloff = calculateFalloff(distance: hexDist, radius: Float(brushRadius))
                        let height = brushOpacity * falloff * brushHardness * pressure
                        // Apply hexagon stamp height
                    }
                }
            }
        }
    }
    
    private func applyStarStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    // Star shape: 5-pointed star pattern
                    let angle = atan2(Float(dy), Float(dx))
                    let starRadius = Float(brushRadius) * (0.4 + 0.6 * abs(cos(5 * angle)))
                    if dist <= starRadius {
                        let falloff = calculateFalloff(distance: dist, radius: starRadius)
                        let height = brushOpacity * falloff * brushHardness * pressure
                        // Apply star stamp height
                    }
                }
            }
        }
    }
    
    private func applyCrossStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                // Cross shape: only apply on horizontal and vertical lines
                if (dx == 0 && abs(dy) <= brushRadius) || (dy == 0 && abs(dx) <= brushRadius) {
                    let dist = sqrt(Float(dx*dx + dy*dy))
                    let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                    let height = brushOpacity * falloff * brushHardness * pressure
                    // Apply cross stamp height
                }
            }
        }
    }
    
    private func applyDiamondStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                // Diamond shape: use Manhattan distance for diamond falloff
                let diamondDist = abs(Float(dx)) + abs(Float(dy))
                if diamondDist <= Float(brushRadius) {
                    let falloff = calculateFalloff(distance: diamondDist, radius: Float(brushRadius))
                    let height = brushOpacity * falloff * brushHardness * pressure
                    // Apply diamond stamp height
                }
            }
        }
    }
    
    private func applyHeartStamp(at position: SIMD3<Float>, pressure: Float) {
        let effectiveSize = brushSize * (1.0 + brushScaleRandomization * Float.random(in: -1...1))
        
        let resolution = terrainResolution
        let centerX = Int(position.x * Float(resolution) / terrainSize)
        let centerY = Int(position.z * Float(resolution) / terrainSize)
        let brushRadius = Int(effectiveSize * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    // Heart shape: mathematical heart function
                    let x = Float(dx) / Float(brushRadius)
                    let y = Float(dy) / Float(brushRadius)
                    let heartValue = pow(x*x + y*y - 1, 3) - x*x * y*y*y
                    
                    if heartValue <= 0 {
                        let falloff = calculateFalloff(distance: dist, radius: Float(brushRadius))
                        let height = brushOpacity * falloff * brushHardness * pressure
                        // Apply heart stamp height
                    }
                }
            }
        }
    }
    
    // MARK: - Grass Operations
    func paintGrassDensity(at position: SIMD3<Float>, intensity: Float) {
        // Implementation for painting grass density
    }
    
    func setGrassType(_ type: GrassType, in region: CGRect) {
        // Implementation for setting grass type in region
    }
    
    // MARK: - Vegetation Operations
    func paintVegetation(at position: SIMD3<Float>, type: VegetationType) {
        let instance = VegetationInstance(
            id: UUID(),
            type: type,
            position: position,
            rotation: Float.random(in: 0...2*Float.pi),
            scale: Float.random(in: 0.8...1.2)
        )
        vegetationInstances.append(instance)
    }
    
    func removeVegetation(at position: SIMD3<Float>, radius: Float) {
        vegetationInstances.removeAll { instance in
            let distance = simd_distance(instance.position, position)
            return distance <= radius
        }
    }
    
    func generateProceduralVegetation() {
        // Implementation for procedural vegetation scattering
    }
    
    // MARK: - Heightmap Operations
    func importHeightmap(url: URL, format: HeightmapFormat) {
        // Implementation for heightmap import
    }
    
    func exportHeightmap(url: URL, format: HeightmapFormat) {
        // Implementation for heightmap export
    }
    
    func normalizeHeightmap() {
        // Implementation for heightmap normalization
    }
    
    func invertHeightmap() {
        // Implementation for heightmap inversion
    }
    
    func fillHeightmap(value: Float, region: CGRect? = nil) {
        // Implementation for heightmap fill
    }
    
    func copyHeightmapRegion(_ region: CGRect) {
        // Implementation for copying heightmap region
    }
    
    func pasteHeightmapRegion(at position: CGPoint) {
        // Implementation for pasting heightmap region
    }
    
    // MARK: - Erosion Operations
    func initializeErosionPresets() {
        erosionPresets = [
            ErosionPreset(
                name: "Desert",
                description: "Wind and thermal erosion for sandy landscapes",
                settings: ErosionSettings(
                    hydraulicIterations: 20,
                    rainAmount: 0.05,
                    evaporationRate: 0.02,
                    sedimentCapacity: 0.8,
                    erosionStrength: 0.2,
                    depositionStrength: 0.1,
                    thermalIterations: 50,
                    talusAngle: 35.0,
                    thermalStrength: 0.4,
                    windEnabled: true,
                    windDirection: SIMD2<Float>(1.0, 0.0),
                    windStrength: 0.3,
                    sandThreshold: 0.05
                )
            ),
            ErosionPreset(
                name: "Alpine",
                description: "Hydraulic and thermal erosion for mountainous terrain",
                settings: ErosionSettings(
                    hydraulicIterations: 80,
                    rainAmount: 0.15,
                    evaporationRate: 0.005,
                    sedimentCapacity: 0.4,
                    erosionStrength: 0.5,
                    depositionStrength: 0.3,
                    thermalIterations: 40,
                    talusAngle: 25.0,
                    thermalStrength: 0.3,
                    windEnabled: false,
                    coastalEnabled: false
                )
            ),
            ErosionPreset(
                name: "Coastal",
                description: "Wave and coastal erosion for shorelines",
                settings: ErosionSettings(
                    hydraulicIterations: 60,
                    rainAmount: 0.1,
                    evaporationRate: 0.01,
                    sedimentCapacity: 0.6,
                    erosionStrength: 0.3,
                    depositionStrength: 0.4,
                    thermalIterations: 20,
                    talusAngle: 20.0,
                    thermalStrength: 0.2,
                    coastalEnabled: true,
                    waveStrength: 0.4,
                    waveFrequency: 0.8,
                    tidalRange: 3.0
                )
            ),
            ErosionPreset(
                name: "River Valley",
                description: "Focused hydraulic erosion for river formation",
                settings: ErosionSettings(
                    hydraulicIterations: 100,
                    rainAmount: 0.2,
                    evaporationRate: 0.008,
                    sedimentCapacity: 0.7,
                    erosionStrength: 0.6,
                    depositionStrength: 0.2,
                    minSlope: 0.005,
                    thermalIterations: 10,
                    talusAngle: 30.0,
                    thermalStrength: 0.15
                )
            )
        ]
    }
    
    func applyErosionPreset(_ preset: ErosionPreset) {
        erosionSettings = preset.settings
        selectedErosionPreset = erosionPresets.firstIndex(where: { $0.id == preset.id }) ?? 0
    }
    
    func simulateHydraulicErosion(iterations: Int) {
        // Implementation for hydraulic erosion simulation
        for i in 0..<iterations {
            // Simulate water flow and sediment transport
            let currentStrength = erosionStrength * Float(i + 1) / Float(iterations)
            
            // Apply erosion based on mask if enabled
            if erosionMaskEnabled {
                // Apply erosion only where mask allows
            } else {
                // Apply erosion everywhere
            }
            
            // Update real-time preview if enabled
            if realTimePreview {
                // Update preview
            }
        }
    }
    
    func simulateThermalErosion(iterations: Int) {
        // Implementation for thermal erosion simulation
        for i in 0..<iterations {
            // Simulate material slumping and talus formation
            let talusRadians = erosionSettings.talusAngle * Float.pi / 180.0
            
            // Apply thermal erosion
            if realTimePreview {
                // Update preview
            }
        }
    }
    
    func simulateWindErosion(iterations: Int) {
        guard erosionSettings.windEnabled else { return }
        
        for i in 0..<iterations {
            // Simulate wind-driven sand transport
            let windDir = normalize(erosionSettings.windDirection)
            let windForce = erosionSettings.windStrength
            
            // Apply wind erosion
            if realTimePreview {
                // Update preview
            }
        }
    }
    
    func simulateCoastalErosion(iterations: Int) {
        guard erosionSettings.coastalEnabled else { return }
        
        for i in 0..<iterations {
            // Simulate wave action and tidal effects
            let wavePhase = Float(i) * erosionSettings.waveFrequency * 2.0 * Float.pi / Float(iterations)
            
            // Apply coastal erosion
            if realTimePreview {
                // Update preview
            }
        }
    }
    
    func runFullErosionSimulation() {
        saveToHistory() // Save state before erosion
        
        simulateHydraulicErosion(iterations: erosionSettings.hydraulicIterations)
        simulateThermalErosion(iterations: erosionSettings.thermalIterations)
        simulateWindErosion(iterations: erosionSettings.windEnabled ? 30 : 0)
        simulateCoastalErosion(iterations: erosionSettings.coastalEnabled ? 20 : 0)
    }
    
    func paintErosionMask(at position: SIMD2<Float>, radius: Float, value: Float) {
        // Implementation for painting erosion mask
        let resolution = 512
        let x = Int(position.x * Float(resolution) / terrainSize)
        let y = Int(position.y * Float(resolution) / terrainSize)
        let brushRadius = Int(radius * Float(resolution) / terrainSize)
        
        for dy in -brushRadius...brushRadius {
            for dx in -brushRadius...brushRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(brushRadius) {
                    let maskX = x + dx
                    let maskY = y + dy
                    // Set mask value with falloff
                }
            }
        }
    }
    
    // MARK: - Water Operations
    func carveRiverbed(spline: RiverSpline) {
        guard spline.controlPoints.count >= 2 else { return }
        
        for i in 0..<spline.controlPoints.count - 1 {
            let start = spline.controlPoints[i]
            let end = spline.controlPoints[i + 1]
            let width = spline.widthAtPoint[min(i, spline.widthAtPoint.count - 1)]
            let depth = spline.depthAtPoint[min(i, spline.depthAtPoint.count - 1)]
            
            // Carve terrain along river path
            let steps = 20
            for step in 0...steps {
                let t = Float(step) / Float(steps)
                let position = start + (end - start) * t
                
                // Lower terrain at this position
                let radius = width / 2.0
                // Apply terrain deformation
            }
        }
    }
    
    func generateRiverPath(startPoint: SIMD3<Float>, endPoint: SIMD3<Float>) -> RiverSpline {
        var spline = RiverSpline()
        
        // Generate intermediate points following terrain slope
        let numPoints = 10
        for i in 0...numPoints {
            let t = Float(i) / Float(numPoints)
            var point = startPoint + (endPoint - startPoint) * t
            
            // Adjust point to follow terrain slope
            // Sample terrain height at this position
            // point.y = terrainHeight
            
            let width = riverWidth * (1.0 + 0.3 * sin(t * Float.pi)) // Vary width along river
            let depth = riverDepth * (0.8 + 0.4 * cos(t * Float.pi * 2)) // Vary depth
            
            spline.addPoint(point, width: width, depth: depth)
        }
        
        return spline
    }
    
    func createLake(center: SIMD2<Float>, radius: Float, waterLevel: Float) -> LakeData {
        let lake = LakeData(center: center, radius: radius, waterLevel: waterLevel, depth: lakeWaterLevel)
        
        // Carve lake basin
        let resolution = 512
        let centerX = Int(center.x * Float(resolution) / terrainSize)
        let centerY = Int(center.y * Float(resolution) / terrainSize)
        let lakeRadius = Int(radius * Float(resolution) / terrainSize)
        
        for dy in -lakeRadius...lakeRadius {
            for dx in -lakeRadius...lakeRadius {
                let dist = sqrt(Float(dx*dx + dy*dy))
                if dist <= Float(lakeRadius) {
                    let x = centerX + dx
                    let y = centerY + dx
                    // Calculate depth based on distance from center
                    let depthFactor = 1.0 - (dist / Float(lakeRadius))
                    let depth = lake.depth * depthFactor * depthFactor // Parabolic profile
                    
                    // Set terrain height to waterLevel - depth
                }
            }
        }
        
        return lake
    }
    
    func fillLakeToWaterLevel(_ lake: LakeData) {
        // Fill lake area with water up to specified level
        // This would interact with the water rendering system
    }
    
    // MARK: - Heightmap History Management
    func createSnapshot() -> HeightmapSnapshot {
        return HeightmapSnapshot(
            id: UUID(),
            timestamp: Date(),
            scale: heightmapScale,
            offset: heightmapOffset,
            minClamp: heightmapMinClamp,
            maxClamp: heightmapMaxClamp,
            description: "Snapshot \(undoHistory.count + 1)"
        )
    }
    
    func saveToHistory() {
        let snapshot = createSnapshot()
        undoHistory.append(snapshot)
        currentSnapshot = snapshot
        redoHistory.removeAll()
        
        // Limit history size
        if undoHistory.count > 50 {
            undoHistory.removeFirst()
        }
    }
    
    func undo() {
        guard let current = undoHistory.popLast() else { return }
        redoHistory.append(current)
        
        if let previous = undoHistory.last {
            restoreSnapshot(previous)
        }
    }
    
    func redo() {
        guard let next = redoHistory.popLast() else { return }
        undoHistory.append(next)
        restoreSnapshot(next)
    }
    
    func restoreSnapshot(_ snapshot: HeightmapSnapshot) {
        heightmapScale = snapshot.scale
        heightmapOffset = snapshot.offset
        heightmapMinClamp = snapshot.minClamp
        heightmapMaxClamp = snapshot.maxClamp
        currentSnapshot = snapshot
    }
    
    // MARK: - LOD Management
    func applyLODPreset(_ preset: LODQualityPreset) {
        lodQualityPreset = preset
        lodLevels = preset.lodLevels
        lodDistances = preset.distances
        lodScreenSpaceError = preset.screenSpaceError
        updateLODMetrics()
    }
    
    func updateLODMetrics() {
        // Simulate LOD metrics calculation
        let baseTriangles = terrainResolution * terrainResolution * 2
        let lodReduction = pow(0.25, Float(lodLevels - 1))
        lodTriangleCount = Int(Float(baseTriangles) * lodReduction)
        lodFrameTime = Float(lodTriangleCount) / 1000000.0 // Simulated frame time
        lodMemoryUsage = Float(lodTriangleCount * 12) / (1024 * 1024) // 12 bytes per triangle
    }
}

struct TerrainLayer: Identifiable {
    let id = UUID()
    var name: String
    var texture: String
    var tiling: Float
    var normalMap: String = ""
    var metallic: Float = 0.0
    var smoothness: Float = 0.5
    var isVisible: Bool = true
    var isLocked: Bool = false
    var textureOffset: SIMD2<Float> = .zero
    var textureAnimationSpeed: SIMD2<Float> = .zero
    var detailMesh: String = ""
    var detailDensity: Float = 1.0
    
    // Texture rules
    var slopeRule: SlopeRule?
    var heightRule: HeightRule?
    var curvatureRule: CurvatureRule?
}

struct SlopeRule {
    var minSlope: Float = 0.0
    var maxSlope: Float = 90.0
    var falloff: Float = 0.1
}

struct HeightRule {
    var minHeight: Float = 0.0
    var maxHeight: Float = 1000.0
    var falloff: Float = 0.1
}

struct CurvatureRule {
    var minCurvature: Float = -1.0
    var maxCurvature: Float = 1.0
    var falloff: Float = 0.1
}

// MARK: - Heightmap Management
struct HeightmapSnapshot: Identifiable {
    let id: UUID
    let timestamp: Date
    let scale: Float
    let offset: Float
    let minClamp: Float
    let maxClamp: Float
    let data: Data
    let description: String
}

// MARK: - Grass System
enum GrassType: String, CaseIterable {
    case standard, tall, short, wild
    
    var icon: String {
        switch self {
        case .standard: return "sprout"
        case .tall: return "sprout.fill"
        case .short: return "leaf"
        case .wild: return "leaf.fill"
        }
    }
}

struct GrassDetailMesh: Identifiable {
    let id = UUID()
    let name: String
    let meshPath: String
    var density: Float
    var windResponse: Float = 0.3
    var colorVariation: SIMD3<Float> = SIMD3<Float>(0.2, 0.2, 0.2)
}

// MARK: - Vegetation System
enum VegetationType: String, CaseIterable {
    case oak, pine, birch, bush, rock
    
    var icon: String {
        switch self {
        case .oak: return "tree"
        case .pine: return "tree.fill"
        case .birch: return "leaf"
        case .bush: return "leaf.fill"
        case .rock: return "mountain"
        }
    }
}

struct VegetationInstance: Identifiable {
    let id: UUID
    let type: VegetationType
    let position: SIMD3<Float>
    let rotation: Float
    let scale: Float
}

struct VegetationPlacementRules {
    var slopeRange: ClosedRange<Float> = 0...45
    var heightRange: ClosedRange<Float> = 0...100
    var densityMap: String = ""
    var randomSeed: Int = 12345
    var minDistance: Float = 2.0
    var maxInstances: Int = 1000
}

struct ExclusionZone: Identifiable {
    let id = UUID()
    let position: SIMD2<Float>
    let radius: Float
    let types: Set<VegetationType>
}

// MARK: - Erosion System
struct ErosionSettings {
    var hydraulicIterations: Int = 50
    var rainAmount: Float = 0.1
    var evaporationRate: Float = 0.01
    var sedimentCapacity: Float = 0.5
    var erosionStrength: Float = 0.3
    var depositionStrength: Float = 0.2
    var minSlope: Float = 0.01
    
    var thermalIterations: Int = 30
    var talusAngle: Float = 30.0
    var thermalStrength: Float = 0.2
    
    var windEnabled: Bool = false
    var windDirection: SIMD2<Float> = SIMD2<Float>(1.0, 0.0)
    var windStrength: Float = 0.1
    var sandThreshold: Float = 0.1
    
    var coastalEnabled: Bool = false
    var waveStrength: Float = 0.2
    var waveFrequency: Float = 0.5
    var tidalRange: Float = 2.0
}

struct ErosionPreset: Identifiable {
    let id = UUID()
    let name: String
    let description: String
    let settings: ErosionSettings
}

struct TerrainState: Identifiable {
    let id = UUID()
    let timestamp: Date
    let heightmapData: Data
    let description: String
}

struct ErosionMask {
    var resolution: Int = 512
    var data: [Float] = []
    var isEnabled: Bool = false
    
    init(resolution: Int) {
        self.resolution = resolution
        self.data = Array(repeating: 1.0, count: resolution * resolution)
    }
    
    mutating func setMaskValue(at x: Int, y: Int, value: Float) {
        guard x >= 0 && x < resolution && y >= 0 && y < resolution else { return }
        data[y * resolution + x] = value
    }
    
    func getMaskValue(at x: Int, y: Int) -> Float {
        guard x >= 0 && x < resolution && y >= 0 && y < resolution else { return 0.0 }
        return data[y * resolution + x]
    }
}

// MARK: - Water System
struct RiverSpline {
    var controlPoints: [SIMD3<Float>] = []
    var widthAtPoint: [Float] = []
    var depthAtPoint: [Float] = []
    var flowSpeed: Float = 1.0
    var foamAmount: Float = 0.1
    var carveTerrain: Bool = true
    var carveDepth: Float = 2.0
    var addVegetation: Bool = true
    
    mutating func addPoint(_ point: SIMD3<Float>, width: Float = 5.0, depth: Float = 2.0) {
        controlPoints.append(point)
        widthAtPoint.append(width)
        depthAtPoint.append(depth)
    }
    
    mutating func removePoint(at index: Int) {
        guard index >= 0 && index < controlPoints.count else { return }
        controlPoints.remove(at: index)
        widthAtPoint.remove(at: index)
        depthAtPoint.remove(at: index)
    }
}

struct LakeData {
    var center: SIMD2<Float>
    var radius: Float
    var waterLevel: Float
    var depth: Float
    var shoreErosion: Float = 0.1
    
    init(center: SIMD2<Float>, radius: Float, waterLevel: Float, depth: Float) {
        self.center = center
        self.radius = radius
        self.waterLevel = waterLevel
        self.depth = depth
    }
}

// MARK: - Heightmap System
enum HeightmapPrecision: String, CaseIterable {
    case bit8, bit16, bit32
    
    var bytesPerPixel: Int {
        switch self {
        case .bit8: return 1
        case .bit16: return 2
        case .bit32: return 4
        }
    }
}

enum HeightmapFormat: String, CaseIterable {
    case png, raw, r16
    
    var fileExtension: String {
        switch self {
        case .png: return "png"
        case .raw: return "raw"
        case .r16: return "r16"
        }
    }
}

struct HeightmapSnapshot {
    let id = UUID()
    let timestamp: Date
    let data: Data
    let description: String
}

struct HeightmapRegion {
    let rect: CGRect
    let data: [[Float]]
    let resolution: Int
}

// MARK: - Terrain Editor View
struct TerrainEditorView: View {
    @ObservedObject var manager = TerrainEditorManager.shared
    @StateObject private var tooltipManager = TerrainTooltipManager.shared
    @StateObject private var analyticsManager = TerrainAnalyticsManager.shared
    @State private var showSettings = false
    @State private var showTooltipSettings = false
    
    var body: some View {
        ZStack {
            VStack(spacing: 0) {
                // Toolbar
                TerrainToolbar(manager: manager, showSettings: $showSettings, showTooltipSettings: $showTooltipSettings)
                
                EditorDivider()
                
                HSplitView {
                    // Brush panel
                    TerrainBrushPanel(manager: manager)
                        .frame(minWidth: 200, maxWidth: 280)
                    
                    // Viewport (placeholder)
                    ZStack {
                        Color.black
                        VStack {
                            Text("Terrain Viewport")
                                .font(DesignSystem.Typography.h2)
                                .foregroundColor(.gray)
                            Text("Brush: \(manager.selectedBrush.rawValue.capitalized)")
                                .foregroundColor(.gray)
                            Text("Size: \(Int(manager.brushSize))")
                                .foregroundColor(.gray)
                        }
                    }
                    
                    // Layers panel
                    TerrainLayersPanel(manager: manager)
                        .frame(minWidth: 200, maxWidth: 280)
                }
            }
            
            // Tooltip overlay
            TerrainTooltipOverlay()
        }
        .sheet(isPresented: $showSettings) {
            TerrainSettingsSheet(manager: manager)
        }
        .sheet(isPresented: $showTooltipSettings) {
            TerrainTooltipSettings()
        }
        .sheet(isPresented: $analyticsManager.showAnalyticsPanel) {
            TerrainAnalyticsView()
        }
    }
}

// MARK: - Terrain Toolbar
struct TerrainToolbar: View {
    @ObservedObject var manager: TerrainEditorManager
    @Binding var showSettings: Bool
    @Binding var showTooltipSettings: Bool
    @StateObject private var tooltipManager = TerrainTooltipManager.shared
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Brush type selector
            Picker("", selection: $manager.selectedBrush) {
                ForEach(TerrainEditorManager.BrushType.allCases, id: \.self) { brush in
                    Label(brush.rawValue.capitalized, systemImage: brush.icon)
                        .tag(brush)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 120)
            
            Divider().frame(height: 20)
            
            // Brush size
            HStack(spacing: 4) {
                Image(systemName: "circle")
                Slider(value: $manager.brushSize, in: 1...100)
                    .frame(width: 100)
                Text("\(Int(manager.brushSize))")
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 30)
            }
            
            // Brush opacity
            HStack(spacing: 4) {
                Image(systemName: "circle.fill")
                    .opacity(Double(manager.brushOpacity))
                Slider(value: $manager.brushOpacity, in: 0...1)
                    .frame(width: 80)
            }
            
            Spacer()
            
            // Import/Export
            Button("Import Heightmap") { }
            Button("Export Heightmap") { }
            
            Divider().frame(height: 20)
            
            // Tooltip toggle
            Button(action: { 
                tooltipManager.showTooltips.toggle()
                tooltipManager.saveTooltipPreferences()
            }) {
                Image(systemName: tooltipManager.showTooltips ? "info.circle.fill" : "info.circle")
                    .foregroundColor(tooltipManager.showTooltips ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .help("Toggle Tooltips (\(tooltipManager.showTooltips ? "On" : "Off"))")
            
            // Tooltip settings
            Button(action: { showTooltipSettings = true }) {
                Image(systemName: "gear.badge.questionmark")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .help("Tooltip Settings")
            
            Divider().frame(height: 20)
            
            // Analytics
            Button(action: { 
                Task {
                    await TerrainAnalyticsManager.shared.analyzeTerrain(TerrainData())
                    TerrainAnalyticsManager.shared.showAnalyticsPanel = true
                }
            }) {
                Image(systemName: "chart.bar.xaxis")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .help("Terrain Analytics")
            
            // Settings
            Button(action: { showSettings = true }) {
                Image(systemName: "gear")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .help("Terrain Settings")
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Terrain Brush Panel
struct TerrainBrushPanel: View {
    @ObservedObject var manager: TerrainEditorManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Brushes")
                .font(DesignSystem.Typography.bodyBold)
                .padding(DesignSystem.Spacing.sm)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 50))], spacing: 8) {
                    ForEach(TerrainEditorManager.BrushType.allCases, id: \.self) { brush in
                        BrushButton(brush: brush, isSelected: manager.selectedBrush == brush) {
                            manager.selectedBrush = brush
                        }
                    }
                }
                .padding(DesignSystem.Spacing.sm)
                
                EditorDivider()
                
                // Brush settings
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                    Text("Brush Settings")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    FloatPropertyEditor(label: "Size", value: $manager.brushSize, range: 1...100, step: 1)
                    FloatPropertyEditor(label: "Opacity", value: $manager.brushOpacity, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Falloff", value: $manager.brushFalloff, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Hardness", value: $manager.brushHardness, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Jitter", value: $manager.brushJitter, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Rotation", value: $manager.brushRotation, range: 0...360, step: 1)
                    FloatPropertyEditor(label: "Rotation Random", value: $manager.brushRotationRandomization, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Scale Random", value: $manager.brushScaleRandomization, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Spacing", value: $manager.brushSpacing, range: 0.01...1, step: 0.01)
                    
                    // Advanced brush settings
                    Toggle("Pressure Sensitivity", isOn: $manager.pressureSensitivity)
                        .font(DesignSystem.Typography.small)
                    
                    Toggle("3D Cursor", isOn: $manager.brushCursor3D)
                        .font(DesignSystem.Typography.small)
                    
                    Toggle("Brush Preview", isOn: $manager.brushPreviewProjection)
                        .font(DesignSystem.Typography.small)
                    
                    Toggle("Mirror Brush", isOn: $manager.mirrorBrush)
                        .font(DesignSystem.Typography.small)
                    
                    // Brush-specific settings
                    switch manager.selectedBrush {
                    case .flatten:
                        FloatPropertyEditor(label: "Target Height", value: $manager.flattenTargetHeight, range: -100...100, step: 0.5)
                    case .smooth:
                        IntPropertyEditor(label: "Smooth Iterations", value: $manager.smoothIterations, range: 1...10, step: 1)
                    case .sharpen:
                        IntPropertyEditor(label: "Sharpen Iterations", value: $manager.sharpenIterations, range: 1...5, step: 1)
                    case .noise:
                        FloatPropertyEditor(label: "Noise Scale", value: $manager.noiseScale, range: 0.1...10, step: 0.1)
                        FloatPropertyEditor(label: "Noise Strength", value: $manager.noiseStrength, range: 0...1, step: 0.05)
                    case .stamp:
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Stamp Shape")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Picker("Stamp Shape", selection: $manager.stampShape) {
                                ForEach(TerrainEditorManager.StampShape.allCases, id: \.self) { shape in
                                    Label(shape.rawValue.capitalized, systemImage: shape.icon)
                                        .tag(shape)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                    case .pinch, .bulge:
                        FloatPropertyEditor(label: "Deform Strength", value: $manager.pinchBulgeStrength, range: 0...1, step: 0.05)
                    case .terrace:
                        FloatPropertyEditor(label: "Step Height", value: $manager.terraceStepHeight, range: 0.1...10, step: 0.1)
                    case .path:
                        FloatPropertyEditor(label: "Path Width", value: $manager.pathWidth, range: 1...50, step: 1)
                    case .bridge:
                        FloatPropertyEditor(label: "Bridge Strength", value: $manager.bridgeStrength, range: 0...1, step: 0.05)
                    case .clone:
                        HStack {
                            Text("Clone Source:")
                                .font(DesignSystem.Typography.small)
                            Spacer()
                            Button("Set Source") {
                                // Set clone source from current position
                            }
                            .font(DesignSystem.Typography.small)
                            Button("Clear Source") {
                                manager.cloneSource = nil
                            }
                            .font(DesignSystem.Typography.small)
                        }
                    default:
                        EmptyView()
                    }
                    
                    // Falloff curve preview
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Falloff Curve")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        GeometryReader { geo in
                            Path { path in
                                path.move(to: CGPoint(x: 0, y: geo.size.height))
                                
                                let falloffType = manager.brushFalloff
                                let steps = 50
                                for i in 0...steps {
                                    let t = Float(i) / Float(steps)
                                    let x = CGFloat(t) * geo.size.width
                                    let y: CGFloat
                                    
                                    switch falloffType {
                                    case 0.0: // Linear
                                        y = geo.size.height * (1.0 - t)
                                    case 0.5: // Smooth
                                        y = geo.size.height * pow(1.0 - t, 2.0)
                                    case 1.0: // Hard
                                        y = t < 0.5 ? geo.size.height : 0
                                    default: // Custom curve
                                        y = geo.size.height * pow(1.0 - t, 1.0 + falloffType * 2.0)
                                    }
                                    
                                    path.addLine(to: CGPoint(x: x, y: y))
                                }
                            }
                            .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                        }
                        .frame(height: 60)
                        .background(DesignSystem.Colors.backgroundPrimary)
                        .cornerRadius(4)
                    }
                }
                .padding(DesignSystem.Spacing.sm)
                
                EditorDivider()
                
                // Grass Settings (shown when grass brushes are selected)
                if manager.selectedBrush == .grassDensity || manager.selectedBrush == .grassType {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        Text("Grass Settings")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        // Grass type selector
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Grass Type")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Picker("Grass Type", selection: $manager.selectedGrassType) {
                                ForEach(GrassType.allCases, id: \.self) { type in
                                    Label(type.rawValue.capitalized, systemImage: type.icon)
                                        .tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        FloatPropertyEditor(label: "Density", value: $manager.grassDensity, range: 0...1, step: 0.05)
                        FloatPropertyEditor(label: "Color Variation", value: $manager.grassColorVariation, range: 0...1, step: 0.05)
                        FloatPropertyEditor(label: "Wind Response", value: $manager.grassWindResponse, range: 0...1, step: 0.05)
                        FloatPropertyEditor(label: "LOD Distance", value: $manager.grassLODDistance, range: 10...500, step: 10)
                        
                        Toggle("Grass Culling", isOn: $manager.grassCullingEnabled)
                            .font(DesignSystem.Typography.small)
                        
                        // Grass detail meshes
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Detail Meshes")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            ForEach(manager.grassDetailMeshes) { mesh in
                                HStack {
                                    Text(mesh.name)
                                        .font(DesignSystem.Typography.small)
                                    Spacer()
                                    Text("Density: \(mesh.density, specifier: "%.1f")")
                                        .font(DesignSystem.Typography.mono)
                                        .foregroundColor(DesignSystem.Colors.textSecondary)
                                }
                                .padding(.vertical, 2)
                            }
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                
                // Vegetation Settings (shown when vegetation brushes are selected)
                if manager.selectedBrush == .vegetationPaint || manager.selectedBrush == .vegetationRemove {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        Text("Vegetation Settings")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        // Vegetation type selector
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Vegetation Type")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Picker("Vegetation Type", selection: $manager.selectedVegetationType) {
                                ForEach(VegetationType.allCases, id: \.self) { type in
                                    Label(type.rawValue.capitalized, systemImage: type.icon)
                                        .tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        // Placement rules
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Placement Rules")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            HStack {
                                Text("Slope:")
                                    .font(DesignSystem.Typography.small)
                                Text("\(manager.vegetationPlacementRules.slopeRange.lowerBound, specifier: "%.0f")° - \(manager.vegetationPlacementRules.slopeRange.upperBound, specifier: "%.0f")°")
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            
                            HStack {
                                Text("Height:")
                                    .font(DesignSystem.Typography.small)
                                Text("\(manager.vegetationPlacementRules.heightRange.lowerBound, specifier: "%.0f") - \(manager.vegetationPlacementRules.heightRange.upperBound, specifier: "%.0f")")
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                        }
                        
                        Toggle("Show Density", isOn: $manager.showVegetationDensity)
                            .font(DesignSystem.Typography.small)
                        
                        Button("Generate Procedural") {
                            manager.generateProceduralVegetation()
                        }
                        .font(DesignSystem.Typography.small)
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                
                // Erosion Settings (shown when erosion brushes are selected)
                if manager.selectedBrush == .erosion || manager.selectedBrush == .erosionMask || manager.selectedBrush == .erosionPreview {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        Text("Erosion Settings")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        // Erosion presets
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Erosion Preset")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Picker("Erosion Preset", selection: $manager.selectedErosionPreset) {
                                ForEach(manager.erosionPresets.indices, id: \.self) { index in
                                    Text(manager.erosionPresets[index].name)
                                        .tag(index)
                                }
                            }
                            .pickerStyle(.menu)
                            .onChange(of: manager.selectedErosionPreset) { newIndex in
                                manager.applyErosionPreset(manager.erosionPresets[newIndex])
                            }
                            
                            Text(manager.erosionPresets[safe: manager.selectedErosionPreset]?.description ?? "")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        
                        FloatPropertyEditor(label: "Iteration Count", value: Binding(
                            get: { Float(manager.erosionIterationCount) },
                            set: { manager.erosionIterationCount = Int($0) }
                        ), range: 1...200, step: 1)
                        
                        FloatPropertyEditor(label: "Erosion Strength", value: $manager.erosionStrength, range: 0...1, step: 0.05)
                        FloatPropertyEditor(label: "Sediment Deposition", value: $manager.sedimentDeposition, range: 0...1, step: 0.05)
                        
                        Toggle("Real-time Preview", isOn: $manager.realTimePreview)
                            .font(DesignSystem.Typography.small)
                        
                        Toggle("Erosion Mask", isOn: $manager.erosionMaskEnabled)
                            .font(DesignSystem.Typography.small)
                        
                        HStack {
                            Button("Run Simulation") {
                                manager.runFullErosionSimulation()
                            }
                            .font(DesignSystem.Typography.small)
                            
                            Button("Undo") {
                                manager.undo()
                            }
                            .font(DesignSystem.Typography.small)
                            
                            Button("Redo") {
                                manager.redo()
                            }
                            .font(DesignSystem.Typography.small)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                
                // Water Settings (shown when water brushes are selected)
                if manager.selectedBrush == .riverCarve || manager.selectedBrush == .lakeFill {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        Text("Water Settings")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        if manager.selectedBrush == .riverCarve {
                            FloatPropertyEditor(label: "River Width", value: $manager.riverWidth, range: 1...50, step: 1)
                            FloatPropertyEditor(label: "River Depth", value: $manager.riverDepth, range: 0.5...10, step: 0.5)
                            
                            Button("Generate River Path") {
                                // Generate river from start to end points
                            }
                            .font(DesignSystem.Typography.small)
                        }
                        
                        if manager.selectedBrush == .lakeFill {
                            FloatPropertyEditor(label: "Water Level", value: $manager.lakeWaterLevel, range: -10...100, step: 1)
                            
                            Button("Create Lake") {
                                // Create lake at current position
                            }
                            .font(DesignSystem.Typography.small)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                
                // Heightmap Settings (shown when heightmap brushes are selected)
                if manager.selectedBrush == .heightmapFill || manager.selectedBrush == .heightmapInvert || manager.selectedBrush == .heightmapNormalize {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        Text("Heightmap Operations")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        if manager.selectedBrush == .heightmapFill {
                            VStack(alignment: .leading, spacing: 4) {
                                Text("Fill Value")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                HStack {
                                    Slider(value: Binding(
                                        get: { manager.heightmapWorldHeightRange.lowerBound },
                                        set: { manager.heightmapWorldHeightRange = $0...manager.heightmapWorldHeightRange.upperBound }
                                    ), in: manager.heightmapWorldHeightRange)
                                    Text("\(manager.heightmapWorldHeightRange.lowerBound, specifier: "%.1f")")
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 50)
                                }
                            }
                        }
                        
                        HStack {
                            Button("Normalize") {
                                manager.normalizeHeightmap()
                            }
                            .font(DesignSystem.Typography.small)
                            
                            Button("Invert") {
                                manager.invertHeightmap()
                            }
                            .font(DesignSystem.Typography.small)
                            
                            Button("Copy Region") {
                                // Copy region implementation
                            }
                            .font(DesignSystem.Typography.small)
                            
                            Button("Paste Region") {
                                // Paste region implementation
                            }
                            .font(DesignSystem.Typography.small)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct BrushButton: View {
    let brush: TerrainEditorManager.BrushType
    let isSelected: Bool
    let action: () -> Void
    @StateObject private var tooltipManager = TerrainTooltipManager.shared
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Image(systemName: brush.icon)
                    .font(.system(size: 20))
                Text(brush.rawValue.capitalized)
                    .font(.system(size: 9))
            }
            .frame(width: 50, height: 50)
            .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
            .background(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
        }
        .buttonStyle(.plain)
        .onHover { isHovering in
            if isHovering {
                let tooltipId = "brush.\(brush.rawValue)"
                tooltipManager.showTooltip(for: tooltipId, at: CGPoint(x: 25, y: 60))
            } else {
                tooltipManager.hideTooltip()
            }
        }
    }
}

// MARK: - Terrain Layers Panel
struct TerrainLayersPanel: View {
    @ObservedObject var manager: TerrainEditorManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack {
                Text("Layers")
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
            
            ScrollView {
                VStack(spacing: 4) {
                    ForEach(Array(manager.layers.enumerated()), id: \.element.id) { index, layer in
                        LayerRow(layer: layer, isSelected: manager.selectedLayer == index) {
                            manager.selectedLayer = index
                        }
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct LayerRow: View {
    let layer: TerrainLayer
    let isSelected: Bool
    let onSelect: () -> Void
    @State private var isHovering = false
    @ObservedObject var manager = TerrainEditorManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            HStack(spacing: 8) {
                // Visibility toggle
                Button(action: {
                    if let index = manager.layers.firstIndex(where: { $0.id == layer.id }) {
                        manager.layers[index].isVisible.toggle()
                    }
                }) {
                    Image(systemName: layer.isVisible ? "eye" : "eye.slash")
                        .foregroundColor(layer.isVisible ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .frame(width: 20)
                
                // Lock toggle
                Button(action: {
                    if let index = manager.layers.firstIndex(where: { $0.id == layer.id }) {
                        manager.layers[index].isLocked.toggle()
                    }
                }) {
                    Image(systemName: layer.isLocked ? "lock" : "lock.open")
                        .foregroundColor(layer.isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .frame(width: 20)
                
                // Texture preview
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.brown.opacity(0.5))
                    .frame(width: 40, height: 40)
                    .overlay(
                        Image(systemName: "photo")
                            .foregroundColor(.white.opacity(0.5))
                    )
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(layer.name)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    Text(layer.texture)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    HStack {
                        Text("T: \(Int(layer.tiling))")
                            .font(DesignSystem.Typography.small)
                        if !layer.normalMap.isEmpty {
                            Text("N")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                        }
                        if !layer.heightMap.isEmpty {
                            Text("H")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                        }
                        if layer.metallic > 0 {
                            Text("M")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                        }
                    }
                }
                
                Spacer()
                
                // Layer settings button
                Button(action: {}) {
                    Image(systemName: "ellipsis")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .padding(8)
            .background(isSelected ? DesignSystem.Colors.selection : (isHovering ? DesignSystem.Colors.hover : Color.clear))
            .cornerRadius(4)
            .onTapGesture(perform: onSelect)
            .onHover { isHovering = $0 }
            
            // Expanded layer details
            if isSelected {
                VStack(alignment: .leading, spacing: 4) {
                    Divider()
                    
                    // Texture properties
                    HStack {
                        Text("Tiling:")
                            .font(DesignSystem.Typography.small)
                        Slider(value: binding(for: layer.id, path: \tiling), in: 0.1...100, step: 0.1)
                            .frame(width: 80)
                        Text("\(layer.tiling, specifier: "%.1f")")
                            .font(DesignSystem.Typography.small)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Metallic:")
                            .font(DesignSystem.Typography.small)
                        Slider(value: binding(for: layer.id, path: \metallic), in: 0...1, step: 0.05)
                            .frame(width: 80)
                        Text("\(layer.metallic, specifier: "%.2f")")
                            .font(DesignSystem.Typography.small)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Roughness:")
                            .font(DesignSystem.Typography.small)
                        Slider(value: binding(for: layer.id, path: \roughness), in: 0...1, step: 0.05)
                            .frame(width: 80)
                        Text("\(layer.roughness, specifier: "%.2f")")
                            .font(DesignSystem.Typography.small)
                            .frame(width: 40)
                    }
                    
                    // Texture rules indicators
                    if layer.slopeRule != nil || layer.heightRule != nil || layer.curvatureRule != nil {
                        HStack {
                            Text("Rules:")
                                .font(DesignSystem.Typography.small)
                            if layer.slopeRule != nil {
                                Text("Slope")
                                    .font(DesignSystem.Typography.small)
                                    .padding(2)
                                    .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                                    .cornerRadius(2)
                            }
                            if layer.heightRule != nil {
                                Text("Height")
                                    .font(DesignSystem.Typography.small)
                                    .padding(2)
                                    .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                                    .cornerRadius(2)
                            }
                            if layer.curvatureRule != nil {
                                Text("Curvature")
                                    .font(DesignSystem.Typography.small)
                                    .padding(2)
                                    .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                                    .cornerRadius(2)
                            }
                        }
                    }
                }
                .padding(.horizontal, 8)
                .padding(.bottom, 8)
            }
        }
    }
    
    private func binding<T>(for id: UUID, path: WritableKeyPath<TerrainLayer, T>) -> Binding<T> {
        return Binding<T>(
            get: {
                guard let index = manager.layers.firstIndex(where: { $0.id == id }) else {
                    fatalError("Layer not found")
                }
                return manager.layers[index][keyPath: path]
            },
            set: { newValue in
                guard let index = manager.layers.firstIndex(where: { $0.id == id }) else { return }
                manager.layers[index][keyPath: path] = newValue
            }
        )
    }
}

// MARK: - Tooltip Overlay

struct TerrainTooltipOverlay: View {
    @StateObject private var tooltipManager = TerrainTooltipManager.shared
    
    var body: some View {
        ZStack {
            if let tooltip = tooltipManager.currentTooltip {
                TerrainTooltipView(
                    tooltip: tooltip,
                    position: tooltipManager.tooltipPosition
                )
                .transition(.opacity.combined(with: .scale(scale: 0.9)))
                .animation(.easeInOut(duration: 0.2), value: tooltip.id)
            }
        }
        .allowsHitTesting(false)
    }
}

// MARK: - Tooltip Settings

struct TerrainTooltipSettings: View {
    @ObservedObject var tooltipManager = TerrainTooltipManager.shared
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Tooltip Settings")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button("Done") {
                    tooltipManager.saveTooltipPreferences()
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
            .padding()
            
            Divider()
            
            // Settings
            Form {
                Section("Display") {
                    Toggle("Show Tooltips", isOn: $tooltipManager.showTooltips)
                        .help("Enable or disable terrain tooltips")
                    
                    if tooltipManager.showTooltips {
                        HStack {
                            Text("Delay:")
                            Slider(value: $tooltipManager.tooltipDelay, in: 0.1...2.0, step: 0.1)
                            Text("\(tooltipManager.tooltipDelay, specifier: "%.1f")s")
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        .help("Time delay before showing tooltips")
                    }
                }
                
                Section("Categories") {
                    ForEach(TerrainTooltipManager.TerrainTooltip.TooltipCategory.allCases, id: \.self) { category in
                        HStack {
                            Image(systemName: category.icon)
                                .foregroundColor(category.color)
                                .frame(width: 20)
                            
                            Text(category.rawValue.capitalized)
                            
                            Spacer()
                            
                            Toggle("", isOn: .constant(true))
                                .labelsHidden()
                        }
                    }
                }
                
                Section("Difficulty Filter") {
                    ForEach(TerrainTooltipManager.TerrainTooltip.DifficultyLevel.allCases, id: \.self) { difficulty in
                        HStack {
                            Circle()
                                .fill(difficulty.color)
                                .frame(width: 8, height: 8)
                            
                            Text(difficulty.rawValue)
                            
                            Spacer()
                            
                            Toggle("", isOn: .constant(true))
                                .labelsHidden()
                        }
                    }
                }
            }
            .padding()
        }
        .frame(width: 400, height: 500)
    }
}

// MARK: - Terrain Settings Sheet
struct TerrainSettingsSheet: View {
    @ObservedObject var manager = TerrainEditorManager.shared
    @Environment(\.dismiss) var dismiss
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Terrain Settings")
                    .font(DesignSystem.Typography.h2)
                Spacer()
                Button("Done") { dismiss() }
            }
            .padding()
            
            Divider()
            
            // Tab selection
            Picker("Settings Tab", selection: $selectedTab) {
                Text("General").tag(0)
                Text("Textures").tag(1)
                Text("Erosion").tag(2)
                Text("Advanced").tag(3)
            }
            .pickerStyle(.segmented)
            .padding()
            
            Divider()
            
            // Tab content
            ScrollView {
                switch selectedTab {
                case 0:
                    generalSettings
                case 1:
                    textureSettings
                case 2:
                    erosionSettings
                case 3:
                    advancedSettings
                default:
                    EmptyView()
                }
            }
            .padding()
        }
        .frame(width: 500, height: 600)
    }
    
    @ViewBuilder
    private var generalSettings: some View {
        Form {
            Section("Resolution") {
                Picker("Heightmap Resolution", selection: $manager.terrainResolution) {
                    Text("256x256").tag(256)
                    Text("512x512").tag(512)
                    Text("1024x1024").tag(1024)
                    Text("2048x2048").tag(2048)
                }
                
                Picker("Splat Map Resolution", selection: $manager.splatMapResolution) {
                    Text("256x256").tag(256)
                    Text("512x512").tag(512)
                    Text("1024x1024").tag(1024)
                    Text("2048x2048").tag(2048)
                }
            }
            
            Section("Size") {
                FloatPropertyEditor(label: "Terrain Size", value: $manager.terrainSize, range: 100...10000, step: 100)
            }
        }
    }
    
    @ViewBuilder
    private var textureSettings: some View {
        Form {
            Section("Texture Projection") {
                Toggle("Triplanar Projection", isOn: $manager.triplanarProjection)
            }
            
            Section("Auto-Painting") {
                Toggle("Enable Auto-Paint", isOn: $manager.autoPaintEnabled)
                Text("Automatically paints textures based on slope, height, and curvature rules")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Section("Texture Masking") {
                HStack {
                    Button("Import Mask") {
                        // TODO: Implement texture mask import
                    }
                    Spacer()
                    Button("Export Mask") {
                        // TODO: Implement texture mask export
                    }
                }
            }
        }
    }
    
    @ViewBuilder
    private var erosionSettings: some View {
        Form {
            Section("Erosion Simulation") {
                Toggle("Hydraulic Erosion", isOn: $manager.hydraulicErosionEnabled)
                Toggle("Thermal Erosion", isOn: $manager.thermalErosionEnabled)
            }
            
            Section("Erosion Parameters") {
                FloatPropertyEditor(label: "Iterations", value: Binding(
                    get: { Float(manager.erosionIterations) },
                    set: { manager.erosionIterations = Int($0) }
                ), range: 1...1000, step: 10)
                
                FloatPropertyEditor(label: "Strength", value: $manager.erosionStrength, range: 0.01...2.0, step: 0.01)
            }
            
            Section("Erosion Presets") {
                VStack(alignment: .leading, spacing: 4) {
                    Button("Desert Erosion") {
                        applyErosionPreset(.desert)
                    }
                    Button("Alpine Erosion") {
                        applyErosionPreset(.alpine)
                    }
                    Button("Coastal Erosion") {
                        applyErosionPreset(.coastal)
                    }
                    Button("River Erosion") {
                        applyErosionPreset(.river)
                    }
                }
            }
        }
    }
    
    @ViewBuilder
    private var advancedSettings: some View {
        Form {
            Section("Heightmap Resolution & Precision") {
                Picker("Heightmap Resolution", selection: $manager.terrainResolution) {
                    Text("129x129 (2^7+1)").tag(129)
                    Text("257x257 (2^8+1)").tag(257)
                    Text("513x513 (2^9+1)").tag(513)
                    Text("1025x1025 (2^10+1)").tag(1025)
                    Text("2049x2049 (2^11+1)").tag(2049)
                }
                
                Picker("Heightmap Precision", selection: $manager.heightmapPrecision) {
                    Text("8-bit").tag(HeightmapPrecision.bit8)
                    Text("16-bit").tag(HeightmapPrecision.bit16)
                    Text("32-bit").tag(HeightmapPrecision.bit32)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("World Height Range")
                        .font(DesignSystem.Typography.small)
                    
                    HStack {
                        Text("Min:")
                            .font(DesignSystem.Typography.small)
                        Slider(value: Binding(
                            get: { manager.heightmapWorldHeightRange.lowerBound },
                            set: { manager.heightmapWorldHeightRange = $0...manager.heightmapWorldHeightRange.upperBound }
                        ), in: -1000...1000)
                        Text("\(manager.heightmapWorldHeightRange.lowerBound, specifier: "%.0f")")
                            .font(DesignSystem.Typography.mono)
                            .frame(width: 50)
                    }
                    
                    HStack {
                        Text("Max:")
                            .font(DesignSystem.Typography.small)
                        Slider(value: Binding(
                            get: { manager.heightmapWorldHeightRange.upperBound },
                            set: { manager.heightmapWorldHeightRange = manager.heightmapWorldHeightRange.lowerBound...$0 }
                        ), in: -1000...1000)
                        Text("\(manager.heightmapWorldHeightRange.upperBound, specifier: "%.0f")")
                            .font(DesignSystem.Typography.mono)
                            .frame(width: 50)
                    }
                }
            }
            
            Section("Heightmap Controls") {
                FloatPropertyEditor(label: "Scale", value: $manager.heightmapScale, range: 0.1...10.0, step: 0.1)
                FloatPropertyEditor(label: "Offset", value: $manager.heightmapOffset, range: -1000...1000, step: 1.0)
                FloatPropertyEditor(label: "Min Clamp", value: $manager.heightmapMinClamp, range: -1000...1000, step: 1.0)
                FloatPropertyEditor(label: "Max Clamp", value: $manager.heightmapMaxClamp, range: -1000...1000, step: 1.0)
                
                HStack {
                    Button("Normalize") {
                        manager.normalizeHeightmap()
                    }
                    Button("Invert") {
                        manager.invertHeightmap()
                    }
                    Button("Fill") {
                        manager.fillHeightmap(value: manager.heightmapWorldHeightRange.lowerBound)
                    }
                    Button("Save Snapshot") {
                        manager.saveToHistory()
                    }
                }
            }
            
            Section("Tiling System") {
                Picker("Tile Size", selection: $manager.heightmapTileSize) {
                    Text("256x256").tag(256)
                    Text("512x512").tag(512)
                    Text("1024x1024").tag(1024)
                    Text("2048x2048").tag(2048)
                }
                
                Toggle("Seamless Tiling", isOn: $manager.seamlessTiling)
                
                HStack {
                    Button("Copy Region") {
                        // Copy region implementation
                    }
                    Button("Paste Region") {
                        // Paste region implementation
                    }
                }
            }
            
            Section("Import/Export") {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text("Import Format:")
                            .font(DesignSystem.Typography.small)
                        Picker("Import Format", selection: $manager.importFormat) {
                            ForEach(HeightmapFormat.allCases, id: \.self) { format in
                                Text(format.rawValue.uppercased()).tag(format)
                            }
                        }
                        .pickerStyle(.menu)
                        .frame(width: 100)
                        
                        Button("Import") {
                            // Import implementation
                        }
                        .font(DesignSystem.Typography.small)
                    }
                    
                    HStack {
                        Text("Export Format:")
                            .font(DesignSystem.Typography.small)
                        Picker("Export Format", selection: $manager.exportFormat) {
                            ForEach(HeightmapFormat.allCases, id: \.self) { format in
                                Text(format.rawValue.uppercased()).tag(format)
                            }
                        }
                        .pickerStyle(.menu)
                        .frame(width: 100)
                        
                        Button("Export") {
                            // Export implementation
                        }
                        .font(DesignSystem.Typography.small)
                    }
                }
            }
            
            Section("Grass Settings") {
                FloatPropertyEditor(label: "Grass LOD Distance", value: $manager.grassLODDistance, range: 10...500, step: 10)
                FloatPropertyEditor(label: "Grass Wind Response", value: $manager.grassWindResponse, range: 0...1, step: 0.05)
                FloatPropertyEditor(label: "Grass Color Variation", value: $manager.grassColorVariation, range: 0...1, step: 0.05)
                Toggle("Grass Culling", isOn: $manager.grassCullingEnabled)
            }
            
            Section("Vegetation Settings") {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Placement Rules")
                        .font(DesignSystem.Typography.small)
                    
                    HStack {
                        Text("Slope Range:")
                            .font(DesignSystem.Typography.small)
                        Text("\(manager.vegetationPlacementRules.slopeRange.lowerBound, specifier: "%.0f")° - \(manager.vegetationPlacementRules.slopeRange.upperBound, specifier: "%.0f")°")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    HStack {
                        Text("Height Range:")
                            .font(DesignSystem.Typography.small)
                        Text("\(manager.vegetationPlacementRules.heightRange.lowerBound, specifier: "%.0f") - \(manager.vegetationPlacementRules.heightRange.upperBound, specifier: "%.0f")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    FloatPropertyEditor(label: "Min Distance", value: Binding(
                        get: { manager.vegetationPlacementRules.minDistance },
                        set: { manager.vegetationPlacementRules.minDistance = $0 }
                    ), range: 0.5...10, step: 0.5)
                    
                    FloatPropertyEditor(label: "Max Instances", value: Binding(
                        get: { Float(manager.vegetationPlacementRules.maxInstances) },
                        set: { manager.vegetationPlacementRules.maxInstances = Int($0) }
                    ), range: 100...10000, step: 100)
                }
            }
            
            Section("LOD Configuration") {
                Picker("Quality Preset", selection: $manager.lodQualityPreset) {
                    ForEach(TerrainEditorManager.LODQualityPreset.allCases, id: \.self) { preset in
                        Text(preset.rawValue).tag(preset)
                    }
                }
                .onChange(of: manager.lodQualityPreset) { preset in
                    manager.applyLODPreset(preset)
                }
                
                IntPropertyEditor(label: "LOD Levels", value: $manager.lodLevels, range: 1...8, step: 1)
                FloatPropertyEditor(label: "Chunk Size", value: $manager.lodChunkSize, range: 16...256, step: 16)
                FloatPropertyEditor(label: "Screen Space Error", value: $manager.lodScreenSpaceError, range: 0.1...10.0, step: 0.1)
                
                Toggle("Seamless Stitching", isOn: $manager.enableSeamlessStitching)
                Toggle("Crack Fixing", isOn: $manager.enableCrackFixing)
                Toggle("Geomorphing", isOn: $manager.enableGeomorphing)
                Toggle("Frustum Culling", isOn: $manager.enableFrustumCulling)
                Toggle("Occlusion Culling", isOn: $manager.enableOcclusionCulling)
                Toggle("GPU Tessellation", isOn: $manager.enableGPUTessellation)
            }
            
            Section("Distance Thresholds") {
                ForEach(0..<manager.lodLevels, id: \.self) { index in
                    if index < manager.lodDistances.count {
                        FloatPropertyEditor(
                            label: "LOD \(index) Distance",
                            value: Binding(
                                get: { manager.lodDistances[index] },
                                set: { manager.lodDistances[index] = $0 }
                            ),
                            range: 10...2000,
                            step: 10
                        )
                    }
                }
            }
            
            Section("Performance Metrics") {
                HStack {
                    Text("Triangle Count:")
                    Spacer()
                    Text("\(manager.lodTriangleCount)")
                        .font(DesignSystem.Typography.mono)
                }
                
                HStack {
                    Text("Frame Time:")
                    Spacer()
                    Text("\(manager.lodFrameTime, specifier: "%.2f") ms")
                        .font(DesignSystem.Typography.mono)
                }
                
                HStack {
                    Text("Memory Usage:")
                    Spacer()
                    Text("\(manager.lodMemoryUsage, specifier: "%.1f") MB")
                        .font(DesignSystem.Typography.mono)
                }
                
                Button("Update Metrics") {
                    manager.updateLODMetrics()
                }
            }
            
            Section("History Management") {
                HStack {
                    Button("Undo") {
                        manager.undo()
                    }
                    .disabled(manager.undoHistory.isEmpty)
                    
                    Button("Redo") {
                        manager.redo()
                    }
                    .disabled(manager.redoHistory.isEmpty)
                }
                
                Text("Undo History: \(manager.undoHistory.count) items")
                Text("Redo History: \(manager.redoHistory.count) items")
            }
        }
    }
    
    private enum ErosionPreset {
        case desert, alpine, coastal, river
    }
    
    private func applyErosionPreset(_ preset: ErosionPreset) {
        switch preset {
        case .desert:
            manager.thermalErosionEnabled = true
            manager.hydraulicErosionEnabled = false
            manager.erosionStrength = 0.8
            manager.erosionIterations = 500
        case .alpine:
            manager.thermalErosionEnabled = true
            manager.hydraulicErosionEnabled = true
            manager.erosionStrength = 0.6
            manager.erosionIterations = 300
        case .coastal:
            manager.hydraulicErosionEnabled = true
            manager.thermalErosionEnabled = false
            manager.erosionStrength = 0.4
            manager.erosionIterations = 200
        case .river:
            manager.hydraulicErosionEnabled = true
            manager.thermalErosionEnabled = false
            manager.erosionStrength = 0.7
            manager.erosionIterations = 400
        }
    }
}

// MARK: - Terrain Analytics View

struct TerrainAnalyticsView: View {
    @StateObject private var analyticsManager = TerrainAnalyticsManager.shared
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Terrain Analytics")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button("Export") {
                    // Export functionality
                }
                .buttonStyle(.bordered)
                
                Button("Done") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
            .padding()
            
            Divider()
            
            if analyticsManager.isAnalyzing {
                VStack(spacing: 20) {
                    ProgressView(value: analyticsManager.analysisProgress)
                        .progressViewStyle(LinearProgressViewStyle())
                    
                    Text("Analyzing terrain...")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("\(Int(analyticsManager.analysisProgress * 100))%")
                        .font(DesignSystem.Typography.mono)
                }
                .padding()
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else if let analytics = analyticsManager.analytics {
                // Analytics content
                HSplitView {
                    // Metrics list
                    VStack(alignment: .leading, spacing: 0) {
                        Text("Metrics")
                            .font(DesignSystem.Typography.bodyBold)
                            .padding()
                        
                        List(TerrainAnalyticsManager.AnalyticsMetric.allCases, id: \.self, selection: $analyticsManager.selectedMetric) { metric in
                            HStack {
                                Image(systemName: metric.icon)
                                    .foregroundColor(metric.color)
                                    .frame(width: 20)
                                
                                Text(metric.rawValue)
                                    .font(DesignSystem.Typography.body)
                                
                                Spacer()
                            }
                            .padding(.vertical, 4)
                        }
                        .listStyle(.plain)
                    }
                    .frame(minWidth: 200, maxWidth: 250)
                    
                    // Metric details
                    MetricDetailView(metric: analyticsManager.selectedMetric, analytics: analytics)
                        .frame(minWidth: 400)
                }
            } else {
                VStack(spacing: 20) {
                    Image(systemName: "chart.bar.xaxis")
                        .font(.system(size: 60))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("No Analytics Data")
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Run terrain analysis to view detailed statistics")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Button("Start Analysis") {
                        Task {
                            await analyticsManager.analyzeTerrain(TerrainData())
                        }
                    }
                    .buttonStyle(.borderedProminent)
                }
                .padding()
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .frame(width: 800, height: 600)
    }
}

// MARK: - Metric Detail View

struct MetricDetailView: View {
    let metric: TerrainAnalyticsManager.AnalyticsMetric
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                switch metric {
                case .heightDistribution:
                    HeightDistributionView(analytics: analytics)
                case .slopeAnalysis:
                    SlopeAnalysisView(analytics: analytics)
                case .textureCoverage:
                    TextureCoverageView(analytics: analytics)
                case .performanceMetrics:
                    PerformanceMetricsView(analytics: analytics)
                case .erosionAnalysis:
                    ErosionAnalysisView(analytics: analytics)
                case .vegetationAnalysis:
                    VegetationAnalysisView(analytics: analytics)
                case .memoryUsage:
                    MemoryUsageView(analytics: analytics)
                case .renderingStats:
                    RenderingStatsView(analytics: analytics)
                }
            }
            .padding()
        }
    }
}

// MARK: - Individual Metric Views

struct HeightDistributionView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Height Distribution")
                .font(DesignSystem.Typography.h3)
            
            HStack(spacing: 20) {
                VStack(alignment: .leading) {
                    Text("Min Height")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("\(String(format: "%.1f", analytics.heightRange.lowerBound)) m")
                        .font(DesignSystem.Typography.bodyBold)
                }
                
                VStack(alignment: .leading) {
                    Text("Max Height")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("\(String(format: "%.1f", analytics.heightRange.upperBound)) m")
                        .font(DesignSystem.Typography.bodyBold)
                }
                
                VStack(alignment: .leading) {
                    Text("Range")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("\(String(format: "%.1f", analytics.heightRange.upperBound - analytics.heightRange.lowerBound)) m")
                        .font(DesignSystem.Typography.bodyBold)
                }
            }
            
            // Placeholder for height distribution histogram
            RoundedRectangle(cornerRadius: 8)
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 200)
                .overlay(
                    Text("Height Distribution Histogram")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                )
        }
    }
}

struct TextureCoverageView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Texture Coverage")
                .font(DesignSystem.Typography.h3)
            
            ForEach(Array(analytics.textureCoverage.keys.sorted()), id: \.self) { texture in
                HStack {
                    Text(texture)
                        .frame(width: 100, alignment: .leading)
                    
                    ProgressView(value: analytics.textureCoverage[texture] ?? 0, total: 100)
                        .progressViewStyle(LinearProgressViewStyle(tint: .blue))
                    
                    Text("\(String(format: "%.1f", analytics.textureCoverage[texture] ?? 0))%")
                        .font(DesignSystem.Typography.smallMono)
                        .frame(width: 50)
                }
            }
        }
    }
}

struct PerformanceMetricsView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Performance Metrics")
                .font(DesignSystem.Typography.h3)
            
            Grid(alignment: .leading, horizontalSpacing: 20, verticalSpacing: 12) {
                GridRow {
                    Text("LOD Levels:")
                    Text("\(analytics.performanceMetrics.lodLevels)")
                    Text("Triangles/Frame:")
                    Text(analytics.performanceMetrics.averageTrianglesPerFrame.formatted())
                }
                
                GridRow {
                    Text("Rendering Time:")
                    Text("\(String(format: "%.1f", analytics.performanceMetrics.renderingTime)) ms")
                    Text("Memory Usage:")
                    Text(ByteCountFormatter.string(fromByteCount: analytics.performanceMetrics.memoryUsage.total, countStyle: .memory))
                }
            }
            .font(DesignSystem.Typography.body)
        }
    }
}

// Placeholder views for other metrics
struct SlopeAnalysisView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Slope Analysis")
                .font(DesignSystem.Typography.h3)
            
            Text("Average Slope: \(String(format: "%.1f", analytics.averageSlope))°")
                .font(DesignSystem.Typography.bodyBold)
        }
    }
}

struct ErosionAnalysisView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Erosion Analysis")
                .font(DesignSystem.Typography.h3)
            
            Text("Total Erosion: \(String(format: "%.1f", analytics.erosionMetrics.totalErosion)) m³")
                .font(DesignSystem.Typography.bodyBold)
        }
    }
}

struct VegetationAnalysisView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Vegetation Analysis")
                .font(DesignSystem.Typography.h3)
            
            Text("Total Instances: \(analytics.vegetationMetrics.totalInstances.formatted())")
                .font(DesignSystem.Typography.bodyBold)
        }
    }
}

struct MemoryUsageView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Memory Usage")
                .font(DesignSystem.Typography.h3)
            
            Text("Total: \(ByteCountFormatter.string(fromByteCount: analytics.performanceMetrics.memoryUsage.total, countStyle: .memory))")
                .font(DesignSystem.Typography.bodyBold)
        }
    }
}

struct RenderingStatsView: View {
    let analytics: TerrainAnalyticsManager.TerrainAnalytics
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Rendering Statistics")
                .font(DesignSystem.Typography.h3)
            
            Text("Rendering Time: \(String(format: "%.1f", analytics.performanceMetrics.renderingTime)) ms")
                .font(DesignSystem.Typography.bodyBold)
        }
    }
}

// MARK: - Array Extensions
extension Array {
    subscript(safe index: Index) -> Element? {
        return indices.contains(index) ? self[index] : nil
    }
}
