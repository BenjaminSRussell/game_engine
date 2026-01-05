import Foundation
import SwiftUI
import Combine

// MARK: - Scene Graph Core

/// Types of items in the scene graph
enum SceneItemType: String, Codable {
    case entity
    case collection
}

/// Base protocol for scene graph items
protocol SceneGraphItem: Identifiable, ObservableObject {
    var id: UUID { get }
    var name: String { get set }
    var isVisible: Bool { get set }
    var isSelectable: Bool { get set }
    var isExpanded: Bool { get set }
    var icon: String { get }
}

// MARK: - View Layer System

/// Represents a configuration of visible collections (like Blender's View Layers)
struct ViewLayer: Identifiable, Hashable {
    let id = UUID()
    var name: String
    var visibleCollections: Set<UUID> = [] // IDs of visible collections
    var activeCollectionID: UUID?
    
    static let `default` = ViewLayer(name: "Layout")
}

// MARK: - Scene Collection

/// A collection of entities and other collections (nested)
class SceneCollection: SceneGraphItem {
    let id: UUID
    @Published var name: String
    @Published var isVisible: Bool = true
    @Published var isRenderable: Bool = true
    @Published var isSelectable: Bool = true
    @Published var isExpanded: Bool = true
    @Published var colorTag: Color = .clear
    
    @Published var childrenCollections: [SceneCollection] = []
    @Published var entities: [EntityNode] = []
    
    weak var parent: SceneCollection?
    
    var icon: String { "folder.fill" }
    
    init(id: UUID = UUID(), name: String, colorTag: Color = .clear) {
        self.id = id
        self.name = name
        self.colorTag = colorTag
    }
    
    func addCollection(_ collection: SceneCollection) {
        collection.parent = self
        childrenCollections.append(collection)
    }
    
    func addEntity(_ entity: EntityNode) {
        // Remove from old parent if exists (handled by manager usually, but good for safety)
        entity.parentCollection = self
        entities.append(entity)
    }
    
    func removeEntity(_ entityID: UUID) {
        entities.removeAll { $0.id == entityID }
    }
    
    func removeCollection(_ collectionID: UUID) {
        childrenCollections.removeAll { $0.id == collectionID }
    }
    
    /// Recursively find an entity
    func findEntity(_ id: UUID) -> EntityNode? {
        if let found = entities.first(where: { $0.id == id }) {
            return found
        }
        for collection in childrenCollections {
            if let found = collection.findEntity(id) {
                return found
            }
        }
        return nil
    }
}

// MARK: - Entity Node

/// Represents an actual object in the scene
class EntityNode: SceneGraphItem {
    let id: UUID
    @Published var name: String
    @Published var isVisible: Bool
    @Published var isLocked: Bool
    @Published var isSelectable: Bool = true
    @Published var isExpanded: Bool = false // For entity hierarchy (bones etc)
    @Published var icon: String
    
    // Parent relationship
    weak var parentCollection: SceneCollection?
    weak var parentEntity: EntityNode?
    @Published var children: [EntityNode] = []
    
    init(id: UUID = UUID(), name: String, icon: String = "cube", isVisible: Bool = true, isLocked: Bool = false, children: [EntityNode] = []) {
        self.id = id
        self.name = name
        self.icon = icon
        self.isVisible = isVisible
        self.isLocked = isLocked
        self.children = children
        
        for child in children {
            child.parentEntity = self
        }
    }
    
    func addChild(_ child: EntityNode) {
        child.parentEntity = self
        children.append(child)
    }
    
    func removeChild(_ childID: UUID) {
        children.removeAll { $0.id == childID }
    }
    
    func toggleVisibility() {
        isVisible.toggle()
        print("[SceneGraph] Entity \(name) visibility: \(isVisible)")
        // Bridge to engine - createEntity will be enhanced with type later
        // EngineBridge.shared.createEntity(name: name)
    }
    
    func toggleLock() {
        isLocked.toggle()
        isSelectable = !isLocked
        print("[SceneGraph] Entity \(name) locked: \(isLocked)")
    }
}
