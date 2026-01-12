import Foundation
import SwiftUI

// MARK: - Selection Manager

@MainActor
class SelectionManager: ObservableObject {
    @Published var selectedEntities: Set<UUID> = []
    @Published var primarySelection: UUID?
    
    // MARK: - Selection Operations
    
    func select(_ entityID: UUID, addToExisting: Bool = false) {
        if addToExisting {
            selectedEntities.insert(entityID)
        } else {
            selectedEntities = [entityID]
        }
        
        if primarySelection == nil || !addToExisting {
            primarySelection = entityID
        }
    }
    
    func deselect(_ entityID: UUID) {
        selectedEntities.remove(entityID)
        
        if primarySelection == entityID {
            primarySelection = selectedEntities.first
        }
    }
    
    func selectAll(_ entityIDs: [UUID]) {
        selectedEntities = Set(entityIDs)
        primarySelection = entityIDs.first
    }
    
    func clearSelection() {
        selectedEntities.removeAll()
        primarySelection = nil
    }
    
    func isSelected(_ entityID: UUID) -> Bool {
        return selectedEntities.contains(entityID)
    }
    
    func isPrimarySelection(_ entityID: UUID) -> Bool {
        return primarySelection == entityID
    }
    
    // MARK: - Batch Operations
    
    func deleteSelected() {
        for entityID in selectedEntities {
            EngineBridge.shared.deleteEntity(entityID)
        }
        clearSelection()
    }
    
    func duplicateSelected() -> [UUID] {
        var newEntities: [UUID] = []
        
        for entityID in selectedEntities {
            // This would need to be implemented in the engine
            // For now, just create a new entity with similar name
            let originalName = EngineBridge.shared.entities[entityID]?.name ?? "Entity"
            let newName = "\(originalName) Copy"
            let newEntity = EngineBridge.shared.createEntity(name: newName)
            newEntities.append(newEntity)
        }
        
        // Select the new entities
        selectAll(newEntities)
        
        return newEntities
    }
}

// MARK: - Transform Data

struct TransformData {
    var position: SIMD3<Float>
    var rotation: SIMD3<Float>
    var scale: SIMD3<Float>
    
    init(position: SIMD3<Float> = SIMD3(0, 0, 0),
         rotation: SIMD3<Float> = SIMD3(0, 0, 0),
         scale: SIMD3<Float> = SIMD3(1, 1, 1)) {
        self.position = position
        self.rotation = rotation
        self.scale = scale
    }
}

// MARK: - Component Bridge Data

struct ComponentBridgeData {
    let type: String
    var data: [String: Any]
    
    init(type: String, data: [String: Any]) {
        self.type = type
        self.data = data
    }
}

// MARK: - Entity Bridge Data

struct EntityBridgeData {
    let id: UUID
    var name: String
    var transform: TransformData
    var meshRenderer: MeshRendererData?
    var physics: PhysicsData?
}

// MARK: - Component Data Types

struct MeshRendererData {
    var material: String
    var castShadows: Bool
    var receiveShadows: Bool
}

struct PhysicsData {
    var mass: Float
    var useGravity: Bool
    var isKinematic: Bool
    var drag: Float
    var angularDrag: Float
    var velocity: SIMD3<Float>
    var angularVelocity: SIMD3<Float>
    
    init(mass: Float = 1.0,
         useGravity: Bool = true,
         isKinematic: Bool = false,
         drag: Float = 0.0,
         angularDrag: Float = 0.05,
         velocity: SIMD3<Float> = SIMD3(0, 0, 0),
         angularVelocity: SIMD3<Float> = SIMD3(0, 0, 0)) {
        self.mass = mass
        self.useGravity = useGravity
        self.isKinematic = isKinematic
        self.drag = drag
        self.angularDrag = angularDrag
        self.velocity = velocity
        self.angularVelocity = angularVelocity
    }
}
