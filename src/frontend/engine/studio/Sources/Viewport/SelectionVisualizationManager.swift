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
    
    // Selection sets
    @Published var selectionSets: [SelectionSet] = []
    
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
        
        // TODO: Save selection set to file
        // TODO: Load selection set from file
    }
    
    // MARK: - Selection Filters
    enum SelectionFilter {
        case byType(String)
        case byTag(String)
        case byLayer(Int)
        case byMaterial(UUID)
        case byMesh(UUID)
        case byComponent(ComponentType)
        
        // TODO: Implement filter matching logic
        func matches(entity: EntityNode) -> Bool {
            return false
        }
    }
    
    // MARK: - Functions
    
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
        // TODO: Add material, mesh, component type criteria
    }
    
    // TODO: Implement selection statistics panel
    func getSelectionStatistics(entityIDs: Set<UUID>, entities: [EntityNode]) -> SelectionStatistics {
        let selected = entities.filter { entityIDs.contains($0.id) }
        
        return SelectionStatistics(
            count: selected.count,
            totalVertices: 0, // TODO: Sum vertices from all selected meshes
            totalTriangles: 0, // TODO: Sum triangles from all selected meshes
            boundingBoxMin: SIMD3<Float>(0, 0, 0), // TODO: Calculate combined bounding box
            boundingBoxMax: SIMD3<Float>(0, 0, 0),
            centerOfMass: SIMD3<Float>(0, 0, 0) // TODO: Calculate center of mass
        )
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
