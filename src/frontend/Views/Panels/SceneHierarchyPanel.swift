import SwiftUI

// MARK: - Scene Hierarchy Panel

struct SceneHierarchyPanel: View {
    @ObservedObject var selectionManager: SelectionManager
    @State private var searchTerm = ""
    @State private var expandedEntities: Set<UUID> = []
    @State private var showingContextMenu = false
    @State private var contextMenuEntity: UUID?
    
    private var filteredEntities: [EntityBridgeData] {
        let allEntities = Array(EngineBridge.shared.entities.values)
        
        if searchTerm.isEmpty {
            return allEntities.sorted { $0.name < $1.name }
        } else {
            return allEntities.filter { entity in
                entity.name.localizedCaseInsensitiveContains(searchTerm)
            }.sorted { $0.name < $1.name }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Scene Hierarchy")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Menu {
                    Button("Create Empty") {
                        let entity = EngineBridge.shared.createEntity(name: "Empty Entity")
                        selectionManager.select(entity)
                    }
                    .keyboardShortcut("n", modifiers: [.command, .shift])
                    
                    Divider()
                    
                    Button("Cube") {
                        let entity = EngineBridge.shared.createEntity(name: "Cube")
                        selectionManager.select(entity)
                    }
                    
                    Button("Sphere") {
                        let entity = EngineBridge.shared.createEntity(name: "Sphere")
                        selectionManager.select(entity)
                    }
                    
                    Button("Plane") {
                        let entity = EngineBridge.shared.createEntity(name: "Plane")
                        selectionManager.select(entity)
                    }
                } label: {
                    Image(systemName: "plus")
                        .font(.system(size: 14, weight: .medium))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            Divider()
            
            // Search Bar
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .font(.system(size: 12))
                
                TextField("Search entities...", text: $searchTerm)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                if !searchTerm.isEmpty {
                    Button(action: { searchTerm = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.system(size: 12))
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(DesignSystem.CornerRadius.tight)
            .padding(DesignSystem.Spacing.md)
            
            // Entity List
            ScrollView {
                LazyVStack(spacing: 1) {
                    ForEach(filteredEntities, id: \.id) { entity in
                        EntityRow(
                            entity: entity,
                            isSelected: selectionManager.isSelected(entity.id),
                            isPrimarySelection: selectionManager.isPrimarySelection(entity.id),
                            isExpanded: expandedEntities.contains(entity.id)
                        )
                        .onTapGesture {
                            handleEntityTap(entity.id)
                        }
                        .onTapGesture(count: 2) {
                            // Focus entity in viewport
                        }
                        .contextMenu {
                            EntityContextMenu(entity: entity, selectionManager: selectionManager)
                        }
                    }
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    private func handleEntityTap(_ entityID: UUID) {
        if selectionManager.isSelected(entityID) {
            selectionManager.primarySelection = entityID
        } else {
            selectionManager.select(entityID)
        }
    }
}

// MARK: - Entity Row

struct EntityRow: View {
    let entity: EntityBridgeData
    let isSelected: Bool
    let isPrimarySelection: Bool
    let isExpanded: Bool
    
    @State private var isHovered = false
    @State private var isRenaming = false
    @State private var newName = ""
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Selection indicator
            Rectangle()
                .fill(isPrimarySelection ? DesignSystem.Colors.accentPrimary : Color.clear)
                .frame(width: 3)
                .animation(DesignSystem.Animations.quick, value: isPrimarySelection)
            
            // Expand/Collapse arrow (placeholder for hierarchy)
            Button(action: {}) {
                Image(systemName: "chevron.right")
                    .font(.system(size: 10, weight: .medium))
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .rotationEffect(.degrees(isExpanded ? 90 : 0))
            }
            .buttonStyle(.plain)
            .frame(width: 16)
            
            // Entity icon
            Image(systemName: entityIcon)
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(iconColor)
                .frame(width: 16)
            
            // Entity name
            if isRenaming {
                TextField("Entity name", text: $newName)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .onSubmit {
                        EngineBridge.shared.setEntityName(entity.id, name: newName)
                        isRenaming = false
                    }
                    .onEscape {
                        newName = entity.name
                        isRenaming = false
                    }
            } else {
                Text(entity.name)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                    .lineLimit(1)
            }
            
            Spacer()
            
            // Component indicators
            HStack(spacing: 4) {
                if entity.meshRenderer != nil {
                    Image(systemName: "cube.fill")
                        .font(.system(size: 8))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                
                if entity.physics != nil {
                    Image(systemName: "atom")
                        .font(.system(size: 8))
                        .foregroundColor(DesignSystem.Colors.accentSecondary)
                }
            }
            .opacity(isSelected ? 1.0 : 0.6)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, 4)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(backgroundColor)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .strokeBorder(isPrimarySelection ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 1)
        )
        .onHover { hovering in
            isHovered = hovering
        }
        .onAppear {
            newName = entity.name
        }
        .gesture(
            TapGesture(count: 2)
                .onEnded { _ in
                    isRenaming = true
                }
        )
    }
    
    private var backgroundColor: Color {
        if isSelected {
            return DesignSystem.Colors.selectionStrong
        } else if isHovered {
            return DesignSystem.Colors.hover
        } else {
            return Color.clear
        }
    }
    
    private var entityIcon: String {
        if entity.meshRenderer != nil {
            return "cube.fill"
        } else {
            return "cube"
        }
    }
    
    private var iconColor: Color {
        return isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary
    }
}

// MARK: - Entity Context Menu

struct EntityContextMenu: View {
    let entity: EntityBridgeData
    let selectionManager: SelectionManager
    
    var body: some View {
        Group {
            Button("Rename") {
                // Trigger rename mode
            }
            .keyboardShortcut("r", modifiers: .command)
            
            Button("Duplicate") {
                let newEntity = EngineBridge.shared.createEntity(name: "\(entity.name) Copy")
                
                // Copy components
                if let transform = EngineBridge.shared.entities[entity.id]?.transform {
                    EngineBridge.shared.setTransform(newEntity, transform: transform)
                }
                
                if let meshRenderer = entity.meshRenderer {
                    EngineBridge.shared.setMeshRenderer(newEntity, data: meshRenderer)
                }
                
                if let physics = entity.physics {
                    EngineBridge.shared.setPhysics(newEntity, data: physics)
                }
                
                selectionManager.select(newEntity)
            }
            .keyboardShortcut("d", modifiers: .command)
            
            Button("Delete") {
                EngineBridge.shared.deleteEntity(entity.id)
                selectionManager.deselect(entity.id)
            }
            .keyboardShortcut(.delete, modifiers: .command)
            
            Divider()
            
            Menu("Create Child") {
                Button("Empty") {
                    let child = EngineBridge.shared.createEntity(name: "Child Entity")
                    // Set parent relationship would go here
                }
                
                Button("Cube") {
                    let child = EngineBridge.shared.createEntity(name: "Cube")
                    // Set parent relationship would go here
                }
            }
            
            Divider()
            
            Button("Copy") {
                // Copy to clipboard
            }
            .keyboardShortcut("c", modifiers: .command)
            
            Button("Paste") {
                // Paste from clipboard
            }
            .keyboardShortcut("v", modifiers: .command)
        }
    }
}

// MARK: - On Escape Gesture

extension View {
    func onEscape(perform action: @escaping () -> Void) -> some View {
        self.simultaneousGesture(
            KeyboardShortcut(.escape)
                .onReceive(NotificationCenter.default.publisher(for: UIResponder.keyboardWillHideNotification)) { _ in
                    action()
                }
        )
    }
}
