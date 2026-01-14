import SwiftUI
import Combine

// MARK: - Command Protocol

protocol Command {
    func execute()
    func undo()
    func canExecute() -> Bool
    var description: String { get }
    var timestamp: Date { get }
}

extension Command {
    func canExecute() -> Bool { return true }
}

// MARK: - Command Manager

class CommandManager: ObservableObject {
    static let shared = CommandManager()
    @Published private(set) var undoStack: [Command] = []
    @Published private(set) var redoStack: [Command] = []
    
    // Potential future extensions:
    // - Command Batching/Transaction Support
    // - Command Throttling/Debouncing
    // - Asynchronous Command Execution
    // - Command History Persistence
    // - Command Macros
    
    private let maxHistorySize = 100
    private var commandGroup: [Command] = []
    private var isGrouping = false
    
    var canUndo: Bool {
        !undoStack.isEmpty
    }
    
    var canRedo: Bool {
        !redoStack.isEmpty
    }
    
    var undoDescription: String? {
        undoStack.last?.description
    }
    
    var redoDescription: String? {
        redoStack.last?.description
    }
    
    // MARK: - Command Execution
    
    func execute(_ command: Command) {
        guard command.canExecute() else {
            EngineBridge.shared.log("Command '\(command.description)' validation failed", level: .warning, source: "CommandManager")
            return
        }
        
        let startTime = CFAbsoluteTimeGetCurrent()
        command.execute()
        let duration = CFAbsoluteTimeGetCurrent() - startTime
        
        // Command execution logging
        EngineBridge.shared.log("Executed '\(command.description)' in \(String(format: "%.2fms", duration * 1000))", level: .debug, source: "CommandManager")
        
        if isGrouping {
            commandGroup.append(command)
        } else {
            addToUndoStack(command)
        }
    }
    
    func undo() {
        guard let command = undoStack.popLast() else { return }
        
        if let group = command as? CompositeCommand {
            // Undo all commands in group in reverse order
            for cmd in group.commands.reversed() {
                cmd.undo()
            }
        } else {
            command.undo()
        }
        
        redoStack.append(command)
        objectWillChange.send()
    }
    
    func redo() {
        guard let command = redoStack.popLast() else { return }
        
        if let group = command as? CompositeCommand {
            // Redo all commands in group
            for cmd in group.commands {
                cmd.execute()
            }
        } else {
            command.execute()
        }
        
        undoStack.append(command)
        objectWillChange.send()
    }
    
    // MARK: - Command Grouping
    
    func beginGroup() {
        isGrouping = true
        commandGroup.removeAll()
    }
    
    func endGroup(description: String) {
        guard isGrouping else { return }
        isGrouping = false
        
        if !commandGroup.isEmpty {
            let group = CompositeCommand(commands: commandGroup, description: description)
            addToUndoStack(group)
            commandGroup.removeAll()
        }
    }
    
    // MARK: - History Management
    
    func clear() {
        undoStack.removeAll()
        redoStack.removeAll()
        objectWillChange.send()
    }
    
    func getHistory() -> [Command] {
        return undoStack
    }
    
    // MARK: - Private
    
    private func addToUndoStack(_ command: Command) {
        undoStack.append(command)
        redoStack.removeAll()
        
        // Limit stack size
        if undoStack.count > maxHistorySize {
            undoStack.removeFirst()
        }
        
        objectWillChange.send()
    }
}

// MARK: - Command Group

struct CompositeCommand: Command {
    let commands: [Command]
    let description: String
    let timestamp: Date
    
    init(commands: [Command], description: String) {
        self.commands = commands
        self.description = description
        self.timestamp = Date()
    }
    
    func execute() {
        for command in commands {
            command.execute()
        }
    }
    
    func undo() {
        for command in commands.reversed() {
            command.undo()
        }
    }
}

// MARK: - Concrete Commands

// Transform Command
struct TransformCommand: Command {
    let entityID: UUID
    let oldTransform: TransformData
    let newTransform: TransformData
    let timestamp: Date
    
    weak var viewModel: SceneHierarchyViewModel?
    
    init(entityID: UUID, oldTransform: TransformData, newTransform: TransformData, viewModel: SceneHierarchyViewModel) {
        self.entityID = entityID
        self.oldTransform = oldTransform
        self.newTransform = newTransform
        self.viewModel = viewModel
        self.timestamp = Date()
    }
    
    var description: String {
        "Transform Entity"
    }
    
    func execute() {
        viewModel?.setTransform(entityID, transform: newTransform)
    }
    
    func undo() {
        viewModel?.setTransform(entityID, transform: oldTransform)
    }
}

struct TransformData: Codable {
    var position: SIMD3<Float>
    var rotation: SIMD3<Float>
    var scale: SIMD3<Float>
    
    init(position: SIMD3<Float> = SIMD3<Float>(0, 0, 0),
         rotation: SIMD3<Float> = SIMD3<Float>(0, 0, 0),
         scale: SIMD3<Float> = SIMD3<Float>(1, 1, 1)) {
        self.position = position
        self.rotation = rotation
        self.scale = scale
    }
}

// Create Entity Command
struct CreateEntityCommand: Command {
    let entityID: UUID
    let entityName: String
    let parentID: UUID?
    let timestamp: Date
    
    weak var viewModel: SceneHierarchyViewModel?
    weak var selectionManager: SelectionManager?
    
    init(entityID: UUID, entityName: String, parentID: UUID? = nil, viewModel: SceneHierarchyViewModel, selectionManager: SelectionManager) {
        self.entityID = entityID
        self.entityName = entityName
        self.parentID = parentID
        self.viewModel = viewModel
        self.selectionManager = selectionManager
        self.timestamp = Date()
    }
    
    var description: String {
        "Create Entity '\(entityName)'"
    }
    
    func execute() {
        viewModel?.createEntity(id: entityID, name: entityName, parentID: parentID)
    }
    
    func undo() {
        guard let selectionManager = selectionManager else { return }
        viewModel?.deleteEntity(entityID, selectionManager: selectionManager)
    }
}

// Delete Entity Command
struct DeleteEntityCommand: Command {
    let entityID: UUID
    let entityData: EntityData
    let timestamp: Date
    
    weak var viewModel: SceneHierarchyViewModel?
    weak var selectionManager: SelectionManager?
    
    init(entityID: UUID, entityData: EntityData, viewModel: SceneHierarchyViewModel, selectionManager: SelectionManager) {
        self.entityID = entityID
        self.entityData = entityData
        self.viewModel = viewModel
        self.selectionManager = selectionManager
        self.timestamp = Date()
    }
    
    var description: String {
        "Delete Entity '\(entityData.name)'"
    }
    
    func execute() {
        guard let selectionManager = selectionManager else { return }
        viewModel?.deleteEntity(entityID, selectionManager: selectionManager)
    }
    
    func undo() {
        viewModel?.restoreEntity(entityData)
    }
}

struct EntityData: Codable {
    let id: UUID
    let name: String
    let parentID: UUID?
    let transform: TransformData
    let components: [String: Data] // Component type -> serialized data
}

// Modify Property Command
struct ModifyPropertyCommand: Command {
    let entityID: UUID
    let propertyPath: String
    let oldValue: Any
    let newValue: Any
    let timestamp: Date
    
    weak var viewModel: InspectorViewModel?
    
    init(entityID: UUID, propertyPath: String, oldValue: Any, newValue: Any, viewModel: InspectorViewModel) {
        self.entityID = entityID
        self.propertyPath = propertyPath
        self.oldValue = oldValue
        self.newValue = newValue
        self.viewModel = viewModel
        self.timestamp = Date()
    }
    
    var description: String {
        "Modify \(propertyPath)"
    }
    
    func execute() {
        viewModel?.setProperty(entityID, path: propertyPath, value: newValue)
    }
    
    func undo() {
        viewModel?.setProperty(entityID, path: propertyPath, value: oldValue)
    }
}

// MARK: - History Panel

struct HistoryPanel: View {
    @ObservedObject var commandManager: CommandManager
    @State private var selectedCommand: UUID?
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("History")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Text("\(commandManager.undoStack.count) actions")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                EditorIconButton(icon: "trash", tooltip: "Clear History") {
                    commandManager.clear()
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Command list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    ForEach(Array(commandManager.undoStack.enumerated()), id: \.offset) { index, command in
                        CommandRow(
                            command: command,
                            index: index,
                            isInUndoStack: true
                        )
                    }
                    
                    if !commandManager.redoStack.isEmpty {
                        Divider()
                            .background(DesignSystem.Colors.accentPrimary)
                            .padding(.vertical, DesignSystem.Spacing.xs)
                        
                        ForEach(Array(commandManager.redoStack.enumerated().reversed()), id: \.offset) { index, command in
                            CommandRow(
                                command: command,
                                index: index,
                                isInUndoStack: false
                            )
                        }
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
        }
    }
}

private struct CommandRow: View {
    let command: Command
    let index: Int
    let isInUndoStack: Bool
    @State private var isHovering = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Index
            Text("\(index + 1)")
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 30, alignment: .trailing)
            
            // Icon
            Image(systemName: isInUndoStack ? "arrow.uturn.backward" : "arrow.uturn.forward")
                .font(.system(size: 12))
                .foregroundColor(isInUndoStack ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            
            // Description
            Text(command.description)
                .font(DesignSystem.Typography.body)
                .foregroundColor(isInUndoStack ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Timestamp
            Text(command.timestamp, style: .time)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
        .onHover { hovering in
            isHovering = hovering
        }
    }
}

// MARK: - Undo/Redo Shortcuts View

struct UndoRedoShortcutsView: View {
    @ObservedObject var commandManager: CommandManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Undo button
            Button(action: {
                commandManager.undo()
            }) {
                Image(systemName: "arrow.uturn.backward")
                    .font(.system(size: 16))
                    .foregroundColor(commandManager.canUndo ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
                    .frame(width: 30, height: 30)
            }
            .buttonStyle(.plain)
            .disabled(!commandManager.canUndo)
            .help(commandManager.canUndo ? "Undo: \(commandManager.undoDescription ?? "")" : "Nothing to undo")
            
            // Redo button
            Button(action: {
                commandManager.redo()
            }) {
                Image(systemName: "arrow.uturn.forward")
                    .font(.system(size: 16))
                    .foregroundColor(commandManager.canRedo ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
                    .frame(width: 30, height: 30)
            }
            .buttonStyle(.plain)
            .disabled(!commandManager.canRedo)
            .help(commandManager.canRedo ? "Redo: \(commandManager.redoDescription ?? "")" : "Nothing to redo")
        }
        .padding(DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

// MARK: - Preview

#Preview {
    VStack {
        UndoRedoShortcutsView(commandManager: CommandManager())
        
        HistoryPanel(commandManager: CommandManager())
    }
    .frame(width: 300, height: 400)
}
