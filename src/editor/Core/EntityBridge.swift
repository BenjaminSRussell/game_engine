import Foundation
import CEngineCore

// MARK: - Swift Wrapper for C Entity

/// Swift-native representation of the C Entity struct from ECS
public struct SwiftEntity: Hashable, Identifiable {
    public let id: UInt32
    public let generation: UInt32
    
    /// Unique identifier for SwiftUI compatibility
    public var identifier: String {
        "\(id)_\(generation)"
    }
    
    /// Initialize from C Entity struct
    public init(from cEntity: Entity) {
        self.id = cEntity.id
        self.generation = cEntity.generation
    }
    
    /// Convert back to C Entity struct
    public func toCEntity() -> Entity {
        return Entity(id: self.id, generation: self.generation)
    }
    
    /// Create an invalid entity
    public static var invalid: SwiftEntity {
        SwiftEntity(from: Entity(id: 0, generation: 0))
    }
    
    /// Check validity
    public var isValid: Bool {
        return id != 0
    }
}

// MARK: - Entity Collection for Query Iteration

/// Collection wrapper for ECS entities, allowing Swift-style iteration
/// Example: for entity in EntityCollection(world, query) { ... }
public struct EntityCollection: Collection {
    public typealias Element = SwiftEntity
    public typealias Index = Int
    
    private let entities: [SwiftEntity]
    
    public var startIndex: Int { entities.startIndex }
    public var endIndex: Int { entities.endIndex }
    
    public init(world: UnsafeMutablePointer<World>) {
        var count: UInt32 = 0
        guard let cEntities = ecs_bridge_get_all_entities(world, &count) else {
            self.entities = []
            return
        }
        
        // Convert C array to Swift array
        self.entities = (0..<Int(count)).map { i in
            SwiftEntity(from: cEntities[i])
        }
        
        // Free C array
        free(cEntities)
    }
    
    public subscript(position: Int) -> SwiftEntity {
        entities[position]
    }
    
    public func index(after i: Int) -> Int {
        entities.index(after: i)
    }
}

// MARK: - Component Type Bridges

/// Swift wrapper for ECS Component ID
public typealias ComponentID = UInt32

/// Component data wrapper with type-safe access
public struct ComponentData {
    public let componentID: ComponentID
    public let name: String
    public let data: UnsafeMutableRawPointer?
    
    public init(world: UnsafeMutablePointer<World>, componentID: ComponentID) {
        self.componentID = componentID
        let cName = ecs_bridge_get_component_name(world, componentID)
        self.name = cName != nil ? String(cString: cName!) : "Unknown"
        self.data = nil
    }
    
    /// Access component data as a typed value
    public func getValue<T>() -> T? {
        guard let data = data else { return nil }
        return data.assumingMemoryBound(to: T.self).pointee
    }
}

// MARK: - Actor for Thread-Safe World Access

/// Thread-safe wrapper for World access from Swift
@available(macOS 14.0, *)
public actor WorldManager {
    private let worldPtr: UnsafeMutablePointer<World>
    
    public init(world: UnsafeMutablePointer<World>) {
        self.worldPtr = world
    }
    
    /// Get all entities in the world
    public func getAllEntities() -> [SwiftEntity] {
        Array(EntityCollection(world: worldPtr))
    }
    
    /// Get component data for an entity
    public func getComponent(_ entity: SwiftEntity, type: ComponentID) -> UnsafeMutableRawPointer? {
        let cEntity = entity.toCEntity()
        return ecs_bridge_get_component_data(worldPtr, cEntity, type)
    }
    
    /// Check if entity has a component
    public func hasComponent(_ entity: SwiftEntity, type: ComponentID) -> Bool {
        let cEntity = entity.toCEntity()
        return ecs_has_component(worldPtr, cEntity, type)
    }
}
