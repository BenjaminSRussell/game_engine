import Foundation
import SwiftUI

// MARK: - Command Protocol

protocol Command {
    var id: UUID { get }
    var description: String { get }
    
    func execute()
    func undo()
    func canUndo() -> Bool
}

// MARK: - Command Manager

@MainActor
class CommandManager: ObservableObject {
    static let shared = CommandManager()
    
    @Published private(set) var canUndo = false
    @Published private(set) var canRedo = false
    @Published private(set) var history: [CommandHistoryItem] = []
    
    private var undoStack: [Command] = []
    private var redoStack: [Command] = []
    private let maxHistorySize = 100
    
    private init() {}
    
    // MARK: - Command Execution
    
    func execute(_ command: Command) {
        command.execute()
        
        // Add to undo stack
        undoStack.append(command)
        
        // Clear redo stack when new command is executed
        redoStack.removeAll()
        
        // Update history
        addToHistory(command, action: .execute)
        
        // Update state
        updateState()
        
        // Limit stack size
        if undoStack.count > maxHistorySize {
            undoStack.removeFirst()
        }
    }
    
    // MARK: - Undo/Redo
    
    func undo() {
        guard let command = undoStack.popLast() else { return }
        
        if command.canUndo() {
            command.undo()
            redoStack.append(command)
            addToHistory(command, action: .undo)
            updateState()
        }
    }
    
    func redo() {
        guard let command = redoStack.popLast() else { return }
        
        command.execute()
        undoStack.append(command)
        addToHistory(command, action: .redo)
        updateState()
    }
    
    // MARK: - Stack Management
    
    func clear() {
        undoStack.removeAll()
        redoStack.removeAll()
        history.removeAll()
        updateState()
    }
    
    private func updateState() {
        canUndo = !undoStack.isEmpty
        canRedo = !redoStack.isEmpty
    }
    
    private func addToHistory(_ command: Command, action: CommandAction) {
        let item = CommandHistoryItem(
            id: UUID(),
            commandId: command.id,
            description: command.description,
            action: action,
            timestamp: Date()
        )
        
        history.append(item)
        
        // Limit history size
        if history.count > maxHistorySize {
            history.removeFirst()
        }
    }
}

// MARK: - Supporting Types

struct CommandHistoryItem: Identifiable {
    let id: UUID
    let commandId: UUID
    let description: String
    let action: CommandAction
    let timestamp: Date
}

enum CommandAction {
    case execute
    case undo
    case redo
    
    var description: String {
        switch self {
        case .execute: return "Execute"
        case .undo: return "Undo"
        case .redo: return "Redo"
        }
    }
    
    var icon: String {
        switch self {
        case .execute: return "play.fill"
        case .undo: return "arrow.uturn.backward"
        case .redo: return "arrow.uturn.forward"
        }
    }
    
    var color: Color {
        switch self {
        case .execute: return .green
        case .undo: return .orange
        case .redo: return .blue
        }
    }
}

// MARK: - Concrete Commands

struct CreateEntityCommand: Command {
    let id = UUID()
    let description: String
    let entityName: String
    private var createdEntityID: UUID?
    
    init(entityName: String) {
        self.entityName = entityName
        self.description = "Create Entity '\(entityName)'"
    }
    
    func execute() {
        createdEntityID = EngineBridge.shared.createEntity(name: entityName)
    }
    
    func undo() {
        guard let entityID = createdEntityID else { return }
        EngineBridge.shared.deleteEntity(entityID)
        createdEntityID = nil
    }
    
    func canUndo() -> Bool {
        return createdEntityID != nil
    }
}

struct DeleteEntityCommand: Command {
    let id = UUID()
    let description: String
    let entityID: UUID
    private var entityData: EntityBridgeData?
    
    init(entity: EntityBridgeData) {
        self.entityID = entity.id
        self.description = "Delete Entity '\(entity.name)'"
    }
    
    func execute() {
        // Store entity data before deletion
        if let entity = EngineBridge.shared.entities[entityID] {
            entityData = entity
        }
        EngineBridge.shared.deleteEntity(entityID)
    }
    
    func undo() {
        guard let data = entityData else { return }
        
        // Recreate entity with same ID
        EngineBridge.shared.createEntityWithID(entityID, name: data.name)
        
        // Restore components
        if let transform = data.transform as? TransformData {
            EngineBridge.shared.setTransform(entityID, transform: transform)
        }
        
        if let meshRenderer = data.meshRenderer {
            EngineBridge.shared.setMeshRenderer(entityID, data: meshRenderer)
        }
        
        if let physics = data.physics {
            EngineBridge.shared.setPhysics(entityID, data: physics)
        }
    }
    
    func canUndo() -> Bool {
        return entityData != nil
    }
}

struct TransformCommand: Command {
    let id = UUID()
    let description: String
    let entityID: UUID
    let newTransform: TransformData
    private var oldTransform: TransformData?
    
    init(entityID: UUID, newTransform: TransformData) {
        self.entityID = entityID
        self.newTransform = newTransform
        self.description = "Transform Entity"
    }
    
    func execute() {
        // Store old transform
        oldTransform = EngineBridge.shared.getTransform(entityID)
        
        // Apply new transform
        EngineBridge.shared.setTransform(entityID, transform: newTransform)
    }
    
    func undo() {
        guard let oldTransform = oldTransform else { return }
        EngineBridge.shared.setTransform(entityID, transform: oldTransform)
    }
    
    func canUndo() -> Bool {
        return oldTransform != nil
    }
}

struct SetPropertyCommand<T>: Command {
    let id = UUID()
    let description: String
    let entityID: UUID
    let property: String
    let newValue: T
    private var oldValue: T?
    
    init(entityID: UUID, property: String, newValue: T) {
        self.entityID = entityID
        self.property = property
        self.newValue = newValue
        self.description = "Set \(property)"
    }
    
    func execute() {
        // Implementation would depend on property type
        // For now, this is a placeholder
    }
    
    func undo() {
        // Implementation would restore old value
    }
    
    func canUndo() -> Bool {
        return oldValue != nil
    }
}
