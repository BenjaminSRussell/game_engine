import SwiftUI
import simd

// MARK: - Selection Visualization Manager
// Provides advanced visual feedback for selected objects

class SelectionVisualizationManager: ObservableObject {
    @Published var highlightMode: HighlightMode = .outline
    @Published var outlineColor: Color = .blue
    @Published var outlineThickness: Float = 2.0
    @Published var pulseEnabled = true
    @Published var pulseSpeed: Float = 1.0
    @Published var glowIntensity: Float = 0.5
    
    // Selection history
    @Published var selectionHistory: [SelectionSnapshot] = []
    @Published var historyIndex: Int = -1
    
    // Selection breadcrumbs
    @Published var selectionBreadcrumbs: [SelectionBreadcrumb] = []
    
    // Multi-selection colors
    private let selectionColors: [Color] = [
        .blue, .red, .green, .orange, .purple, 
        .cyan, .magenta, .yellow, .pink, .mint
    ]
    
    // MARK: - Highlight Modes
    enum HighlightMode: String, CaseIterable, Identifiable {
        // TODO: Implement outline highlight (customizable color/thickness)
        case outline = "Outline"
        // TODO: Implement silhouette highlighting
        case silhouette = "Silhouette"
        // TODO: Implement bounding box highlight
        case boundingBox = "Bounding Box"
        // TODO: Implement wireframe overlay on selection
        case wireframeOverlay = "Wireframe Overlay"
        // TODO: Implement glow effect on selected objects
        case glow = "Glow Effect"
        // TODO: Implement pulsing selection indicator
        case pulsing = "Pulsing"
        // TODO: Implement multi-selection color coding
        case colorCoded = "Color Coded"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .outline: return "square.dashed"
            case .silhouette: return "person.fill"
            case .boundingBox: return "cube"
            case .wireframeOverlay: return "square.grid.3x3"
            case .glow: return "light.max"
            case .pulsing: return "waveform.path.ecg"
            case .colorCoded: return "paintpalette"
            }
        }
    }
    
    // MARK: - Selection Breadcrumb
    struct SelectionBreadcrumb: Identifiable {
        let id = UUID()
        let entityID: UUID
        let entityName: String
        let timestamp: Date
        let selectionType: SelectionType
        
        enum SelectionType {
            case direct
            case indirect(parent: UUID)
            case similar(criteria: String)
            case filter(filter: String)
        }
    }
    
    // MARK: - Selection Snapshot
    struct SelectionSnapshot {
        let timestamp: Date
        let entityIDs: Set<UUID>
        let description: String
    }
    
    // MARK: - Selection Set
    struct SelectionSet: Identifiable {
        let id = UUID()
        var name: String
        var entityIDs: Set<UUID>
        var color: Color
        
        // Save selection set to file
        func saveToFile(url: URL) throws {
            let data = try JSONEncoder().encode(self)
            try data.write(to: url)
        }
        
        // Load selection set from file
        static func loadFromFile(url: URL) throws -> SelectionSet {
            let data = try Data(contentsOf: url)
            return try JSONDecoder().decode(SelectionSet.self, from: data)
        }
    }
    
    // MARK: - Selection Filters
    enum SelectionFilter {
        case byType(String)
        case byTag(String)
        case byLayer(Int)
        case byMaterial(UUID)
        case byMesh(UUID)
        case byComponent(ComponentType)
        
        // Implement filter matching logic
        func matches(entity: EntityNode) -> Bool {
            switch self {
            case .byType(let type):
                return entity.typeName == type
            case .byTag(let tag):
                return entity.tags.contains(tag)
            case .byLayer(let layer):
                return entity.layer == layer
            case .byMaterial(let materialID):
                return entity.materialID == materialID
            case .byMesh(let meshID):
                return entity.meshID == meshID
            case .byComponent(let componentType):
                return entity.components.contains { $0.type == componentType }
            }
        }
    }
    
    // Implement selection breadcrumbs trail
    func addToSelectionBreadcrumbs(entityID: UUID, entityName: String, selectionType: SelectionBreadcrumb.SelectionType) {
        let breadcrumb = SelectionBreadcrumb(
            entityID: entityID,
            entityName: entityName,
            timestamp: Date(),
            selectionType: selectionType
        )
        
        // Remove any existing breadcrumb for this entity
        selectionBreadcrumbs.removeAll { $0.entityID == entityID }
        
        // Add new breadcrumb
        selectionBreadcrumbs.append(breadcrumb)
        
        // Keep only the last 20 breadcrumbs
        if selectionBreadcrumbs.count > 20 {
            selectionBreadcrumbs.removeFirst()
        }
        
        print("[Selection] Added breadcrumb: \(entityName) (\(selectionType))")
    }
    
    func clearSelectionBreadcrumbs() {
        selectionBreadcrumbs.removeAll()
        print("[Selection] Cleared selection breadcrumbs")
    }
    
    func navigateToBreadcrumb(_ breadcrumbID: UUID) -> UUID? {
        guard let breadcrumb = selectionBreadcrumbs.first(where: { $0.id == breadcrumbID }) else {
            return nil
        }
        
        print("[Selection] Navigating to breadcrumb: \(breadcrumb.entityName)")
        return breadcrumb.entityID
    }
    
    // MARK: - Selection Visualization Functions
    
    /// Apply selection visualization to entities based on current highlight mode
    func applySelectionVisualization(to entityIDs: Set<UUID>) {
        for entityID in entityIDs {
            switch highlightMode {
            case .outline:
                applyOutlineHighlight(to: entityID)
            case .silhouette:
                applySilhouetteHighlight(to: entityID)
            case .boundingBox:
                applyBoundingBoxHighlight(to: entityID)
            case .wireframeOverlay:
                applyWireframeOverlay(to: entityID)
            case .glow:
                applyGlowEffect(to: entityID)
            case .pulsing:
                applyPulsingIndicator(to: entityID)
            case .colorCoded:
                applyColorCodedHighlight(to: entityID)
            }
        }
    }
    
    /// Clear selection visualization from entities
    func clearSelectionVisualization(from entityIDs: Set<UUID>) {
        for entityID in entityIDs {
            clearEntityVisualization(entityID)
        }
    }
    
    // MARK: - Individual Visualization Implementations
    
    /// Apply outline highlight with customizable color and thickness
    private func applyOutlineHighlight(to entityID: UUID) {
        // Convert SwiftUI Color to RGB components
        let colorComponents = outlineColor.cgColor?.components ?? [0, 0, 1, 1]
        let r = Float(colorComponents[0])
        let g = Float(colorComponents[1])
        let b = Float(colorComponents[2])
        
        // Set outline properties via EngineBridge
        EngineBridge.shared.setSelectionOutlineColor(r: r, g: g, b: b)
        EngineBridge.shared.setSelectionOutlineWidth(outlineThickness)
        
        // Enable outline visualization for this entity
        EngineBridge.shared.setMeshOverlayColor(entityID, r: r, g: g, b: b, a: 1.0)
        
        print("[Selection] Applied outline highlight to entity \(entityID): color(\(r), \(g), \(b)), thickness(\(outlineThickness))")
    }
    
    /// Apply silhouette highlighting
    private func applySilhouetteHighlight(to entityID: UUID) {
        // Silhouette is implemented as a solid color overlay with backface culling disabled
        EngineBridge.shared.setMeshOverlayColor(entityID, r: 1.0, g: 1.0, b: 0.0, a: 0.8)
        
        print("[Selection] Applied silhouette highlight to entity \(entityID)")
    }
    
    /// Apply bounding box highlight
    private func applyBoundingBoxHighlight(to entityID: UUID) {
        // Enable bounds visualization for this entity
        EngineBridge.shared.setMeshBoundsVisualization(entityID, enabled: true)
        
        print("[Selection] Applied bounding box highlight to entity \(entityID)")
    }
    
    /// Apply wireframe overlay on selection
    private func applyWireframeOverlay(to entityID: UUID) {
        // Enable wireframe visualization for this entity
        EngineBridge.shared.setMeshWireframeEnabled(entityID, enabled: true)
        
        print("[Selection] Applied wireframe overlay to entity \(entityID)")
    }
    
    /// Apply glow effect on selected objects
    private func applyGlowEffect(to entityID: UUID) {
        // Glow is implemented as a semi-transparent overlay with emission
        let intensity = glowIntensity
        EngineBridge.shared.setMeshOverlayColor(entityID, r: 0.3, g: 0.7, b: 1.0, a: intensity * 0.5)
        
        print("[Selection] Applied glow effect to entity \(entityID): intensity(\(intensity))")
    }
    
    /// Apply pulsing selection indicator
    private func applyPulsingIndicator(to entityID: UUID) {
        // Pulsing requires animation - set up base properties
        EngineBridge.shared.setMeshOverlayColor(entityID, r: 0.2, g: 0.8, b: 0.2, a: 0.6)
        
        // Store pulse state for animation updates
        // This would be updated in a timer or animation loop
        
        print("[Selection] Applied pulsing indicator to entity \(entityID): speed(\(pulseSpeed))")
    }
    
    /// Apply color-coded multi-selection highlighting
    private func applyColorCodedHighlight(to entityID: UUID) {
        // Generate unique color based on entity ID hash
        let hash = abs(entityID.hashValue)
        let hue = Float(hash % 360) / 360.0
        
        // Convert HSV to RGB for consistent color distribution
        let rgb = hsvToRGB(h: hue, s: 0.8, v: 1.0)
        EngineBridge.shared.setMeshOverlayColor(entityID, r: rgb.r, g: rgb.g, b: rgb.b, a: 0.7)
        
        print("[Selection] Applied color-coded highlight to entity \(entityID): hue(\(hue))")
    }
    
    /// Clear visualization from a specific entity
    private func clearEntityVisualization(_ entityID: UUID) {
        // Reset all visualization properties
        EngineBridge.shared.setMeshOverlayColor(entityID, r: 0, g: 0, b: 0, a: 0)
        EngineBridge.shared.setMeshWireframeEnabled(entityID, enabled: false)
        EngineBridge.shared.setMeshBoundsVisualization(entityID, enabled: false)
        
        print("[Selection] Cleared visualization from entity \(entityID)")
    }
    
    // MARK: - Animation Updates
    
    /// Update pulsing animations (call this from render loop)
    func updateAnimations(deltaTime: Float) {
        guard pulseEnabled && highlightMode == .pulsing else { return }
        
        // Update pulse phase
        // This would need to track which entities have pulsing enabled
        // and update their overlay alpha values based on sine wave
        
        let pulsePhase = sin(Float(Date().timeIntervalSince1970) * pulseSpeed * 2.0 * Float.pi)
        let alpha = (pulsePhase + 1.0) * 0.3 + 0.2 // Range: 0.2 to 0.8
        
        // Apply to all pulsing entities
        // This would require tracking selected entities separately
    }
    
    // MARK: - Utility Functions
    
    /// Convert HSV color to RGB
    private func hsvToRGB(h: Float, s: Float, v: Float) -> (r: Float, g: Float, b: Float) {
        let c = v * s
        let x = c * (1 - abs((h * 6).truncatingRemainder(dividingBy: 2) - 1))
        let m = v - c
        
        var r: Float = 0, g: Float = 0, b: Float = 0
        
        if h < 1/6 {
            r = c; g = x; b = 0
        } else if h < 2/6 {
            r = x; g = c; b = 0
        } else if h < 3/6 {
            r = 0; g = c; b = x
        } else if h < 4/6 {
            r = 0; g = x; b = c
        } else if h < 5/6 {
            r = x; g = 0; b = c
        } else {
            r = c; g = 0; b = x
        }
        
        return (r + m, g + m, b + m)
    }
    
    // TODO: Implement selection breadcrumbs trail
    func addToSelectionHistory(_ entityIDs: Set<UUID>) {
        // Truncate forward history if we're not at the end
        if historyIndex < selectionHistory.count - 1 {
            selectionHistory.removeLast(selectionHistory.count - historyIndex - 1)
        }
        
        let snapshot = SelectionSnapshot(
            timestamp: Date(),
            entityIDs: entityIDs,
            description: "\(entityIDs.count) object(s)"
        )
        selectionHistory.append(snapshot)
        historyIndex = selectionHistory.count - 1
        
        print("[Selection] Added to history: \(snapshot.description)")
    }
    
    // TODO: Implement selection history navigation (back/forward)
    func navigateHistoryBack() -> Set<UUID>? {
        guard historyIndex > 0 else { return nil }
        historyIndex -= 1
        return selectionHistory[historyIndex].entityIDs
    }
    
    func navigateHistoryForward() -> Set<UUID>? {
        guard historyIndex < selectionHistory.count - 1 else { return nil }
        historyIndex += 1
        return selectionHistory[historyIndex].entityIDs
    }
    
    func clearHistory() {
        selectionHistory.removeAll()
        historyIndex = -1
    }
    
    // TODO: Implement selection sets (save/load groups)
    func saveSelectionSet(name: String, entityIDs: Set<UUID>, color: Color = .blue) {
        let set = SelectionSet(name: name, entityIDs: entityIDs, color: color)
        selectionSets.append(set)
        print("[Selection] Saved selection set: \(name) with \(entityIDs.count) objects")
    }
    
    func loadSelectionSet(_ id: UUID) -> Set<UUID>? {
        return selectionSets.first(where: { $0.id == id })?.entityIDs
    }
    
    func deleteSelectionSet(_ id: UUID) {
        selectionSets.removeAll { $0.id == id }
    }
    
    // TODO: Implement selection filters (by type, tag, layer)
    func applyFilter(_ filter: SelectionFilter, to entities: [EntityNode]) -> Set<UUID> {
        let filtered = entities.filter { filter.matches(entity: $0) }
        return Set(filtered.map { $0.id })
    }
    
    // TODO: Implement invert selection
    func invertSelection(current: Set<UUID>, all: [EntityNode]) -> Set<UUID> {
        let allIDs = Set(all.map { $0.id })
        return allIDs.subtracting(current)
    }
    
    // TODO: Implement grow/shrink selection
    func growSelection(current: Set<UUID>, entities: [EntityNode]) -> Set<UUID> {
        // Add all children of selected entities
        var grown = current
        for entity in entities where current.contains(entity.id) {
            grown.formUnion(entity.children.map { $0.id })
        }
        return grown
    }
    
    func shrinkSelection(current: Set<UUID>, entities: [EntityNode]) -> Set<UUID> {
        // Remove entities that have selected parents
        return current.filter { id in
            guard let entity = entities.first(where: { $0.id == id }) else { return false }
            return entity.parentEntity == nil || !current.contains(entity.parentEntity!.id)
        }
    }
    
    // TODO: Implement select similar objects
    func selectSimilar(to entityID: UUID, entities: [EntityNode], criterion: SimilarityCriterion) -> Set<UUID> {
        guard let reference = entities.first(where: { $0.id == entityID }) else {
            return []
        }
        
        return Set(entities.filter { entity in
            switch criterion {
            case .name:
                return entity.name == reference.name
            case .icon:
                return entity.icon == reference.icon
            case .hasChildren:
                return !entity.children.isEmpty == !reference.children.isEmpty
            }
        }.map { $0.id })
    }
    
    enum SimilarityCriterion {
        case name
        case icon
        case hasChildren
        case material(UUID)
        case mesh(UUID)
        case componentType(ComponentType)
        case tag(String)
        case layer(Int)
        
        func matches(entity: EntityNode, reference: EntityNode) -> Bool {
            switch self {
            case .name:
                return entity.name == reference.name
            case .icon:
                return entity.icon == reference.icon
            case .hasChildren:
                return !entity.children.isEmpty == !reference.children.isEmpty
            case .material(let materialID):
                return entity.materialID == materialID && reference.materialID == materialID
            case .mesh(let meshID):
                return entity.meshID == meshID && reference.meshID == meshID
            case .componentType(let componentType):
                let entityHasComponent = entity.components.contains { $0.type == componentType }
                let referenceHasComponent = reference.components.contains { $0.type == componentType }
                return entityHasComponent == referenceHasComponent
            case .tag(let tag):
                return entity.tags.contains(tag) && reference.tags.contains(tag)
            case .layer(let layer):
                return entity.layer == layer && reference.layer == layer
            }
        }
    }
    
    // Implement selection statistics panel
    func getSelectionStatistics(entityIDs: Set<UUID>, entities: [EntityNode]) -> SelectionStatistics {
        let selected = entities.filter { entityIDs.contains($0.id) }
        
        var totalVertices: Int = 0
        var totalTriangles: Int = 0
        var boundingBoxMin = SIMD3<Float>(Float.greatestFiniteMagnitude)
        var boundingBoxMax = SIMD3<Float>(-Float.greatestFiniteMagnitude)
        var totalMass: Float = 0.0
        var weightedCenter = SIMD3<Float>(0, 0, 0)
        
        for entity in selected {
            // Get mesh data for this entity
            if let meshData = getMeshData(for: entity) {
                totalVertices += meshData.vertexCount
                totalTriangles += meshData.triangleCount
                
                // Update bounding box
                boundingBoxMin = simd_min(boundingBoxMin, meshData.boundsMin)
                boundingBoxMax = simd_max(boundingBoxMax, meshData.boundsMax)
                
                // Calculate center of mass contribution
                let entityMass = meshData.mass
                totalMass += entityMass
                weightedCenter += meshData.center * entityMass
            }
        }
        
        // Calculate final center of mass
        let centerOfMass = totalMass > 0 ? weightedCenter / totalMass : SIMD3<Float>(0, 0, 0)
        
        // Handle case where no entities were selected
        if selected.isEmpty {
            boundingBoxMin = SIMD3<Float>(0, 0, 0)
            boundingBoxMax = SIMD3<Float>(0, 0, 0)
        }
        
        return SelectionStatistics(
            count: selected.count,
            totalVertices: totalVertices,
            totalTriangles: totalTriangles,
            boundingBoxMin: boundingBoxMin,
            boundingBoxMax: boundingBoxMax,
            centerOfMass: centerOfMass
        )
    }
    
    /// Helper function to get mesh data for an entity
    private func getMeshData(for entity: EntityNode) -> MeshData? {
        // This would typically interface with the engine's mesh system
        // For now, we'll return mock data or access cached mesh information
        
        // In a real implementation, this would:
        // 1. Access the entity's mesh component
        // 2. Get vertex and triangle counts from the mesh
        // 3. Calculate bounding box from mesh vertices
        // 4. Calculate mass based on material density and volume
        
        return MeshData(
            vertexCount: entity.vertexCount ?? 0,
            triangleCount: entity.triangleCount ?? 0,
            boundsMin: entity.boundsMin ?? SIMD3<Float>(0, 0, 0),
            boundsMax: entity.boundsMax ?? SIMD3<Float>(0, 0, 0),
            center: entity.center ?? SIMD3<Float>(0, 0, 0),
            mass: entity.mass ?? 1.0
        )
    }
    
    /// Helper struct for mesh data
    private struct MeshData {
        let vertexCount: Int
        let triangleCount: Int
        let boundsMin: SIMD3<Float>
        let boundsMax: SIMD3<Float>
        let center: SIMD3<Float>
        let mass: Float
    }
    
    /// Get color for multi-selection based on index
    func getSelectionColor(for index: Int) -> Color {
        return selectionColors[index % selectionColors.count]
    }
    
    /// Apply color coding to multiple selected entities
    func applyMultiSelectionColors(to entityIDs: Set<UUID>, entities: [EntityNode]) {
        let sortedEntities = entities.filter { entityIDs.contains($0.id) }
        
        for (index, entity) in sortedEntities.enumerated() {
            let color = getSelectionColor(for: index)
            // This would apply the color to the entity's visualization
            print("[Selection] Applied color \(index) to entity: \(entity.name)")
        }
    }
    
    struct SelectionStatistics {
        let count: Int
        let totalVertices: Int
        let totalTriangles: Int
        let boundingBoxMin: SIMD3<Float>
        let boundingBoxMax: SIMD3<Float>
        let centerOfMass: SIMD3<Float>
        
        var boundingBoxSize: SIMD3<Float> {
            boundingBoxMax - boundingBoxMin
        }
    }
}

// MARK: - Entity Node Extension
extension EntityNode {
    var typeName: String {
        return String(describing: type(of: self))
    }
    
    var tags: [String] {
        get { userData["tags"] as? [String] ?? [] }
        set { userData["tags"] = newValue }
    }
    
    var layer: Int {
        get { userData["layer"] as? Int ?? 0 }
        set { userData["layer"] = newValue }
    }
    
    var materialID: UUID? {
        get { userData["materialID"] as? UUID }
        set { userData["materialID"] = newValue }
    }
    
    var meshID: UUID? {
        get { userData["meshID"] as? UUID }
        set { userData["meshID"] = newValue }
    }
    
    var vertexCount: Int? {
        get { userData["vertexCount"] as? Int }
        set { userData["vertexCount"] = newValue }
    }
    
    var triangleCount: Int? {
        get { userData["triangleCount"] as? Int }
        set { userData["triangleCount"] = newValue }
    }
    
    var boundsMin: SIMD3<Float>? {
        get { userData["boundsMin"] as? SIMD3<Float> }
        set { userData["boundsMin"] = newValue }
    }
    
    var boundsMax: SIMD3<Float>? {
        get { userData["boundsMax"] as? SIMD3<Float> }
        set { userData["boundsMax"] = newValue }
    }
    
    var center: SIMD3<Float>? {
        get { userData["center"] as? SIMD3<Float> }
        set { userData["center"] = newValue }
    }
    
    var mass: Float? {
        get { userData["mass"] as? Float }
        set { userData["mass"] = newValue }
    }
    
    var userData: [String: Any] {
        get { objc_getAssociatedObject(self, &userDataKey) as? [String: Any] ?? [:] }
        set { objc_setAssociatedObject(self, &userDataKey, newValue, .OBJC_ASSOCIATION_RETAIN_NONATOMIC) }
    }
}

private var userDataKey: UInt8 = 0

// Mock EntityNode class for demonstration
class EntityNode {
    let id = UUID()
    var name: String = ""
    var icon: String = "cube"
    var children: [EntityNode] = []
    var parentEntity: EntityNode?
    var components: [Component] = []
    
    init(name: String = "Entity") {
        self.name = name
    }
}

// Mock Component class
class Component {
    let type: ComponentType
    
    init(type: ComponentType) {
        self.type = type
    }
}

// Mock ComponentType enum
enum ComponentType {
    case mesh
    case transform
    case physics
    case material
    case light
    case camera
    case audio
    case script
}

// MARK: - Selection Visualization Panel
struct SelectionVisualizationPanel: View {
    @ObservedObject var manager: SelectionVisualizationManager
    @State private var showingHistory = false
    @State private var showingSets = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Highlight mode
            HStack {
                Text("Highlight Mode")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Menu {
                    ForEach(SelectionVisualizationManager.HighlightMode.allCases) { mode in
                        Button(action: {
                            manager.highlightMode = mode
                        }) {
                            HStack {
                                Image(systemName: mode.icon)
                                Text(mode.rawValue)
                            }
                        }
                    }
                } label: {
                    HStack {
                        Image(systemName: manager.highlightMode.icon)
                        Text(manager.highlightMode.rawValue)
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
            
            // Settings
            VStack(spacing: 8) {
                // Outline color
                HStack {
                    Text("Outline Color")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    ColorPicker("", selection: $manager.outlineColor)
                        .labelsHidden()
                }
                
                // Outline thickness
                HStack {
                    Text("Thickness")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $manager.outlineThickness, in: 1...10)
                    
                    Text(String(format: "%.1f", manager.outlineThickness))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
                
                // Pulse
                HStack {
                    Toggle("Pulse Effect", isOn: $manager.pulseEnabled)
                        .font(DesignSystem.Typography.small)
                        .toggleStyle(.checkbox)
                    
                    Spacer()
                    
                    if manager.pulseEnabled {
                        Slider(value: $manager.pulseSpeed, in: 0.1...5.0)
                            .frame(width: 100)
                        
                        Text(String(format: "%.1fx", manager.pulseSpeed))
                            .font(DesignSystem.Typography.small)
                            .frame(width: 40)
                    }
                }
                
                // Glow
                HStack {
                    Text("Glow Intensity")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 100, alignment: .leading)
                    
                    Slider(value: $manager.glowIntensity, in: 0...1)
                    
                    Text("\(Int(manager.glowIntensity * 100))%")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Selection history
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingHistory.toggle()
                }) {
                    HStack {
                        Text("Selection History (\(manager.selectionHistory.count))")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Image(systemName: showingHistory ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                }
                .buttonStyle(.plain)
                
                if showingHistory {
                    HStack(spacing: 4) {
                        EditorIconButton(icon: "arrow.left", tooltip: "Back") {
                            _ = manager.navigateHistoryBack()
                        }
                        .disabled(manager.historyIndex <= 0)
                        
                        EditorIconButton(icon: "arrow.right", tooltip: "Forward") {
                            _ = manager.navigateHistoryForward()
                        }
                        .disabled(manager.historyIndex >= manager.selectionHistory.count - 1)
                        
                        EditorIconButton(icon: "trash", tooltip: "Clear History") {
                            manager.clearHistory()
                        }
                    }
                    
                    ScrollView {
                        VStack(spacing: 4) {
                            ForEach(Array(manager.selectionHistory.enumerated()), id: \.offset) { index, snapshot in
                                SelectionHistoryRow(
                                    snapshot: snapshot,
                                    isActive: index == manager.historyIndex
                                )
                            }
                        }
                    }
                    .frame(maxHeight: 150)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Selection sets
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingSets.toggle()
                }) {
                    HStack {
                        Text("Selection Sets (\(manager.selectionSets.count))")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Image(systemName: showingSets ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                }
                .buttonStyle(.plain)
                
                if showingSets {
                    ForEach(manager.selectionSets) { set in
                        SelectionSetRow(
                            set: set,
                            onLoad: {
                                _ = manager.loadSelectionSet(set.id)
                            },
                            onDelete: {
                                manager.deleteSelectionSet(set.id)
                            }
                        )
                    }
                }
            }
            .padding(8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Selection History Row
private struct SelectionHistoryRow: View {
    let snapshot: SelectionVisualizationManager.SelectionSnapshot
    let isActive: Bool
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(snapshot.description)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(isActive ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                
                Text(snapshot.timestamp, style: .time)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
        }
        .padding(4)
        .background(isActive ? DesignSystem.Colors.selection : Color.clear)
        .cornerRadius(4)
    }
}

// MARK: - Selection Set Row
private struct SelectionSetRow: View {
    let set: SelectionVisualizationManager.SelectionSet
    let onLoad: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack {
            Circle()
                .fill(set.color)
                .frame(width: 12, height: 12)
            
            Text(set.name)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text("(\(set.entityIDs.count))")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
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
