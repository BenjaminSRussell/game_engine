import SwiftUI

// MARK: - Entity Node Model

/// Represents an entity in the scene hierarchy
// MARK: - Scene Hierarchy Panel
// Uses SceneGraphModels.swift


struct SceneHierarchyPanel: View {
    @StateObject private var viewModel = SceneHierarchyViewModel()
    @EnvironmentObject var selectionManager: SelectionManager
    @State private var searchText = ""
    @State private var draggedItem: SceneGraphItemWrapper?
    @State private var dropTarget: UUID?
    
    // Wrapper to handle drag data
    struct SceneGraphItemWrapper: Identifiable {
        let id: UUID
        let type: SceneItemType
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Outliner")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                
                // View Layer Selector (Simple for now)
                Menu {
                    Button("Layout") { }
                    Button("Modeling") { }
                    Button("Rendering") { }
                } label: {
                    HStack(spacing: 4) {
                        Image(systemName: "square.3.layers.3d")
                        Text("Layout")
                    }
                    .font(DesignSystem.Typography.small)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                }
                .menuStyle(.borderlessButton)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                
                EditorIconButton(icon: "folder.badge.plus", tooltip: "New Collection") {
                    viewModel.createCollection()
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Search / Filter
            HStack {
                EditorSearchBar(text: $searchText, placeholder: "Search...")
                
                // Filter Toggles
                HStack(spacing: 2) {
                    FilterToggleButton(icon: "lightbulb", isSelected: .constant(true))
                    FilterToggleButton(icon: "camera", isSelected: .constant(true))
                    FilterToggleButton(icon: "cube", isSelected: .constant(true))
                }
            }
            .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Outliner Tree
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    // Render Root Collections
                    ForEach(viewModel.sceneCollections) { collection in
                        SceneCollectionRow(
                            collection: collection,
                            selectionManager: selectionManager,
                            draggedItem: $draggedItem,
                            dropTarget: $dropTarget,
                            viewModel: viewModel,
                            indentLevel: 0
                        )
                    }
                }
                .padding(.vertical, DesignSystem.Spacing.xs)
            }
            .background(DesignSystem.Colors.backgroundSecondary)
        }
    }
}

struct FilterToggleButton: View {
    let icon: String
    @Binding var isSelected: Bool
    
    var body: some View {
        Button(action: { isSelected.toggle() }) {
            Image(systemName: icon)
                .font(.system(size: 10))
                .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
                .frame(width: 20, height: 20)
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Scene Collection Row

struct SceneCollectionRow: View {
    @ObservedObject var collection: SceneCollection
    @ObservedObject var selectionManager: SelectionManager
    @Binding var draggedItem: SceneHierarchyPanel.SceneGraphItemWrapper?
    @Binding var dropTarget: UUID?
    var viewModel: SceneHierarchyViewModel
    let indentLevel: Int
    
    var isSelected: Bool {
        selectionManager.selectedEntities.contains(collection.id)
    }
    
    var isDropTarget: Bool {
        dropTarget == collection.id
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Drop indicator
            if isDropTarget {
                Rectangle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(height: 2)
                    .padding(.leading, CGFloat(indentLevel) * 16)
            }
            
            // Row Content
            HStack(spacing: DesignSystem.Spacing.xs) {
                // Indent
                Spacer().frame(width: CGFloat(indentLevel) * 16)
                
                // Expand Arrow
                Button(action: { withAnimation { collection.isExpanded.toggle() } }) {
                    Image(systemName: collection.isExpanded ? "chevron.down" : "chevron.right")
                    .font(.system(size: 10))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 14, height: 14)
                }
                .buttonStyle(.plain)
                
                // Icon
                Image(systemName: "folder.fill")
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .font(.system(size: 12))
                
                // Name
                Text(collection.name)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Visibility Toggles
                HStack(spacing: 8) {
                    VisibilityToggle(isOn: $collection.isVisible, activeIcon: "eye", inactiveIcon: "eye.slash")
                    VisibilityToggle(isOn: $collection.isRenderable, activeIcon: "camera", inactiveIcon: "camera.slash")
                    VisibilityToggle(isOn: $collection.isSelectable, activeIcon: "arrow.up.left.square.fill", inactiveIcon: "arrow.up.left.square")
                }
            }
            .padding(.vertical, 4)
            .padding(.horizontal, 8)
            .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
            .contentShape(Rectangle())
            .onTapGesture {
                selectionManager.selectEntity(collection.id)
            }
            
            // Children
            if collection.isExpanded {
                // Nested Collections
                ForEach(collection.childrenCollections) { child in
                    SceneCollectionRow(
                        collection: child,
                        selectionManager: selectionManager,
                        draggedItem: $draggedItem,
                        dropTarget: $dropTarget,
                        viewModel: viewModel,
                        indentLevel: indentLevel + 1
                    )
                }
                
                // Entities
                ForEach(collection.entities) { entity in
                    EntityRow(
                        entity: entity,
                        selectionManager: selectionManager,
                        draggedItem: $draggedItem,
                        dropTarget: $dropTarget,
                        viewModel: viewModel,
                        indentLevel: indentLevel + 1
                    )
                }
            }
        }
    }
}

// MARK: - Entity Row

struct EntityRow: View {
    @ObservedObject var entity: EntityNode
    @ObservedObject var selectionManager: SelectionManager
    @Binding var draggedItem: SceneHierarchyPanel.SceneGraphItemWrapper?
    @Binding var dropTarget: UUID?
    var viewModel: SceneHierarchyViewModel
    let indentLevel: Int
    
    var isSelected: Bool {
        selectionManager.selectedEntities.contains(entity.id)
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Indent
            Spacer().frame(width: CGFloat(indentLevel) * 16)
            
            // Icon spacer
            Spacer().frame(width: 14)
            
            // Icon
            Image(systemName: entity.icon)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .font(.system(size: 12))
            
            // Name
            Text(entity.name)
                .font(DesignSystem.Typography.body)
                .foregroundColor(entity.isVisible ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
            
            Spacer()
            
            // Visibility Toggles
            HStack(spacing: 8) {
                VisibilityToggle(isOn: $entity.isVisible, activeIcon: "eye", inactiveIcon: "eye.slash")
                VisibilityToggle(isOn: $entity.isLocked, activeIcon: "lock", inactiveIcon: "lock.open")
            }
        }
        .padding(.vertical, 2)
        .padding(.horizontal, 8)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .contentShape(Rectangle())
        .onTapGesture {
            selectionManager.selectEntity(entity.id)
        }
        .contextMenu {
            Button("Rename") { /* Rename logic */ }
            Button("Delete", role: .destructive) { viewModel.deleteEntity(entity.id, selectionManager: selectionManager) }
        }
    }
}

struct VisibilityToggle: View {
    @Binding var isOn: Bool
    let activeIcon: String
    let inactiveIcon: String
    
    var body: some View {
        Button(action: { isOn.toggle() }) {
            Image(systemName: isOn ? activeIcon : inactiveIcon)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .font(.system(size: 10))
                .frame(width: 16, height: 16)
        }
        .buttonStyle(.plain)
    }
}

// MARK: - View Model

class SceneHierarchyViewModel: ObservableObject {
    @Published var sceneCollections: [SceneCollection] = []
    
    init() {
        setupDemoScene()
    }
    
    func setupDemoScene() {
        let masterCollection = SceneCollection(name: "Scene Collection")
        
        let envCollection = SceneCollection(name: "Environment")
        envCollection.addEntity(EntityNode(name: "Sky", icon: "cloud.sun.fill"))
        envCollection.addEntity(EntityNode(name: "Terrain", icon: "mountain.2.fill"))
        
        let charCollection = SceneCollection(name: "Characters")
        charCollection.addEntity(EntityNode(name: "Player", icon: "person.fill"))
        charCollection.addEntity(EntityNode(name: "NPC_01", icon: "person.fill"))
        
        let lightsCollection = SceneCollection(name: "Lighting")
        lightsCollection.addEntity(EntityNode(name: "Sun", icon: "sun.max.fill"))
        
        masterCollection.addCollection(envCollection)
        masterCollection.addCollection(charCollection)
        masterCollection.addCollection(lightsCollection)
        
        sceneCollections = [masterCollection]
    }
    
    func createCollection() {
        let newCollection = SceneCollection(name: "Collection \(Int.random(in: 1...100))")
        if let root = sceneCollections.first {
            root.addCollection(newCollection)
        } else {
            sceneCollections.append(newCollection)
        }
    }
    
    func deleteEntity(_ id: UUID, selectionManager: SelectionManager) {
        // Delete from engine
        EngineBridge.shared.deleteEntity(id)
        
        // Delete from UI
        deleteEntityRecursive(id, in: &sceneCollections)
        if selectionManager.selectedEntities.contains(id) {
            selectionManager.selectedEntities.remove(id)
        }
        objectWillChange.send()
        
        print("[Swift] Deleted entity: \(id)")
    }
    
    // MARK: - Command System API
    
    func setTransform(_ entityID: UUID, transform: TransformData) {
        EngineBridge.shared.setTransform(entityID, position: transform.position, rotation: transform.rotation, scale: transform.scale)
        print("[Swift] Set transform for entity: \(entityID)")
    }
    
    func createEntity(id: UUID, name: String, parentID: UUID?) {
        EngineBridge.shared.createEntityWithID(id, name: name)
        
        // Create UI node
        let newEntity = EntityNode(id: id, name: name, icon: "cube")
        
        // Find parent
        if let parentID = parentID {
            if let parentEntity = findEntity(parentID, in: sceneCollections) {
                parentEntity.addChild(newEntity)
            } else if let parentCollection = findCollection(parentID, in: sceneCollections) {
                parentCollection.addEntity(newEntity)
            } else {
                // Fallback to root collection if parent not found
                sceneCollections.first?.addEntity(newEntity)
            }
        } else {
            // Default to first user collection or root
            sceneCollections.first?.addEntity(newEntity)
        }
        
        objectWillChange.send()
        print("[Swift] Created entity with ID: \(id)")
    }
    
    func restoreEntity(_ data: EntityData) {
        EngineBridge.shared.createEntityWithID(data.id, name: data.name)
        EngineBridge.shared.setTransform(data.id, position: data.transform.position, rotation: data.transform.rotation, scale: data.transform.scale)
        
        let restoredEntity = EntityNode(id: data.id, name: data.name, icon: "cube")
        
        // Try to place back in original parent
        if let parentID = data.parentID {
            if let parentEntity = findEntity(parentID, in: sceneCollections) {
                parentEntity.addChild(restoredEntity)
            } else if let parentCollection = findCollection(parentID, in: sceneCollections) {
                parentCollection.addEntity(restoredEntity)
            } else {
                sceneCollections.first?.addEntity(restoredEntity)
            }
        } else {
            sceneCollections.first?.addEntity(restoredEntity)
        }
        
        objectWillChange.send()
        print("[Swift] Restored entity: \(data.name)")
    }
    
    // MARK: - Helpers
    
    private func deleteEntityRecursive(_ id: UUID, in collections: inout [SceneCollection]) {
        for collection in collections {
            collection.removeEntity(id)
            // Recurse into children entities
            for entity in collection.entities {
                deleteEntityFromEntityRecursive(id, parent: entity)
            }
            // Recurse into children collections
            deleteEntityRecursive(id, in: &collection.childrenCollections)
        }
    }
    
    private func deleteEntityFromEntityRecursive(_ id: UUID, parent: EntityNode) {
        parent.removeChild(id)
        for child in parent.children {
            deleteEntityFromEntityRecursive(id, parent: child)
        }
    }
    
    private func findEntity(_ id: UUID, in collections: [SceneCollection]) -> EntityNode? {
        for collection in collections {
            if let found = collection.findEntity(id) {
                return found
            }
        }
        return nil
    }
    
    private func findCollection(_ id: UUID, in collections: [SceneCollection]) -> SceneCollection? {
        for collection in collections {
            if collection.id == id { return collection }
            if let found = findCollection(id, in: collection.childrenCollections) {
                return found
            }
        }
        return nil
    }
}
