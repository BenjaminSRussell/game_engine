import Foundation
import simd

public struct EntityID: Hashable, Sendable {
    let id: UInt64
}

public protocol Component: Sendable {}

public struct TransformComponent: Component {
    public var position: SIMD3<Float>
    public var rotation: simd_quatf
    public var scale: SIMD3<Float>
    
    public init(position: SIMD3<Float> = .zero, rotation: simd_quatf = simd_quatf(ix: 0, iy: 0, iz: 0, r: 1), scale: SIMD3<Float> = .one) {
        self.position = position
        self.rotation = rotation
        self.scale = scale
    }
}

public final class Archetype {
    let signature: Set<String>
    var entities: [EntityID] = []
    var componentData: [String: [Any]] = [:] // ComponentType -> Array of components
    
    init(signature: Set<String>) {
        self.signature = signature
        for type in signature {
            componentData[type] = []
        }
    }
    
    func addEntity(_ entity: EntityID, components: [String: Component]) {
        entities.append(entity)
        for (type, component) in components {
            componentData[type]?.append(component)
        }
    }
    
    func removeEntity(at index: Int) {
        entities.remove(at: index)
        for type in signature {
            componentData[type]?.remove(at: index)
        }
    }
}

public actor ECSManager {
    public static let shared = ECSManager()
    
    private var nextEntityID: UInt64 = 0
    private var archetypes: [Set<String>: Archetype] = [:]
    private var entityToArchetype: [EntityID: (Set<String>, Int)] = [:]
    
    private init() {}
    
    public func createEntity(with components: [Component]) -> EntityID {
        let id = EntityID(id: nextEntityID)
        nextEntityID += 1
        
        var componentMap: [String: Component] = [:]
        var signature: Set<String> = []
        
        for c in components {
            let typeName = String(describing: type(of: c))
            componentMap[typeName] = c
            signature.insert(typeName)
        }
        
        let archetype = archetypes[signature] ?? {
            let newArchetype = Archetype(signature: signature)
            archetypes[signature] = newArchetype
            return newArchetype
        }()
        
        let index = archetype.entities.count
        archetype.addEntity(id, components: componentMap)
        entityToArchetype[id] = (signature, index)
        
        return id
    }
    
    public func getComponent<T: Component>(_ type: T.Type, for entity: EntityID) -> T? {
        guard let (signature, index) = entityToArchetype[entity],
              let archetype = archetypes[signature],
              let data = archetype.componentData[String(describing: T.self)] else {
            return nil
        }
        return data[index] as? T
    }
    
    public func query(signature: Set<String>) -> [(EntityID, [String: Component])] {
        var results: [(EntityID, [String: Component])] = []
        
        for (archSig, archetype) in archetypes {
            if signature.isSubset(of: archSig) {
                for i in 0..<archetype.entities.count {
                    var comps: [String: Component] = [:]
                    for type in signature {
                        if let comp = archetype.componentData[type]?[i] as? Component {
                            comps[type] = comp
                        }
                    }
                    results.append((archetype.entities[i], comps))
                }
            }
        }
        
        return results
    }
}
